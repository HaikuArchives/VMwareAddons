/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	Copyright 2010-2011 Joshua Stein <jcs@jcs.org>
	Copyright 2018 Gerasim Troeglazov <3dEyes@gmail.com>
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMW_BACK_H
#define VMW_BACK_H

#include "VMWCoreBackdoor.h"

class VMWBackdoor : public VMWCoreBackdoor {
public:
	// Only listing values still seen on VMware WS 17.x
	enum gui_setting {
		POINTER_GRAB_UNGRAB	= 0x0003,	// Grab (0x1)/Ungrab(0x2) are combined on newer versions.
										// VMware-wide preference.
		CLIP_BOARD_SHARING	= 0x0010,	// per-VM setting.
		UNKOWN_SETTING_1	= 0x0200,
		TIME_SYNC			= 0x0400,	// per-VM setting. Only one still used on open_vmware_tools code.
		UNKOWN_SETTING_2	= 0x0800,
	};

				VMWBackdoor();
	virtual		~VMWBackdoor();

	status_t	EnableMouseSharing();
	status_t	DisableMouseSharing();
	status_t	GetCursorPosition(int32& x, int32& y);
	void		GetCursorStatus(uint16& status, uint16& to_read);
	status_t	GetHostClipboard(char** text, size_t *text_length);
	status_t	SetHostClipboard(char* text, size_t length);
	bool		GetGUISetting(gui_setting setting);
	// void		SetGUISetting(gui_setting setting, bool enabled);
	ulong		GetHostClock();
};

#endif
