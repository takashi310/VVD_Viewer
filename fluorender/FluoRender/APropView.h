#ifndef _APROPVIEW_H_
#define _APROPVIEW_H_

#include "DLLExport.h"
#include "DataManager.h"
#include "VRenderView.h"
#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/glcanvas.h>
#include <wx/clrpicker.h>
#include <wx/slider.h>

using namespace std;

class EXPORT_API APropView : public wxPanel
{
	enum
	{
		ID_MemoText = wxID_HIGHEST+1801,
		ID_MemoUpdateBtn,
        ID_diff_picker,
        ID_alpha_sldr,
        ID_alpha_text,
        ID_th_sldr,
        ID_th_text
	};

public:
	APropView(wxWindow* frame, wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0,
		const wxString& name = "APropView");
	~APropView();

	void SetAnnotations(Annotations* ann, VRenderView* vrv);
	Annotations* GetAnnotations();
	void RefreshVRenderViews(bool tree=false);

	void GetSettings();

private:
	wxWindow* m_frame;
	Annotations* m_ann;
	VRenderView* m_vrv;

	wxTextCtrl* m_memo_text;
	wxButton* m_memo_update_btn;
    
    wxColourPickerCtrl *m_diff_picker;
    wxSlider *m_alpha_sldr;
    wxTextCtrl* m_alpha_text;
    wxSlider *m_th_sldr;
    wxTextCtrl* m_th_text;

    wxStaticText* m_num_particles;

	//memo
	void OnMemoUpdateBtn(wxCommandEvent& event);
    void OnDiffChange(wxColourPickerEvent& event);
    void OnAlphaChange(wxScrollEvent & event);
    void OnAlphaText(wxCommandEvent& event);
    void OnThresholdChange(wxScrollEvent & event);
    void OnThresholdText(wxCommandEvent& event);
    
	DECLARE_EVENT_TABLE();
};

#endif//_APROPVIEW_H_
