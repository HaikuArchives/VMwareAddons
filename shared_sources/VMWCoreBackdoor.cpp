/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWCoreBackdoor.h"

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SupportDefs.h>

// http://chitchat.at.infoseek.co.jp/vmware/backdoor.html
#define VMW_BACK_PORT				0x00005658UL

#define VMW_BACK_GET_VERSION		0x0A

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

void
VMWCoreBackdoor::BackdoorCall(regs_t* regs, ulong command, ulong param)
{
	regs->eax = VMW_BACK_MAGIC;
	regs->esi = param;
	regs->ecx = command;
	regs->edx = VMW_BACK_PORT;

	// The VMWare backdoor get/sets EBX, but this register
	// is used by PIC. So we use ESI instead.

	asm volatile(
		"pushl	%%ebx			\n\t" // Save ebx
		"movl	%%esi,	%%ebx	\n\t" // esi => ebx

		"inl	(%%dx)			\n\t" // Backdoor call

		"movl	%%ebx,	%%esi	\n\t" // ebx => esi
		"popl	%%ebx			\n\t" // Restore ebx
		:"=a"(regs->eax), "=S"(regs->esi), "=c"(regs->ecx), "=d"(regs->edx)
		:"a"(regs->eax), "S"(regs->esi), "c"(regs->ecx), "d"(regs->edx)
	);
}

void
VMWCoreBackdoor::BackdoorRPCCall(regs_t* regs, ulong command, ulong param)
{
	// Same problem here, but worse because every register is used as input :(
	// Fortunately, the value returned in EAX is not useful.

	regs->eax = param;
	regs->ecx = command;
	regs->edx = rpc_channel | VMW_BACK_RPC_PORT;
	regs->esi = rpc_cookie1;
	regs->edi = rpc_cookie2;

	asm volatile(
		"pushl	%%ebx				\n\t" // Save ebx
		"movl	%%eax,	%%ebx		\n\t" // eax => ebx
		"movl	$0x564D5868, %%eax	\n\t" // Init eax

		"inl	(%%dx)				\n\t" // Backdoor call

		"movl	%%ebx,	%%eax		\n\t" // ebx => eax
		"popl	%%ebx				\n\t" // Restore ebx
		:"=a"(regs->eax), "=c"(regs->ecx), "=d"(regs->edx), "=S"(regs->esi), "=D"(regs->edi)
		:"a"(regs->eax), "c"(regs->ecx), "d"(regs->edx), "S"(regs->esi), "D"(regs->edi)
	);
}

void
VMWCoreBackdoor::BackdoorRPCSend(regs_t* regs, char* data, size_t length)
{
	regs->eax = rpc_cookie1;

	regs->ecx = length;
	regs->edx = rpc_channel | VMW_BACK_RPC_PORT2;
	regs->esi = (ulong)data;
	regs->edi = rpc_cookie2;

	asm volatile(
		"pushl	%%ebx				\n\t" // Save ebx
		"pushl	%%ebp				\n\t" // and ebp
		"movl	%%eax,	%%ebp		\n\t" // eax => ebp
		"movl	$0x564D5868, %%eax	\n\t" // Init eax
		"movl	$0x00010000, %%ebx	\n\t" // Init ebx

		"cld						\n\t" // Backdoor call
		"rep outsb (%%esi), (%%dx)	\n\t"

		"movl	%%ebx,	%%eax		\n\t" // ebx => eax
		"popl	%%ebp				\n\t" // Restore ebp
		"popl	%%ebx				\n\t" // and ebx
		:"=a"(regs->eax), "=c"(regs->ecx), "=d"(regs->edx), "=S"(regs->esi), "=D"(regs->edi)
		:"a"(regs->eax), "c"(regs->ecx), "d"(regs->edx), "S"(regs->esi), "D"(regs->edi)
	);
}

void
VMWCoreBackdoor::BackdoorRPCGet(regs_t* regs, char* data, size_t length)
{
	// TODO : if possible, combine this function with the previous one.

	regs->eax = rpc_cookie2;

	regs->ecx = length;
	regs->edx = rpc_channel | VMW_BACK_RPC_PORT2;
	regs->esi = rpc_cookie1;
	regs->edi = (ulong)data;

	asm volatile(
		"pushl	%%ebx				\n\t" // Save ebx
		"pushl	%%ebp				\n\t" // and ebp
		"movl	%%eax,	%%ebp		\n\t" // eax => ebp
		"movl	$0x564D5868, %%eax	\n\t" // Init eax
		"movl	$0x00010000, %%ebx	\n\t" // Init ebx

		"cld						\n\t" // Backdoor call
		"rep insb (%%dx), (%%edi)	\n\t"

		"movl	%%ebx,	%%eax		\n\t" // ebx => eax
		"popl	%%ebp				\n\t" // Restore ebp
		"popl	%%ebx				\n\t" // and ebx
		:"=a"(regs->eax), "=c"(regs->ecx), "=d"(regs->edx), "=S"(regs->esi), "=D"(regs->edi)
		:"a"(regs->eax), "c"(regs->ecx), "d"(regs->edx), "S"(regs->esi), "D"(regs->edi)
	);
}

#ifndef _KERNEL_MODE
// Avoid segfault when trying to access the backdoor on real hardware...
static sigjmp_buf sigbuffer;

static void sighandler(int sig)
{
	siglongjmp(sigbuffer, sig);
}
#endif

VMWCoreBackdoor::VMWCoreBackdoor()
{
	rpc_opened = false;

	// Are we running in a VMware virtual machine ?

#ifndef _KERNEL_MODE
	if (sigsetjmp(sigbuffer, true) != 0) {
		// We crashed during the backdoor access
		in_vmware = false;
		signal(SIGILL, SIG_DFL);

		return;
	}

	signal(SIGILL, sighandler);
#endif

	regs_t regs;
	BackdoorCall(&regs, VMW_BACK_GET_VERSION, 0);
	in_vmware = (regs.esi == VMW_BACK_MAGIC);

#ifndef _KERNEL_MODE
	signal(SIGILL, SIG_DFL);
#endif

	if (!in_vmware)
			return;

	backdoor_access = create_sem(1, "vmware backdoor lock");
}

VMWCoreBackdoor::~VMWCoreBackdoor()
{
	delete_sem(backdoor_access);
}

bool
VMWCoreBackdoor::InVMware() const
{
	return in_vmware;
}

status_t
VMWCoreBackdoor::OpenRPCChannel()
{
	if (!in_vmware) return B_NOT_ALLOWED;

	if (rpc_opened)
		return B_ERROR;

	regs_t regs;
	rpc_channel = rpc_cookie1 = rpc_cookie2 = 0;
	BackdoorRPCCall(&regs, VMW_BACK_RPC_OPEN, VMW_BACK_RPC_MAGIC);

	if (regs.ecx != VMW_BACK_RPC_OK)
		return B_ERROR;

	rpc_opened = true;
	rpc_channel = regs.edx;
	rpc_cookie1 = regs.esi;
	rpc_cookie2 = regs.edi;

	return B_OK;
}

status_t
VMWCoreBackdoor::SendMessage(const char* message, bool check_status, size_t length)
{
	if (!in_vmware) return B_NOT_ALLOWED;

	if (message == NULL)
		length = 0;
	else if (length <= 0)
		length = strlen(message);

	if (acquire_sem(backdoor_access) != B_OK)
		return B_ERROR;

	regs_t regs;
	// Send command length
	BackdoorRPCCall(&regs, VMW_BACK_RPC_SEND_LENGTH, length);

	if (regs.ecx != VMW_BACK_RPC_SEND_L_OK) {
		release_sem(backdoor_access);
		return B_ERROR;
	}

	if (length > 0) {
		// Send command data
		BackdoorRPCSend(&regs, (char*)message, length);

		if (regs.eax != VMW_BACK_RPC_OK) {
			release_sem(backdoor_access);
			return B_ERROR;
		}
	}

	release_sem(backdoor_access);

	if (check_status) {
		char* response = GetMessage();
		if (response == NULL)
			return B_ERROR;

		char status = response[0];

		free(response);

		return (status = '1' ? B_OK : B_ERROR);
	}

	return B_OK;
}

char* VMWCoreBackdoor::GetMessage(size_t* _length)
{
	if (!in_vmware) return NULL;

	if (acquire_sem(backdoor_access) != B_OK)
		return NULL;

	regs_t regs;

	// Get data length
	BackdoorRPCCall(&regs, VMW_BACK_RPC_GET_LENGTH, 0);

	if (regs.ecx != VMW_BACK_RPC_GET_L_OK) {
		release_sem(backdoor_access);
		return NULL;
	}

	size_t length = regs.eax;
	if (_length != NULL)
		*_length = length;

	char* data = (char*)malloc(length + 1);
	uint reply_id = HIGH_BITS(regs.edx);

	// Get data
	BackdoorRPCGet(&regs, data, length);

	if (regs.eax != VMW_BACK_RPC_OK) {
		release_sem(backdoor_access);
		free(data);
		return NULL;
	}

	// Confirm...
	BackdoorRPCCall(&regs, VMW_BACK_RPC_ACK, reply_id);

	release_sem(backdoor_access);

	if (regs.ecx != VMW_BACK_RPC_OK) {
		free(data);
		return NULL;
	}

	data[length] = '\0';

	return data;
}

status_t
VMWCoreBackdoor::CloseRPCChannel()
{
	if (!in_vmware) return B_NOT_ALLOWED;

	if (!rpc_opened)
		return B_ERROR;

	regs_t regs;
	BackdoorRPCCall(&regs, VMW_BACK_RPC_CLOSE, 0);

	if (regs.ecx != VMW_BACK_RPC_OK)
		return B_ERROR;

	rpc_opened = false;

	return B_OK;
}
