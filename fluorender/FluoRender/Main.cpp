/*
For more information, please see: http://software.sci.utah.edu

The MIT License

Copyright (c) 2014 Scientific Computing and Imaging Institute,
University of Utah.


Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "Main.h"
#include <cstdio>
#include <iostream>
#include <wx/cmdline.h>
#include <wx/stdpaths.h>
#include <wx/filefn.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include "VRenderFrame.h"
#include "compatibility.h"
// -- application --

bool m_open_by_web_browser = false;

IMPLEMENT_APP(VRenderApp)

static const wxCmdLineEntryDesc g_cmdLineDesc [] =
{
   { wxCMD_LINE_PARAM, NULL, NULL, NULL,
      wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL|wxCMD_LINE_PARAM_MULTIPLE },
   { wxCMD_LINE_NONE }
};

bool VRenderApp::OnInit()
{
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);

   char cpath[FILENAME_MAX];
   GETCURRENTDIR(cpath, sizeof(cpath));
   ::wxSetWorkingDirectory(wxString(s2ws(std::string(cpath))));
   // call default behaviour (mandatory)
   if (!wxApp::OnInit())
      return false;
   //add png handler
   wxImage::AddHandler(new wxPNGHandler);
   //the frame
/*   std::string title =  std::string(FLUORENDER_TITLE) + std::string(" ") +
      std::string(VERSION_MAJOR_TAG) +  std::string(".") +
      std::string(VERSION_MINOR_TAG);
*/   
   std::string title =  std::string(FLUORENDER_TITLE) + "1.00";

   m_frame = new VRenderFrame(
         (wxFrame*) NULL,
         wxString(title),
         50,50,1024,768);
   
   if(m_server)
	   m_server->SetFrame((VRenderFrame *)m_frame);

#ifdef WITH_DATABASE
#ifdef _WIN32
/*
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
*/
#endif
#endif
   
   SetTopWindow(m_frame);
   m_frame->Show();

   SettingDlg *setting_dlg = ((VRenderFrame *)m_frame)->GetSettingDlg();
   if (setting_dlg)
	   ((VRenderFrame *)m_frame)->SetRealtimeCompression(setting_dlg->GetRealtimeCompress());

   if (m_files.Count()>0)
      ((VRenderFrame*)m_frame)->StartupLoad(m_files);

   if (setting_dlg)
   {
	   setting_dlg->SetRealtimeCompress(((VRenderFrame *)m_frame)->GetRealtimeCompression());
	   setting_dlg->UpdateUI();
   }
   return true;
}

int VRenderApp::OnExit()
{
	if (m_server) delete m_server;

	return 0;
}

void VRenderApp::OnInitCmdLine(wxCmdLineParser& parser)
{
   parser.SetDesc (g_cmdLineDesc);
}

bool VRenderApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
   int i=0;
   wxString params;
   for (i = 0; i < (int)parser.GetParamCount(); i++)
   {
      wxString file = parser.GetParam(i);

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
			  }
		  }
		  break;
	  }
	  
	  m_files.Add(file);

   }
    
#ifdef _WIN32
   {
	   wxString message;

	   int fnum = m_files.GetCount();
	   for (i = 0; i < fnum; i++) message += m_files[i] + (i == fnum - 1 ? wxT("") : wxT(","));

	   wxLogNull lognull;

	   wxString server = "8001";
	   wxString hostName = "localhost";

	   MyClient *client = new MyClient;
	   ClientConnection *connection = (ClientConnection *)client->MakeConnection(hostName, server, "IPC TEST");
//	   ClientConnection *connection = (ClientConnection *)client->MakeConnection(hostName, "50001", "IPC TEST");
//	   connection->StartAdvise(wxString("test"));

	   if (!connection)
	   {
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
	   delete client;
   }
#endif
    
   return true;
}


#ifdef _DARWIN
void VRenderApp::MacOpenFiles(const wxArrayString& fileNames)
{
    if (m_frame)
        ((VRenderFrame*)m_frame)->StartupLoad(fileNames);
}
#endif
