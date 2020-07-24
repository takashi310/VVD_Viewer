#include "VulkanWindow.h"
#include "VulkanException.h"

VulkanWindow::VulkanWindow(wxWindow* parent, wxWindowID id, const wxString &title)
    : wxFrame(parent, id, title), m_canvas(nullptr)
{
    m_canvas = new VulkanCanvas(this, wxID_ANY, wxDefaultPosition, { 1600, 1200 });
    Fit();

	Bind(wxEVT_SIZE, &VulkanWindow::OnResize, this);
	Bind(wxEVT_CLOSE_WINDOW, &VulkanWindow::OnClose, this);
}


VulkanWindow::~VulkanWindow()
{
}

void VulkanWindow::OnResize(wxSizeEvent& event)
{
    wxSize clientSize = GetClientSize();
    m_canvas->SetSize(clientSize);
}

void VulkanWindow::OnClose(wxCloseEvent& event)
{
	m_canvas->stop();
	event.Skip();
}

