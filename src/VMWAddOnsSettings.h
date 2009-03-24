/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/


#ifndef VMWSETTINGS_H
#define VMWSETTINGS_H

#include <File.h>
#include <Message.h>
#include <SupportDefs.h>

class VMWAddOnsSettings {
public:
				VMWAddOnsSettings();
				VMWAddOnsSettings(node_ref* nref);
	virtual		~VMWAddOnsSettings();
	
	void		Reload();
	bool		GetBool(char* name, bool default_value);
	void		SetBool(char* name, bool value);

private:
	BFile		settings_file;
	BMessage	settings_msg;

	status_t	OpenSettings();
};

#endif
