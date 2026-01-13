/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * Copyright 2010-2011 Joshua Stein <jcs@jcs.org>
 * Copyright 2018 Gerasim Troeglazov <3dEyes@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef VMW_BACK_CORE_H
#define VMW_BACK_CORE_H

#include <OS.h>
#include <SupportDefs.h>

// https://web.archive.org/web/20100610223425/http://chitchat.at.infoseek.co.jp/vmware/backdoor.html
#define VMW_BACK_MAGIC				0x564D5868UL
#define VMW_BACK_PORT				0x00005658UL

#define VMWARE_ERROR				0xFFFF

#define VMW_BACK_RPC_MAGIC			0x49435052UL
#define VMW_BACK_RPC_PORT			0x5658
#define VMW_BACK_RPC_PORT2			0x5659
#define VMW_BACK_RPC_OPEN			0x0000001EUL
#define VMW_BACK_RPC_SEND_LENGTH	0x0001001EUL
#define VMW_BACK_RPC_GET_LENGTH		0x0003001EUL
#define VMW_BACK_RPC_ACK			0x0005001EUL
#define VMW_BACK_RPC_CLOSE			0x0006001EUL
#define VMW_BACK_RPC_OK				0x00010000UL
#define VMW_BACK_RPC_SEND_L_OK		0x00810000UL
#define VMW_BACK_RPC_GET_L_OK		0x00830000UL

#define VMW_BACK_GET_CURSOR 		0x04
#define VMW_BACK_GET_CLIP_LENGTH	0x06
#define VMW_BACK_GET_CLIP_DATA		0x07
#define VMW_BACK_SET_CLIP_LENGTH	0x08
#define VMW_BACK_SET_CLIP_DATA		0x09
#define VMW_BACK_GET_VERSION		0x0A
#define VMW_BACK_GET_GUI_SETTING	0x0D
#define VMW_BACK_SET_GUI_SETTING	0x0E
#define VMW_BACK_GET_HOST_TIME		0x17
// Absolute pointer commands
#define VMW_BACK_MOUSE_DATA			0x27
#define VMW_BACK_MOUSE_STATUS		0x28
#define VMW_BACK_MOUSE_COMMAND		0x29

#define VMW_BACK_MOUSE_VERSION		0x3442554a
// Sub-commmands to VMW_BACK_MOUSE_COMMAND
#define VMW_BACK_MOUSE_DISABLE		0x000000f5
#define VMW_BACK_MOUSE_READ			0x45414552
#define VMW_BACK_MOUSE_REQ_ABSOLUTE	0x53424152


#define HIGH_BITS(x) (((x) >> 16) & 0xFFFF)
#define LOW_BITS(x) ((x) & 0xFFFF)
#define TO_HIGH(x) ((x) << 16)


typedef struct regs_t {
	ulong eax;
	ulong ebx;
	ulong ecx;
	ulong edx;
	ulong esi;
	ulong edi;
} regs_t;


class VMWCoreBackdoor {
public:
				VMWCoreBackdoor();
	virtual		~VMWCoreBackdoor();
	bool		InVMware() const;

	status_t	OpenRPCChannel();
	status_t	SendMessage(const char* message, bool check_status, size_t length = 0);
	status_t	SendAndGet(char* buffer, size_t* length, size_t buffer_length);
	char*		GetMessage(size_t* _length = NULL);
	status_t	CloseRPCChannel();

protected:
	void		BackdoorCall(regs_t* regs, ulong command, ulong param = 0);

private:
	void		BackdoorRPCCall(regs_t* regs, ulong command, ulong param);
	void		BackdoorRPCSend(regs_t* regs, char* data, size_t length);
	void		BackdoorRPCGet(regs_t* regs, char* data, size_t length);

	bool		fInVMWare;

	bool		fRPCOpened;
	ulong		fRPCChannel;
	ulong		fRPCCookie1;
	ulong		fRPCCookie2;

	sem_id		fBackdoorAccess;
};

#endif
