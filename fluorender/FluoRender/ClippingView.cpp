/*
For more information, please see: http://software.sci.utah.edu

The MIT License

Copyright (c) 2014 Scientific Computing and Imaging Institute,
University of Utah.


Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/
#include "ClippingView.h"
#include "VRenderFrame.h"
#include "compatibility.h"
#include <wx/valnum.h>
#include <wx/stdpaths.h>

BEGIN_EVENT_TABLE(ClippingView, wxPanel)
	EVT_CHECKBOX(ID_LinkChannelsChk, ClippingView::OnLinkChannelsCheck)
	EVT_COMBOBOX(ID_PlaneModesCombo, ClippingView::OnPlaneModesCombo)
	EVT_BUTTON(ID_SetZeroBtn, ClippingView::OnSetZeroBtn)
	EVT_CHECKBOX(ID_FixRotsChk, ClippingView::OnFixRotsCheck)
    EVT_CHECKBOX(ID_HoldPlanesChk, ClippingView::OnHoldPlanesCheck)
	EVT_BUTTON(ID_RotResetBtn, ClippingView::OnRotResetBtn)
	EVT_BUTTON(ID_ClipResetBtn, ClippingView::OnClipResetBtn)

	EVT_COMMAND_SCROLL(ID_X1ClipSldr, ClippingView::OnX1ClipChange)
	EVT_COMMAND_SCROLL(ID_X2ClipSldr, ClippingView::OnX2ClipChange)
	EVT_COMMAND_SCROLL(ID_Y1ClipSldr, ClippingView::OnY1ClipChange)
	EVT_COMMAND_SCROLL(ID_Y2ClipSldr, ClippingView::OnY2ClipChange)
	EVT_COMMAND_SCROLL(ID_Z1ClipSldr, ClippingView::OnZ1ClipChange)
	EVT_COMMAND_SCROLL(ID_Z2ClipSldr, ClippingView::OnZ2ClipChange)

	EVT_TEXT(ID_X1ClipText, ClippingView::OnX1ClipEdit)
	EVT_TEXT(ID_X2ClipText, ClippingView::OnX2ClipEdit)
	EVT_TEXT(ID_Y1ClipText, ClippingView::OnY1ClipEdit)
	EVT_TEXT(ID_Y2ClipText, ClippingView::OnY2ClipEdit)
	EVT_TEXT(ID_Z1ClipText, ClippingView::OnZ1ClipEdit)
	EVT_TEXT(ID_Z2ClipText, ClippingView::OnZ2ClipEdit)

	EVT_TIMER(ID_CLTimer, ClippingView::OnIdle)

	EVT_CHECKBOX(ID_LinkXChk, ClippingView::OnLinkXCheck)
	EVT_CHECKBOX(ID_LinkYChk, ClippingView::OnLinkYCheck)
	EVT_CHECKBOX(ID_LinkZChk, ClippingView::OnLinkZCheck)

	EVT_COMMAND_SCROLL(ID_XRotSldr, ClippingView::OnXRotChange)
	EVT_COMMAND_SCROLL(ID_YRotSldr, ClippingView::OnYRotChange)
	EVT_COMMAND_SCROLL(ID_ZRotSldr, ClippingView::OnZRotChange)
	EVT_TEXT(ID_XRotText, ClippingView::OnXRotEdit)
	EVT_TEXT(ID_YRotText, ClippingView::OnYRotEdit)
	EVT_TEXT(ID_ZRotText, ClippingView::OnZRotEdit)

	//spin buttons
	EVT_SPIN_UP(ID_XRotSpin, ClippingView::OnXRotSpinUp)
	EVT_SPIN_DOWN(ID_XRotSpin, ClippingView::OnXRotSpinDown)
	EVT_SPIN_UP(ID_YRotSpin, ClippingView::OnYRotSpinUp)
	EVT_SPIN_DOWN(ID_YRotSpin, ClippingView::OnYRotSpinDown)
	EVT_SPIN_UP(ID_ZRotSpin, ClippingView::OnZRotSpinUp)
	EVT_SPIN_DOWN(ID_ZRotSpin, ClippingView::OnZRotSpinDown)

	//clip buttons
	EVT_BUTTON(ID_YZClipBtn, ClippingView::OnYZClipBtn)
	EVT_BUTTON(ID_XZClipBtn, ClippingView::OnXZClipBtn)
	EVT_BUTTON(ID_XYClipBtn, ClippingView::OnXYClipBtn)
END_EVENT_TABLE()

ClippingView::ClippingView(wxWindow* frame,
						   wxWindow* parent,
						   wxWindowID id,
						   const wxPoint& pos,
						   const wxSize& size,
						   long style,
						   const wxString& name):
wxPanel(parent, id, pos, size, style, name),
m_frame(frame),
m_sel_type(0),
m_vd(0),
m_md(0),
m_draw_clip(false),
m_hold_planes(false),
m_plane_mode(PM_NORMAL),
m_link_x(false),
m_link_y(false),
m_link_z(false),
m_fix_rots(false),
m_mouse_in(false),
m_update_only_ui(false)
{
	SetEvtHandlerEnabled(false);
	Freeze();

	//validator: floating point 1
	wxFloatingPointValidator<double> vald_fp1(1);
	vald_fp1.SetRange(-180.0, 180.0);
	//validator: integer
	wxIntegerValidator<unsigned int> vald_int;
    
#ifdef _DARWIN
    wxSize sldrsize = wxDefaultSize;
#else
    wxSize sldrsize = wxSize(20,-1);
#endif

	wxStaticText *st = 0;

	//sync channels
	wxBoxSizer* sizer_1 = new wxBoxSizer(wxHORIZONTAL);
	m_link_channels = new wxCheckBox(this, ID_LinkChannelsChk, "Sync All Chan.");
	m_link_channels->SetValue(false);
	sizer_1->Add(5, 5, 0);
	sizer_1->Add(m_link_channels, 0, wxALIGN_CENTER, 0);

	wxBoxSizer* sizer_pm = new wxBoxSizer(wxVERTICAL);
	// display mode chooser
	st = new wxStaticText(this, 0, "Display Mode:",
		wxDefaultPosition, wxSize(100, -1), wxALIGN_CENTER);
	m_plane_mode_combo = new wxComboBox(this, ID_PlaneModesCombo, "",
		wxDefaultPosition, wxSize(115, 24), 0, NULL, wxCB_READONLY);
	vector<string>dispmode_list;
	dispmode_list.push_back("Normal");
	dispmode_list.push_back("Frame");
	dispmode_list.push_back("LowTrans");
	dispmode_list.push_back("LowTransBack");
	dispmode_list.push_back("NormalBack");
	for (size_t i=0; i<dispmode_list.size(); ++i)
		m_plane_mode_combo->Append(dispmode_list[i]);
	m_plane_mode_combo->SetSelection(m_plane_mode);
	sizer_pm->Add(st, 0, wxALIGN_CENTER, 0);
	sizer_pm->Add(m_plane_mode_combo, 0, wxALIGN_CENTER, 0);
	
	//rotations
	wxBoxSizer* sizer_f = new wxBoxSizer(wxHORIZONTAL);
	m_fix_rots_chk = new wxCheckBox(this, ID_FixRotsChk, "Fix");
	m_fix_rots_chk->SetValue(false);
    m_hold_planes_chk = new wxCheckBox(this, ID_HoldPlanesChk, "Display Planes");
    m_hold_planes_chk->SetValue(false);
	sizer_f->Add(5, 5, 0);
	sizer_f->Add(m_fix_rots_chk, 0, wxALIGN_CENTER);

	//set sero rotation for clipping planes
	wxBoxSizer* sizer_2 = new wxBoxSizer(wxHORIZONTAL);
	m_set_zero_btn = new wxButton(this, ID_SetZeroBtn, "Align to View",
		wxDefaultPosition, wxSize(100, 22));
	sizer_2->Add(5, 5, 0);
	sizer_2->Add(m_set_zero_btn, 0, wxALIGN_CENTER);

	//reset rotations
	wxBoxSizer* sizer_3 = new wxBoxSizer(wxHORIZONTAL);
	m_rot_reset_btn = new wxButton(this, ID_RotResetBtn, "Reset to 0",
		wxDefaultPosition, wxSize(85, 22));
	sizer_3->Add(5, 5, 0);
	sizer_3->Add(m_rot_reset_btn, 0, wxALIGN_CENTER);

	//reset clipping
	wxBoxSizer* sizer_4 = new wxBoxSizer(wxHORIZONTAL);
	m_clip_reset_btn = new wxButton(this, ID_ClipResetBtn, "Reset Clips",
		wxDefaultPosition, wxSize(85, 22));
	sizer_4->Add(5, 5, 0);
	sizer_4->Add(m_clip_reset_btn, 0, wxALIGN_CENTER);

	//clip buttons
	wxBoxSizer* sizer_5 = new wxBoxSizer(wxHORIZONTAL);
	m_yz_clip_btn = new wxButton(this, ID_YZClipBtn, "YZ",
		wxDefaultPosition, wxSize(34, 22));
	m_xz_clip_btn = new wxButton(this, ID_XZClipBtn, "XZ",
		wxDefaultPosition, wxSize(34, 22));
	m_xy_clip_btn = new wxButton(this, ID_XYClipBtn, "XY",
		wxDefaultPosition, wxSize(34, 22));
	sizer_5->Add(m_yz_clip_btn, 1, wxEXPAND);
	sizer_5->AddSpacer(5);
	sizer_5->Add(m_xz_clip_btn, 1, wxEXPAND);
	sizer_5->AddSpacer(5);
	sizer_5->Add(m_xy_clip_btn, 1, wxEXPAND);

	//clip distance
	wxBoxSizer* sizer_6 = new wxBoxSizer(wxHORIZONTAL);
	m_yz_dist_text = new wxTextCtrl(this, ID_YZDistText, "1",
		wxDefaultPosition, wxSize(34, 22), 0, vald_int);
	m_xz_dist_text = new wxTextCtrl(this, ID_XZDistText, "1",
		wxDefaultPosition, wxSize(34, 22), 0, vald_int);
	m_xy_dist_text = new wxTextCtrl(this, ID_XYDistText, "1",
		wxDefaultPosition, wxSize(34, 22), 0, vald_int);
	sizer_6->Add(m_yz_dist_text, 1, wxEXPAND);
	sizer_6->AddSpacer(5);
	sizer_6->Add(m_xz_dist_text, 1, wxEXPAND);
	sizer_6->AddSpacer(5);
	sizer_6->Add(m_xy_dist_text, 1, wxEXPAND);

	wxStaticText* st_cb = 0;

	//sliders for clipping planes
	//x
	wxBoxSizer* sizer_cx = new wxBoxSizer(wxVERTICAL);
	m_xpanel = new wxPanel(this);
	st = new wxStaticText(this, 0, "X");
	m_x1_clip_sldr = new wxSlider(m_xpanel, ID_X1ClipSldr, 0, 0, 512,
		wxPoint(0,0), sldrsize, wxSL_VERTICAL);
	m_xBar = new wxStaticText(m_xpanel, 0, "",
		wxPoint(20,10), wxSize(3, m_x1_clip_sldr->GetSize().GetHeight() - 20));
	m_xBar->SetBackgroundColour(wxColor(255, 128, 128));
	m_x2_clip_sldr = new wxSlider(m_xpanel, ID_X2ClipSldr, 512, 0, 512,
		wxPoint(23,0), sldrsize, wxSL_VERTICAL);
	m_x1_clip_text = new wxTextCtrl(this, ID_X1ClipText, "0",
		wxDefaultPosition, wxSize(34, 20), 0, vald_int);
	st_cb = new wxStaticText(this, 0, "",
		wxDefaultPosition, wxSize(5, 5));
	st_cb->SetBackgroundColour(wxColor(255, 128, 128));
	m_x2_clip_text = new wxTextCtrl(this, ID_X2ClipText, "512",
		wxDefaultPosition, wxSize(34, 20), 0, vald_int);
	//add the items
	sizer_cx->Add(5, 5, 0);
	sizer_cx->Add(st, 0, wxALIGN_CENTER, 0);
	sizer_cx->Add(m_x1_clip_text, 0, wxALIGN_CENTER, 0);
	sizer_cx->Add(5, 5, 0);
	sizer_cx->Add(st_cb, 0, wxEXPAND);
	sizer_cx->Add(m_xpanel, 1, wxALIGN_CENTER, 0);
	st_cb = new wxStaticText(this, 0, "",
		wxDefaultPosition, wxSize(5, 5));
	st_cb->SetBackgroundColour(wxColor(255, 128, 255));
	sizer_cx->Add(st_cb, 0, wxEXPAND);
	sizer_cx->Add(5, 5, 0);
	sizer_cx->Add(m_x2_clip_text, 0, wxALIGN_CENTER, 0);
	//link x
	m_link_x_chk = new wxCheckBox(this, ID_LinkXChk, "");
	m_link_x_chk->SetValue(false);
	st = new wxStaticText(this, 0, "Link");
	sizer_cx->Add(5, 10, 0);
	sizer_cx->Add(m_link_x_chk, 0, wxALIGN_CENTER, 0);
	sizer_cx->Add(st, 0, wxALIGN_CENTER, 0);

	//y
	wxBoxSizer* sizer_cy = new wxBoxSizer(wxVERTICAL);
	wxPanel * ypanel = new wxPanel(this);
	st = new wxStaticText(this, 0, "Y");
	m_y1_clip_sldr = new wxSlider(ypanel, ID_Y1ClipSldr, 0, 0, 512,
		wxPoint(0,0), sldrsize, wxSL_VERTICAL);
	m_yBar = new wxStaticText(ypanel, 0, "",
		wxPoint(20,10), wxSize(3, m_x1_clip_sldr->GetSize().GetHeight() - 20));
	m_yBar->SetBackgroundColour(wxColor(128, 255, 128));
	m_y2_clip_sldr = new wxSlider(ypanel, ID_Y2ClipSldr, 512, 0, 512,
		wxPoint(23,0), sldrsize, wxSL_VERTICAL);
	m_y1_clip_text = new wxTextCtrl(this, ID_Y1ClipText, "0",
		wxDefaultPosition, wxSize(34, 20), 0, vald_int);
	st_cb = new wxStaticText(this, 0, "",
		wxDefaultPosition, wxSize(5, 5));
	st_cb->SetBackgroundColour(wxColor(128, 255, 128));
	m_y2_clip_text = new wxTextCtrl(this, ID_Y2ClipText, "512",
		wxDefaultPosition, wxSize(34, 20), 0, vald_int);
	//add the items
	sizer_cy->Add(5, 5, 0);
	sizer_cy->Add(st, 0, wxALIGN_CENTER, 0);
	sizer_cy->Add(m_y1_clip_text, 0, wxALIGN_CENTER, 0);
	sizer_cy->Add(5, 5, 0);
	sizer_cy->Add(st_cb, 0, wxEXPAND);
	sizer_cy->Add(ypanel, 1, wxALIGN_CENTER, 0);
	st_cb = new wxStaticText(this, 0, "",
		wxDefaultPosition, wxSize(5, 5));
	st_cb->SetBackgroundColour(wxColor(255, 255, 128));
	sizer_cy->Add(st_cb, 0, wxEXPAND);
	sizer_cy->Add(5, 5, 0);
	sizer_cy->Add(m_y2_clip_text, 0, wxALIGN_CENTER, 0);
	//link y
	m_link_y_chk = new wxCheckBox(this, ID_LinkYChk, "");
	m_link_y_chk->SetValue(false);
	st = new wxStaticText(this, 0, "Link");
	sizer_cy->Add(5, 10, 0);
	sizer_cy->Add(m_link_y_chk, 0, wxALIGN_CENTER, 0);
	sizer_cy->Add(st, 0, wxALIGN_CENTER, 0);

	//z
	wxBoxSizer* sizer_cz = new wxBoxSizer(wxVERTICAL);
	wxPanel * zpanel = new wxPanel(this);
	st = new wxStaticText(this, 0, "Z");
	m_z1_clip_sldr = new wxSlider(zpanel, ID_Z1ClipSldr, 0, 0, 512,
		wxPoint(0,0), sldrsize, wxSL_VERTICAL);
	m_zBar = new wxStaticText(zpanel, 0, "",
		wxPoint(20,10), wxSize(3, m_x1_clip_sldr->GetSize().GetHeight() - 20));
	m_zBar->SetBackgroundColour(wxColor(128, 128, 255));
	m_z2_clip_sldr = new wxSlider(zpanel, ID_Z2ClipSldr, 512, 0, 512,
		wxPoint(23,0), sldrsize, wxSL_VERTICAL);
	m_z1_clip_text = new wxTextCtrl(this, ID_Z1ClipText, "0",
		wxDefaultPosition, wxSize(34, 20), 0, vald_int);
	st_cb = new wxStaticText(this, 0, "",
		wxDefaultPosition, wxSize(5, 5));
	st_cb->SetBackgroundColour(wxColor(128, 128, 255));
	m_z2_clip_text = new wxTextCtrl(this, ID_Z2ClipText, "512",
		wxDefaultPosition, wxSize(34, 20), 0, vald_int);
	//add the items
	sizer_cz->Add(5, 5, 0);
	sizer_cz->Add(st, 0, wxALIGN_CENTER, 0);
	sizer_cz->Add(m_z1_clip_text, 0, wxALIGN_CENTER, 0);
	sizer_cz->Add(5, 5, 0);
	sizer_cz->Add(st_cb, 0, wxEXPAND);
	sizer_cz->Add(zpanel, 1, wxALIGN_CENTER, 0);
	st_cb = new wxStaticText(this, 0, "",
		wxDefaultPosition, wxSize(5, 5));
	st_cb->SetBackgroundColour(wxColor(128, 255, 255));
	sizer_cz->Add(st_cb, 0, wxEXPAND);
	sizer_cz->Add(5, 5, 0);
	sizer_cz->Add(m_z2_clip_text, 0, wxALIGN_CENTER, 0);
	//link z
	m_link_z_chk = new wxCheckBox(this, ID_LinkZChk, "");
	m_link_z_chk->SetValue(false);
	st = new wxStaticText(this, 0, "Link");
	sizer_cz->Add(5, 10, 0);
	sizer_cz->Add(m_link_z_chk, 0, wxALIGN_CENTER, 0);
	sizer_cz->Add(st, 0, wxALIGN_CENTER, 0);

	//link sliders
	m_x1_clip_sldr->Connect(ID_X1ClipSldr, wxEVT_RIGHT_DOWN,
		wxCommandEventHandler(ClippingView::OnSliderRClick),
		NULL, this);
	m_x2_clip_sldr->Connect(ID_X2ClipSldr, wxEVT_RIGHT_DOWN,
		wxCommandEventHandler(ClippingView::OnSliderRClick),
		NULL, this);
	m_y1_clip_sldr->Connect(ID_Y1ClipSldr, wxEVT_RIGHT_DOWN,
		wxCommandEventHandler(ClippingView::OnSliderRClick),
		NULL, this);
	m_y2_clip_sldr->Connect(ID_Y2ClipSldr, wxEVT_RIGHT_DOWN,
		wxCommandEventHandler(ClippingView::OnSliderRClick),
		NULL, this);
	m_z1_clip_sldr->Connect(ID_Z1ClipSldr, wxEVT_RIGHT_DOWN,
		wxCommandEventHandler(ClippingView::OnSliderRClick),
		NULL, this);
	m_z2_clip_sldr->Connect(ID_Z2ClipSldr, wxEVT_RIGHT_DOWN,
		wxCommandEventHandler(ClippingView::OnSliderRClick),
		NULL, this);

	//keys
	m_x1_clip_sldr->Connect(ID_X1ClipSldr, wxEVT_KEY_DOWN,
		wxKeyEventHandler(ClippingView::OnSliderKeyDown),
		NULL, this);
	m_x2_clip_sldr->Connect(ID_X2ClipSldr, wxEVT_KEY_DOWN,
		wxKeyEventHandler(ClippingView::OnSliderKeyDown),
		NULL, this);
	m_y1_clip_sldr->Connect(ID_Y1ClipSldr, wxEVT_KEY_DOWN,
		wxKeyEventHandler(ClippingView::OnSliderKeyDown),
		NULL, this);
	m_y2_clip_sldr->Connect(ID_Y2ClipSldr, wxEVT_KEY_DOWN,
		wxKeyEventHandler(ClippingView::OnSliderKeyDown),
		NULL, this);
	m_z1_clip_sldr->Connect(ID_Z1ClipSldr, wxEVT_KEY_DOWN,
		wxKeyEventHandler(ClippingView::OnSliderKeyDown),
		NULL, this);
	m_z2_clip_sldr->Connect(ID_Z2ClipSldr, wxEVT_KEY_DOWN,
		wxKeyEventHandler(ClippingView::OnSliderKeyDown),
		NULL, this);

	//h2
	wxBoxSizer *sizer_h2 = new wxBoxSizer(wxHORIZONTAL);
	sizer_h2->Add(sizer_cx, 1, wxEXPAND);
	sizer_h2->Add(sizer_cy, 1, wxEXPAND);
	sizer_h2->Add(sizer_cz, 1, wxEXPAND);

	//sliders for rotating clipping planes 
	//x
	wxBoxSizer* sizer_rx = new wxBoxSizer(wxVERTICAL);
	st = new wxStaticText(this, 0, "X");
	m_x_rot_sldr = new wxSlider(this, ID_XRotSldr, 0, -180, 180,
		wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL|wxSL_INVERSE);
	m_x_rot_text = new wxTextCtrl(this, ID_XRotText, "0.0",
		wxDefaultPosition, wxSize(34, 20), 0, vald_fp1);
	m_x_rot_spin = new wxSpinButton(this, ID_XRotSpin,
		wxDefaultPosition, wxSize(30, 20), wxSP_VERTICAL);
	sizer_rx->Add(5, 5, 0);
	sizer_rx->Add(m_x_rot_text, 0, wxALIGN_CENTER, 0);
	sizer_rx->Add(st, 0, wxALIGN_CENTER, 0);
	sizer_rx->Add(5, 5, 0);
	sizer_rx->Add(m_x_rot_spin, 0, wxALIGN_CENTER, 0);
	sizer_rx->Add(m_x_rot_sldr, 1, wxALIGN_CENTER, 0);
	//y
	wxBoxSizer* sizer_ry = new wxBoxSizer(wxVERTICAL);
	st = new wxStaticText(this, 0, "Y");
	m_y_rot_sldr = new wxSlider(this, ID_YRotSldr, 0, -180, 180,
		wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL|wxSL_INVERSE);
	m_y_rot_text = new wxTextCtrl(this, ID_YRotText, "0.0",
		wxDefaultPosition, wxSize(34, 20), 0, vald_fp1);
	m_y_rot_spin = new wxSpinButton(this, ID_YRotSpin,
		wxDefaultPosition, wxSize(30, 20), wxSP_VERTICAL);
	sizer_ry->Add(5, 5, 0);
	sizer_ry->Add(m_y_rot_text, 0, wxALIGN_CENTER, 0);
	sizer_ry->Add(st, 0, wxALIGN_CENTER, 0);
	sizer_ry->Add(5, 5, 0);
	sizer_ry->Add(m_y_rot_spin, 0, wxALIGN_CENTER, 0);
	sizer_ry->Add(m_y_rot_sldr, 1, wxALIGN_CENTER, 0);
	//z
	wxBoxSizer* sizer_rz = new wxBoxSizer(wxVERTICAL);
	st = new wxStaticText(this, 0, "Z");
	m_z_rot_sldr = new wxSlider(this, ID_ZRotSldr, 0, -180, 180,
		wxDefaultPosition, wxDefaultSize, wxSL_VERTICAL|wxSL_INVERSE);
	m_z_rot_text = new wxTextCtrl(this, ID_ZRotText, "0.0",
		wxDefaultPosition, wxSize(34, 20), 0, vald_fp1);
	m_z_rot_spin = new wxSpinButton(this, ID_ZRotSpin,
		wxDefaultPosition, wxSize(30, 20), wxSP_VERTICAL);
	sizer_rz->Add(5, 5, 0);
	sizer_rz->Add(m_z_rot_text, 0, wxALIGN_CENTER, 0);
	sizer_rz->Add(st, 0, wxALIGN_CENTER, 0);
	sizer_rz->Add(5, 5, 0);
	sizer_rz->Add(m_z_rot_spin, 0, wxALIGN_CENTER, 0);
	sizer_rz->Add(m_z_rot_sldr, 1, wxALIGN_CENTER, 0);
	

	//sizer 9
	wxBoxSizer *sizer_h1 = new wxBoxSizer(wxHORIZONTAL);
	sizer_h1->Add(sizer_rx, 1, wxEXPAND);
	sizer_h1->Add(sizer_ry, 1, wxEXPAND);
	sizer_h1->Add(sizer_rz, 1, wxEXPAND);

	//v
	wxBoxSizer *sizer_v = new wxBoxSizer(wxVERTICAL);
//	st = new wxStaticText(this, 0, "Clippings:");
//	sizer_v->Add(st, 0, wxALIGN_CENTER);
	sizer_v->Add(5, 5, 0);
	sizer_v->Add(sizer_1, 0, wxALIGN_LEFT);
	sizer_v->Add(5, 5, 0);
	sizer_v->Add(sizer_pm, 0, wxALIGN_CENTER);
	sizer_v->Add(5, 5, 0);
	sizer_v->Add(sizer_h2, 3, wxEXPAND);
	sizer_v->Add(5, 5, 0);
	sizer_v->Add(sizer_4, 0, wxALIGN_CENTER);
	sizer_v->Add(5, 5, 0);
	sizer_v->Add(sizer_5, 0, wxALIGN_CENTER);
	sizer_v->Add(5, 5, 0);
	sizer_v->Add(sizer_6, 0, wxALIGN_CENTER);
	sizer_v->Add(5, 5, 0);

	st = new wxStaticText(this, 0, "", wxDefaultPosition, wxSize(5, 5));
	st->SetBackgroundColour(wxColor(100, 100, 100));
	sizer_v->Add(10, 10, 0);
	sizer_v->Add(st, 0, wxEXPAND);

	st = new wxStaticText(this, 0, "Clipping Plane Rotations:");
	sizer_v->Add(10, 10, 0);
	sizer_v->Add(st, 0, wxALIGN_CENTER);
	sizer_v->Add(10, 10, 0);
	sizer_v->Add(sizer_f, 0, wxALIGN_CENTER);
    sizer_v->Add(m_hold_planes_chk, 0, wxALIGN_CENTER);
    sizer_v->Add(5, 5, 0);
	sizer_v->Add(sizer_2, 0, wxALIGN_CENTER);
	sizer_v->Add(sizer_3, 0, wxALIGN_CENTER);
	sizer_v->Add(10, 10, 0);
	sizer_v->Add(sizer_h1, 2, wxEXPAND);

	SetSizer(sizer_v);
	Layout();

	LoadDefault();

	DisableAll();

	Thaw();
	SetEvtHandlerEnabled(true);

	m_timer = new wxTimer(this, ID_CLTimer);
	m_timer->Start(100);
    
    for(auto &e : m_linked_plane_params)
        e = 0.0;
}

ClippingView::~ClippingView()
{
	m_timer->Stop();
	wxDELETE(m_timer);
	SaveDefault();
}

int ClippingView::GetSelType()
{
	return m_sel_type;
}

VolumeData* ClippingView::GetVolumeData()
{
	return m_vd;
}

MeshData* ClippingView::GetMeshData()
{
	return m_md;
}

void ClippingView::SetVolumeData(VolumeData* vd)
{
	if (!vd) return;
	m_vd = vd;
	m_md = NULL;
	m_sel_type = 2;
	GetSettings();
}

void ClippingView::SetMeshData(MeshData* md)
{
	if (!md) return;
	m_md = md;
	m_vd = NULL;
	m_sel_type = 3;
	GetSettings();
}

void ClippingView::SetDataManager(DataManager* mgr)
{
	m_mgr = mgr;
}

void ClippingView::RefreshVRenderViews(bool interactive)
{
	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	vrender_frame->RefreshVRenderViews(false, interactive);
}

void ClippingView::RefreshVRenderViewsOverlay()
{
	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	vrender_frame->RefreshVRenderViewsOverlay(false);
}

void ClippingView::ClearData()
{
	m_vd = NULL;
	m_md = NULL;
	m_sel_type = -1;
	DisableAll();
}

int ClippingView::GetXdist()
{
    wxString str = m_x1_clip_text->GetValue();
    long ival = 0;
    str.ToLong(&ival);
    str = m_x2_clip_text->GetValue();
    long ival2 = 0;
    str.ToLong(&ival2);
    return (int)abs(ival2 - ival);
}

int ClippingView::GetYdist()
{
    wxString str = m_y1_clip_text->GetValue();
    long ival = 0;
    str.ToLong(&ival);
    str = m_y2_clip_text->GetValue();
    long ival2 = 0;
    str.ToLong(&ival2);
    return (int)abs(ival2 - ival);
}

int ClippingView::GetZdist()
{
    wxString str = m_z1_clip_text->GetValue();
    long ival = 0;
    str.ToLong(&ival);
    str = m_z2_clip_text->GetValue();
    long ival2 = 0;
    str.ToLong(&ival2);
    return (int)abs(ival2 - ival);
}

void ClippingView::CalcBoundingBoxDemensions(double &w, double &h, double &d)
{
    double min_spcx = DBL_MAX;
    double min_spcy = DBL_MAX;
    double min_spcz = DBL_MAX;
    if (m_link_channels->GetValue() && m_mgr)
    {
        for (int i=0; i<m_mgr->GetVolumeNum(); i++)
        {
            VolumeData* vd = m_mgr->GetVolumeData(i);
            if (!vd)
                continue;
            double spcx = DBL_MAX;
            double spcy = DBL_MAX;
            double spcz = DBL_MAX;
            vd->GetSpacings(spcx, spcy, spcz, 0);
            min_spcx = min_spcx > spcx ? spcx : min_spcx;
            min_spcy = min_spcy > spcy ? spcy : min_spcy;
            min_spcz = min_spcz > spcz ? spcz : min_spcz;
        }
    }
    if (min_spcx == DBL_MAX) min_spcx = 1.0;
    if (min_spcy == DBL_MAX) min_spcy = 1.0;
    if (min_spcz == DBL_MAX) min_spcz = 1.0;
    
    vector<Plane*> *planes = 0;
    Transform tform;
    tform.load_identity();
    switch (m_sel_type)
    {
        case 2:    //volume
            if (m_vd->GetVR() && m_vd->GetTexture() && m_vd->GetTexture()->transform())
            {
                planes = m_vd->GetVR()->get_planes();
                tform = *m_vd->GetTexture()->transform();
            }
            break;
        case 3:    //mesh
            if (m_md->GetMR())
            {
                planes = m_md->GetMR()->get_planes();
                BBox bb = m_md->GetBounds();
                Vector sc(bb.max().x() - bb.min().x(), bb.max().y() - bb.min().y(), bb.max().z() - bb.min().z());
                tform.post_scale(sc);
            }
            break;
    }
    if (!planes)
        return;
    if (planes->size()!=6)    //it has to be 6
        return;
    
    int resx, resy, resz;
    switch (m_sel_type)
    {
        case 2:    //volume
            m_vd->GetResolution(resx, resy, resz);
            if (m_link_channels->GetValue())
            {
                double mvmat[16];
                tform.get_trans(mvmat);
                swap(mvmat[3], mvmat[12]);
                swap(mvmat[7], mvmat[13]);
                swap(mvmat[11], mvmat[14]);
                tform.set(mvmat);
                
                double dims[3];
                for (int j = 0; j < 3; j++)
                {
                    double param = (*planes)[2*j]->GetParam();
                    (*planes)[2*j]->SetParam(0.0);
                    Point p0 = (*planes)[2*j]->get_point();
                    (*planes)[2*j]->SetParam(1.0);
                    Point p1 = (*planes)[2*j]->get_point();
                    (*planes)[2*j]->SetParam(param);
                    p0 = tform.project(p0);
                    p1 = tform.project(p1);
                    dims[j] = (p1 - p0).length();
                }
                w = dims[0] / min_spcx;
                h = dims[1] / min_spcy;
                d = dims[2] / min_spcz;
            }
            else
            {
                w = resx;
                h = resy;
                d = resz;
            }
            break;
        case 3:    //mesh
            resx = resy = resz = 0;
            if (m_md)
            {
                Vector dim = m_md->GetBounds().diagonal();
                if (m_link_channels->GetValue())
                {
                    double dims[3];
                    for (int j = 0; j < 3; j++)
                    {
                        double param = (*planes)[2*j]->GetParam();
                        (*planes)[2*j]->SetParam(0.0);
                        Point p0 = (*planes)[2*j]->get_point();
                        (*planes)[2*j]->SetParam(1.0);
                        Point p1 = (*planes)[2*j]->get_point();
                        (*planes)[2*j]->SetParam(param);
                        p0 = tform.project(p0);
                        p1 = tform.project(p1);
                        dims[j] = (p1 - p0).length();
                    }
                    w = dims[0] / min_spcx;
                    h = dims[1] / min_spcy;
                    d = dims[2] / min_spcz;
                }
                else
                {
                    w = (int)dim.x();
                    h = (int)dim.y();
                    d = (int)dim.z();
                }
            }
            break;
    }
    
    w = w > 0 ? w : 1;
    h = h > 0 ? h : 1;
    d = d > 0 ? d : 1;
}

void ClippingView::GetSettings()
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
	{
		DisableAll();
		return;
	}

	EnableAll();

    vector<Plane*> *planes = 0;
    Transform tform;
    tform.load_identity();
    switch (m_sel_type)
    {
        case 2:    //volume
            if (m_vd->GetVR() && m_vd->GetTexture() && m_vd->GetTexture()->transform())
            {
                planes = m_vd->GetVR()->get_planes();
                tform = *m_vd->GetTexture()->transform();
            }
            break;
        case 3:    //mesh
            if (m_md->GetMR())
            {
                planes = m_md->GetMR()->get_planes();
                BBox bb = m_md->GetBounds();
                Vector sc(bb.max().x() - bb.min().x(), bb.max().y() - bb.min().y(), bb.max().z() - bb.min().z());
                tform.post_scale(sc);
            }
            break;
    }
    if (!planes)
        return;
    if (planes->size()!=6)    //it has to be 6
        return;

	int resx, resy, resz;
	int resx_n, resy_n, resz_n;
    double bdw, bdh, bdd;
    resx_n = resy_n = resz_n = 0;
    bdw = bdh = bdd = 0.0;
    CalcBoundingBoxDemensions(bdw, bdh, bdd);
    resx = round(bdw);
    resy = round(bdh);
    resz = round(bdd);

	//slider range
	m_x1_clip_sldr->SetRange(resx_n, resx);
	m_x2_clip_sldr->SetRange(resx_n, resx);
	m_y1_clip_sldr->SetRange(resy_n, resy);
	m_y2_clip_sldr->SetRange(resy_n, resy);
	m_z1_clip_sldr->SetRange(resz_n, resz);
	m_z2_clip_sldr->SetRange(resz_n, resz);
	//text range
	wxIntegerValidator<int>* vald_i;
	if ((vald_i = (wxIntegerValidator<int>*)m_x1_clip_text->GetValidator()))
		vald_i->SetRange(0, int(resx));
	if ((vald_i = (wxIntegerValidator<int>*)m_x2_clip_text->GetValidator()))
		vald_i->SetRange(0, int(resx));
	if ((vald_i = (wxIntegerValidator<int>*)m_y1_clip_text->GetValidator()))
		vald_i->SetRange(0, int(resy));
	if ((vald_i = (wxIntegerValidator<int>*)m_y2_clip_text->GetValidator()))
		vald_i->SetRange(0, int(resy));
	if ((vald_i = (wxIntegerValidator<int>*)m_z1_clip_text->GetValidator()))
		vald_i->SetRange(0, int(resz));
	if ((vald_i = (wxIntegerValidator<int>*)m_z2_clip_text->GetValidator()))
		vald_i->SetRange(0, int(resz));

	//clip distance
	switch (m_sel_type)
	{
	case 2:	//volume
		{
			int distx, disty, distz;
			m_vd->GetClipDistance(distx, disty, distz);
			if (distx == 0) distx = 1;
			if (disty == 0) disty = 1;
			if (distz == 0) distz = 1;
			m_yz_dist_text->SetValue(
				wxString::Format("%d", distx));
			m_xz_dist_text->SetValue(
				wxString::Format("%d", disty));
			m_xy_dist_text->SetValue(
				wxString::Format("%d", distz));
			m_vd->SetClipDistance(distx, disty, distz);
		}
		break;
	case 3:	//mesh
		{
			int distx, disty, distz;
			m_md->GetClipDistance(distx, disty, distz);
			if (distx == 0) distx = 1;
			if (disty == 0) disty = 1;
			if (distz == 0) distz = 1;
			m_yz_dist_text->SetValue(
				wxString::Format("%d", distx));
			m_xz_dist_text->SetValue(
				wxString::Format("%d", disty));
			m_xy_dist_text->SetValue(
				wxString::Format("%d", distz));
			m_md->SetClipDistance(distx, disty, distz);
		}
		break;
	}

	wxString str;
	Plane* plane = 0;
	int val = 0;
	double param;

	//x1
	plane = (*planes)[0];
    if (m_link_channels->GetValue())
        param = m_linked_plane_params[0];
    else
        param = plane->GetParam();
	val = fabs(param*resx)+0.499;
	m_x1_clip_sldr->SetValue(val);
	double percent = (double)val/(double)m_x1_clip_sldr->GetMax();
	int barsize = (m_x1_clip_sldr->GetSize().GetHeight() - 20);
	str = wxString::Format("%d", val);
	m_x1_clip_text->ChangeValue(str);
	//x2
	plane = (*planes)[1];
    if (m_link_channels->GetValue())
        param = 1.0 - m_linked_plane_params[1];
    else
        param = 1.0 - plane->GetParam();
    val = fabs(param*resx)+0.499;
	m_x2_clip_sldr->SetValue(val);
	m_xBar->SetPosition(wxPoint(20,10+percent*barsize));
	m_xBar->SetSize(wxSize(3,barsize*((double)
		(val - m_x1_clip_sldr->GetValue())/(double)m_x1_clip_sldr->GetMax())));
	str = wxString::Format("%d", val);
	m_x2_clip_text->ChangeValue(str);
	//y1
	plane = (*planes)[2];
    if (m_link_channels->GetValue())
        param = m_linked_plane_params[2];
    else
        param = plane->GetParam();
    val = fabs(param*resy)+0.499;
	m_y1_clip_sldr->SetValue(val);
	percent = (double)val/(double)m_y1_clip_sldr->GetMax();
	barsize = (m_y1_clip_sldr->GetSize().GetHeight() - 20);
	str = wxString::Format("%d", val);
	m_y1_clip_text->ChangeValue(str);
	//y2
	plane = (*planes)[3];
    if (m_link_channels->GetValue())
        param = 1.0 - m_linked_plane_params[3];
    else
        param = 1.0 - plane->GetParam();
    val = fabs(param*resy)+0.499;
	m_y2_clip_sldr->SetValue(val);
	m_yBar->SetPosition(wxPoint(20,10+percent*barsize));
	m_yBar->SetSize(wxSize(3,barsize*((double)
		(val - m_y1_clip_sldr->GetValue())/(double)m_y1_clip_sldr->GetMax())));
	str = wxString::Format("%d", val);
	m_y2_clip_text->ChangeValue(str);
	//z1
	plane = (*planes)[4];
    if (m_link_channels->GetValue())
        param = m_linked_plane_params[4];
    else
        param = plane->GetParam();
    val = fabs(param*resz)+0.499;
	m_z1_clip_sldr->SetValue(val);
	percent = (double)val/(double)m_z1_clip_sldr->GetMax();
	barsize = (m_z1_clip_sldr->GetSize().GetHeight() - 20);
	str = wxString::Format("%d", val);
	m_z1_clip_text->ChangeValue(str);
	//z2
	plane = (*planes)[5];
    if (m_link_channels->GetValue())
        param = 1.0 - m_linked_plane_params[5];
    else
        param = 1.0 - plane->GetParam();
    val = fabs(param*resz)+0.499;
	m_zBar->SetPosition(wxPoint(20,10+percent*barsize));
	m_zBar->SetSize(wxSize(3,barsize*((double)
		(val - m_z1_clip_sldr->GetValue())/(double)m_z1_clip_sldr->GetMax())));
	m_z2_clip_sldr->SetValue(val);
	str = wxString::Format("%d", val);
	m_z2_clip_text->ChangeValue(str);
    
    VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
    if (vrender_frame)
    {
        for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
        {
            VRenderView *vrv = (*vrender_frame->GetViewList())[i];
            if (vrv)
            {
                double rotx, roty, rotz;
                vrv->GetClippingPlaneRotations(rotx, roty, rotz);
                vrv->SetClippingPlaneRotations(rotx, roty, rotz);
                vrv->RefreshGL();
            }
        }
    }
}

void ClippingView::CalcAndSetCombinedClippingPlanes()
{
    if (!m_mgr)
        return;
    
    VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
    if (vrender_frame)
    {
        for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
        {
            VRenderView *vrv = (*vrender_frame->GetViewList())[i];
            if (vrv)
            {
                vrv->CalcAndSetCombinedClippingPlanes();
            }
        }
    }
    if (m_mgr)
    {
        int i;
        bool spim = false;
        for (i=0; i<m_mgr->GetVolumeNum(); i++)
        {
            VolumeData* vd = m_mgr->GetVolumeData(i);
            if (!vd)
                continue;
            if (vd->GetTexture())
            {
                Transform *tform = vd->GetTexture()->additional_transform();
                if (tform && !tform->is_identity())
                {
                    spim = true;
                    break;
                }
            }
        }
        
        vector<Plane*> *planes = 0;
        if (!spim)
        {
            if (m_sel_type == 2)
            {
                if (m_vd->GetVR())
                    planes = m_vd->GetVR()->get_planes();
            }
            else if (m_sel_type == 3)
            {
                if (m_md->GetMR())
                    planes = m_md->GetMR()->get_planes();
            }
            if(planes && planes->size() == 6)
            {
                for (i = 0; i < 6; i++)
                    m_linked_plane_params[i] = (*planes)[i]->GetParam();
            }
        }

        for (i=0; i<m_mgr->GetVolumeNum(); i++)
        {
            VolumeData* vd = m_mgr->GetVolumeData(i);
            if (!vd)
                continue;
            
            planes = 0;
            if (vd->GetVR())
                planes = vd->GetVR()->get_planes();
            if (!planes)
                continue;
            if (planes->size() != 6)
                continue;
            
            for (int j = 0; j < 6; j++)
                (*planes)[j]->SetParam(m_linked_plane_params[j]);
        }
        for (i=0; i<m_mgr->GetMeshNum(); i++)
        {
            MeshData* md = m_mgr->GetMeshData(i);
            if (!md)
                continue;
            
            planes = 0;
            if (md->GetMR())
                planes = md->GetMR()->get_planes();
            if (!planes)
                continue;
            if (planes->size() != 6)
                continue;
            
            for (int j = 0; j < 6; j++)
                (*planes)[j]->SetParam(m_linked_plane_params[j]);
        }
    }
    GetSettings();
}

void ClippingView::OnLinkChannelsCheck(wxCommandEvent &event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;

	if (m_link_channels->GetValue())
	{
        CalcAndSetCombinedClippingPlanes();
	}
    else
    {
        if (m_mgr)
        {
            int i;
            bool spim = false;
            for (i=0; i<m_mgr->GetVolumeNum(); i++)
            {
                VolumeData* vd = m_mgr->GetVolumeData(i);
                if (!vd)
                    continue;
                if (vd->GetTexture())
                {
                    Transform *tform = vd->GetTexture()->additional_transform();
                    if (tform && !tform->is_identity())
                    {
                        spim = true;
                        break;
                    }
                }
            }
            
            vector<Plane*> *planes;
            for (i=0; i<m_mgr->GetVolumeNum(); i++)
            {
                VolumeData* vd = m_mgr->GetVolumeData(i);
                if (!vd)
                    continue;
                
                planes = 0;
                if (vd->GetVR())
                    planes = vd->GetVR()->get_planes();
                if (!planes)
                    continue;
                if (planes->size() != 6)
                    continue;
                
                Plane* plane = (*planes)[0];
                plane->ChangePlane(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0));
                plane = (*planes)[1];
                plane->ChangePlane(Point(1.0, 0.0, 0.0), Vector(-1.0, 0.0, 0.0));
                plane = (*planes)[2];
                plane->ChangePlane(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0));
                plane = (*planes)[3];
                plane->ChangePlane(Point(0.0, 1.0, 0.0), Vector(0.0, -1.0, 0.0));
                plane = (*planes)[4];
                plane->ChangePlane(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0));
                plane = (*planes)[5];
                plane->ChangePlane(Point(0.0, 0.0, 1.0), Vector(0.0, 0.0, -1.0));
                
                (*planes)[0]->SetRange((*planes)[0]->get_point(), (*planes)[0]->normal(), (*planes)[1]->get_point(), (*planes)[1]->normal());
                (*planes)[1]->SetRange((*planes)[1]->get_point(), (*planes)[1]->normal(), (*planes)[0]->get_point(), (*planes)[0]->normal());
                (*planes)[2]->SetRange((*planes)[2]->get_point(), (*planes)[2]->normal(), (*planes)[3]->get_point(), (*planes)[3]->normal());
                (*planes)[3]->SetRange((*planes)[3]->get_point(), (*planes)[3]->normal(), (*planes)[2]->get_point(), (*planes)[2]->normal());
                (*planes)[4]->SetRange((*planes)[4]->get_point(), (*planes)[4]->normal(), (*planes)[5]->get_point(), (*planes)[5]->normal());
                (*planes)[5]->SetRange((*planes)[5]->get_point(), (*planes)[5]->normal(), (*planes)[4]->get_point(), (*planes)[4]->normal());
                
                    for (auto p : *planes)
                    {
                        if (!spim)
                            p->SetParam(p->GetParam());
                        else
                            p->RestoreParam();
                    }
            }
            for (i=0; i<m_mgr->GetMeshNum(); i++)
            {
                MeshData* md = m_mgr->GetMeshData(i);
                if (!md)
                    continue;
                
                planes = 0;
                if (md->GetMR())
                    planes = md->GetMR()->get_planes();
                if (!planes)
                    continue;
                if (planes->size() != 6)
                    continue;
                
                Plane* plane = (*planes)[0];
                plane->ChangePlane(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0));
                plane = (*planes)[1];
                plane->ChangePlane(Point(1.0, 0.0, 0.0), Vector(-1.0, 0.0, 0.0));
                plane = (*planes)[2];
                plane->ChangePlane(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0));
                plane = (*planes)[3];
                plane->ChangePlane(Point(0.0, 1.0, 0.0), Vector(0.0, -1.0, 0.0));
                plane = (*planes)[4];
                plane->ChangePlane(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0));
                plane = (*planes)[5];
                plane->ChangePlane(Point(0.0, 0.0, 1.0), Vector(0.0, 0.0, -1.0));
                
                (*planes)[0]->SetRange((*planes)[0]->get_point(), (*planes)[0]->normal(), (*planes)[1]->get_point(), (*planes)[1]->normal());
                (*planes)[1]->SetRange((*planes)[1]->get_point(), (*planes)[1]->normal(), (*planes)[0]->get_point(), (*planes)[0]->normal());
                (*planes)[2]->SetRange((*planes)[2]->get_point(), (*planes)[2]->normal(), (*planes)[3]->get_point(), (*planes)[3]->normal());
                (*planes)[3]->SetRange((*planes)[3]->get_point(), (*planes)[3]->normal(), (*planes)[2]->get_point(), (*planes)[2]->normal());
                (*planes)[4]->SetRange((*planes)[4]->get_point(), (*planes)[4]->normal(), (*planes)[5]->get_point(), (*planes)[5]->normal());
                (*planes)[5]->SetRange((*planes)[5]->get_point(), (*planes)[5]->normal(), (*planes)[4]->get_point(), (*planes)[4]->normal());
                
                for (auto p : *planes)
                {
                    if (!spim)
                        p->SetParam(p->GetParam());
                    else
                        p->RestoreParam();
                }
            }
        }
        GetSettings();
    }
    RefreshVRenderViews();
}

void ClippingView::OnPlaneModesCombo(wxCommandEvent &event)
{
	m_plane_mode = m_plane_mode_combo->GetCurrentSelection();
	
	RefreshVRenderViews();
}

void ClippingView::OnClipResetBtn(wxCommandEvent &event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;
	
    int resx, resy, resz;
    double bdw, bdh, bdd;
    bdw = bdh = bdd = 0.0;
    CalcBoundingBoxDemensions(bdw, bdh, bdd);
    resx = round(bdw);
    resy = round(bdh);
    resz = round(bdd);
    
	vector<Plane*> *planes = 0;

	if (m_sel_type == 2)
	{
		if (m_vd->GetVR())
			planes = m_vd->GetVR()->get_planes();
	}
	else if (m_sel_type == 3)
	{
		if (m_md->GetMR())
			planes = m_md->GetMR()->get_planes();
	}

	if(!planes)
		return;
	if (planes->size()!=6)
		return;

	Plane* plane = (*planes)[0];
	plane->ChangePlane(Point(0.0, 0.0, 0.0), Vector(1.0, 0.0, 0.0));
	plane = (*planes)[1];
	plane->ChangePlane(Point(1.0, 0.0, 0.0), Vector(-1.0, 0.0, 0.0));
	plane = (*planes)[2];
	plane->ChangePlane(Point(0.0, 0.0, 0.0), Vector(0.0, 1.0, 0.0));
	plane = (*planes)[3];
	plane->ChangePlane(Point(0.0, 1.0, 0.0), Vector(0.0, -1.0, 0.0));
	plane = (*planes)[4];
	plane->ChangePlane(Point(0.0, 0.0, 0.0), Vector(0.0, 0.0, 1.0));
	plane = (*planes)[5];
	plane->ChangePlane(Point(0.0, 0.0, 1.0), Vector(0.0, 0.0, -1.0));
    
    (*planes)[0]->SetRange((*planes)[0]->get_point(), (*planes)[0]->normal(), (*planes)[1]->get_point(), (*planes)[1]->normal());
    (*planes)[1]->SetRange((*planes)[1]->get_point(), (*planes)[1]->normal(), (*planes)[0]->get_point(), (*planes)[0]->normal());
    (*planes)[2]->SetRange((*planes)[2]->get_point(), (*planes)[2]->normal(), (*planes)[3]->get_point(), (*planes)[3]->normal());
    (*planes)[3]->SetRange((*planes)[3]->get_point(), (*planes)[3]->normal(), (*planes)[2]->get_point(), (*planes)[2]->normal());
    (*planes)[4]->SetRange((*planes)[4]->get_point(), (*planes)[4]->normal(), (*planes)[5]->get_point(), (*planes)[5]->normal());
    (*planes)[5]->SetRange((*planes)[5]->get_point(), (*planes)[5]->normal(), (*planes)[4]->get_point(), (*planes)[4]->normal());
    for (auto p : *planes)
    {
        p->SetParam(0.0);
        p->RememberParam();
    }
    
	//links
	m_link_x = false;
	m_link_y = false;
	m_link_z = false;
	m_link_x_chk->SetValue(false);
	m_link_y_chk->SetValue(false);
	m_link_z_chk->SetValue(false);
    
	//link
	if (m_link_channels->GetValue())
	{
		if (m_mgr)
		{
            VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
            if (vrender_frame)
            {
                for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
                {
                    VRenderView *vrv = (*vrender_frame->GetViewList())[i];
                    if (vrv)
                    {
                        vrv->CalcAndSetCombinedClippingPlanes();
                    }
                }
            }
            for (int i=0; i<m_mgr->GetVolumeNum(); i++)
            {
                VolumeData* vd = m_mgr->GetVolumeData(i);
                if (!vd || vd == m_vd)
                    continue;
                planes = nullptr;
                if (vd->GetVR())
                    planes = vd->GetVR()->get_planes();
                if (!planes)
                    continue;
                for (auto plane : *planes)
                {
                    plane->SetParam(0.0);
                    plane->RememberParam();
                }
                
            }
            for (int i=0; i<m_mgr->GetMeshNum(); i++)
            {
                MeshData* md = m_mgr->GetMeshData(i);
                if (!md || md == m_md)
                    continue;
                
                planes = nullptr;
                if (md->GetMR())
                    planes = md->GetMR()->get_planes();
                if (!planes)
                    continue;
                if (planes->size() != 6)
                    continue;
                for (auto plane : *planes)
                {
                    plane->SetParam(0.0);
                    plane->RememberParam();
                }
            }
		}
        for (int i = 0; i < 6; i++)
            m_linked_plane_params[i] = 0.0;
	}
    
    GetSettings();

	//views
	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				vrv->m_glview->m_clip_mask = -1;

				double rotx, roty, rotz;
				vrv->GetRotations(rotx, roty, rotz);
				vrv->SetRotations(rotx, roty, rotz);

				vrv->RefreshGL();
			}
		}
	}
}

void ClippingView::OnX1ClipChange(wxScrollEvent &event)
{
	int ival = event.GetPosition();
	m_x1_clip_text->SetValue(wxString::Format("%d", ival));
}

void ClippingView::OnX1ClipEdit(wxCommandEvent &event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;
	int resx, resy, resz;
	vector<Plane*> *planes = 0;

	if (m_sel_type == 2)
    {
		if (m_vd->GetVR())
			planes = m_vd->GetVR()->get_planes();
	}
	else if (m_sel_type == 3)
	{
		if (m_md->GetMR())
			planes = m_md->GetMR()->get_planes();
	}
    
    if (!m_x1_clip_sldr || !m_y1_clip_sldr || !m_z1_clip_sldr)
        return;
    resx = m_x1_clip_sldr->GetMax();
    resy = m_y1_clip_sldr->GetMax();
    resz = m_z1_clip_sldr->GetMax();

	if(!planes)
		return;
	if (planes->size()!=6)
		return;

	wxString str = m_x1_clip_text->GetValue();
	long ival = 0;
	str.ToLong(&ival);
	int ival2 = m_x2_clip_sldr->GetValue();
	double val, val2;

	if (m_link_x)
	{
		if (ival + m_x_sldr_dist > resx)
		{
			ival = resx - m_x_sldr_dist;
			ival2 = resx;
		}
		else
			ival2 = ival+m_x_sldr_dist;
	}
	else if (ival > ival2)
		ival = ival2;

	val = (double)ival/(double)resx;
	//str = wxString::Format("%d", ival);
	//m_x1_clip_text->ChangeValue(str);
	m_x1_clip_sldr->SetValue(ival);
	int barsize = (m_x1_clip_sldr->GetSize().GetHeight() - 20);
	m_xBar->SetPosition(wxPoint(20,10+val*barsize));
	m_xBar->SetSize(wxSize(3,barsize*((double)
		(m_x2_clip_sldr->GetValue()-ival)/(double)m_x1_clip_sldr->GetMax())));
	
    Plane* plane = (*planes)[0];
    if (m_link_channels->GetValue())
        m_linked_plane_params[0] = val;
    plane->SetParam(val);
    
	if (m_link_x)
	{
		val2 = 1.0 - (double)ival2/(double)resx;
		str = wxString::Format("%d", ival2);
		m_x2_clip_text->ChangeValue(str);
		m_x2_clip_sldr->SetValue(ival2);
		
        plane = (*planes)[1];
        if (m_link_channels->GetValue())
            m_linked_plane_params[1] = val2;
        plane->SetParam(val2);
	}

	if (m_link_channels->GetValue())
	{
		if (m_mgr)
		{
			int i;
			for (i=0; i<m_mgr->GetVolumeNum(); i++)
			{
				VolumeData* vd = m_mgr->GetVolumeData(i);
				if (!vd || vd == m_vd)
					continue;

				planes = 0;
				if (vd->GetVR())
					planes = vd->GetVR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;

                (*planes)[0]->SetParam(val);
				if (m_link_x)
					(*planes)[1]->SetParam(val2);
			}
			for (i=0; i<m_mgr->GetMeshNum(); i++)
			{
				MeshData* md = m_mgr->GetMeshData(i);
				if (!md || md == m_md)
					continue;

				planes = 0;
				if (md->GetMR())
					planes = md->GetMR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;

                (*planes)[0]->SetParam(val);
                if (m_link_x)
                    (*planes)[1]->SetParam(val2);
			}
		}
	}

	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				if (m_link_x)
					vrv->m_glview->m_clip_mask = 3;
				else
					vrv->m_glview->m_clip_mask = 1;

				double rotx, roty, rotz;
				vrv->GetRotations(rotx, roty, rotz);
				vrv->SetRotations(rotx, roty, rotz);
			}
		}
	}
	RefreshVRenderViews(true);
}

void ClippingView::OnX2ClipChange(wxScrollEvent &event)
{
	int ival = event.GetPosition();
	int ival2 = m_x1_clip_sldr->GetValue();

	if (!m_link_x && ival<ival2)
	{
		ival = ival2;
		m_x2_clip_sldr->SetValue(ival);
	}
	m_x2_clip_text->SetValue(wxString::Format("%d", ival));
}

void ClippingView::OnX2ClipEdit(wxCommandEvent &event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;
	int resx, resy, resz;
	vector<Plane*> *planes = 0;

	if (m_sel_type == 2)
	{
		if (m_vd->GetVR())
			planes = m_vd->GetVR()->get_planes();
	}
	else if (m_sel_type == 3)
	{
		if (m_md->GetMR())
			planes = m_md->GetMR()->get_planes();
	}
    
    if (!m_x1_clip_sldr || !m_y1_clip_sldr || !m_z1_clip_sldr)
        return;
    resx = m_x1_clip_sldr->GetMax();
    resy = m_y1_clip_sldr->GetMax();
    resz = m_z1_clip_sldr->GetMax();

	if (!planes)
		return;
	if (planes->size()!=6)
		return;

	wxString str = m_x2_clip_text->GetValue();
	long ival = 0;
	str.ToLong(&ival);
	int ival2 = m_x1_clip_sldr->GetValue();
	double val, val2;

	if (m_link_x)
	{
		if (ival - m_x_sldr_dist < 0)
		{
			ival = m_x_sldr_dist;
			ival2 = 0;
		}
		else
			ival2 = ival - m_x_sldr_dist;
	}
	else if (ival < ival2)
		return;

	val = 1.0 - (double)ival/(double)resx;
	//str = wxString::Format("%d", ival);
	//m_x2_clip_text->ChangeValue(str);
	m_x2_clip_sldr->SetValue(ival);
	int barsize = (m_x1_clip_sldr->GetSize().GetHeight() - 20);
	m_xBar->SetPosition(wxPoint(20,10+((double)m_x1_clip_sldr->GetValue()/
		(double)m_x1_clip_sldr->GetMax())*barsize));
	m_xBar->SetSize(wxSize(3,barsize*((double)
		(ival - m_x1_clip_sldr->GetValue())/(double)m_x1_clip_sldr->GetMax())));
	
    Plane* plane = (*planes)[1];
    if (m_link_channels->GetValue())
        m_linked_plane_params[1] = val;
    plane->SetParam(val);
    
	if (m_link_x)
	{
		val2 = (double)ival2/(double)resx;
		str = wxString::Format("%d", ival2);
		m_x1_clip_text->ChangeValue(str);
		m_x1_clip_sldr->SetValue(ival2);
		
        plane = (*planes)[0];
        if (m_link_channels->GetValue())
            m_linked_plane_params[0] = val2;
        plane->SetParam(val2);
	}

	if (m_link_channels->GetValue())
	{
		if (m_mgr)
		{
			int i;
			for (i=0; i<m_mgr->GetVolumeNum(); i++)
			{
				VolumeData* vd = m_mgr->GetVolumeData(i);
				if (!vd || vd == m_vd)
					continue;

				planes = 0;
				if (vd->GetVR())
					planes = vd->GetVR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;
                
                (*planes)[1]->SetParam(val);
                if (m_link_x)
                    (*planes)[0]->SetParam(val2);
			}
			for (i=0; i<m_mgr->GetMeshNum(); i++)
			{
				MeshData* md = m_mgr->GetMeshData(i);
				if (!md || md == m_md)
					continue;

				planes = 0;
				if (md->GetMR())
					planes = md->GetMR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;
                
                (*planes)[1]->SetParam(val);
                if (m_link_x)
                    (*planes)[0]->SetParam(val2);
			}
		}
	}

	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				if (m_link_x)
					vrv->m_glview->m_clip_mask = 3;
				else
					vrv->m_glview->m_clip_mask = 2;

				double rotx, roty, rotz;
				vrv->GetRotations(rotx, roty, rotz);
				vrv->SetRotations(rotx, roty, rotz);
			}
		}
	}

	RefreshVRenderViews(true);
}

void ClippingView::OnY1ClipChange(wxScrollEvent &event)
{
	int ival = event.GetPosition();
	m_y1_clip_text->SetValue(wxString::Format("%d", ival));
}

void ClippingView::OnY1ClipEdit(wxCommandEvent &event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;
	int resx, resy, resz;
	vector<Plane*> *planes = 0;

	if (m_sel_type == 2)
	{
		if (m_vd->GetVR())
			planes = m_vd->GetVR()->get_planes();
	}
	else if (m_sel_type == 3)
	{
		if (m_md->GetMR())
			planes = m_md->GetMR()->get_planes();
	}
    
    if (!m_x1_clip_sldr || !m_y1_clip_sldr || !m_z1_clip_sldr)
        return;
    resx = m_x1_clip_sldr->GetMax();
    resy = m_y1_clip_sldr->GetMax();
    resz = m_z1_clip_sldr->GetMax();

	if (!planes)
		return;
	if (planes->size()!=6)
		return;

	wxString str = m_y1_clip_text->GetValue();
	long ival = 0;
	str.ToLong(&ival);
	int ival2 = m_y2_clip_sldr->GetValue();
	double val, val2;

	if (m_link_y)
	{
		if (ival + m_y_sldr_dist >resy)
		{
			ival = resy - m_y_sldr_dist;
			ival2 = resy;
		}
		else
			ival2 = ival+m_y_sldr_dist;
	}
	else if (ival > ival2)
		ival = ival2;

	val = (double)ival/(double)resy;
	//str = wxString::Format("%d", ival);
	//m_y1_clip_text->ChangeValue(str);
	m_y1_clip_sldr->SetValue(ival);
	int barsize = (m_y1_clip_sldr->GetSize().GetHeight() - 20);
	m_yBar->SetPosition(wxPoint(20,10+val*barsize));
	m_yBar->SetSize(wxSize(3,barsize*((double)
		(m_y2_clip_sldr->GetValue()-ival)/(double)m_y1_clip_sldr->GetMax())));
	
    Plane* plane = (*planes)[2];
    if (m_link_channels->GetValue())
        m_linked_plane_params[2] = val;
    plane->SetParam(val);
    
	if (m_link_y)
	{
		val2 = 1.0 - (double)ival2/(double)resy;
		str = wxString::Format("%d", ival2);
		m_y2_clip_text->ChangeValue(str);
		m_y2_clip_sldr->SetValue(ival2);
		
        plane = (*planes)[3];
        if (m_link_channels->GetValue())
            m_linked_plane_params[3] = val2;
        plane->SetParam(val2);
	}

	if (m_link_channels->GetValue())
	{
		if (m_mgr)
		{
			int i;
			for (i=0; i<m_mgr->GetVolumeNum(); i++)
			{
				VolumeData* vd = m_mgr->GetVolumeData(i);
				if (!vd || vd == m_vd)
					continue;

				planes = 0;
				if (vd->GetVR())
					planes = vd->GetVR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;

                (*planes)[2]->SetParam(val);
                if (m_link_y)
                    (*planes)[3]->SetParam(val2);
			}
			for (i=0; i<m_mgr->GetMeshNum(); i++)
			{
				MeshData* md = m_mgr->GetMeshData(i);
				if (!md || md == m_md)
					continue;

				planes = 0;
				if (md->GetMR())
					planes = md->GetMR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;

                (*planes)[2]->SetParam(val);
                if (m_link_y)
                    (*planes)[3]->SetParam(val2);
			}
		}
	}

	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				if (m_link_y)
					vrv->m_glview->m_clip_mask = 12;
				else
					vrv->m_glview->m_clip_mask = 4;

				double rotx, roty, rotz;
				vrv->GetRotations(rotx, roty, rotz);
				vrv->SetRotations(rotx, roty, rotz);
			}
		}
	}

	RefreshVRenderViews(true);
}

void ClippingView::OnY2ClipChange(wxScrollEvent &event)
{
	int ival = event.GetPosition();
	int ival2 = m_y1_clip_sldr->GetValue();

	if (!m_link_y && ival<ival2)
	{
		ival = ival2;
		m_y2_clip_sldr->SetValue(ival);
	}
	m_y2_clip_text->SetValue(wxString::Format("%d", ival));
}

void ClippingView::OnY2ClipEdit(wxCommandEvent &event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;
	int resx, resy, resz;
	vector<Plane*> *planes = 0;

	if (m_sel_type == 2)
	{
		if (m_vd->GetVR())
			planes = m_vd->GetVR()->get_planes();
	}
	else if (m_sel_type == 3)
	{
		if (m_md->GetMR())
			planes = m_md->GetMR()->get_planes();
	}
    
    if (!m_x1_clip_sldr || !m_y1_clip_sldr || !m_z1_clip_sldr)
        return;
    resx = m_x1_clip_sldr->GetMax();
    resy = m_y1_clip_sldr->GetMax();
    resz = m_z1_clip_sldr->GetMax();

	if (!planes)
		return;
	if (planes->size()!=6)
		return;

	wxString str = m_y2_clip_text->GetValue();
	long ival = 0;
	str.ToLong(&ival);
	int ival2 = m_y1_clip_sldr->GetValue();
	double val, val2;

	if (m_link_y)
	{
		if (ival - m_y_sldr_dist < 0)
		{
			ival = m_y_sldr_dist;
			ival2 = 0;
		}
		else
			ival2 = ival - m_y_sldr_dist;
	}
	else if (ival < ival2)
		return;
	
	val = 1.0 - (double)ival/(double)resy;
	//str = wxString::Format("%d", ival);
	//m_y2_clip_text->ChangeValue(str);
	m_y2_clip_sldr->SetValue(ival);
	int barsize = (m_y1_clip_sldr->GetSize().GetHeight() - 20);
	m_yBar->SetPosition(wxPoint(20,10+((double)m_y1_clip_sldr->GetValue()/
		(double)m_y1_clip_sldr->GetMax())*barsize));
	m_yBar->SetSize(wxSize(3,barsize*((double)
		(ival - m_y1_clip_sldr->GetValue())/(double)m_y1_clip_sldr->GetMax())));
	
    Plane* plane = (*planes)[3];
    if (m_link_channels->GetValue())
        m_linked_plane_params[3] = val;
    plane->SetParam(val);
    
	if (m_link_y)
	{
		val2 = (double)ival2/(double)resy;
		str = wxString::Format("%d", ival2);
		m_y1_clip_text->ChangeValue(str);
		m_y1_clip_sldr->SetValue(ival2);
		
        plane = (*planes)[2];
        if (m_link_channels->GetValue())
            m_linked_plane_params[2] = val2;
        plane->SetParam(val2);
	}

	if (m_link_channels->GetValue())
	{
		if (m_mgr)
		{
			int i;
			for (i=0; i<m_mgr->GetVolumeNum(); i++)
			{
				VolumeData* vd = m_mgr->GetVolumeData(i);
				if (!vd || vd == m_vd)
					continue;

				planes = 0;
				if (vd->GetVR())
					planes = vd->GetVR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;

                (*planes)[3]->SetParam(val);
                if (m_link_y)
                    (*planes)[2]->SetParam(val2);
			}
			for (i=0; i<m_mgr->GetMeshNum(); i++)
			{
				MeshData* md = m_mgr->GetMeshData(i);
				if (!md || md == m_md)
					continue;

				planes = 0;
				if (md->GetMR())
					planes = md->GetMR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;

                (*planes)[3]->SetParam(val);
                if (m_link_y)
                    (*planes)[2]->SetParam(val2);
			}
		}
	}

	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				if (m_link_y)
					vrv->m_glview->m_clip_mask = 12;
				else
					vrv->m_glview->m_clip_mask = 8;

				double rotx, roty, rotz;
				vrv->GetRotations(rotx, roty, rotz);
				vrv->SetRotations(rotx, roty, rotz);
			}
		}
	}

	RefreshVRenderViews(true);
}

void ClippingView::OnZ1ClipChange(wxScrollEvent &event)
{
	int ival = event.GetPosition();
	m_z1_clip_text->SetValue(wxString::Format("%d", ival));
}

void ClippingView::OnZ1ClipEdit(wxCommandEvent &event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;
	int resx, resy, resz;
	vector<Plane*> *planes = 0;

	if (m_sel_type == 2)
	{
		if (m_vd->GetVR())
			planes = m_vd->GetVR()->get_planes();
	}
	else if (m_sel_type == 3)
	{
		if (m_md->GetMR())
			planes = m_md->GetMR()->get_planes();
	}
    
    if (!m_x1_clip_sldr || !m_y1_clip_sldr || !m_z1_clip_sldr)
        return;
    resx = m_x1_clip_sldr->GetMax();
    resy = m_y1_clip_sldr->GetMax();
    resz = m_z1_clip_sldr->GetMax();

	if (!planes)
		return;
	if (planes->size()!=6)
		return;

	wxString str = m_z1_clip_text->GetValue();
	long ival = 0;
	str.ToLong(&ival);
	int ival2 = m_z2_clip_sldr->GetValue();
	double val, val2;

	if (m_link_z)
	{
		if (ival + m_z_sldr_dist > resz)
		{
			ival = resz - m_z_sldr_dist;
			ival2 = resz;
		}
		else
			ival2 = ival+m_z_sldr_dist;
	}
	else if (ival > ival2)
		ival = ival2;
	
	val = (double)ival/(double)resz;
	//str = wxString::Format("%d", ival);
	//m_z1_clip_text->ChangeValue(str);
	m_z1_clip_sldr->SetValue(ival);
	int barsize = (m_z1_clip_sldr->GetSize().GetHeight() - 20);
	m_zBar->SetPosition(wxPoint(20,10+val*barsize));
	m_zBar->SetSize(wxSize(3,barsize*((double)
		(m_z2_clip_sldr->GetValue()-ival)/(double)m_z1_clip_sldr->GetMax())));
	
    Plane* plane = (*planes)[4];
    if (m_link_channels->GetValue())
        m_linked_plane_params[4] = val;
    plane->SetParam(val);
    
	if (m_link_z)
	{
		val2 = 1.0 - (double)ival2/(double)resz;
		str = wxString::Format("%d", ival2);
		m_z2_clip_text->ChangeValue(str);
		m_z2_clip_sldr->SetValue(ival2);
		
        plane = (*planes)[5];
        if (m_link_channels->GetValue())
            m_linked_plane_params[5] = val2;
        plane->SetParam(val2);
	}

	if (m_link_channels->GetValue())
	{
		if (m_mgr)
		{
			int i;
			for (i=0; i<m_mgr->GetVolumeNum(); i++)
			{
				VolumeData* vd = m_mgr->GetVolumeData(i);
				if (!vd || vd == m_vd)
					continue;

				planes = 0;
				if (vd->GetVR())
					planes = vd->GetVR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;

                (*planes)[4]->SetParam(val);
                if (m_link_z)
                    (*planes)[5]->SetParam(val2);
			}
			for (i=0; i<m_mgr->GetMeshNum(); i++)
			{
				MeshData* md = m_mgr->GetMeshData(i);
				if (!md || md == m_md)
					continue;

				planes = 0;
				if (md->GetMR())
					planes = md->GetMR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;

                (*planes)[4]->SetParam(val);
                if (m_link_z)
                    (*planes)[5]->SetParam(val2);
			}
		}
	}

	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				if (m_link_z)
					vrv->m_glview->m_clip_mask = 48;
				else
					vrv->m_glview->m_clip_mask = 16;

				double rotx, roty, rotz;
				vrv->GetRotations(rotx, roty, rotz);
				vrv->SetRotations(rotx, roty, rotz);
			}
		}
	}

	RefreshVRenderViews(true);
}

void ClippingView::OnZ2ClipChange(wxScrollEvent &event)
{
	int ival = event.GetPosition();
	int ival2 = m_z1_clip_sldr->GetValue();

	if (!m_link_z && ival<ival2)
	{
		ival = ival2;
		m_z2_clip_sldr->SetValue(ival);
	}
	m_z2_clip_text->SetValue(wxString::Format("%d", ival));
}

void ClippingView::OnZ2ClipEdit(wxCommandEvent &event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;
	int resx, resy, resz;
	vector<Plane*> *planes = 0;

	if (m_sel_type == 2)
	{
		if (m_vd->GetVR())
			planes = m_vd->GetVR()->get_planes();
	}
	else if (m_sel_type == 3)
	{
		if (m_md->GetMR())
			planes = m_md->GetMR()->get_planes();
	}
    
    if (!m_x1_clip_sldr || !m_y1_clip_sldr || !m_z1_clip_sldr)
        return;
    resx = m_x1_clip_sldr->GetMax();
    resy = m_y1_clip_sldr->GetMax();
    resz = m_z1_clip_sldr->GetMax();

	if (!planes)
		return;
	if (planes->size()!=6)
		return;

	wxString str = m_z2_clip_text->GetValue();
	long ival = 0;
	str.ToLong(&ival);
	int ival2 = m_z1_clip_sldr->GetValue();
	double val, val2;

	if (m_link_z)
	{
		if (ival - m_z_sldr_dist < 0)
		{
			ival = m_z_sldr_dist;
			ival2 = 0;
		}
		else
			ival2 = ival - m_z_sldr_dist;
	}
	else if (ival < ival2)
		return;
	
	val = 1.0 - (double)ival/(double)resz;
	//str = wxString::Format("%d", ival);
	//m_z2_clip_text->ChangeValue(str);
	m_z2_clip_sldr->SetValue(ival);
	int barsize = (m_z1_clip_sldr->GetSize().GetHeight() - 20);
	m_zBar->SetPosition(wxPoint(20,10+((double)m_z1_clip_sldr->GetValue()/
		(double)m_z1_clip_sldr->GetMax())*barsize));
	m_zBar->SetSize(wxSize(3,barsize*((double)
		(ival - m_z1_clip_sldr->GetValue())/(double)m_z1_clip_sldr->GetMax())));
	
    Plane* plane = (*planes)[5];
    if (m_link_channels->GetValue())
        m_linked_plane_params[5] = val;
    plane->SetParam(val);
    
	if (m_link_z)
	{
		val2 = (double)ival2/(double)resz;
		str = wxString::Format("%d", ival2);
		m_z1_clip_text->ChangeValue(str);
		m_z1_clip_sldr->SetValue(ival2);
		
        plane = (*planes)[4];
        if (m_link_channels->GetValue())
            m_linked_plane_params[4] = val2;
        plane->SetParam(val2);
        
	}

	if (m_link_channels->GetValue())
	{
		if (m_mgr)
		{
			int i;
			for (i=0; i<m_mgr->GetVolumeNum(); i++)
			{
				VolumeData* vd = m_mgr->GetVolumeData(i);
				if (!vd || vd == m_vd)
					continue;

				planes = 0;
				if (vd->GetVR())
					planes = vd->GetVR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;

                (*planes)[5]->SetParam(val);
                if (m_link_z)
                    (*planes)[4]->SetParam(val2);
			}
			for (i=0; i<m_mgr->GetMeshNum(); i++)
			{
				MeshData* md = m_mgr->GetMeshData(i);
				if (!md || md == m_md)
					continue;

				planes = 0;
				if (md->GetMR())
					planes = md->GetMR()->get_planes();
				if (!planes)
					continue;
				if (planes->size() != 6)
					continue;

                (*planes)[5]->SetParam(val);
                if (m_link_z)
                    (*planes)[4]->SetParam(val2);
			}
		}
	}

	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				if (m_link_z)
					vrv->m_glview->m_clip_mask = 48;
				else
					vrv->m_glview->m_clip_mask = 32;

				double rotx, roty, rotz;
				vrv->GetRotations(rotx, roty, rotz);
				vrv->SetRotations(rotx, roty, rotz);
			}
		}
	}

	RefreshVRenderViews(true);
}

void ClippingView::OnIdle(wxTimerEvent& event)
{
	if (!IsShown())
		return;
	int sz = m_xpanel->GetSize().GetHeight();
	if (m_x1_clip_sldr->GetSize().GetHeight() != sz) {
		m_x1_clip_sldr->SetSize(20,sz);
		m_x2_clip_sldr->SetSize(20,sz);
		m_y1_clip_sldr->SetSize(20,sz);
		m_y2_clip_sldr->SetSize(20,sz);
		m_z1_clip_sldr->SetSize(20,sz);
		m_z2_clip_sldr->SetSize(20,sz);
		int barsize = sz - 20;
		//x
		int mx = m_x1_clip_sldr->GetMax();
		int v1 = m_x1_clip_sldr->GetValue();
		int v2 = m_x2_clip_sldr->GetValue();
		double clipSz = ((double)(v2 - v1))/((double)mx);
		double pct = ((double)v1)/((double)mx);
		m_xBar->SetPosition(wxPoint(20,10+pct*barsize));
		m_xBar->SetSize(wxSize(3,barsize*clipSz));
		//y
		mx = m_y1_clip_sldr->GetMax();
		v1 = m_y1_clip_sldr->GetValue();
		v2 = m_y2_clip_sldr->GetValue();
		clipSz = ((double)(v2 - v1))/((double)mx);
		pct = ((double)v1)/((double)mx);
		m_yBar->SetPosition(wxPoint(20,10+pct*barsize));
		m_yBar->SetSize(wxSize(3,barsize*clipSz));
		//z
		mx = m_z1_clip_sldr->GetMax();
		v1 = m_z1_clip_sldr->GetValue();
		v2 = m_z2_clip_sldr->GetValue();
		clipSz = ((double)(v2 - v1))/((double)mx);
		pct = ((double)v1)/((double)mx);
		m_zBar->SetPosition(wxPoint(20,10+pct*barsize));
		m_zBar->SetSize(wxSize(3,barsize*clipSz));
	}

	int i;
	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (!vrender_frame)
		return;

	for (i=0; i<vrender_frame->GetViewNum(); i++)
	{
		VRenderView *vrv = vrender_frame->GetView(i);
		if (vrv)
		{
			if (vrv->m_glview->m_capture)
				return;
		}
	}

	wxPoint pos = wxGetMousePosition();
	wxRect reg = GetScreenRect();
	wxWindow *window = wxWindow::FindFocus();
    
    if (m_hold_planes)
    {
        m_draw_clip = true;
        
        if (window && reg.Contains(pos))
        {
            if (!m_mouse_in)
            {
                vector <VRenderView*>* vrv_list = vrender_frame->GetViewList();
                for (i=0; i<(int)vrv_list->size(); i++)
                {
                    if ((*vrv_list)[i])
                        (*vrv_list)[i]->m_glview->m_clip_mask = -1;
                }
                if (m_plane_mode == PM_LOWTRANSBACK || m_plane_mode == PM_NORMALBACK)
                    RefreshVRenderViews();
                else
                    RefreshVRenderViewsOverlay();
            }
            m_mouse_in = true;
        }
        else
            m_mouse_in = false;
        
        return;
    }
    
	if (window && reg.Contains(pos))
	{
		if (!m_draw_clip)
		{
			vector <VRenderView*>* vrv_list = vrender_frame->GetViewList();
			for (i=0; i<(int)vrv_list->size(); i++)
			{
				if ((*vrv_list)[i])
				{
					(*vrv_list)[i]->m_glview->m_draw_clip = true;
					(*vrv_list)[i]->m_glview->m_clip_mask = -1;
				}
			}
			if (m_plane_mode == PM_LOWTRANSBACK || m_plane_mode == PM_NORMALBACK)
				RefreshVRenderViews();
			else 
				RefreshVRenderViewsOverlay();
			m_draw_clip = true;
		}
        m_mouse_in = true;
	}
	else
	{
		if (m_draw_clip && !m_hold_planes)
		{
			vector <VRenderView*>* vrv_list = vrender_frame->GetViewList();
			for (i=0; i<(int)vrv_list->size(); i++)
			{
				if ((*vrv_list)[i])
					(*vrv_list)[i]->m_glview->m_draw_clip = false;
			}
			if (m_plane_mode == PM_LOWTRANSBACK || m_plane_mode == PM_NORMALBACK)
				RefreshVRenderViews();
			else 
				RefreshVRenderViewsOverlay();
			m_draw_clip = false;
            m_mouse_in = false;
		}
	}

}

void ClippingView::OnLinkXCheck(wxCommandEvent &event)
{
	m_link_x = m_link_x_chk->GetValue();
	if (m_link_x)
	{
		m_x_sldr_dist = m_x2_clip_sldr->GetValue() -
			m_x1_clip_sldr->GetValue();
	}
}

void ClippingView::OnLinkYCheck(wxCommandEvent &event)
{
	m_link_y = m_link_y_chk->GetValue();
	if (m_link_y)
	{
		m_y_sldr_dist = m_y2_clip_sldr->GetValue() -
			m_y1_clip_sldr->GetValue();
	}
}

void ClippingView::OnLinkZCheck(wxCommandEvent &event)
{
	m_link_z = m_link_z_chk->GetValue();
	if (m_link_z)
	{
		m_z_sldr_dist = m_z2_clip_sldr->GetValue() -
			m_z1_clip_sldr->GetValue();
	}
}

void ClippingView::OnSetZeroBtn(wxCommandEvent &event)
{
	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				vrv->SetClipMode(2);
				vrv->RefreshGL();
				double rotx, roty, rotz;
				vrv->GetClippingPlaneRotations(rotx, roty, rotz);
				m_x_rot_sldr->SetValue(int(rotx));
				m_y_rot_sldr->SetValue(int(roty));
				m_z_rot_sldr->SetValue(int(rotz));
				m_x_rot_text->ChangeValue(wxString::Format("%.1f", rotx));
				m_y_rot_text->ChangeValue(wxString::Format("%.1f", roty));
				m_z_rot_text->ChangeValue(wxString::Format("%.1f", rotz));
			}
		}
	}
}

void ClippingView::OnRotResetBtn(wxCommandEvent &event)
{
	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				//reset rotations
				vrv->SetClippingPlaneRotations(0.0, 0.0, 0.0);
				vrv->RefreshGL();
			}
		}
	}
	wxString str = "0.0";
	m_x_rot_sldr->SetValue(0);
	m_x_rot_text->ChangeValue(str);
	m_y_rot_sldr->SetValue(0);
	m_y_rot_text->ChangeValue(str);
	m_z_rot_sldr->SetValue(0);
	m_z_rot_text->ChangeValue(str);
}

void ClippingView::OnXRotChange(wxScrollEvent &event)
{
	int val = event.GetPosition();
	wxString str = wxString::Format("%.1f", double(val));
	m_x_rot_text->SetValue(str);
}

void ClippingView::OnXRotEdit(wxCommandEvent &event)
{
	wxString str = m_x_rot_text->GetValue();
	double val = 0.0;
	str.ToDouble(&val);
	m_x_rot_sldr->SetValue(int(val));

	if (m_update_only_ui)
		return;

	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				double rotx, roty, rotz;
				vrv->GetClippingPlaneRotations(rotx, roty, rotz);
				vrv->SetClippingPlaneRotations(val, roty, rotz);
				vrv->RefreshGL();
			}
		}
	}
}

void ClippingView::OnYRotChange(wxScrollEvent &event)
{
	int val = event.GetPosition();
	wxString str = wxString::Format("%.1f", double(val));
	m_y_rot_text->SetValue(str);
}

void ClippingView::OnYRotEdit(wxCommandEvent &event)
{
	wxString str = m_y_rot_text->GetValue();
	double val = 0.0;
	str.ToDouble(&val);
	m_y_rot_sldr->SetValue(int(val));

	if (m_update_only_ui)
		return;

	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				double rotx, roty, rotz;
				vrv->GetClippingPlaneRotations(rotx, roty, rotz);
				vrv->SetClippingPlaneRotations(rotx, val, rotz);
				vrv->RefreshGL();
			}
		}
	}
}

void ClippingView::OnZRotChange(wxScrollEvent &event)
{
	int val = event.GetPosition();
	wxString str = wxString::Format("%.1f", double(val));
	m_z_rot_text->SetValue(str);
}

void ClippingView::OnZRotEdit(wxCommandEvent &event)
{
	wxString str = m_z_rot_text->GetValue();
	double val = 0.0;
	str.ToDouble(&val);
	m_z_rot_sldr->SetValue(int(val));

	if (m_update_only_ui)
		return;

	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
	{
		for (int i=0; i<(int)vrender_frame->GetViewList()->size(); i++)
		{
			VRenderView *vrv = (*vrender_frame->GetViewList())[i];
			if (vrv)
			{
				double rotx, roty, rotz;
				vrv->GetClippingPlaneRotations(rotx, roty, rotz);
				vrv->SetClippingPlaneRotations(rotx, roty, val);
				vrv->RefreshGL();
			}
		}
	}
}

void ClippingView::OnXRotSpinUp(wxSpinEvent& event)
{
	wxString str_val = m_x_rot_text->GetValue();
	double val = STOD(str_val.fn_str()) + 1.0;
	if (val > 180.0) val -= 360.0;
	if (val <-180.0) val += 360.0;
	wxString str = wxString::Format("%.1f", val);
	m_x_rot_text->SetValue(str);
}

void ClippingView::OnXRotSpinDown(wxSpinEvent& event)
{
	wxString str_val = m_x_rot_text->GetValue();
	double val = STOD(str_val.fn_str()) - 1.0;
	if (val > 180.0) val -= 360.0;
	if (val <-180.0) val += 360.0;
	wxString str = wxString::Format("%.1f", val);
	m_x_rot_text->SetValue(str);
}

void ClippingView::OnYRotSpinUp(wxSpinEvent& event)
{
	wxString str_val = m_y_rot_text->GetValue();
	double val = STOD(str_val.fn_str()) + 1.0;
	if (val > 180.0) val -= 360.0;
	if (val <-180.0) val += 360.0;
	wxString str = wxString::Format("%.1f", val);
	m_y_rot_text->SetValue(str);
}

void ClippingView::OnYRotSpinDown(wxSpinEvent& event)
{
	wxString str_val = m_y_rot_text->GetValue();
	double val = STOD(str_val.fn_str()) - 1.0;
	if (val > 180.0) val -= 360.0;
	if (val <-180.0) val += 360.0;
	wxString str = wxString::Format("%.1f", val);
	m_y_rot_text->SetValue(str);
}

void ClippingView::OnZRotSpinUp(wxSpinEvent& event)
{
	wxString str_val = m_z_rot_text->GetValue();
	double val = STOD(str_val.fn_str()) + 1.0;
	if (val > 180.0) val -= 360.0;
	if (val <-180.0) val += 360.0;
	wxString str = wxString::Format("%.1f", val);
	m_z_rot_text->SetValue(str);
}

void ClippingView::OnZRotSpinDown(wxSpinEvent& event)
{
	wxString str_val = m_z_rot_text->GetValue();
	double val = STOD(str_val.fn_str()) - 1.0;
	if (val > 180.0) val -= 360.0;
	if (val <-180.0) val += 360.0;
	wxString str = wxString::Format("%.1f", val);
	m_z_rot_text->SetValue(str);
}

void ClippingView::OnSliderRClick(wxCommandEvent& event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;

    int resx, resy, resz;
    double bdw, bdh, bdd;
    bdw = bdh = bdd = 0.0;
    CalcBoundingBoxDemensions(bdw, bdh, bdd);
    resx = round(bdw);
    resy = round(bdh);
    resz = round(bdd);
    
    vector<Plane*> *planes = 0;
    if (m_sel_type == 2)
    {
        if (m_vd->GetVR())
            planes = m_vd->GetVR()->get_planes();
    }
    else if (m_sel_type == 3)
    {
        if (m_md->GetMR())
            planes = m_md->GetMR()->get_planes();
    }

	int id = event.GetId();

	wxString str;
	double val;

	//good rate
/*	if (m_vd->GetSampleRate()<2.0)
		m_vd->SetSampleRate(2.0);
	if (m_link_channels->GetValue())
	{
		if (m_mgr)
		{
			int i;
			for (i=0; i<m_mgr->GetVolumeNum(); i++)
			{
				VolumeData* vd = m_mgr->GetVolumeData(i);
				if (!vd || vd == m_vd)
					continue;
				if (vd->GetSampleRate()<2.0)
					vd->SetSampleRate(2.0);
			}
		}
	}
*/
	
	if (id == ID_X1ClipSldr)
	{
		str = m_x1_clip_text->GetValue();
		str.ToDouble(&val);
		if (val < resx)
		{
			SetXLink(false);
			m_x2_clip_text->SetValue(
				wxString::Format("%d", (int)(val+1.0)));
			SetXLink(true);
		}
		m_x1_clip_sldr->SetFocus();
	}
	else if (id == ID_X2ClipSldr)
	{
		str = m_x2_clip_text->GetValue();
		str.ToDouble(&val);
		if (val > 0)
		{
			SetXLink(false);
			m_x1_clip_text->SetValue(
				wxString::Format("%d", (int)(val-1.0)));
			SetXLink(true);
		}
		m_x2_clip_sldr->SetFocus();
	}
	else if (id == ID_Y1ClipSldr)
	{
		str = m_y1_clip_text->GetValue();
		str.ToDouble(&val);
		if (val < resy)
		{
			SetYLink(false);
			m_y2_clip_text->SetValue(
				wxString::Format("%d", (int)(val+1.0)));
			SetYLink(true);
		}
		m_y1_clip_sldr->SetFocus();
	}
	else if (id == ID_Y2ClipSldr)
	{
		str = m_y2_clip_text->GetValue();
		str.ToDouble(&val);
		if (val > 0)
		{
			SetYLink(false);
			m_y1_clip_text->SetValue(
				wxString::Format("%d", (int)(val-1.0)));
			SetYLink(true);
		}
		m_y2_clip_sldr->SetFocus();
	}
	else if (id == ID_Z1ClipSldr)
	{
		str = m_z1_clip_text->GetValue();
		str.ToDouble(&val);
		if (val < resz)
		{
			SetZLink(false);
			m_z2_clip_text->SetValue(
				wxString::Format("%d", (int)(val+1.0)));
			SetZLink(true);
		}
		m_z1_clip_sldr->SetFocus();
	}
	else if (id == ID_Z2ClipSldr)
	{
		str = m_z2_clip_text->GetValue();
		str.ToDouble(&val);
		if (val > 0)
		{
			SetZLink(false);
			m_z1_clip_text->SetValue(
				wxString::Format("%d", (int)(val-1.0)));
			SetZLink(true);
		}
		m_z2_clip_sldr->SetFocus();
	}

	//reset others
	if(!planes)
		return;
	if (planes->size()!=6)
		return;

	if (id == ID_X1ClipSldr ||
		id == ID_X2ClipSldr)
	{
		m_link_y = false;
		m_link_y_chk->SetValue(false);
		m_link_z = false;
		m_link_z_chk->SetValue(false);
		m_y1_clip_text->SetValue("0");
		m_y2_clip_text->SetValue(
			wxString::Format("%d", resy));
		m_z1_clip_text->SetValue("0");
		m_z2_clip_text->SetValue(
			wxString::Format("%d", resz));
	}
	else if (id == ID_Y1ClipSldr ||
		id == ID_Y2ClipSldr)
	{
		m_link_x = false;
		m_link_x_chk->SetValue(false);
		m_link_z = false;
		m_link_z_chk->SetValue(false);
		m_x1_clip_text->SetValue("0");
		m_x2_clip_text->SetValue(
			wxString::Format("%d", resx));
		m_z1_clip_text->SetValue("0");
		m_z2_clip_text->SetValue(
			wxString::Format("%d", resz));
	}
	else if (id == ID_Z1ClipSldr ||
		id == ID_Z2ClipSldr)
	{
		m_link_x = false;
		m_link_x_chk->SetValue(false);
		m_link_y = false;
		m_link_y_chk->SetValue(false);
		m_x1_clip_text->SetValue("0");
		m_x2_clip_text->SetValue(
			wxString::Format("%d", resx));
		m_y1_clip_text->SetValue("0");
		m_y2_clip_text->SetValue(
			wxString::Format("%d", resy));
	}
}

void ClippingView::OnYZClipBtn(wxCommandEvent& event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;
    
    int resx, resy, resz;
    int resx_n, resy_n, resz_n;
    double bdw, bdh, bdd;
    resx_n = resy_n = resz_n = 0;
    bdw = bdh = bdd = 0.0;
    CalcBoundingBoxDemensions(bdw, bdh, bdd);
    resx = round(bdw);
    resy = round(bdh);
    resz = round(bdd);

	wxString str = m_yz_dist_text->GetValue();
	long dist;
	str.ToLong(&dist);

	//reset yz
	m_link_y = false;
	m_link_y_chk->SetValue(false);
	m_link_z = false;
	m_link_z_chk->SetValue(false);
	m_y1_clip_text->SetValue("0");
	m_y2_clip_text->SetValue(
		wxString::Format("%d", resy));
	m_z1_clip_text->SetValue("0");
	m_z2_clip_text->SetValue(
		wxString::Format("%d", resz));

	//set x
	SetXLink(false);
	if (dist < resx)
	{
		int x1 = resx/2-dist/2;
		int x2 = x1 + dist;
		int cur_x1 = m_x1_clip_sldr->GetValue();
		int cur_x2 = m_x2_clip_sldr->GetValue();
		if (x1 <= cur_x2)
		{
			m_x1_clip_text->SetValue(
				wxString::Format("%d", x1));
			m_x2_clip_text->SetValue(
				wxString::Format("%d", x2));
		}
		else
		{
			m_x2_clip_text->SetValue(
				wxString::Format("%d", x2));
			m_x1_clip_text->SetValue(
				wxString::Format("%d", x1));
		}
		SetXLink(true);
		int distx, disty, distz;
		if (m_sel_type == 2)
		{
			m_vd->GetClipDistance(distx, disty, distz);
			m_vd->SetClipDistance(dist, disty, distz);
		}
		else if (m_sel_type == 3)
		{
			m_md->GetClipDistance(distx, disty, distz);
			m_md->SetClipDistance(dist, disty, distz);
		}
	}
	else
	{
		m_x1_clip_text->SetValue("0");
		m_x2_clip_text->SetValue(
			wxString::Format("%d", resx));
	}

	m_x1_clip_sldr->SetFocus();
}

void ClippingView::OnXZClipBtn(wxCommandEvent& event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;

    int resx, resy, resz;
    int resx_n, resy_n, resz_n;
    double bdw, bdh, bdd;
    resx_n = resy_n = resz_n = 0;
    bdw = bdh = bdd = 0.0;
    CalcBoundingBoxDemensions(bdw, bdh, bdd);
    resx = round(bdw);
    resy = round(bdh);
    resz = round(bdd);

	wxString str = m_xz_dist_text->GetValue();
	long dist;
	str.ToLong(&dist);

	//reset xz
	m_link_x = false;
	m_link_x_chk->SetValue(false);
	m_link_z = false;
	m_link_z_chk->SetValue(false);
	m_x1_clip_text->SetValue("0");
	m_x2_clip_text->SetValue(
		wxString::Format("%d", resx));
	m_z1_clip_text->SetValue("0");
	m_z2_clip_text->SetValue(
		wxString::Format("%d", resz));

	//set y
	SetYLink(false);
	if (dist < resy)
	{
		int y1 = resy/2-dist/2;
		int y2 = y1 + dist;
		int cur_y1 = m_y1_clip_sldr->GetValue();
		int cur_y2 = m_y2_clip_sldr->GetValue();
		if (y1 <= cur_y2)
		{
			m_y1_clip_text->SetValue(
				wxString::Format("%d", y1));
			m_y2_clip_text->SetValue(
				wxString::Format("%d", y2));
		}
		else
		{
			m_y2_clip_text->SetValue(
				wxString::Format("%d", y2));
			m_y1_clip_text->SetValue(
				wxString::Format("%d", y1));
		}
		SetYLink(true);
		int distx, disty, distz;
		if (m_sel_type == 2)
		{
			m_vd->GetClipDistance(distx, disty, distz);
			m_vd->SetClipDistance(distx, dist, distz);
		}
		else if (m_sel_type == 3)
		{
			m_md->GetClipDistance(distx, disty, distz);
			m_md->SetClipDistance(distx, dist, distz);
		}
	}
	else
	{
		m_y1_clip_text->SetValue("0");
		m_y2_clip_text->SetValue(
			wxString::Format("%d", resy));
	}

	m_y1_clip_sldr->SetFocus();
}

void ClippingView::OnXYClipBtn(wxCommandEvent& event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;

    int resx, resy, resz;
    int resx_n, resy_n, resz_n;
    double bdw, bdh, bdd;
    resx_n = resy_n = resz_n = 0;
    bdw = bdh = bdd = 0.0;
    CalcBoundingBoxDemensions(bdw, bdh, bdd);
    resx = round(bdw);
    resy = round(bdh);
    resz = round(bdd);

	wxString str = m_xy_dist_text->GetValue();
	long dist;
	str.ToLong(&dist);

	//reset xy
	m_link_x = false;
	m_link_x_chk->SetValue(false);
	m_link_y = false;
	m_link_y_chk->SetValue(false);
	m_x1_clip_text->SetValue("0");
	m_x2_clip_text->SetValue(
		wxString::Format("%d", resx));
	m_y1_clip_text->SetValue("0");
	m_y2_clip_text->SetValue(
		wxString::Format("%d", resy));

	//set z
	SetZLink(false);
	if (dist < resz)
	{
		int z1 = resz/2-dist/2;
		int z2 = z1 + dist;
		int cur_z1 = m_z1_clip_sldr->GetValue();
		int cur_z2 = m_z2_clip_sldr->GetValue();
		if (z1 <= cur_z2)
		{
			m_z1_clip_text->SetValue(
				wxString::Format("%d", z1));
			m_z2_clip_text->SetValue(
				wxString::Format("%d", z2));
		}
		else
		{
			m_z2_clip_text->SetValue(
				wxString::Format("%d", z2));
			m_z1_clip_text->SetValue(
				wxString::Format("%d", z1));
		}
		SetZLink(true);
		int distx, disty, distz;
		if (m_sel_type == 2)
		{
			m_vd->GetClipDistance(distx, disty, distz);
			m_vd->SetClipDistance(distx, disty, dist);
		}
		else if (m_sel_type == 3)
		{
			m_md->GetClipDistance(distx, disty, distz);
			m_md->SetClipDistance(distx, disty, dist);
		}
	}
	else
	{
		m_z1_clip_text->SetValue("0");
		m_z2_clip_text->SetValue(
			wxString::Format("%d", resz));
	}

	m_z1_clip_sldr->SetFocus();
}

//move linked clipping planes
//dir: 0-lower; 1-higher
void ClippingView::MoveLinkedClippingPlanes(int dir)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;

    int resx, resy, resz;
    double bdw, bdh, bdd;
    bdw = bdh = bdd = 0.0;
    CalcBoundingBoxDemensions(bdw, bdh, bdd);
    resx = round(bdw);
    resy = round(bdh);
    resz = round(bdd);

	wxString str;
	long dist;

	if (dir == 0)
	{
		//moving lower
		if (m_link_x)
		{
			dist = (long)(m_x2_clip_sldr->GetValue() - m_x1_clip_sldr->GetValue());
			str = m_x1_clip_text->GetValue();
			long x1;
			str.ToLong(&x1);
			x1 = x1 - dist;
			x1 = x1<0?0:x1;
			m_x1_clip_text->SetValue(
				wxString::Format("%d", x1));
		}
		if (m_link_y)
		{
			dist = (long)(m_y2_clip_sldr->GetValue() - m_y1_clip_sldr->GetValue());
			str = m_y1_clip_text->GetValue();
			long y1;
			str.ToLong(&y1);
			y1 = y1 - dist;
			y1 = y1<0?0:y1;
			m_y1_clip_text->SetValue(
				wxString::Format("%d", y1));
		}
		if (m_link_z)
		{
			dist = (long)(m_z2_clip_sldr->GetValue() - m_z1_clip_sldr->GetValue());
			str = m_z1_clip_text->GetValue();
			long z1;
			str.ToLong(&z1);
			z1 = z1 - dist;
			z1 = z1<0?0:z1;
			m_z1_clip_text->SetValue(
				wxString::Format("%d", z1));
		}
	}
	else
	{
		//moving higher
		if (m_link_x)
		{
			dist = (long)(m_x2_clip_sldr->GetValue() - m_x1_clip_sldr->GetValue());
			str = m_x2_clip_text->GetValue();
			long x2;
			str.ToLong(&x2);
			x2 = x2 + dist;
			x2 = x2>resx?resx:x2;
			m_x2_clip_text->SetValue(
				wxString::Format("%d", x2));
		}
		if (m_link_y)
		{
			dist = (long)(m_y2_clip_sldr->GetValue() - m_y1_clip_sldr->GetValue());
			str.ToLong(&dist);
			str = m_y2_clip_text->GetValue();
			long y2;
			str.ToLong(&y2);
			y2 = y2 + dist;
			y2 = y2>resy?resy:y2;
			m_y2_clip_text->SetValue(
				wxString::Format("%d", y2));
		}
		if (m_link_z)
		{
			dist = (long)(m_z2_clip_sldr->GetValue() - m_z1_clip_sldr->GetValue());
			str.ToLong(&dist);
			str = m_z2_clip_text->GetValue();
			long z2;
			str.ToLong(&z2);
			z2 = z2 + dist;
			z2 = z2>resz?resz:z2;
			m_z2_clip_text->SetValue(
				wxString::Format("%d", z2));
		}
	}
}

void ClippingView::OnSliderKeyDown(wxKeyEvent& event)
{
	if ( !((m_sel_type==2 && m_vd) || (m_sel_type==3 && m_md)) )
		return;

    int resx, resy, resz;
    int resx_n, resy_n, resz_n;
    double bdw, bdh, bdd;
    resx_n = resy_n = resz_n = 0;
    bdw = bdh = bdd = 0.0;
    CalcBoundingBoxDemensions(bdw, bdh, bdd);
    resx = round(bdw);
    resy = round(bdh);
    resz = round(bdd);

	int id = event.GetId();
	int key = event.GetKeyCode();

	wxString str;
	long dist;

	if (key == wxKeyCode(','))
	{
		if (id == ID_X1ClipSldr ||
			id == ID_X2ClipSldr)
		{
			if (!m_link_x)
			{
				event.Skip();
				return;
			}

			dist = (long)(m_x2_clip_sldr->GetValue() - m_x1_clip_sldr->GetValue());
			str = m_x1_clip_text->GetValue();
			long x1;
			str.ToLong(&x1);
			x1 = x1 - dist;
			x1 = x1<0?0:x1;
			m_x1_clip_text->SetValue(
				wxString::Format("%d", x1));
		}
		else if (id == ID_Y1ClipSldr ||
			id == ID_Y2ClipSldr)
		{
			if (!m_link_y)
			{
				event.Skip();
				return;
			}

			dist = (long)(m_y2_clip_sldr->GetValue() - m_y1_clip_sldr->GetValue());
			str = m_y1_clip_text->GetValue();
			long y1;
			str.ToLong(&y1);
			y1 = y1 - dist;
			y1 = y1<0?0:y1;
			m_y1_clip_text->SetValue(
				wxString::Format("%d", y1));
		}
		else if (id == ID_Z1ClipSldr ||
			id == ID_Z2ClipSldr)
		{
			if (!m_link_z)
			{
				event.Skip();
				return;
			}

			dist = (long)(m_z2_clip_sldr->GetValue() - m_z1_clip_sldr->GetValue());
			str = m_z1_clip_text->GetValue();
			long z1;
			str.ToLong(&z1);
			z1 = z1 - dist;
			z1 = z1<0?0:z1;
			m_z1_clip_text->SetValue(
				wxString::Format("%d", z1));
		}
	}
	else if (key == wxKeyCode('.'))
	{
		if (id == ID_X1ClipSldr ||
			id == ID_X2ClipSldr)
		{
			if (!m_link_x)
			{
				event.Skip();
				return;
			}

			dist = (long)(m_x2_clip_sldr->GetValue() - m_x1_clip_sldr->GetValue());
			str = m_x2_clip_text->GetValue();
			long x2;
			str.ToLong(&x2);
			x2 = x2 + dist;
			x2 = x2>resx?resx:x2;
			m_x2_clip_text->SetValue(
				wxString::Format("%d", x2));
		}
		else if (id == ID_Y1ClipSldr ||
			id == ID_Y2ClipSldr)
		{
			if (!m_link_y)
			{
				event.Skip();
				return;
			}

			dist = (long)(m_y2_clip_sldr->GetValue() - m_y1_clip_sldr->GetValue());
			str = m_y2_clip_text->GetValue();
			long y2;
			str.ToLong(&y2);
			y2 = y2 + dist;
			y2 = y2>resy?resy:y2;
			m_y2_clip_text->SetValue(
				wxString::Format("%d", y2));
		}
		else if (id == ID_Z1ClipSldr ||
			id == ID_Z2ClipSldr)
		{
			if (!m_link_z)
			{
				event.Skip();
				return;
			}

			dist = (long)(m_z2_clip_sldr->GetValue() - m_z1_clip_sldr->GetValue());
			str = m_z2_clip_text->GetValue();
			long z2;
			str.ToLong(&z2);
			z2 = z2 + dist;
			z2 = z2>resz?resz:z2;
			m_z2_clip_text->SetValue(
				wxString::Format("%d", z2));
		}
	}

	event.Skip();
}

void ClippingView::OnFixRotsCheck(wxCommandEvent& event)
{
	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;

	if (vrender_frame)
	{
		if (m_fix_rots_chk->GetValue())
		{
			for (int i = 0; i < (int)vrender_frame->GetViewList()->size(); i++)
			{
				VRenderView* vrv = (*vrender_frame->GetViewList())[i];
				if (vrv)
				{
					vrv->SetClipMode(3);
					vrv->RefreshGL();
					double rotx, roty, rotz;
					vrv->GetClippingPlaneRotations(rotx, roty, rotz);
					m_x_rot_sldr->SetValue(int(rotx));
					m_y_rot_sldr->SetValue(int(roty));
					m_z_rot_sldr->SetValue(int(rotz));
					m_x_rot_text->ChangeValue(wxString::Format("%.1f", rotx));
					m_y_rot_text->ChangeValue(wxString::Format("%.1f", roty));
					m_z_rot_text->ChangeValue(wxString::Format("%.1f", rotz));
                    vrv->RefreshGL();
				}
			}
			m_fix_rots = true;
			DisableRotations();
		}
		else
		{
			VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
			if (vrender_frame)
			{
				for (int i = 0; i < (int)vrender_frame->GetViewList()->size(); i++)
				{
					VRenderView* vrv = (*vrender_frame->GetViewList())[i];
					if (vrv)
					{
						vrv->SetClipMode(4);
                        vrv->RefreshGL();
					}
				}
			}
			m_fix_rots = false;
			EnableRotations();
		}
	}
}

void ClippingView::OnHoldPlanesCheck(wxCommandEvent& event)
{
    if (m_hold_planes_chk->GetValue())
        m_hold_planes = true;
    else
        m_hold_planes = false;
}

void ClippingView::LoadDefault()
{
	wxString expath = wxStandardPaths::Get().GetExecutablePath();
	expath = expath.BeforeLast(GETSLASH(),NULL);
#ifdef _WIN32
	wxString dft = expath + "\\default_clip_settings.dft";
	if (!wxFileExists(dft))
		dft = wxStandardPaths::Get().GetUserConfigDir() + "\\default_clip_settings.dft";
#else
	wxString dft = expath + "/../Resources/default_clip_settings.dft";
#endif
	wxFileInputStream is(dft);
	if (!is.IsOk())
		return;
	wxFileConfig fconfig(is);

	long ival;
	if (fconfig.Read("plane_mode", &ival))
	{
		m_plane_mode_combo->SetSelection((int)ival);
		m_plane_mode = (int)ival;
	}

	bool bval;
	if (fconfig.Read("link_channels", &bval))
		m_link_channels->SetValue(bval);
}

void ClippingView::SaveDefault()
{
	VRenderFrame* vr_frame = (VRenderFrame*)m_frame;
	if (!vr_frame)
		return;
	DataManager *mgr = vr_frame->GetDataManager();
	if (!mgr)
		return;

	wxFileConfig fconfig("FluoRender default clip settings");
	wxString str;

	//plane mode
	int ival = m_plane_mode_combo->GetCurrentSelection();
	fconfig.Write("plane_mode", ival);

	//link
	bool link = m_link_channels->GetValue();
	fconfig.Write("link_channels", link);
	
	wxString expath = wxStandardPaths::Get().GetExecutablePath();
	expath = expath.BeforeLast(GETSLASH(),NULL);
#ifdef _WIN32
	wxString dft = expath + "\\default_clip_settings.dft";
	wxString dft2 = wxStandardPaths::Get().GetUserConfigDir() + "\\default_clip_settings.dft";
	if (!wxFileExists(dft) && wxFileExists(dft2))
		dft = dft2;
#else
	wxString dft = expath + "/../Resources/default_clip_settings.dft";
#endif
	wxFileOutputStream os(dft);
	fconfig.Save(os);
}

void ClippingView::EnableAll()
{
	m_link_channels->Enable();
	m_set_zero_btn->Enable();
	m_rot_reset_btn->Enable();
	m_x_rot_sldr->Enable();
	m_y_rot_sldr->Enable();
	m_z_rot_sldr->Enable();
	m_x_rot_text->Enable();
	m_y_rot_text->Enable();
	m_z_rot_text->Enable();
	m_x_rot_spin->Enable();
	m_y_rot_spin->Enable();
	m_z_rot_spin->Enable();
	m_x1_clip_sldr->Enable();
	m_x1_clip_text->Enable();
	m_x2_clip_sldr->Enable();
	m_x2_clip_text->Enable();
	m_y1_clip_sldr->Enable();
	m_y1_clip_text->Enable();
	m_y2_clip_sldr->Enable();
	m_y2_clip_text->Enable();
	m_z1_clip_sldr->Enable();
	m_z1_clip_text->Enable();
	m_z2_clip_sldr->Enable();
	m_z2_clip_text->Enable();
	m_link_x_chk->Enable();
	m_link_y_chk->Enable();
	m_link_z_chk->Enable();
	m_clip_reset_btn->Enable();
	m_yz_clip_btn->Enable();
	m_xz_clip_btn->Enable();
	m_xy_clip_btn->Enable();
	m_yz_dist_text->Enable();
	m_xz_dist_text->Enable();
	m_xy_dist_text->Enable();
	m_fix_rots_chk->Enable();
    m_hold_planes_chk->Enable();
}

void ClippingView::DisableAll()
{
	m_link_channels->Disable();
	m_set_zero_btn->Disable();
	m_rot_reset_btn->Disable();
	m_x_rot_sldr->Disable();
	m_y_rot_sldr->Disable();
	m_z_rot_sldr->Disable();
	m_x_rot_text->Disable();
	m_y_rot_text->Disable();
	m_z_rot_text->Disable();
	m_x_rot_spin->Disable();
	m_y_rot_spin->Disable();
	m_z_rot_spin->Disable();
	m_x1_clip_sldr->Disable();
	m_x1_clip_text->Disable();
	m_x2_clip_sldr->Disable();
	m_x2_clip_text->Disable();
	m_y1_clip_sldr->Disable();
	m_y1_clip_text->Disable();
	m_y2_clip_sldr->Disable();
	m_y2_clip_text->Disable();
	m_z1_clip_sldr->Disable();
	m_z1_clip_text->Disable();
	m_z2_clip_sldr->Disable();
	m_z2_clip_text->Disable();
	m_link_x_chk->Disable();
	m_link_y_chk->Disable();
	m_link_z_chk->Disable();
	m_clip_reset_btn->Disable();
	m_yz_clip_btn->Disable();
	m_xz_clip_btn->Disable();
	m_xy_clip_btn->Disable();
	m_yz_dist_text->Disable();
	m_xz_dist_text->Disable();
	m_xy_dist_text->Disable();
	m_fix_rots_chk->Disable();
    m_hold_planes_chk->Disable();
}

void ClippingView::EnableRotations()
{
	m_set_zero_btn->Enable();
	m_rot_reset_btn->Enable();
	m_x_rot_sldr->Enable();
	m_y_rot_sldr->Enable();
	m_z_rot_sldr->Enable();
	m_x_rot_text->Enable();
	m_y_rot_text->Enable();
	m_z_rot_text->Enable();
	m_x_rot_spin->Enable();
	m_y_rot_spin->Enable();
	m_z_rot_spin->Enable();
}

void ClippingView::DisableRotations()
{
	m_set_zero_btn->Disable();
	m_rot_reset_btn->Disable();
	m_x_rot_sldr->Disable();
	m_y_rot_sldr->Disable();
	m_z_rot_sldr->Disable();
	m_x_rot_text->Disable();
	m_y_rot_text->Disable();
	m_z_rot_text->Disable();
	m_x_rot_spin->Disable();
	m_y_rot_spin->Disable();
	m_z_rot_spin->Disable();
}
