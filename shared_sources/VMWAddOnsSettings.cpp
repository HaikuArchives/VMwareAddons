/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "VMWAddOnsSettings.h"

#include <FindDirectory.h>
#include <Path.h>


VMWAddOnsSettings::VMWAddOnsSettings()
{
	Reload();
}


VMWAddOnsSettings::VMWAddOnsSettings(node_ref* nref)
{
	Reload();
	fSettingsFile.GetNodeRef(nref);
}


status_t
VMWAddOnsSettings::OpenSettings()
{
	BPath settingsPath;

	find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath);
	settingsPath.Append(SETTINGS_FILE_NAME);

	fSettingsFile.SetTo(settingsPath.Path(), B_READ_WRITE | B_CREATE_FILE);

	return fSettingsFile.InitCheck();
}


void
VMWAddOnsSettings::Reload()
{
	if (OpenSettings() == B_OK)
		fSettingsMsg.Unflatten(&fSettingsFile);
}


bool
VMWAddOnsSettings::GetBool(const char* name, bool defaultValue)
{
	bool value;
	if (fSettingsMsg.FindBool(name, &value) == B_OK)
		return value;

	return defaultValue;
}


void
VMWAddOnsSettings::SetBool(const char* name, bool value)
{
	if (fSettingsMsg.ReplaceBool(name, value) == B_NAME_NOT_FOUND)
		fSettingsMsg.AddBool(name, value);

	if (OpenSettings() == B_OK)
		fSettingsMsg.Flatten(&fSettingsFile);

	fSettingsFile.Unset();
}
