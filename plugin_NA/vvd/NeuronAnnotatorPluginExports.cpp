#include "stdwx.h"
#include <wxGuiPluginBase.h>
#include "NeuronAnnotatorPlugin.h"

PLUGIN_EXPORTED_API wxGuiPluginBase * CreatePlugin()
{
	return new NAGuiPlugin;
}

PLUGIN_EXPORTED_API void DeletePlugin(wxGuiPluginBase * plugin)
{
	wxDELETE(plugin);
}
