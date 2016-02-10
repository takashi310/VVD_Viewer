#ifndef _VIANNOVIEW_H_
#define _VIANNOVIEW_H_

#include "DataManager.h"
#include "VRenderView.h"
#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/glcanvas.h>
#include <wx/clrpicker.h>
#include <wx/slider.h>

using namespace std;

class myTextCtrl : public wxTextCtrl
{
public:
	myTextCtrl(wxWindow* frame,
		wxWindow* parent,
		wxWindowID id,
		const wxString& text = wxT(""),
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style=0);
	~myTextCtrl();

	void KillFocus();

private:
	wxWindow *m_frame;
	wxButton *m_dummy;
	long m_style;

private:
	void OnSetChildFocus(wxChildFocusEvent& event);
	void OnSetFocus(wxFocusEvent& event);
	void OnKillFocus(wxFocusEvent& event);

	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);

	void OnText(wxCommandEvent& event);
	void OnEnter(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};


class VAnnoView : public wxPanel
{
	enum
	{
		ID_InfoText = wxID_HIGHEST+1801,
		ID_MemoUpdateBtn
	};

public:
	VAnnoView(wxWindow* frame, wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0,
		const wxString& name = "VAnnoView");
	~VAnnoView();

	void SetData(TreeLayer *data);
	void SaveInfo();
	wxString GetText(){return m_text ? wxString() : m_text->GetValue();}
	void RefreshVRenderViews(bool tree=false);

	void GetSettings();

private:
	wxWindow* m_frame;
	TreeLayer* m_data;

	wxTextCtrl* m_text;
	
	wxBoxSizer* m_root_sizer;

	//memo
	void OnColorBtn(wxColourPickerEvent& event);
//	DECLARE_EVENT_TABLE();
};

#endif//_VIANNOVIEW_H_
