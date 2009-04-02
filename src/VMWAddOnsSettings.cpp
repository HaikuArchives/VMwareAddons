/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWAddOnsSettings.h"
#include "VMWAddOns.h"

#include <FindDirectory.h>
#include <Path.h>

#include <stdio.h>

VMWAddOnsSettings::VMWAddOnsSettings()
{
	Reload();
}

VMWAddOnsSettings::VMWAddOnsSettings(node_ref* nref)
{
	Reload();
	printf("GetNodeRef: %ld\n", settings_file.GetNodeRef(nref));
}

VMWAddOnsSettings::~VMWAddOnsSettings()
{
}

status_t
VMWAddOnsSettings::OpenSettings()
{
	BPath settings_path;
	
	find_directory(B_USER_SETTINGS_DIRECTORY, &settings_path);
	settings_path.Append(SETTINGS_FILE_NAME);
	
	settings_file.SetTo(settings_path.Path(), B_READ_WRITE | B_CREATE_FILE);

	return settings_file.InitCheck();
}
	
void
VMWAddOnsSettings::Reload()
{
	if (OpenSettings() == B_OK) {
		settings_msg.Unflatten(&settings_file);
	}
}

bool
VMWAddOnsSettings::GetBool(const char* name, bool default_value)
{
	bool value;
	if (settings_msg.FindBool(name, &value) == B_OK) {
		return value;
	}
	
	return default_value;
}

void
VMWAddOnsSettings::SetBool(const char* name, bool value)
{
	if (settings_msg.ReplaceBool(name, value) == B_NAME_NOT_FOUND) {
		settings_msg.AddBool(name, value);
	}
	
	if (OpenSettings() == B_OK) {
		settings_msg.Flatten(&settings_file);
	}
	
	settings_file.Unset();
}
	
	
