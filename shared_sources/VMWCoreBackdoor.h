/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMW_BACK_CORE_H
#define VMW_BACK_CORE_H

#include <Message.h>


#define VMW_BACK_MAGIC				0x564D5868UL

#define HIGH_BITS(x) ((x) >> 16)
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

	bool		in_vmware;

	bool		rpc_opened;
	ulong		rpc_channel;
	ulong		rpc_cookie1;
	ulong		rpc_cookie2;

	sem_id		backdoor_access;
};

#endif
