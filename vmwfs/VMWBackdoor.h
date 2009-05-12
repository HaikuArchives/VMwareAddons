/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMW_BACK_H
#define VMW_BACK_H

#include <Message.h>

class VMWBackdoor {
public:
				VMWBackdoor();
	virtual		~VMWBackdoor();
	bool		InVMware() const;
	status_t	SyncCursor(BMessage* cursor_message);
	status_t	GetHostClipboard(char** text, size_t *text_length);
	status_t	SetHostClipboard(char* text, size_t length);
	
	status_t	OpenRPCChannel();
	status_t	SendMessage(const char* message, bool check_status, size_t length = 0);
	char*		GetMessage(size_t* _length = NULL);
	status_t	CloseRPCChannel();
	
private:	
	bool			in_vmware;
	
	bool			rpc_opened;
	ulong			rpc_channel;
	ulong			rpc_cookie1;
	ulong			rpc_cookie2;
	
	sem_id			backdoor_access;
};

#endif
