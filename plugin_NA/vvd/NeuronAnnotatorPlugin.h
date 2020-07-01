#pragma once

#include <wxGuiPluginBase.h>
#include "utility.h"
#include "VRenderFrame.h"

#define NA_PLUGIN_VERSION "1.00"

#define NA_TOGGLE_VISIBILITY 1
#define NA_SET_VISIBLE 2
#define NA_SET_INVISIBLE 3
#define NA_SET_ACTIVE 4
#define NA_SET_VISIBLE_WITHOUT_REFRESH 5
#define NA_SET_ACTIVE_WITHOUT_REFRESH 6
#define NA_PUSH_HISTORY 7
#define NA_REFRESH_RENDERVIEWS 8
#define NA_SWITCH_PROPPANEL 9


struct NASegment
{
    int id;
    size_t size;
    BBox bbox;
    wxImage image;
	wxImage thumbnail;
	int visible;
};

class NAGuiPlugin : public wxGuiPluginBase, public Notifier
{
	DECLARE_DYNAMIC_CLASS(NAGuiPlugin)
public:
	NAGuiPlugin();
	NAGuiPlugin(wxEvtHandler * handler, wxWindow * vvd);
	virtual ~NAGuiPlugin();

	virtual wxString GetName() const;
    virtual wxString GetDisplayName() const;
	virtual wxString GetId() const;
	virtual wxWindow * CreatePanel(wxWindow * parent);
	virtual void OnInit();
	virtual void OnDestroy();
	virtual void OnTreeUpdate();
	virtual bool OnRun(wxString options);

	void Lock() { m_lock = true; }
	void Unlock() { m_lock = false; }

	void SetIDXImagePath(wxString path) { m_id_path = path; }
	wxString GetIDXImagePath() { return m_id_path; }
    void SetVolImagePath(wxString path) { m_vol_path = path; }
    wxString GetVolImagePath() { return m_vol_path; }
	
	wxString GetPID() { return m_pid; }

	void LoadConfigFile();
	void SaveConfigFile();

	bool runNALoader();
	bool runNALoader(wxString id_path, wxString vol_path, wxString chflags=wxT("sssr"), wxString spacings=wxT(""), wxString prefix=wxT(""), bool set_dirty=false);
	bool runNALoaderRemote(wxString url, wxString usr, wxString pwd, wxString outdir, wxString ofname);
	bool LoadFragment(wxString name, int id);
    bool LoadSWC(wxString name, wxString swc_zip_path);
    bool LoadFiles(wxString path, size_t datasize = 0ULL);
	bool LoadNrrd(int id);
	void SetSegmentVisibility(int id, int vis, bool refresh = true);
	int GetSegmentVisibility(int id);
	void ToggleSegmentVisibility(int id);
	void PushVisHistory();
    void RefreshRenderViews();
    
    void SwitchPropPanel(bool ref);
    
    void SaveCombinedFragment(wxString path);

	void ResizeThumbnails(int w);
    
	void OnTimer(wxTimerEvent& event);

	wxImage* getSegMIPThumbnail(int id=0);
	wxImage* getSegMIP(int id=-1);
	wxImage* getRefMIPThumbnail();
	wxImage* getRefMIP();
	int getSegCount() { return m_segs.size(); }
    
    bool isRefExists() { return m_nrrd_r ? true : false; }
    bool isSegExists() { return m_nrrd_s[0] ? true : false; }
    
    static bool sort_data_asc(const NASegment s1, const NASegment s2)
    { return s2.size < s1.size; }

	bool isDirty() { return m_dirty; }
	void setDirty(bool val) { m_dirty = val; }
    bool isReloaded() { return m_reload_list; }
    void setReloaded(bool val) { m_reload_list = val; }

	V3DPBDReader* m_lbl_reader;
	BaseReader* m_vol_reader;
	vector<NASegment> m_segs;
	wxImage m_ref_image;
	wxImage m_sig_image;
	wxImage m_ref_image_thumb;
	wxImage m_sig_image_thumb;

	Nrrd* m_lbl_nrrd;
	Nrrd* m_nrrd_r;
	wxString m_vol_r;
	Nrrd* m_nrrd_s[3];
	wxString m_vol_s[3];

	double m_gmaxvals[3];

	wxCriticalSection m_pThreadCS;
	int m_running_mip_th;

	int m_scount;

	double m_xspc;
	double m_yspc;
	double m_zspc;

	wxString m_vol_suffix;

	bool m_dirty;
    
    bool m_reload_list;

	bool m_allsig_visible;

	bool m_lock;
	
private:
	wxString m_id_path;
	wxString m_vol_path;
	wxString m_prefix;
	wxTimer m_timer;
	wxStopWatch m_watch;
	wxString m_pid;
    
};

class MIPGeneratorThread : public wxThread
{
public:
	MIPGeneratorThread(NAGuiPlugin* plugin, const std::vector<int>& queue);
	~MIPGeneratorThread();
protected:
	virtual ExitCode Entry();
	NAGuiPlugin* m_plugin;
	std::vector<int> m_queue;
};
