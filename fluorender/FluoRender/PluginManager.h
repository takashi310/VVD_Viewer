
#include <PluginControllerBase.h>
#include <wxNonGuiPluginBase.h>
#include <wxGuiPluginBase.h>
#include "DLLExport.h"

// We need to know which DLL produced the specific plugin object.
WX_DECLARE_HASH_MAP(wxNonGuiPluginBase*, wxDynamicLibrary*, \
					wxPointerHash, wxPointerEqual, \
					wxNonGuiPluginToDllDictionary);
WX_DECLARE_HASH_MAP(wxGuiPluginBase*, wxDynamicLibrary*, \
					wxPointerHash, wxPointerEqual, \
					wxGuiPluginToDllDictionary);
// And separate list of loaded plugins for faster access.
WX_DECLARE_LIST(wxNonGuiPluginBase, wxNonGuiPluginBaseList);
WX_DECLARE_LIST(wxGuiPluginBase, wxGuiPluginBaseList);

class EXPORT_API PluginManager : public PluginControllerBase
{
public:
	PluginManager() : PluginControllerBase() {m_MainFrame = NULL;}
	PluginManager(wxWindow* mainFrame) : PluginControllerBase() {m_MainFrame = mainFrame;}
	virtual ~PluginManager();
	virtual bool LoadAllPlugins(bool forceProgramPath);
	virtual bool UnloadAllPlugins();
	void InitPlugins();
	void OnTreeUpdate();
	void FinalizePligins();

	const wxNonGuiPluginBaseList& GetNonGuiPlugins() const;
	const wxGuiPluginBaseList& GetGuiPlugins() const;

	wxNonGuiPluginBase* GetNonGuiPlugin(wxString name) const;
	wxGuiPluginBase* GetGuiPlugin(wxString name) const;
private:
	wxNonGuiPluginToDllDictionary m_MapNonGuiPluginsDll;
	wxNonGuiPluginBaseList m_NonGuiPlugins;
	wxGuiPluginToDllDictionary m_MapGuiPluginsDll;
	wxGuiPluginBaseList m_GuiPlugins;
	wxWindow* m_MainFrame;
};