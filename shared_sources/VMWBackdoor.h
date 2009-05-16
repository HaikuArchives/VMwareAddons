/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMW_BACK_H
#define VMW_BACK_H

#include "VMWCoreBackdoor.h"

class VMWBackdoor : public VMWCoreBackdoor {
public:
				VMWBackdoor();
	virtual		~VMWBackdoor();

	status_t	SyncCursor(BMessage* cursor_message);
	status_t	GetHostClipboard(char** text, size_t *text_length);
	status_t	SetHostClipboard(char* text, size_t length);
};

#endif
