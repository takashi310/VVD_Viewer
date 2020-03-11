#pragma once

#include <wxGuiPluginBase.h>
#include "utility.h"

#define NB_PLUGIN_VERSION "1.00"

#define NB_OPEN_FILE 1
#define NB_SET_IMAGE 2

class NBLASTGuiPlugin : public wxGuiPluginBase, public Notifier
{
	DECLARE_DYNAMIC_CLASS(NBLASTGuiPlugin)
public:
	NBLASTGuiPlugin();
	NBLASTGuiPlugin(wxEvtHandler * handler, wxWindow * vvd);
	virtual ~NBLASTGuiPlugin();

	virtual wxString GetName() const;
	virtual wxString GetId() const;
	virtual wxWindow * CreatePanel(wxWindow * parent);
	virtual void OnInit();
	virtual void OnDestroy();
	virtual bool OnRun(wxString options);

	void SetRPath(wxString path) { m_R_path = path; }
	wxString GetRPath() { return m_R_path; }
	void SetNlibPath(wxString path) { m_nlib_path = path; }
	wxString GetNlibPath() { return m_nlib_path; }
	void SetOutDir(wxString path) { m_out_dir = path; }
	wxString GetOutDir() { return m_out_dir; }
	void SetFileName(wxString name) { m_ofname = name; }
	wxString GetFileName() { return m_ofname; }
	void SetResultNum(wxString num) { m_rnum = num; }
	wxString GetResultNum() { return m_rnum; }
	void SetDatabaseNames(wxString num) { m_db_names = num; }
	wxString GetDatabaseNames() { return m_db_names; }
	void SetScoringMethod(wxString mtd) { m_scmtd = mtd; }
	wxString GetScoringMethod() { return m_scmtd; }
	static void SetExportMIPs(bool val) { m_exp_mip = val; }
	static bool GetExportMIPs() { return m_exp_mip; }
	static void SetExportSWCPrevImgs(bool val) { m_exp_swcprev = val; }
	static bool GetExportSWCPrevImgs() { return m_exp_swcprev; }
	static void SetExportSWCs(bool val) { m_exp_swc = val; }
	static bool GetExportSWCs() { return m_exp_swc; }
	static void SetExportVolumes(bool val) { m_exp_vol = val; }
	static bool GetExportVolumes() { return m_exp_vol; }
	static void SetPrefixScore(bool val) { m_pfx_score = val; }
	static bool GetPrefixScore() { return m_pfx_score; }
	static void SetPrefixDatabase(bool val) { m_pfx_db = val; }
	static bool GetPrefixDatabase() { return m_pfx_db; }

	wxString GetPID() { return m_pid; }

	void LoadConfigFile();
	void SaveConfigFile();

	bool runNBLAST();
	bool runNBLAST(wxString rpath, wxString nlibpath, wxString outdir, wxString ofname, wxString rnum, wxString db_names=wxString(), wxString scmethod=wxT("forward"));
	bool runNBLASTremote(wxString url, wxString usr, wxString pwd, wxString nlibpath, wxString outdir, wxString ofname, wxString rnum, wxString db_names=wxString(), wxString scmethod=wxT("forward"));
	bool LoadSWC(wxString name, wxString swc_zip_path);
	bool LoadFiles(wxString path);
	bool skeletonizeMask();

	void OnTimer(wxTimerEvent& event);

private:
	wxString m_R_path;
	wxString m_out_dir;
	wxString m_nlib_path;
	wxString m_ofname;
	wxString m_rnum;
	wxString m_db_names;
	wxString m_scmtd;
	wxTimer m_timer;
	wxStopWatch m_watch;
	wxString m_pid;
	wxProcess *m_R_process;

	static bool m_exp_swc;
	static bool m_exp_swcprev;
	static bool m_exp_mip;
	static bool m_exp_vol;
	static bool m_pfx_score;
	static bool m_pfx_db;
};