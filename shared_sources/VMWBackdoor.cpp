/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	Copyright 2010-2011 Joshua Stein <jcs@jcs.org>
	Copyright 2018 Gerasim Troeglazov <3dEyes@gmail.com>
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWBackdoor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>


VMWBackdoor::VMWBackdoor()
{
}

VMWBackdoor::~VMWBackdoor()
{
}

status_t
VMWBackdoor::EnableMouseSharing()
{
	if (!InVMware()) return B_NOT_ALLOWED;
	
	regs_t regs;
	BackdoorCall(&regs, VMW_BACK_MOUSE_COMMAND, VMW_BACK_MOUSE_READ);
	
	// Check the status
	BackdoorCall(&regs, VMW_BACK_MOUSE_STATUS, 0);
	int16 status = HIGH_BITS(regs.eax);

	if (status != 1)
		return B_ERROR;
	
	// Check VMware's mouse driver version
	BackdoorCall(&regs, VMW_BACK_MOUSE_DATA, 1);
	if (regs.eax != VMW_BACK_MOUSE_VERSION)
		return B_UNSUPPORTED;
	
	// We want absolute coordinates
	BackdoorCall(&regs, VMW_BACK_MOUSE_COMMAND, VMW_BACK_MOUSE_REQ_ABSOLUTE);

	return B_OK;
}

status_t
VMWBackdoor::DisableMouseSharing()
{
	if (!InVMware()) return B_NOT_ALLOWED;
	
	regs_t regs;
	BackdoorCall(&regs, VMW_BACK_MOUSE_COMMAND, VMW_BACK_MOUSE_DISABLE);
	
	// Check the status
	BackdoorCall(&regs, VMW_BACK_MOUSE_STATUS, 0);
	int16 status = HIGH_BITS(regs.eax);

	if (status != -1)
		return B_ERROR;
	
	return B_OK;
}

status_t
VMWBackdoor::GetCursorPosition(int32& x, int32& y)
{
	if (!InVMware())
		return B_NOT_ALLOWED;

	uint16 status;
	uint16 to_read;

	GetCursorStatus(status, to_read);

	if (status == VMWARE_ERROR)
		return B_ERROR;

	if (to_read == 0)
		return B_INTERRUPTED;

	regs_t regs;
	BackdoorCall(&regs, VMW_BACK_MOUSE_DATA, 4);
	x = regs.esi;
	y = regs.ecx;

	return B_OK;
}


void
VMWBackdoor::GetCursorStatus(uint16& status, uint16& to_read)
{
	regs_t regs;
	BackdoorCall(&regs, VMW_BACK_MOUSE_STATUS, 0);
	status = HIGH_BITS(regs.eax);
	to_read = LOW_BITS(regs.eax);
}


status_t
VMWBackdoor::GetHostClipboard(char** text, size_t* text_length)
{
	if (!InVMware())
		return B_NOT_ALLOWED;

	if (text == NULL)
		return B_ERROR;

	regs_t regs;
	BackdoorCall(&regs, VMW_BACK_GET_CLIP_LENGTH, 0);

	uint32_t total, left;
	total = left = regs.eax;

	// Max length is 65436, text will be truncated to that size.
	// If we get 0xFFFFFFFF it means that either:
	// the cliboard data is not text, or it was already transfered once.
	if (total == 0 || total > 65436) {
		// syslog(LOG_INFO, "VMWBackdoor: invalid GET_CLIP_LENGTH: %u\n", total);
		return B_ERROR;
	}

	char* buffer = NULL;
	if ((buffer = (char*)malloc((size_t)(total + 1))) == NULL)
		return B_NO_MEMORY;

	for (;;) {
		regs_t regs;
		BackdoorCall(&regs, VMW_BACK_GET_CLIP_DATA, 0);

		memcpy(buffer + (total - left), &regs.eax, left > 4 ? 4 : left);

		if (left <= 4) {
			buffer[total] = '\0';
			break;
		} else
			left -= 4;
	}

	*text = buffer;
	*text_length = strlen(buffer);

	return B_OK;
}

status_t
VMWBackdoor::SetHostClipboard(char* text, size_t text_length)
{
	if (!InVMware())
		return B_NOT_ALLOWED;

	if (text == NULL || text_length == 0)
		return B_ERROR;

	text[text_length] = '\0';

	uint32_t total, left;
	total = left = strlen(text);

	regs_t regs;
	BackdoorCall(&regs, VMW_BACK_SET_CLIP_LENGTH, total);

	for (;;) {
		ulong param;
		memcpy(&param, text + (total - left), left > 4 ? 4 : left);
		BackdoorCall(&regs, VMW_BACK_SET_CLIP_DATA, param);

		if (left <= 4)
			break;
		else
			left -= 4;
	}

	return B_OK;
}

int32
VMWBackdoor::GetHostClock()
{
	if (!InVMware()) return 0;
	
	regs_t regs;
	BackdoorCall(&regs, VMW_BACK_GET_HOST_TIME);
	
	if (regs.eax <= 0)
		return 0;
	
	// EAX contains the GMT (in seconds since 1/1/1970), EDX contains the
	// timezone offset (in minutes)
	
	return regs.eax - 60 * regs.edx;
}
