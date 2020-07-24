#ifndef _FL_IPC_H_
#define _FL_IPC_H_

#define wxUSE_DDE_FOR_IPC 0

#include <wx/wx.h>
#include <wx/ipc.h>
#include "DLLExport.h"

class VRenderFrame;

class EXPORT_API ServerConnection: public wxConnection
{
public:
    ServerConnection(void) : wxConnection() { m_vframe = NULL; }
    ~ServerConnection(void) { }

    bool OnAdvise(const wxString& topic, const wxString& item, char *data,
                  int size, wxIPCFormat format);
	bool OnStartAdvise(const wxString& topic, const wxString& item);
	bool OnStopAdvise(const wxString& topic,
		const wxString& item);

	void SetFrame(VRenderFrame *vframe){ m_vframe = vframe; }

protected:
	VRenderFrame *m_vframe;
	wxString m_advise;
};

class EXPORT_API ClientConnection: public wxConnection
{
public:
    ClientConnection(void) : wxConnection() { }
    ~ClientConnection(void) { }

    bool OnAdvise(const wxString& topic, const wxString& item, char *data,
                  int size, wxIPCFormat format);
};

class EXPORT_API MyClient: public wxClient
{
public:
	MyClient(void) : wxClient() { m_connection = NULL; }
	~MyClient(void);
	ClientConnection *GetConnection() { return m_connection; };

    wxConnectionBase* OnMakeConnection(void)
    {
        m_connection = new ClientConnection;
		return m_connection;
    }

protected:
	ClientConnection *m_connection;
};

class EXPORT_API MyServer: public wxServer
{
public:
	MyServer(void) : wxServer() { m_connection = NULL; m_vframe = NULL;}
	~MyServer(void);
	ServerConnection *GetConnection() { return m_connection; };

    wxConnectionBase* OnAcceptConnection(const wxString &topic)
    {
        m_connection = new ServerConnection;
		m_connection->SetFrame(m_vframe);
		return m_connection;
    }

	void SetFrame(VRenderFrame *vframe){ m_vframe = vframe; }

protected:
	ServerConnection *m_connection;
	VRenderFrame *m_vframe;
};

#endif//_FL_IPC_H_