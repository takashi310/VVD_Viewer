#include "PluginControllerBaseSettings.h"

PluginControllerBaseSettings::PluginControllerBaseSettings()
	: m_bStoreInAppData(false)
{

}

PluginControllerBaseSettings::PluginControllerBaseSettings(const PluginControllerBaseSettings & settings)
{
	CopyFrom(settings);
}

PluginControllerBaseSettings & PluginControllerBaseSettings::operator = (const PluginControllerBaseSettings & settings)
{
	if (this != &settings)
	{
		CopyFrom(settings);
	}
	return *this;
}

PluginControllerBaseSettings::~PluginControllerBaseSettings()
{

}

void PluginControllerBaseSettings::CopyFrom(const PluginControllerBaseSettings & settings)
{
	m_bStoreInAppData = settings.m_bStoreInAppData;
}

void PluginControllerBaseSettings::SetStoreInAppData(const bool & value)
{
	m_bStoreInAppData = value;
}

bool PluginControllerBaseSettings::GetStoreInAppData() const
{
	return m_bStoreInAppData;
}