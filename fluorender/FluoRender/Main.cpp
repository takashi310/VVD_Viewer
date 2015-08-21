//#include "Check.h"
#include <cstdio>
#include <iostream>
//#include <fstream>
#include <wx/wx.h>
#include <wx/cmdline.h>
#include <wx/stdpaths.h>
#include <wx/filefn.h>
#include <wx/tokenzr.h>
#include "VRenderFrame.h"
#include "compatibility.h"
#include <wx/ipc.h>
// -- application --

bool m_open_by_web_browser = false;

class ServerConnection: public wxConnection
{
public:
    ServerConnection(void) : wxConnection() { m_vframe = NULL; }
    ~ServerConnection(void) { }

    bool OnAdvise(const wxString& topic, const wxString& item, char *data,
                  int size, wxIPCFormat format)
    {
        //wxMessageBox(topic, data);
    }

	bool OnStartAdvise(const wxString& topic,
		const wxString& item)
	{
		//wxMessageBox(wxString::Format("OnStartAdvise(\"%s\",\"%s\")", topic.c_str(), item.c_str()));
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
	bool OnStopAdvise(const wxString& topic,
		const wxString& item)
	{
		//wxMessageBox(wxString::Format("OnStopAdvise(\"%s\",\"%s\")", topic.c_str(), item.c_str()));
		return true;
	}

	void SetFrame(VRenderFrame *vframe){ m_vframe = vframe; }

protected:
	VRenderFrame *m_vframe;
	wxString m_advise;
};

class ClientConnection: public wxConnection
{
public:
    ClientConnection(void) : wxConnection() { }
    ~ClientConnection(void) { }

    bool OnAdvise(const wxString& topic, const wxString& item, char *data,
                  int size, wxIPCFormat format)
    {
        //wxMessageBox(topic, data);
    }
};

class MyClient: public wxClient
{
public:
	MyClient(void) : wxClient() { m_connection = NULL; }
	ClientConnection *GetConnection() { return m_connection; };

    wxConnectionBase* OnMakeConnection(void)
    {
        m_connection = new ClientConnection;
		return m_connection;
    }

protected:
	ClientConnection *m_connection;
};

class MyServer: public wxServer
{
public:
	MyServer(void) : wxServer() { m_connection = NULL; m_vframe = NULL;}
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


class VRenderApp : public wxApp
{
   public:
	   VRenderApp(void) : wxApp() { m_server = NULL; m_client = NULL; }
      virtual bool OnInit();
      void OnInitCmdLine(wxCmdLineParser& parser);
      bool OnCmdLineParsed(wxCmdLineParser& parser);

   private:
      wxArrayString m_files;
	  MyClient *m_client;
	  MyServer *m_server;
};

static const wxCmdLineEntryDesc g_cmdLineDesc [] =
{
   { wxCMD_LINE_PARAM, NULL, NULL, NULL,
      wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL|wxCMD_LINE_PARAM_MULTIPLE },
   { wxCMD_LINE_NONE }
};

DECLARE_APP(VRenderApp)
IMPLEMENT_APP(VRenderApp)

bool VRenderApp::OnInit()
{
#ifdef _WIN32
#include <direct.h>
#include <shellapi.h>
#define GetCurrentDir _getcwd
	wxStandardPaths::Get().DontIgnoreAppSubDir();
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif
	char cpath[FILENAME_MAX];
   GetCurrentDir(cpath, sizeof(cpath));
   ::wxSetWorkingDirectory(wxString(s2ws(std::string(cpath))));
   // call default behaviour (mandatory)
   if (!wxApp::OnInit())
      return false;

   //add png handler
   wxImage::AddHandler(new wxPNGHandler);
   //the frame
   std::string title =  std::string(FLUORENDER_TITLE) + std::string(" ") +
      std::string(VERSION_MAJOR_TAG) +  std::string(".") +
      std::string(VERSION_MINOR_TAG);
   wxFrame* frame = new VRenderFrame(
         (wxFrame*) NULL,
         wxString(title),
         50,50,1024,768);
   
   if(m_server)
	   m_server->SetFrame((VRenderFrame *)frame);

#ifdef WITH_DATABASE
#ifdef _WIN32
   bool registered = true;

   wxString install = wxStandardPaths::Get().GetLocalDataDir() + wxFileName::GetPathSeparator() + "install";
   if (wxFileName::FileExists(install))
   {
	   wxFile tmpif(install, wxFile::read);
	   wxString data;
	   tmpif.ReadAll(&data);
	   if(data != install) registered = false;
   }
   else registered = false;

   if (!registered)
   {

	   wxString regfile = wxStandardPaths::Get().GetLocalDataDir() + wxFileName::GetPathSeparator() + "regist.reg";
	   
	   wxString exepath = wxStandardPaths::Get().GetExecutablePath();
	   exepath.Replace(wxT("\\"), wxT("\\\\"));

	   wxString reg("Windows Registry Editor Version 5.00\n"\
					"[HKEY_CLASSES_ROOT\\vvd]\n"\
					"@=\"URL:VVD Protocol\"\n"\
					"\"URL Protocol\"=\"\"\n"\
					"[HKEY_CLASSES_ROOT\\vvd\\DefaultIcon]\n"\
					"[HKEY_CLASSES_ROOT\\vvd\\shell]\n"\
					"[HKEY_CLASSES_ROOT\\vvd\\shell\\open]\n"\
					"[HKEY_CLASSES_ROOT\\vvd\\shell\\open\\command]\n"\
					"@=\"\\\"");
	   reg += exepath;
	   reg += wxT("\\\" \\\"%1\\\"\"");

	   wxFile tmpof(regfile, wxFile::write);
	   tmpof.Write(reg);
	   tmpof.Close();

	   wxString regfilearg = wxT("/s ") + regfile;
	   TCHAR args[1024];
	   const wxChar* regfileargChars = regfilearg.c_str();  
	   for (int i = 0; i < regfilearg.Len(); i++) {
		   args[i] = regfileargChars [i];
	   }
	   args[regfilearg.Len()] = _T('\0');
	   ShellExecute(NULL, NULL, _T("regedit.exe"), args, NULL, SW_HIDE);

	   wxFile of(install, wxFile::write);
	   of.Write(install);
	   of.Close();
   }
#endif
#endif
   
   SetTopWindow(frame);
   frame->Show();

   SettingDlg *setting_dlg = ((VRenderFrame *)frame)->GetSettingDlg();
   if (setting_dlg)
	   ((VRenderFrame *)frame)->SetRealtimeCompression(setting_dlg->GetRealtimeCompress());

   if (m_files.Count()>0)
      ((VRenderFrame*)frame)->StartupLoad(m_files);

   if (setting_dlg)
   {
	   setting_dlg->SetRealtimeCompress(((VRenderFrame *)frame)->GetRealtimeCompression());
	   setting_dlg->UpdateUI();
   }

   return true;
}

void VRenderApp::OnInitCmdLine(wxCmdLineParser& parser)
{
   parser.SetDesc (g_cmdLineDesc);
}

bool VRenderApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	//ofstream ofs("D:\\test.txt");
   int i=0;
   wxString params;
   for (i = 0; i < (int)parser.GetParamCount(); i++)
   {
      wxString file = parser.GetParam(i);
	  //ofs << file.ToStdString() << endl;

	  if (file.StartsWith(wxT("vvd:")))
	  {
		  m_open_by_web_browser = true;
		  if(file.Length() >= 5)
		  {
			  params = file.SubString(4, file.Length()-1);
			  wxStringTokenizer tkz(params, wxT(","));
			  while(tkz.HasMoreTokens())
			  {
				  wxString path = tkz.GetNextToken();
				  m_files.Add(path);
				  //ofs << path.ToStdString() << endl;
			  }
		  }
		  break;
	  }
	  
	  m_files.Add(file);

   }

#ifdef WITH_DATABASE
   {
	   wxString message;

	   int fnum = m_files.GetCount();
	   for (i = 0; i < fnum; i++) message += m_files[i] + (i == fnum - 1 ? wxT("") : wxT(","));

	   wxLogNull lognull;

	   wxString server = "50001";
	   wxString hostName = "VVDClient";

	   m_client = new MyClient;
	   ClientConnection *connection = (ClientConnection *)m_client->MakeConnection(hostName, server, "IPC TEST");

	   if (!connection)
	   {
		   delete m_client;
		   //wxMessageBox("Failed to make connection to server", "Client Demo");
		   m_server = new MyServer;
		   m_server->Create(server);
	   }
	   else
	   {
		   connection->StartAdvise(message);
		   connection->Disconnect();
		   wxExit();
		   return true;
	   }
   }
#endif

   //ofs.close();

   return true;
}
