/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWBackdoor.h"

#include <stdlib.h>

// http://chitchat.at.infoseek.co.jp/vmware/backdoor.html
#define VMW_BACK_GET_CURSOR 		0x04
#define VMW_BACK_GET_CLIP_LENGTH	0x06
#define VMW_BACK_GET_CLIP_DATA		0x07
#define VMW_BACK_SET_CLIP_LENGTH	0x08
#define VMW_BACK_SET_CLIP_DATA		0x09

VMWBackdoor::VMWBackdoor()
{
}

VMWBackdoor::~VMWBackdoor()
{
}

status_t
VMWBackdoor::SyncCursor(BMessage* cursor_message)
{
	if (!InVMware()) return B_NOT_ALLOWED;
	
	regs_t regs;
	BackdoorCall(&regs, VMW_BACK_GET_CURSOR, 0);
	
	if (regs.eax == VMW_BACK_MAGIC)
		return B_ERROR;
	
	int16 x = HIGH_BITS(regs.eax);
	int16 y = LOW_BITS(regs.eax);
	
	if (x < 0) // The VM is not focused
		return B_OK;
	
	cursor_message->ReplacePoint("where", BPoint(x, y));
	
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
	
	if (regs.eax != 0) {
		return B_ERROR;
	}
	
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
