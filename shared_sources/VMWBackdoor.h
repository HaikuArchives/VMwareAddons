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
				VMWBackdoor();
	virtual		~VMWBackdoor();

	status_t	EnableMouseSharing();
	status_t	DisableMouseSharing();
	status_t	GetCursorPosition(int32& x, int32& y);
	status_t	GetHostClipboard(char** text, size_t *text_length);
	status_t	SetHostClipboard(char* text, size_t length);
	int32		GetHostClock();
};

#endif
