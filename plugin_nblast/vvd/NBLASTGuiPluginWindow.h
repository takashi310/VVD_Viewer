#ifndef _NBLASTGuiPluginWindow_H_
#define _NBLASTGuiPluginWindow_H_

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

#define SYMBOL_NBLASTGuiPluginWindow_STYLE wxTAB_TRAVERSAL
#define SYMBOL_NBLASTGuiPluginWindow_TITLE _("NBLASTGuiPluginWindow")
#define SYMBOL_NBLASTGuiPluginWindow_IDNAME ID_NBLASTGuiPluginWindow
#define SYMBOL_NBLASTGuiPluginWindow_SIZE wxSize(400, 300)
#define SYMBOL_NBLASTGuiPluginWindow_POSITION wxDefaultPosition

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

struct NBLASTDBListItemData
{
	wxString name;
	wxString path;
	wxString usr;
	wxString pwd;
	bool state;
};

class NBLASTDatabaseListCtrl : public wxListCtrl
{
	enum
	{
		ID_NameDispText = wxID_HIGHEST+12001,
		ID_ColorPicker,
		ID_DescriptionText,
		ID_Setting
	};

public:
	NBLASTDatabaseListCtrl(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style=wxLC_REPORT|wxLC_SINGLE_SEL);
	~NBLASTDatabaseListCtrl();

	
	void Add(wxString path, wxString name=wxString(), bool selection=false);
	void UpdateList();
	void UpdateText();
	void DeleteSelection();
	void DeleteAll();

	void SetColumnWidthAuto();

	wxString GetText(long item, int col);
	void SetText(long item, int col, const wxString &str);
	
	void SetState(size_t id, bool state) { if (id < m_list.size()) m_list[id].state = state; }
	bool GetState(size_t id)
	{
		if (id < m_list.size()) return m_list[id].state;
		else return false;
	}

	void SetUsrPwd(size_t id, wxString usr, wxString pwd)
	{
		if (id < m_list.size())
		{
			m_list[id].usr = usr;
			m_list[id].pwd = pwd;
		}
	}
	void GetUsrPwd(size_t id, wxString &usr, wxString &pwd)
	{
		if (id < m_list.size())
		{
			usr = m_list[id].usr;
			pwd = m_list[id].pwd;
		}
		else
		{
			usr = wxString();
			pwd = wxString();
		}
	}

	std::vector<NBLASTDBListItemData> getList() { return m_list; }
private:
	
	wxTextCtrl *m_name_disp;
	wxFilePickerCtrl *m_path_disp;
	wxButton *m_setting_b;
	std::vector<NBLASTDBListItemData> m_list;

	long m_editing_item;
	long m_dragging_to_item;
	long m_dragging_item;

private:
	void Append(wxString path, wxString name=wxString());
	void EndEdit();
	void OnAct(wxListEvent &event);
	void OnEndSelection(wxListEvent &event);
	void OnSelection(wxListEvent &event);
	void OnNameDispText(wxCommandEvent& event);
	void OnPathText(wxCommandEvent& event);
	void OnEnterInTextCtrl(wxCommandEvent& event);
	void OnBeginDrag(wxListEvent& event);
	void OnDragging(wxMouseEvent& event);
	void OnEndDrag(wxMouseEvent& event);
	void OnLeftDClick(wxMouseEvent& event);

	void OnColumnSizeChanged(wxListEvent &event);

	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);

	void OnScroll(wxScrollWinEvent& event);
	void OnScroll(wxMouseEvent& event);

	void OnResize(wxSizeEvent& event);

	void OnSettingButton(wxCommandEvent &event);

	void ShowTextCtrls(long item);

	DECLARE_EVENT_TABLE()
protected: //Possible TODO
	wxSize GetSizeAvailableForScrollTarget(const wxSize& size) {
		return size - GetEffectiveMinSize();
	}
};


class wxDBListDialog : public wxDialog
{
	enum
	{
		ID_AddButton = wxID_HIGHEST+12101,
		ID_NewDBPick
	};

public:
	wxDBListDialog(wxWindow* parent, wxWindowID id, const wxString &title,
					const wxPoint &pos = wxDefaultPosition,
					const wxSize &size = wxDefaultSize,
					long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

	std::vector<NBLASTDBListItemData> getList(){ return m_list ? m_list->getList() : std::vector<NBLASTDBListItemData>(); }
	
	void setState(size_t id, bool state) { if (m_list) m_list->SetState(id, state); }
	bool getState(size_t id)
	{
		if (m_list) return m_list->GetState(id);
		else return false;
	}

	void LoadList();
	void SaveList();

private:
	NBLASTDatabaseListCtrl *m_list;
	wxFilePickerCtrl *m_new_db_pick;
	wxButton* m_add_button;

public:
	void OnAddButtonClick( wxCommandEvent& event );
	void OnOk( wxCommandEvent& event );

	DECLARE_EVENT_TABLE();
};


class wxImagePanel : public wxPanel
{
	wxImage  m_image;
	wxImage  *m_orgimage;
	wxImage  m_olimage;
	wxImage  m_bgimage;
	wxBitmap *m_resized;
	int m_w, m_h;

public:
	wxImagePanel(wxWindow* parent, int w, int h);
	~wxImagePanel();
	void SetImage(wxString file, wxBitmapType format);
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

struct NBLASTListItemData
{
	int dbid;
	wxString name;
	wxString dbname;
	int swcid;
	int mipid;
	wxString score;
	int itemid;
};

class NBLASTListCtrl : public wxListCtrl, public Notifier
{
	enum
	{
		Menu_AddTo = wxID_HIGHEST+12201,
		Menu_Save
	};

public:
	NBLASTListCtrl(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos=wxDefaultPosition,
		const wxSize& size=wxDefaultSize,
		long style=wxLC_REPORT|wxLC_SINGLE_SEL);
	~NBLASTListCtrl();

	void Append(wxString name, wxString dbname, wxString score, int mipid=-1, int swcid=-1, int dbid=-1, int index=-1);
	wxString GetText(long item, int col);
	int GetImageId(long item, int col);
	
	void LoadResults(wxString csvfilepath);
	void SaveResults(wxString txtpath, bool export_swc=false, bool export_swcprev=false, bool export_mip=false, bool export_vol=false, bool pfx_score=false, bool pfx_db=false, bool zip=false);
	wxString GetListFilePath() {return m_rfpath;}

	void DeleteSelection();
	void DeleteAll();
	void AddHistory(const NBLASTListItemData &data);
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
	
	DECLARE_EVENT_TABLE()
protected: //Possible TODO
	wxSize GetSizeAvailableForScrollTarget(const wxSize& size) {
		return size - GetEffectiveMinSize();
	}

private:
	wxImageList *m_images;
	wxStopWatch m_watch;
	wxArrayString m_dbdirs;
	wxArrayString m_dbpaths;
	wxArrayString m_dbnames;
	std::vector<NBLASTListItemData> m_listdata;
	std::vector<NBLASTListItemData> m_history;
	int m_history_pos;
	wxString m_rfpath;
};

class wxNBLASTSettingDialog : public wxDialog
{
	enum
	{
		ID_AddButton = wxID_HIGHEST+12201,
		ID_NBS_RPicker,
		ID_NBS_TempDirPicker,
		ID_NBS_ResultNumText
	};

public:
	wxNBLASTSettingDialog(NBLASTGuiPlugin *plugin, wxWindow* parent, wxWindowID id, const wxString &title,
					const wxPoint &pos = wxDefaultPosition,
					const wxSize &size = wxDefaultSize,
					long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

	void LoadSettings();
	void SaveSettings();

private:
	wxFilePickerCtrl* m_RPickCtrl;
	wxDirPickerCtrl* m_tmpdirPickCtrl;
	wxTextCtrl* m_rnumTextCtrl;
	NBLASTGuiPlugin* m_plugin;
public:
	void OnOk( wxCommandEvent& event );

	DECLARE_EVENT_TABLE();
};

class NBLASTGuiPluginWindow: public wxGuiPluginWindowBase, public Observer
{    
    DECLARE_DYNAMIC_CLASS( NBLASTGuiPluginWindow )
    DECLARE_EVENT_TABLE()

	enum
	{
		ID_NBLASTGuiPluginWindow = wxID_HIGHEST+10001,
		ID_NB_RPicker,
		ID_NB_NlibPicker,
		ID_NB_OutputPicker,
		ID_NB_OutFileText,
		ID_NB_ResultNumText,
		ID_NB_ResultPicker,
		ID_NB_OverlayCheckBox,
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
    NBLASTGuiPluginWindow();
    NBLASTGuiPluginWindow(wxGuiPluginBase * plugin, wxWindow* parent, wxWindowID id = SYMBOL_NBLASTGuiPluginWindow_IDNAME, const wxPoint& pos = SYMBOL_NBLASTGuiPluginWindow_POSITION, const wxSize& size = SYMBOL_NBLASTGuiPluginWindow_SIZE, long style = SYMBOL_NBLASTGuiPluginWindow_STYLE );

    /// Creation
    bool Create(wxGuiPluginBase * plugin, wxWindow* parent, wxWindowID id = SYMBOL_NBLASTGuiPluginWindow_IDNAME, const wxPoint& pos = SYMBOL_NBLASTGuiPluginWindow_POSITION, const wxSize& size = SYMBOL_NBLASTGuiPluginWindow_SIZE, long style = SYMBOL_NBLASTGuiPluginWindow_STYLE );

    /// Destructor
    ~NBLASTGuiPluginWindow();

    /// Initialises member variables
    void Init();

    /// Creates the controls and sizers
    void CreateControls();

////@begin NBLASTGuiPluginWindow event handler declarations

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

////@end NBLASTGuiPluginWindow event handler declarations

////@begin NBLASTGuiPluginWindow member function declarations

    /// Retrieves bitmap resources
    wxBitmap GetBitmapResource( const wxString& name );

    /// Retrieves icon resources
    wxIcon GetIconResource( const wxString& name );

	void doAction(ActionInfo *info);

	void EnableControls(bool enable=true);

////@end NBLASTGuiPluginWindow member function declarations

    static bool ShowToolTips();

	static wxWindow* CreateExtraNBLASTControl(wxWindow* parent);

private:

	wxSplitterWindow* m_splitterWindow;
    NBLASTListCtrl* m_results;
	wxImagePanel* m_swcImagePanel;
	wxImagePanel* m_mipImagePanel;
	wxButton* m_CommandButton;
	wxCheckBox* m_overlayChk;
	wxTimer* m_wtimer;
	wxProgressDialog* m_prg_diag;
	bool m_waitingforR;
	bool m_waitingforFiji;
	std::vector<NBLASTDBListItemData> m_nlib_list;
	std::vector<wxCheckBox*> m_nlib_chks;
	wxStaticBoxSizer *m_nlib_box;
	wxPanel *m_nbpanel;
	wxToolBar *m_tb;
	wxTimer *m_idleTimer;
	wxRadioButton *m_sc_forward;
	wxRadioButton *m_sc_mean;
};

#endif
    // _NBLASTGuiPluginWindow_H_
