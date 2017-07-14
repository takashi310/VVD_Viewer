#include "stdwx.h"
#include "wxGuiPluginBase.h"

DEFINE_EVENT_TYPE(wxEVT_GUI_PLUGIN_INTEROP)

IMPLEMENT_ABSTRACT_CLASS(wxGuiPluginBase, wxObject)

wxGuiPluginBase::wxGuiPluginBase(wxEvtHandler * handler, wxWindow * vvd)
: m_Handler(handler), m_vvd(vvd)
{
}

wxGuiPluginBase::~wxGuiPluginBase()
{
}

wxEvtHandler * wxGuiPluginBase::GetEventHandler()
{
	return m_Handler;
}

void wxGuiPluginBase::SetEventHandler(wxEvtHandler * handler)
{
	m_Handler = handler;
}

void wxGuiPluginBase::SetVVDMainFrame(wxWindow * vvd)
{
	m_vvd = vvd;
}

wxWindow * wxGuiPluginBase::GetVVDMainFrame()
{
	return m_vvd;
}