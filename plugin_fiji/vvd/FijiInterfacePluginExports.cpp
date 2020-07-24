#include "stdwx.h"
#include <wxGuiPluginBase.h>
#include "FijiInterfacePlugin.h"

PLUGIN_EXPORTED_API wxGuiPluginBase * CreatePlugin()
{
	return new SampleGuiPlugin1;
}

PLUGIN_EXPORTED_API void DeletePlugin(wxGuiPluginBase * plugin)
{
	wxDELETE(plugin);
}
