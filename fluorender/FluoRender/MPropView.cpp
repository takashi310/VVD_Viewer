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
#include "MPropView.h"
#include "VRenderFrame.h"
#include <wx/valnum.h>

////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(mpTextCtrl, wxTextCtrl)
	EVT_SET_FOCUS(mpTextCtrl::OnSetFocus)
	EVT_KILL_FOCUS(mpTextCtrl::OnKillFocus)
	EVT_TEXT(wxID_ANY, mpTextCtrl::OnText)
	EVT_TEXT_ENTER(wxID_ANY, mpTextCtrl::OnEnter)
	EVT_KEY_DOWN(mpTextCtrl::OnKeyDown)
	EVT_KEY_UP(mpTextCtrl::OnKeyUp)
END_EVENT_TABLE()

	mpTextCtrl::mpTextCtrl(
	wxWindow* frame,
	wxWindow* parent,
	wxWindowID id,
	const wxString& text,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxValidator& valid) :
wxTextCtrl(parent, id, text, pos, size, style, valid),
	m_frame(frame),
	m_style(style)
{
	SetHint(text);

	m_dummy = new wxButton(parent, id, text, pos, size);
	m_dummy->Hide();
}

mpTextCtrl::~mpTextCtrl()
{

}

void mpTextCtrl::OnSetChildFocus(wxChildFocusEvent& event)
{
	VRenderFrame *vrf = (VRenderFrame *)m_frame;
	if (vrf) vrf->SetKeyLock(true);
	event.Skip();
}

void mpTextCtrl::OnSetFocus(wxFocusEvent& event)
{
	VRenderFrame *vrf = (VRenderFrame *)m_frame;
	if (vrf) vrf->SetKeyLock(true);
	event.Skip();
}

void mpTextCtrl::OnKillFocus(wxFocusEvent& event)
{
	VRenderFrame *vrf = (VRenderFrame *)m_frame;
	if (vrf) vrf->SetKeyLock(false);
	wxCommandEvent ev(wxEVT_COMMAND_TEXT_ENTER);
	if (this->GetParent())
		((MPropView *)this->GetParent())->UpdateRadScale();
	event.Skip();
}

void mpTextCtrl::OnText(wxCommandEvent &event)
{
	VRenderFrame *vrf = (VRenderFrame *)m_frame;
	if (vrf) vrf->SetKeyLock(true);
}

void mpTextCtrl::OnEnter(wxCommandEvent &event)
{
	if (m_style & wxTE_PROCESS_ENTER)
		m_dummy->SetFocus();

//	event.Skip();
}

void mpTextCtrl::OnKeyDown(wxKeyEvent& event)
{
	event.Skip();
}

void mpTextCtrl::OnKeyUp(wxKeyEvent& event)
{
	event.Skip();
}

/////////////////////////////////////////////////////////////////////////


BEGIN_EVENT_TABLE(MPropView, wxPanel)
	//lighting
	EVT_CHECKBOX(ID_light_chk, MPropView::OnLightingCheck)
	EVT_COLOURPICKER_CHANGED(ID_diff_picker, MPropView::OnDiffChange)
	EVT_COLOURPICKER_CHANGED(ID_spec_picker, MPropView::OnSpecChange)
	EVT_COMMAND_SCROLL(ID_shine_sldr, MPropView::OnShineChange)
	EVT_COMMAND_SCROLL(ID_alpha_sldr, MPropView::OnAlphaChange)
	EVT_COMMAND_SCROLL(ID_scale_sldr, MPropView::OnScaleChange)
	EVT_TEXT(ID_shine_text, MPropView::OnShineText)
	EVT_TEXT(ID_alpha_text, MPropView::OnAlphaText)
	EVT_TEXT(ID_scale_text, MPropView::OnScaleText)
	//shadow
	EVT_CHECKBOX(ID_shadow_chk, MPropView::OnShadowCheck)
	EVT_COMMAND_SCROLL(ID_shadow_sldr, MPropView::OnShadowChange)
	EVT_TEXT(ID_shadow_text, MPropView::OnShadowText)
	//size limiter
	EVT_CHECKBOX(ID_size_chk, MPropView::OnSizeCheck)
	EVT_COMMAND_SCROLL(ID_size_sldr, MPropView::OnSizeChange)
	EVT_TEXT(ID_size_text, MPropView::OnSizeText)
	EVT_TEXT_ENTER(ID_r_text, MPropView::OnEnterInRadScaleText)
	EVT_CHECKBOX(ID_sync_chk, MPropView::OnSyncCheck)
END_EVENT_TABLE()

MPropView::MPropView(wxWindow* frame, wxWindow* parent,
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& name) :
wxPanel(parent, id, pos, size,style, name),
m_frame(frame),
m_md(0),
m_vrv(0),
m_sync(false)
{
	SetEvtHandlerEnabled(false);
	Freeze();

	wxBoxSizer* sizer_v1 = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* sizer_1 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizer_2 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizer_3 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizer_4 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizer_5 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* sizer_6 = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* st = 0;

	//validator: floating point 1
	wxFloatingPointValidator<double> vald_fp2(2);
	//validator: integer
	wxIntegerValidator<unsigned int> vald_int;

	st = new wxStaticText(this, 0, " Transparency: ",
		wxDefaultPosition, wxSize(100, 20));
	m_alpha_sldr = new wxSlider(this, ID_alpha_sldr, 255, 0, 255, 
		wxDefaultPosition, wxSize(200, 20), wxSL_HORIZONTAL);
	m_alpha_text = new wxTextCtrl(this, ID_alpha_text, "1.00",
		wxDefaultPosition, wxSize(50, 20), 0, vald_fp2);
	sizer_1->Add(20, 5, 0);
	sizer_1->Add(st, 0, wxALIGN_CENTER, 0);
	sizer_1->Add(m_alpha_sldr, 0, wxALIGN_CENTER, 0);
	sizer_1->Add(m_alpha_text, 0, wxALIGN_CENTER, 0);

	m_shadow_chk = new wxCheckBox(this, ID_shadow_chk, "Shadow: ",
		wxDefaultPosition, wxSize(100, 20));
	m_shadow_sldr = new wxSlider(this, ID_shadow_sldr, 60, 0, 100,
		wxDefaultPosition, wxSize(200, 20), wxSL_HORIZONTAL);
	m_shadow_text = new wxTextCtrl(this, ID_shadow_text, "0.60",
		wxDefaultPosition, wxSize(50, 20), 0, vald_fp2);
	sizer_2->Add(20, 5, 0);
	sizer_2->Add(m_shadow_chk, 0, wxALIGN_CENTER, 0);
	sizer_2->Add(m_shadow_sldr, 0, wxALIGN_CENTER, 0);
	sizer_2->Add(m_shadow_text, 0, wxALIGN_CENTER, 0);

	m_light_chk = new wxCheckBox(this, ID_light_chk, "Lighting",
		wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT);
	sizer_3->Add(20, 5, 0);
	sizer_3->Add(m_light_chk, 0, wxALIGN_CENTER, 0);

	wxBoxSizer *group1 = new wxStaticBoxSizer(
		new wxStaticBox(this, wxID_ANY, "Material"),
		wxVERTICAL);

	st = new wxStaticText(this, 0, " Diffuse Color: ",
		wxDefaultPosition, wxSize(110, 20));
	m_diff_picker = new wxColourPickerCtrl(this, ID_diff_picker, *wxWHITE, 
		wxDefaultPosition, wxSize(180, 30));
	sizer_4->Add(st, 0, wxALIGN_LEFT, 0);
	sizer_4->Add(m_diff_picker, 0, wxALIGN_CENTER, 0);

	st = new wxStaticText(this, 0, " Specular Color: ",
		wxDefaultPosition, wxSize(110, 20));
	m_spec_picker = new wxColourPickerCtrl(this, ID_spec_picker, *wxWHITE, 
		wxDefaultPosition, wxSize(180, 30));
	sizer_5->Add(st, 0, wxALIGN_LEFT, 0);
	sizer_5->Add(m_spec_picker, 0, wxALIGN_CENTER, 0);

	st = new wxStaticText(this, 0, " Shininess: ",
		wxDefaultPosition, wxSize(100, 20));
	m_shine_sldr = new wxSlider(this, ID_shine_sldr, 30, 0, 128, 
		wxDefaultPosition, wxSize(200, 20), wxSL_HORIZONTAL);
	m_shine_text = new wxTextCtrl(this, ID_shine_text, "30",
		wxDefaultPosition, wxSize(50, 20), 0, vald_int);
	sizer_6->Add(st, 0, wxALIGN_CENTER, 0);
	sizer_6->Add(m_shine_sldr, 0, wxALIGN_CENTER, 0);
	sizer_6->Add(m_shine_text, 0, wxALIGN_CENTER, 0);

	group1->Add(sizer_4, 0, wxALIGN_LEFT);
	group1->Add(sizer_5, 0, wxALIGN_LEFT);
	group1->Add(sizer_6, 0, wxALIGN_LEFT);

	sizer_v1->Add(group1, 0, wxALIGN_LEFT);

	wxBoxSizer* sizer_v2 = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* sizer_7 = new wxBoxSizer(wxHORIZONTAL);
	st = new wxStaticText(this, 0, " Scaling: ",
		wxDefaultPosition, wxSize(100, 20));
	m_scale_sldr = new wxSlider(this, ID_scale_sldr, 100, 50, 200, 
		wxDefaultPosition, wxSize(200, 20), wxSL_HORIZONTAL);
	m_scale_text = new wxTextCtrl(this, ID_scale_text, "1.00",
		wxDefaultPosition, wxSize(50, 20), 0, vald_fp2);
	sizer_7->Add(20, 5, 0);
	sizer_7->Add(st, 0, wxALIGN_CENTER, 0);
	sizer_7->Add(m_scale_sldr, 0, wxALIGN_CENTER, 0);
	sizer_7->Add(m_scale_text, 0, wxALIGN_CENTER, 0);

	//size limiter
	wxBoxSizer* sizer_8 = new wxBoxSizer(wxHORIZONTAL);
	m_size_chk = new wxCheckBox(this, ID_size_chk, " Size limit: ",
		wxDefaultPosition, wxSize(100, 20));
	m_size_sldr = new wxSlider(this, ID_size_sldr, 50, 0, 250,
		wxDefaultPosition, wxSize(200, 20), wxSL_HORIZONTAL);
	m_size_text = new wxTextCtrl(this, ID_size_text, "50",
		wxDefaultPosition, wxSize(50, 20), 0, vald_int);
	sizer_8->Add(20, 5, 0);
	sizer_8->Add(m_size_chk, 0, wxALIGN_CENTER);
	sizer_8->Add(m_size_sldr, 0, wxALIGN_CENTER);
	sizer_8->Add(m_size_text, 0, wxALIGN_CENTER);

	wxBoxSizer* sizer_9 = new wxBoxSizer(wxHORIZONTAL);
	m_r_st = new wxStaticText(this, 0, " Radius Scale: ",
		wxDefaultPosition, wxSize(100, 20));
	m_r_text = new mpTextCtrl(m_frame, this, ID_r_text, "1.00",
		wxDefaultPosition, wxSize(50, 20), wxTE_PROCESS_ENTER, vald_fp2);
	sizer_9->Add(20, 5, 0);
	sizer_9->Add(m_r_st, 0, wxALIGN_CENTER);
	sizer_9->Add(m_r_text, 0, wxALIGN_CENTER);

	wxBoxSizer* sizer_10 = new wxBoxSizer(wxHORIZONTAL);
	m_sync_chk = new wxCheckBox(this, ID_sync_chk, " Sync",
		wxDefaultPosition, wxSize(100, 20));

	sizer_10->Add(20, 5, 0);
	sizer_10->Add(m_sync_chk, 0, wxALIGN_CENTER);

	sizer_v2->Add(5,5);
	sizer_v2->Add(sizer_1, 0, wxALIGN_LEFT);
	sizer_v2->Add(sizer_2, 0, wxALIGN_LEFT);
	sizer_v2->Add(sizer_3, 0, wxALIGN_LEFT);
	sizer_v2->Add(sizer_7, 0, wxALIGN_LEFT);
	sizer_v2->Add(sizer_8, 0, wxALIGN_LEFT);
	sizer_v2->Add(sizer_9, 0, wxALIGN_LEFT);
	sizer_v2->Add(sizer_10, 0, wxALIGN_RIGHT);

	wxBoxSizer* sizer_all = new wxBoxSizer(wxHORIZONTAL);
	sizer_all->Add(sizer_v1, 0, wxALIGN_TOP);
	sizer_all->Add(sizer_v2, 0, wxALIGN_TOP);

	SetSizer(sizer_all);

	m_r_st->Disable();
	m_r_text->Disable();

	Layout();

	Thaw();
	SetEvtHandlerEnabled(true);
}

MPropView::~MPropView()
{
}

void MPropView::GetSettings()
{
	if (!m_md)
		return;

	SetEvtHandlerEnabled(false);

	wxString str;
	Color amb, diff, spec;
	double shine, alpha;
	m_md->GetMaterial(amb, diff, spec, shine, alpha);

	wxColor c;
	c = wxColor(diff.r()*255, diff.g()*255, diff.b()*255);
	m_diff_picker->SetColour(c);
	c = wxColor(spec.r()*255, spec.g()*255, spec.b()*255);
	m_spec_picker->SetColour(c);

	//lighting
	m_light_chk->SetValue(m_md->GetLighting());
	//shine
	m_shine_sldr->SetValue(int(shine));
	str = wxString::Format("%.0f", shine);
	m_shine_text->ChangeValue(str);
	//alpha
	m_alpha_sldr->SetValue(int(alpha*255));
	str = wxString::Format("%.2f", alpha);
	m_alpha_text->ChangeValue(str);
	//scaling
	double sx, sy, sz;
	m_md->GetScaling(sx, sy, sz);
	m_scale_sldr->SetValue(int(sx*100.0+0.5));
	str = wxString::Format("%.2f", sx);
	m_scale_text->ChangeValue(str);
	//shadow
	double darkness;
	m_shadow_chk->SetValue(m_md->GetShadow());
	m_md->GetShadowParams(darkness);
	m_shadow_sldr->SetValue(int(darkness*100.0+0.5));
	str = wxString::Format("%.2f", darkness);
	m_shadow_text->ChangeValue(str);
	//size limiter
	m_size_chk->SetValue(m_md->GetLimit());
	int limit = m_md->GetLimitNumber();
	m_size_sldr->SetValue(limit);
	m_size_text->SetValue(wxString::Format("%d", limit));
	//swc
	if (m_md->isSWC())
	{
		m_r_st->Enable();
		m_r_text->Enable();
		str = wxString::Format("%.2f", m_md->GetRadScale());
		m_r_text->ChangeValue(str);
	}
	else
	{
		m_r_st->Disable();
		m_r_text->Disable();
		str = wxString::Format("%.2f", m_md->GetRadScale());
		m_r_text->ChangeValue(str);
	}

	SetEvtHandlerEnabled(true);
}

void MPropView::UpdateSync()
{
	if (!m_vrv)
		return;

	wxString str;
	Color amb, diff, spec;
	double shine, alpha;
	m_md->GetMaterial(amb, diff, spec, shine, alpha);

	bool lighting = m_md->GetLighting();
	//scaling
	double sx, sy, sz;
	m_md->GetScaling(sx, sy, sz);
	
	//shadow
	double darkness;
	bool shadow = m_md->GetShadow();
	m_md->GetShadowParams(darkness);
	
	//size limiter
	bool limit = m_md->GetLimit();
	int limitnum = m_md->GetLimitNumber();
	
	double rs = m_md->GetRadScale();
	
	int mesh_num = m_vrv->GetMeshNum();
	for (int i = 0; i < mesh_num; i++)
	{
		MeshData *md = m_vrv->GetMeshData(i);
		if (!md) continue;

		double dummy1, dummy2;
		md->GetMaterial(amb, diff, spec, dummy1, dummy2);
		
		md->SetMaterial(amb, diff, spec, shine, alpha);
		md->SetFloat(alpha, MESH_FLOAT_ALPHA);
		md->SetLighting(lighting);
		md->SetScaling(sx, sy, sz);
		md->SetShadow(shadow);
		md->SetLimit(limit);
		md->SetLimitNumer(limitnum);
		md->SetRadScale(rs);
	}
}

void MPropView::SetMeshData(MeshData* md, VRenderView* vrv)
{
	m_md = md;
	GetSettings();

	m_vrv = vrv;
}

MeshData* MPropView::GetMeshData()
{
	return m_md;
}

void MPropView::OnSyncCheck(wxCommandEvent& event)
{
	m_sync = m_sync_chk->GetValue();

	if (m_sync)
	{
		UpdateSync();
		RefreshVRenderViews();
	}
}

void MPropView::OnEnterInRadScaleText(wxCommandEvent& event)
{
//	UpdateRadScale();
}

void MPropView::UpdateRadScale()
{
	wxString str = m_r_text->GetValue();
	double r;
	str.ToDouble(&r);
	
	if (m_md)
	{
		m_md->SetRadScale(r);
		if (m_sync) UpdateSync();
		RefreshVRenderViews();
	}
}

void MPropView::RefreshVRenderViews(bool tree)
{
	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
		vrender_frame->RefreshVRenderViews(tree);
}

//lighting
void MPropView::OnLightingCheck(wxCommandEvent& event)
{
	if (m_md && m_vrv)
	{
		bool val = m_light_chk->GetValue();
		m_md->SetLighting(val);
		for (int i=0; i<m_vrv->GetMeshNum(); i++)
		{
			MeshData* md = m_vrv->GetMeshData(i);
			if (md)
				md->SetLighting(val);
		}
		RefreshVRenderViews();
	}
}

void MPropView::OnDiffChange(wxColourPickerEvent& event)
{
	wxColor c = event.GetColour();
	Color color(c.Red()/255.0, c.Green()/255.0, c.Blue()/255.0);
	if (m_md)
	{
		m_md->SetColor(color, MESH_COLOR_DIFF);
        Color amb = color * 0.3;
		m_md->SetColor(amb, MESH_COLOR_AMB);
		RefreshVRenderViews(true);
	}
}

void MPropView::OnSpecChange(wxColourPickerEvent& event)
{
	wxColor c = event.GetColour();
	Color color(c.Red()/255.0, c.Green()/255.0, c.Blue()/255.0);
	if (m_md)
	{
		m_md->SetColor(color, MESH_COLOR_SPEC);
		RefreshVRenderViews();
	}
}

void MPropView::OnShineChange(wxScrollEvent & event)
{
	double val = (double)event.GetPosition();
	wxString str = wxString::Format("%.0f", val);
	m_shine_text->SetValue(str);
}

void MPropView::OnShineText(wxCommandEvent& event)
{
	wxString str = m_shine_text->GetValue();
	double shine;
	str.ToDouble(&shine);
	m_shine_sldr->SetValue(int(shine));

	if (m_md)
	{
		m_md->SetFloat(shine, MESH_FLOAT_SHN);
		if (m_sync) UpdateSync();
		RefreshVRenderViews();
	}
}

void MPropView::OnAlphaChange(wxScrollEvent & event)
{
	double val = (double)event.GetPosition() / 255.0;
	wxString str = wxString::Format("%.2f", val);
	m_alpha_text->SetValue(str);
}

void MPropView::OnAlphaText(wxCommandEvent& event)
{
	wxString str = m_alpha_text->GetValue();
	double alpha;
	str.ToDouble(&alpha);
	m_alpha_sldr->SetValue(int(alpha*255.0+0.5));

	if (m_md)
	{
		m_md->SetFloat(alpha, MESH_FLOAT_ALPHA);
		if (m_sync) UpdateSync();
		RefreshVRenderViews();
	}
}

void MPropView::OnScaleChange(wxScrollEvent & event)
{
	double val = event.GetPosition() / 100.0;
	wxString str = wxString::Format("%.2f", val);
	m_scale_text->SetValue(str);
}

void MPropView::OnScaleText(wxCommandEvent& event)
{
	wxString str = m_scale_text->GetValue();
	double dval;
	str.ToDouble(&dval);
	m_scale_sldr->SetValue(int(dval*100.0+0.5));

	if (m_md)
	{
		m_md->SetScaling(dval, dval, dval);
		if (m_sync) UpdateSync();
		RefreshVRenderViews();
	}
}

//shadow
void MPropView::OnShadowCheck(wxCommandEvent& event)
{
	if (m_md && m_vrv)
	{
		bool val = m_shadow_chk->GetValue();
		m_md->SetShadow(val);
		for (int i=0; i<m_vrv->GetMeshNum(); i++)
		{
			MeshData* md = m_vrv->GetMeshData(i);
			if (md)
				md->SetShadow(val);
		}
		RefreshVRenderViews();
	}
}

void MPropView::OnShadowChange(wxScrollEvent& event)
{
	double val = event.GetPosition() / 100.0;
	wxString str = wxString::Format("%.2f", val);
	m_shadow_text->SetValue(str);
}

void MPropView::OnShadowText(wxCommandEvent& event)
{
	wxString str = m_shadow_text->GetValue();
	double dval;
	str.ToDouble(&dval);
	m_shadow_sldr->SetValue(int(dval*100.0+0.5));

	if (m_md && m_vrv)
	{
		m_md->SetShadowParams(dval);
		for (int i=0; i<m_vrv->GetMeshNum(); i++)
		{
			MeshData* md = m_vrv->GetMeshData(i);
			if (md)
				md->SetShadowParams(dval);
		}
		RefreshVRenderViews();
	}
}

//size limiter
void MPropView::OnSizeCheck(wxCommandEvent& event)
{
	bool bval = m_size_chk->GetValue();
	if (m_md)
	{
		m_md->SetLimit(bval);
		if (m_sync) UpdateSync();
		RefreshVRenderViews();
	}
}

void MPropView::OnSizeChange(wxScrollEvent& event)
{
	int val = event.GetPosition();
	wxString str = wxString::Format("%d", val);
	m_size_text->SetValue(str);
}

void MPropView::OnSizeText(wxCommandEvent& event)
{
	wxString str = m_size_text->GetValue();
	long val;
	str.ToLong(&val);
	m_size_sldr->SetValue(val);

	if (m_md)
	{
		m_md->SetLimitNumer(val);
		if (m_sync) UpdateSync();
		RefreshVRenderViews();
	}
}
