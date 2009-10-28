/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWBackdoor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

// http://chitchat.at.infoseek.co.jp/vmware/backdoor.html
#define VMW_BACK_GET_CURSOR 		0x04
#define VMW_BACK_GET_CLIP_LENGTH	0x06
#define VMW_BACK_GET_CLIP_DATA		0x07
#define VMW_BACK_SET_CLIP_LENGTH	0x08
#define VMW_BACK_SET_CLIP_DATA		0x09
#define VMW_BACK_GET_HOST_TIME		0x17
#define VMW_BACK_MOUSE_DATA			0x27
#define VMW_BACK_MOUSE_STATUS		0x28
#define VMW_BACK_MOUSE_COMMAND		0x29

// Mouse sharing commands
#define VMW_BACK_MOUSE_VERSION		0x3442554a
#define VMW_BACK_MOUSE_DISABLE		0x000000f5
#define VMW_BACK_MOUSE_READ			0x45414552
#define VMW_BACK_MOUSE_REQ_ABSOLUTE	0x53424152

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
	if (!InVMware()) return B_NOT_ALLOWED;
	
	regs_t regs;
	
	// Get status
	BackdoorCall(&regs, VMW_BACK_MOUSE_STATUS, 0);
	int16 status = HIGH_BITS(regs.eax);
	int16 to_read = LOW_BITS(regs.eax);
	if (status == -1)
		return B_ERROR;
	
	if (to_read == 0)
		return B_INTERRUPTED;
	
	
	BackdoorCall(&regs, VMW_BACK_MOUSE_DATA, 4);
	x = regs.esi;
	y = regs.ecx;
	
	return B_OK;
	
}

status_t
VMWBackdoor::GetHostClipboard(char** text, size_t *text_length)
{
	if (!InVMware()) return B_NOT_ALLOWED;

	regs_t regs;
	BackdoorCall(&regs, VMW_BACK_GET_CLIP_LENGTH, 0);

	if (text == NULL) {
		// We just want to clear the clipboard if it contains data ;
		// This is needed to set data into it.
		return B_OK;
	}

	ulong length = regs.eax;

	*text_length = length;

	if (length == 0)
		return B_OK;

	if (length > 0xFFFF) // No data (or an error occurred)
		return B_ERROR;

	char* buffer = NULL;
	buffer = (char*)malloc(length + 1);

	if (buffer == NULL)
		return B_NO_MEMORY;

	buffer[length] = '\0';

	*text = buffer;

	long i = length;

	while (i > 0) {
		regs.eax = 0;

		BackdoorCall(&regs, VMW_BACK_GET_CLIP_DATA, 0);

		memcpy(buffer, &regs.eax, (i < (signed)sizeof(regs.eax) ? i : sizeof(regs.eax)));

		buffer += sizeof(regs.eax);
		i -= sizeof(regs.eax);
	}

	return B_OK;
}

status_t
VMWBackdoor::SetHostClipboard(char* text, size_t text_length)
{
	if (!InVMware()) return B_NOT_ALLOWED;

	GetHostClipboard(NULL, NULL);

	regs_t regs;
	BackdoorCall(&regs, VMW_BACK_SET_CLIP_LENGTH, text_length);

	if (regs.eax != 0) return B_ERROR;

	long i = text_length;
	char* buffer = text;

	while (i > 0) {
		ulong data = 0;
		memcpy(&data, buffer, (i < (signed)sizeof(data) ? i : sizeof(data)));

		BackdoorCall(&regs, VMW_BACK_SET_CLIP_DATA, data);

		buffer += sizeof(data);
		i -= sizeof(data);
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
