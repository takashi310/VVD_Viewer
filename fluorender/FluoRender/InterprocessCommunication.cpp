#include "InterprocessCommunication.h"
#include "VRenderFrame.h"
#include <wx/tokenzr.h>

MyClient::~MyClient(void)
{
	if (m_connection)
	{
		m_connection->Disconnect();
		delete m_connection;
	}
}

MyServer::~MyServer(void)
{
	if (m_connection)
	{
		m_connection->Disconnect();
		delete m_connection;
	}
}

bool ServerConnection::OnAdvise(const wxString& topic, const wxString& item, char *data,
				                int size, wxIPCFormat format)
{
	wxMessageBox(topic, data);

	return true;
}

bool ServerConnection::OnStartAdvise(const wxString& topic, const wxString& item)
{
	wxMessageBox(wxString::Format("OnStartAdvise(\"%s\",\"%s\")", topic.c_str(), item.c_str()));
	if (!m_vframe) return false;

	SettingDlg *setting_dlg = m_vframe->GetSettingDlg();
	if (setting_dlg)
		m_vframe->SetRealtimeCompression(setting_dlg->GetRealtimeCompress());

	wxArrayString files;
	wxStringTokenizer tkz(item, wxT(","));
	while(tkz.HasMoreTokens())
	{
		wxString path = tkz.GetNextToken();
		files.Add(path);
	}
	m_vframe->StartupLoad(files);

	if (setting_dlg)
	{
		setting_dlg->SetRealtimeCompress(m_vframe->GetRealtimeCompression());
		setting_dlg->UpdateUI();
	}

	return true;
}

bool ServerConnection::OnStopAdvise(const wxString& topic, const wxString& item)
{
	//wxMessageBox(wxString::Format("OnStopAdvise(\"%s\",\"%s\")", topic.c_str(), item.c_str()));
	return true;
}

bool ClientConnection::OnAdvise(const wxString& topic, const wxString& item, char *data,
				                int size, wxIPCFormat format)
{
	//wxMessageBox(topic, data);
	return true;
}

