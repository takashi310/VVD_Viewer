#ifndef _SAMPLEGUIPLUGINWINDOW1_H_
#define _SAMPLEGUIPLUGINWINDOW1_H_

#include "stdwx.h"
#include <wxGuiPluginWindowBase.h>
#include <wx/filepicker.h>
#include "utility.h"
#include "wx/progdlg.h"
#include <wx/stopwatch.h>

#define SYMBOL_SAMPLEGUIPLUGINWINDOW1_STYLE wxTAB_TRAVERSAL
#define SYMBOL_SAMPLEGUIPLUGINWINDOW1_TITLE _("SampleGuiPluginWindow1")
#define SYMBOL_SAMPLEGUIPLUGINWINDOW1_IDNAME ID_SAMPLEGUIPLUGINWINDOW1
#define SYMBOL_SAMPLEGUIPLUGINWINDOW1_SIZE wxSize(400, 300)
#define SYMBOL_SAMPLEGUIPLUGINWINDOW1_POSITION wxDefaultPosition



class SampleGuiPluginWindow1: public wxGuiPluginWindowBase, public Observer
{    
    DECLARE_DYNAMIC_CLASS( SampleGuiPluginWindow1 )
    DECLARE_EVENT_TABLE()

	enum
	{
		ID_SAMPLEGUIPLUGINWINDOW1 = wxID_HIGHEST+11001,
		ID_SAMPLE_TEXTCTRL,
		ID_SAMPLE_FIJI,
		ID_SAMPLE_COMMAND,
		ID_SAMPLE_MASK,
		ID_SAMPLE_STARTUP,
		ID_SEND_EVENT_BUTTON,
		ID_PendingCommandTimer,
		ID_WaitTimer
	};

public:
    /// Constructors
    SampleGuiPluginWindow1();
    SampleGuiPluginWindow1(wxGuiPluginBase * plugin, wxWindow* parent, wxWindowID id = SYMBOL_SAMPLEGUIPLUGINWINDOW1_IDNAME, const wxPoint& pos = SYMBOL_SAMPLEGUIPLUGINWINDOW1_POSITION, const wxSize& size = SYMBOL_SAMPLEGUIPLUGINWINDOW1_SIZE, long style = SYMBOL_SAMPLEGUIPLUGINWINDOW1_STYLE );

    /// Creation
    bool Create(wxGuiPluginBase * plugin, wxWindow* parent, wxWindowID id = SYMBOL_SAMPLEGUIPLUGINWINDOW1_IDNAME, const wxPoint& pos = SYMBOL_SAMPLEGUIPLUGINWINDOW1_POSITION, const wxSize& size = SYMBOL_SAMPLEGUIPLUGINWINDOW1_SIZE, long style = SYMBOL_SAMPLEGUIPLUGINWINDOW1_STYLE );

    /// Destructor
    ~SampleGuiPluginWindow1();

    /// Initialises member variables
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

////@begin SampleGuiPluginWindow1 event handler declarations

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_SEND_EVENT_BUTTON
    void OnSENDEVENTBUTTONClick( wxCommandEvent& event );

	void OnPendingCommandTimer(wxTimerEvent& event);
	void OnWaitTimer(wxTimerEvent& event);
	
	void OnClose(wxCloseEvent& event);

////@end SampleGuiPluginWindow1 event handler declarations

////@begin SampleGuiPluginWindow1 member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );

	void doAction(ActionInfo *info);

	void EnableControls(bool enable=true);

	void SendCommand(wxString com, bool send_mask);

////@end SampleGuiPluginWindow1 member function declarations

    static bool ShowToolTips();

private:

    wxTextCtrl* m_VerTextCtrl;
	wxFilePickerCtrl* m_FijiPickCtrl;
	wxTextCtrl* m_CommandTextCtrl;
	wxButton* m_CommandButton;
	wxCheckBox* m_LaunchChk;
	wxCheckBox* m_SendMaskChk;
	wxTimer* m_pctimer;
	wxTimer* m_wtimer;
	wxStopWatch m_pcwatch;
	wxProgressDialog* m_prg_diag;
	bool m_waitingforfiji;
	wxString pendingcommand;
	bool pendingcom_msk;
};

#endif
    // _SAMPLEGUIPLUGINWINDOW1_H_
