#pragma once

#include "Declarations.h"

class DEMO_API wxGuiPluginBase : public wxObject
{
	DECLARE_ABSTRACT_CLASS(wxGuiPluginBase)
public:
	wxGuiPluginBase(wxEvtHandler * handler, wxWindow * vvd);
	virtual ~wxGuiPluginBase();
	
	virtual wxString GetName() const = 0;
	virtual wxString GetId() const = 0;

	//This method is called when the plugin is starting.
	virtual wxWindow * CreatePanel(wxWindow * parent) = 0;

	//This method is called when the application is starting.
	virtual void OnInit() = 0;
	//This method is called when the application is closing.
	virtual void OnDestroy() = 0;
	
	//For command line.
	virtual bool OnRun(wxString options);
	
	wxEvtHandler * GetEventHandler();
	virtual void SetEventHandler(wxEvtHandler * handler);
	virtual void SetVVDMainFrame(wxWindow * vvd);
	wxWindow * GetVVDMainFrame();

protected:
	wxEvtHandler * m_Handler;
	wxWindow *m_vvd;
};

DECLARE_EXPORTED_EVENT_TYPE(DEMO_API, wxEVT_GUI_PLUGIN_INTEROP, wxEVT_USER_FIRST + 100)

typedef wxGuiPluginBase * (*CreateGuiPlugin_function)();
typedef void (*DeleteGuiPlugin_function)(wxGuiPluginBase * plugin);