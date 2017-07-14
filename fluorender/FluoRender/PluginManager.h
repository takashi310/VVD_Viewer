
#include <PluginControllerBase.h>
#include <wxNonGuiPluginBase.h>
#include <wxGuiPluginBase.h>

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

class PluginManager : public PluginControllerBase
{
public:
	virtual ~PluginManager();
	virtual bool LoadAllPlugins(bool forceProgramPath);
	virtual bool UnloadAllPlugins();

	const wxNonGuiPluginBaseList& GetNonGuiPlugins() const;
	const wxGuiPluginBaseList& GetGuiPlugins() const;

	wxNonGuiPluginBase* GetNonGuiPlugin(wxString name) const;
	wxGuiPluginBase* GetGuiPlugin(wxString name) const;
private:
	wxNonGuiPluginToDllDictionary m_MapNonGuiPluginsDll;
	wxNonGuiPluginBaseList m_NonGuiPlugins;
	wxGuiPluginToDllDictionary m_MapGuiPluginsDll;
	wxGuiPluginBaseList m_GuiPlugins;
};