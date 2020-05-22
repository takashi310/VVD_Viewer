#ifndef _NeuronAnnotatorPluginWindow_H_
#define _NeuronAnnotatorPluginWindow_H_

#include <wxGuiPluginWindowBase.h>
#include <wx/dialog.h>
#include <wx/filepicker.h>
#include <wx/progdlg.h>
#include <wx/listctrl.h>
#include <wx/stopwatch.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/stopwatch.h>
#include "utility.h"

#define SYMBOL_NAPluginWindow_STYLE wxTAB_TRAVERSAL
#define SYMBOL_NAPluginWindow_TITLE _("_NeuronAnnotatorPluginWindow_H_")
#define SYMBOL_NAPluginWindow_IDNAME ID_NAGuiPluginWindow
#define SYMBOL_NAPluginWindow_SIZE wxSize(800, 600)
#define SYMBOL_NAPluginWindow_POSITION wxDefaultPosition

class wxUsrPwdDialog : public wxDialog
{
public:
	wxUsrPwdDialog(wxWindow* parent, wxWindowID id, const wxString &title,
					const wxPoint &pos = wxDefaultPosition,
					const wxSize &size = wxDefaultSize,
					long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

	wxString GetUserNameText();
	wxString GetPasswordText();
	void SetUserNameText(wxString usr);
	void SetPasswordText(wxString pwd);

private:
	wxTextCtrl *m_usrtxt;
	wxTextCtrl *m_pwdtxt;
};

struct NAListItemData
{
	int dbid;
	wxString name;
	wxString dbname;
	int swcid;
	int mipid;
	wxString score;
	int itemid;
	int imgid;
	bool visibility;
};

class NAListCtrl : public wxListCtrl, public Notifier
{
	static constexpr int IMG_ID_REF = -2;
	static constexpr int IMG_ID_ALLSIG = -1;

	enum
	{
		Menu_AddTo = wxID_HIGHEST+12201,
		Menu_Save
	};

public:
	NAListCtrl(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos=wxDefaultPosition,
		const wxSize& size=wxDefaultSize,
		long style=wxLC_REPORT|wxLC_SINGLE_SEL);
	~NAListCtrl();

	void Append(int imgid, wxString name, int mipid, bool visibility);
	wxString GetText(long item, int col);
	int GetImageId(long item, int col);

	void SetPlugin(NAGuiPlugin* plugin) { m_plugin = plugin; }
	
	void LoadResults(wxString idpath, wxString volpath, wxString chspec, wxString prefix=wxT(""));
	void UpdateResults();
	wxString GetListFilePath() {return m_rfpath;}

	void DeleteSelection();
	void DeleteAll();
	void AddHistory(const NAListItemData &data);
	void Undo();
	void Redo();


private:
	void OnSelect(wxListEvent &event);
	//void OnAct(wxListEvent &event);
	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnMouse(wxMouseEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void OnScroll(wxMouseEvent& event);
	void OnColBeginDrag(wxListEvent& event);
	void OnLeftDClick(wxMouseEvent& event);
	void OnSize(wxSizeEvent& event);
	
	DECLARE_EVENT_TABLE()
protected: //Possible TODO
	wxSize GetSizeAvailableForScrollTarget(const wxSize& size) {
		return size - GetEffectiveMinSize();
	}

private:
	wxImageList *m_images;
	wxImageList* m_vis_images;
	wxStopWatch m_watch;
	wxArrayString m_dbdirs;
	wxArrayString m_dbpaths;
	wxArrayString m_dbnames;
	std::vector<NAListItemData> m_listdata;
	std::vector<NAListItemData> m_history;
	int m_history_pos;
	wxString m_rfpath;
	NAGuiPlugin* m_plugin;
};

class wxNASettingDialog : public wxDialog
{
	enum
	{
		ID_AddButton = wxID_HIGHEST+12201,
		ID_NAS_RPicker,
		ID_NAS_TempDirPicker,
		ID_NAS_ResultNumText
	};

public:
	wxNASettingDialog(NAGuiPlugin *plugin, wxWindow* parent, wxWindowID id, const wxString &title,
					const wxPoint &pos = wxDefaultPosition,
					const wxSize &size = wxDefaultSize,
					long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

	void LoadSettings();
	void SaveSettings();

private:
	wxFilePickerCtrl* m_RPickCtrl;
	wxDirPickerCtrl* m_tmpdirPickCtrl;
	wxTextCtrl* m_rnumTextCtrl;
	NAGuiPlugin* m_plugin;
public:
	void OnOk( wxCommandEvent& event );

	DECLARE_EVENT_TABLE();
};

class wxImagePanel : public wxPanel
{
    wxImage  m_image;
    wxImage  m_orgimage;
    wxImage  m_olimage;
    wxImage  m_bgimage;
    wxBitmap *m_resized;
    int m_w, m_h;
    
public:
    wxImagePanel(wxWindow* parent, int w, int h);
    ~wxImagePanel();
    void SetImage(const wxImage *img);
    void SetOverlayImage(wxString file, wxBitmapType format, bool show=true);
    void SetBackgroundImage(wxString file, wxBitmapType format);
    void ResetImage();
    void UpdateImage(bool ov_show);
    wxSize CalcImageSizeKeepAspectRatio(int w, int h);
    double GetAspectRatio();
    
    void OnDraw(wxPaintEvent & evt);
    void PaintNow();
    void OnSize(wxSizeEvent& event);
    void Render(wxDC& dc);
    void OnEraseBackground(wxEraseEvent& event);
    
    DECLARE_EVENT_TABLE()
};

class NAGuiPluginWindow: public wxGuiPluginWindowBase, public Observer
{    
    DECLARE_DYNAMIC_CLASS( NAGuiPluginWindow )
    DECLARE_EVENT_TABLE()

	enum
	{
		ID_NAGuiPluginWindow = wxID_HIGHEST+10001,
		ID_NA_RPicker,
		ID_NA_NlibPicker,
		ID_NA_OutputPicker,
		ID_NA_OutFileText,
		ID_NA_ResultNumText,
		ID_NA_ResultPicker,
		ID_NA_OverlayCheckBox,
		ID_SEND_EVENT_BUTTON,
		ID_EDIT_DB_BUTTON,
		ID_SAVE_BUTTON,
		ID_IMPORT_RESULTS_BUTTON,
		ID_SC_FWD,
		ID_SC_MEAN,
		ID_SETTING,
		ID_WaitTimer,
		ID_IdleTimer,
	};

public:
    /// Constructors
    NAGuiPluginWindow();
    NAGuiPluginWindow(wxGuiPluginBase * plugin, wxWindow* parent, wxWindowID id = SYMBOL_NAPluginWindow_IDNAME, const wxPoint& pos = SYMBOL_NAPluginWindow_POSITION, const wxSize& size = SYMBOL_NAPluginWindow_SIZE, long style = SYMBOL_NAPluginWindow_STYLE );

    /// Creation
    bool Create(wxGuiPluginBase * plugin, wxWindow* parent, wxWindowID id = SYMBOL_NAPluginWindow_IDNAME, const wxPoint& pos = SYMBOL_NAPluginWindow_POSITION, const wxSize& size = SYMBOL_NAPluginWindow_SIZE, long style = SYMBOL_NAPluginWindow_STYLE );

    /// Destructor
    ~NAGuiPluginWindow();

    /// Initialises member variables
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

////@begin NAGuiPluginWindow event handler declarations

    void OnSENDEVENTBUTTONClick( wxCommandEvent& event );
	void OnImportResultsButtonClick( wxCommandEvent& event );
	void OnSettingButtonClick( wxCommandEvent& event );
	void OnEditDBButtonClick( wxCommandEvent& event );
	void OnSaveButtonClick( wxCommandEvent& event );
	void OnClose(wxCloseEvent& event);
	void OnShowHide(wxShowEvent& event);
	void OnInteropMessageReceived(wxCommandEvent & event);
	void OnOverlayCheck(wxCommandEvent& event);
	void OnMIPImageExportCheck(wxCommandEvent& event);
	void OnSWCImageExportCheck(wxCommandEvent& event);
	void OnSWCExportCheck(wxCommandEvent& event);
	void OnVolumeExportCheck(wxCommandEvent& event);
	void OnScorePrefixCheck(wxCommandEvent& event);
	void OnDatabasePrefixCheck(wxCommandEvent& event);
	void OnScoringMethodCheck(wxCommandEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnIdle(wxTimerEvent& event);

////@end NAGuiPluginWindow event handler declarations

////@begin NAGuiPluginWindow member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );

	void doAction(ActionInfo *info);

	void EnableControls(bool enable=true);

////@end NAGuiPluginWindow member function declarations

    static bool ShowToolTips();

	static wxWindow* CreateExtraNBLASTControl(wxWindow* parent);

private:

	wxSplitterWindow* m_splitterWindow;
    NAListCtrl* m_results;
	wxImagePanel* m_swcImagePanel;
	wxImagePanel* m_mipImagePanel;
	wxButton* m_CommandButton;
	wxCheckBox* m_overlayChk;
	wxTimer* m_wtimer;
	wxProgressDialog* m_prg_diag;
	bool m_waitingforR;
	bool m_waitingforFiji;
	std::vector<wxCheckBox*> m_nlib_chks;
	wxStaticBoxSizer *m_nlib_box;
	wxPanel *m_nbpanel;
	wxPanel* m_imgpanel;
	wxToolBar *m_tb;
	wxTimer *m_idleTimer;
	wxRadioButton *m_sc_forward;
	wxRadioButton *m_sc_mean;
};

#endif
    // _NeuronAnnotatorPluginWindow_H_
