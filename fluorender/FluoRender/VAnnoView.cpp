#include "VAnnoView.h"
#include "VRenderFrame.h"
#include <wx/valnum.h>

////////////////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(myTextCtrl, wxTextCtrl)
	EVT_SET_FOCUS(myTextCtrl::OnSetFocus)
	EVT_KILL_FOCUS(myTextCtrl::OnKillFocus)
	EVT_TEXT(wxID_ANY, myTextCtrl::OnText)
	EVT_TEXT_ENTER(wxID_ANY, myTextCtrl::OnEnter)
	EVT_KEY_DOWN(myTextCtrl::OnKeyDown)
	EVT_KEY_UP(myTextCtrl::OnKeyUp)
END_EVENT_TABLE()

	myTextCtrl::myTextCtrl(
	wxWindow* frame,
	wxWindow* parent,
	wxWindowID id,
	const wxString& text,
	const wxPoint& pos,
	const wxSize& size,
	long style) :
wxTextCtrl(parent, id, "", pos, size, style),
	m_frame(frame),
	m_style(style)
{
	SetHint(text);

	m_dummy = new wxButton(parent, id, text, pos, size);
	m_dummy->Hide();
}

myTextCtrl::~myTextCtrl()
{

}

void myTextCtrl::KillFocus()
{
	VRenderFrame *vrf = (VRenderFrame *)m_frame;
	if (vrf) vrf->SetKeyLock(false);
	m_dummy->SetFocus();
}

void myTextCtrl::OnSetChildFocus(wxChildFocusEvent& event)
{
	VRenderFrame *vrf = (VRenderFrame *)m_frame;
	if (vrf) vrf->SetKeyLock(true);
	event.Skip();
}

void myTextCtrl::OnSetFocus(wxFocusEvent& event)
{
	VRenderFrame *vrf = (VRenderFrame *)m_frame;
	if (vrf) vrf->SetKeyLock(true);
	event.Skip();
}

void myTextCtrl::OnKillFocus(wxFocusEvent& event)
{
	VRenderFrame *vrf = (VRenderFrame *)m_frame;
	if (vrf) vrf->SetKeyLock(false);
	event.Skip();
}

void myTextCtrl::OnText(wxCommandEvent &event)
{
	VRenderFrame *vrf = (VRenderFrame *)m_frame;
	if (vrf) vrf->SetKeyLock(true);
}

void myTextCtrl::OnEnter(wxCommandEvent &event)
{
	if (m_style & wxTE_PROCESS_ENTER) m_dummy->SetFocus();
	event.Skip();
}

void myTextCtrl::OnKeyDown(wxKeyEvent& event)
{
	event.Skip();
}

void myTextCtrl::OnKeyUp(wxKeyEvent& event)
{
	event.Skip();
}

/////////////////////////////////////////////////////////////////////////

/*
BEGIN_EVENT_TABLE(VAnnoView, wxPanel)
END_EVENT_TABLE()
*/

VAnnoView::VAnnoView(wxWindow* frame, wxWindow* parent,
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& name) :
wxPanel(parent, id, pos, size, style, name),
m_frame(frame),
m_data(0)
{
	m_root_sizer = new wxBoxSizer(wxVERTICAL);
	wxStaticText* st = 0;

	wxBoxSizer* sizer_1 = new wxBoxSizer(wxHORIZONTAL);
	st = new wxStaticText(this, 0, "Info:",
		wxDefaultPosition, wxDefaultSize);
	sizer_1->Add(10, 10);
	sizer_1->Add(st);

	wxBoxSizer* sizer_2 = new wxBoxSizer(wxHORIZONTAL);
	m_text = new myTextCtrl(frame, this, wxID_ANY, "",
		wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxHSCROLL);
	sizer_2->Add(10, 10);
	sizer_2->Add(m_text, 1, wxEXPAND);
	sizer_2->Add(10, 10);

	
	m_root_sizer->Add(10, 10);
	m_root_sizer->Add(sizer_1, 0, wxALIGN_LEFT);
	m_root_sizer->Add(sizer_2, 1, wxEXPAND);
	m_root_sizer->Add(10, 10);

	//m_root_sizer->Hide(m_roi_root_sizer);
	
	SetSizer(m_root_sizer);
	Layout();
}

VAnnoView::~VAnnoView()
{
}

void VAnnoView::SaveInfo()
{
	if (!m_data) return;

	switch (m_data->IsA())
	{
	case 2: //volume
		{
			BaseReader *reader = ((VolumeData *)m_data)->GetReader();
			if (reader)
				reader->SetInfo(m_text->GetValue().ToStdWstring());
		}
		break;
	case 3: //mesh
		//m_text->SetValue(wxString(((MeshData *)m_data)->GetInfo()));
		break;
	}
}

void VAnnoView::SetData(TreeLayer *data)
{
	SaveInfo();

	m_data = data;

	if (!m_data) return;

	switch (m_data->IsA())
	{
	case 2: //volume
		{
			BaseReader *reader = ((VolumeData *)m_data)->GetReader();
			if (reader)
				m_text->SetValue(wxString(reader->GetInfo()));
		}
		break;
	case 3: //mesh
		//m_text->SetValue(wxString(((MeshData *)m_data)->GetInfo()));
		break;
	}

}

void VAnnoView::GetSettings()
{
	if (!m_data)
		return;

}

void VAnnoView::RefreshVRenderViews(bool tree)
{
	VRenderFrame* vrender_frame = (VRenderFrame*)m_frame;
	if (vrender_frame)
		vrender_frame->RefreshVRenderViews(tree);
}
