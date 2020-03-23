#pragma once

#include <wxGuiPluginBase.h>
#include "utility.h"
#include "VRenderFrame.h"

#define NA_PLUGIN_VERSION "1.00"

#define NA_OPEN_FILE 1
#define NA_SET_IMAGE 2

struct NASegment
{
    int id;
    size_t size;
    BBox bbox;
    wxImage image;
};

class NAGuiPlugin : public wxGuiPluginBase, public Notifier
{
	DECLARE_DYNAMIC_CLASS(NAGuiPlugin)
public:
	NAGuiPlugin();
	NAGuiPlugin(wxEvtHandler * handler, wxWindow * vvd);
	virtual ~NAGuiPlugin();

	virtual wxString GetName() const;
	virtual wxString GetId() const;
	virtual wxWindow * CreatePanel(wxWindow * parent);
	virtual void OnInit();
	virtual void OnDestroy();
	virtual bool OnRun(wxString options);

	void SetIDXImagePath(wxString path) { m_id_path = path; }
	wxString GetIDXImagePath() { return m_id_path; }
    void SetVolImagePath(wxString path) { m_vol_path = path; }
    wxString GetVolImagePath() { return m_vol_path; }
	
	wxString GetPID() { return m_pid; }

	void LoadConfigFile();
	void SaveConfigFile();

	bool runNALoader();
	bool runNALoader(wxString id_path, wxString vol_path);
	bool runNALoaderRemote(wxString url, wxString usr, wxString pwd, wxString outdir, wxString ofname);
	bool LoadFragment(wxString name, int id);
    bool LoadSWC(wxString name, wxString swc_zip_path);
    bool LoadFiles(wxString path);
    
	void OnTimer(wxTimerEvent& event);
    
    V3DPBDReader *m_lbl_reader;
    H5JReader *m_vol_reader;
    vector<NASegment> m_segs;
    static bool sort_data_asc(const NASegment s1, const NASegment s2)
    { return s2.size < s1.size; }

private:
	wxString m_id_path;
	wxString m_vol_path;
	wxTimer m_timer;
	wxStopWatch m_watch;
	wxString m_pid;
    
};
