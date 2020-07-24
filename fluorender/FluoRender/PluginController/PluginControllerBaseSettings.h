#pragma once

#include "stdwx.h"
#ifndef _MODULAR_CORESET_H_
#define _MODULAR_CORESET_H_

class EXPORT_API PluginControllerBaseSettings
{
public:
	PluginControllerBaseSettings();
	PluginControllerBaseSettings(const PluginControllerBaseSettings & settings);
	PluginControllerBaseSettings & operator = (const PluginControllerBaseSettings & settings);
	virtual ~PluginControllerBaseSettings();

	void SetStoreInAppData(const bool & val);
	bool GetStoreInAppData() const;
protected:
	virtual void CopyFrom(const PluginControllerBaseSettings & settings);
private:
	bool m_bStoreInAppData; // Should we store data in Application Data folder or in .exe folder
};

#endif