/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * Copyright 2010-2011 Joshua Stein <jcs@jcs.org>
 * Copyright 2018 Gerasim Troeglazov <3dEyes@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef VMW_BACK_H
#define VMW_BACK_H

#include "VMWCoreBackdoor.h"

class VMWBackdoor : public VMWCoreBackdoor {
public:
	// Only listing values still seen (and used) on VMware WS 17.x
	enum gui_setting {
		POINTER_GRAB_UNGRAB	= 0x0003,	// Grab (0x1)/Ungrab(0x2) are combined on newer versions.
										// VMware-wide preference.
		CLIP_BOARD_SHARING	= 0x0010,	// per-VM setting.
		TIME_SYNC			= 0x0400,	// per-VM setting. Only one still used on open_vmware_tools code.
	};

	status_t	EnableMouseSharing();
	status_t	DisableMouseSharing();

	status_t	GetCursorPosition(int32& x, int32& y);
	void		GetCursorStatus(uint16& status, uint16& toRead);

	status_t	GetHostClipboard(char** text, size_t* length);
	status_t	SetHostClipboard(char* text, size_t length);

	bool		GetGUISetting(gui_setting setting);
#ifdef SET_GUI_SETTINGS
	void		SetGUISetting(gui_setting setting, bool enabled);
#endif

	ulong		GetHostClock();
};

#endif
