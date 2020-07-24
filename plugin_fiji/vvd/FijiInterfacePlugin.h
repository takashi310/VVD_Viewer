#pragma once

#include "stdwx.h"
#include <wxGuiPluginBase.h>
#include "utility.h"

#define FI_PLUGIN_VERSION "1.01"

#define FI_CONNECT 1
#define FI_VERSION_CHECK 2
#define FI_VOLUMEDATA 3
#define FI_COMMAND_FINISHED 4
#define FI_CONFIRM 5
#define FI_PID 6
#define FI_RUN 7
#define TMP_OVOX_NONE 0
#define TMP_OVOX_TRUE 1
#define TMP_OVOX_FALSE 2

class FijiServerConnection : public wxConnection, public Notifier
{
public:
    FijiServerConnection(void) : wxConnection() { m_vframe = NULL; }
    ~FijiServerConnection(void) { char ret[1] = {11}; m_sock->Write(ret, 1); }

	void SetTimeout(long seconds);
	long GetTimeout();
    
    void SetSndBufSize(size_t size);

    bool OnAdvise(const wxString& topic, const wxString& item, char *data,
                  int size, wxIPCFormat format);
	bool OnStartAdvise(const wxString& topic, const wxString& item);
	bool OnStopAdvise(const wxString& topic,
		const wxString& item);
	bool OnPoke(const wxString& topic, const wxString& item, const void *data, size_t size, wxIPCFormat format);
	bool OnExec(const wxString &topic, const wxString &data);

	void SetFrame(wxWindow *vframe){ m_vframe = vframe; }

protected:
	wxWindow *m_vframe;
	wxString m_advise;
};

class FijiServer : public wxServer, public Notifier
{
public:
	FijiServer(void) : wxServer() { m_connection = NULL; m_vframe = NULL;}
	~FijiServer(void);
	FijiServerConnection *GetConnection() { return m_connection; };

    wxConnectionBase* OnAcceptConnection(const wxString &topic);
    
	void SetFrame(wxWindow *vframe){ m_vframe = vframe; }
    
    void DeleteConnection();

protected:
	FijiServerConnection *m_connection;
	wxWindow *m_vframe;
};

class SampleGuiPlugin1 : public wxGuiPluginBase, public Observer, public Notifier
{
	DECLARE_DYNAMIC_CLASS(SampleGuiPlugin1)
public:
	SampleGuiPlugin1();
	SampleGuiPlugin1(wxEvtHandler * handler, wxWindow * vvd);
	virtual ~SampleGuiPlugin1();

	virtual wxString GetName() const;
    virtual wxString GetDisplayName() const;
	virtual wxString GetId() const;
	virtual wxWindow * CreatePanel(wxWindow * parent);
	virtual void OnInit();
	virtual void OnDestroy();
	virtual bool OnRun(wxString options);

	void SetFijiPath(wxString path) { m_fiji_path = path; }
	wxString GetFijiPath() { return m_fiji_path; }
	wxString GetFijiPluginVer() { return m_fiji_plugin_ver; }
	void SetSendMask(bool val) { m_sendmask = val; }
	bool GetSendMask() { return m_sendmask; }
	void SetLaunchFijiAtStartup(bool val) { m_launch_fiji_startup = val; }
	bool GetLaunchFijiAtStartup() { return m_launch_fiji_startup; }
    void SetTempOverrideVox(int flag) { m_tmp_ovvox = flag; }
    int GetTempOverrideVox() { return m_tmp_ovvox; }

	wxString GetPID() { return m_pid; }

	void LoadConfigFile();
	void SaveConfigFile();

	bool StartFiji();
	void CloseFiji();
	bool isReady();

	void doAction(ActionInfo *info);

	bool SendCommand(wxString command, bool send_mask=false);
    bool SendCurrentVolume(bool send_mask);

	void OnTimer(wxTimerEvent& event);

private:
	wxString m_fiji_path;
	bool m_initialized;
	bool m_booting;
	bool m_sendmask;
	bool m_launch_fiji_startup;
    int m_tmp_ovvox;
    bool m_ovvox;
	bool m_gpsyncspc;
	bool m_gpsync;
	wxTimer m_timer;
	wxStopWatch m_watch;
	wxString m_pid;
	wxString m_sent_mask;
	FijiServer *m_server;
	wxString m_fiji_plugin_ver;
	wxProcess *m_fijiprocess;
};
