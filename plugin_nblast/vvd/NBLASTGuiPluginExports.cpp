#include "stdwx.h"
#include <wxGuiPluginBase.h>
#include "NBLASTGuiPlugin.h"

PLUGIN_EXPORTED_API wxGuiPluginBase * CreatePlugin()
{
	return new NBLASTGuiPlugin;
}

PLUGIN_EXPORTED_API void DeletePlugin(wxGuiPluginBase * plugin)
{
	wxDELETE(plugin);
}
