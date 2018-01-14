/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWBackdoor.h"

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SupportDefs.h>

// http://chitchat.at.infoseek.co.jp/vmware/backdoor.html
#define VMW_BACK_MAGIC				0x564D5868UL
#define VMW_BACK_PORT				0x00005658UL

#define VMW_BACK_GET_CURSOR 		0x04
#define VMW_BACK_GET_CLIP_LENGTH	0x06
#define VMW_BACK_GET_CLIP_DATA		0x07
#define VMW_BACK_SET_CLIP_LENGTH	0x08
#define VMW_BACK_SET_CLIP_DATA		0x09
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

extern "C" void
backdoor_call(regs_t* regs, ulong command, ulong param)
{
	regs->eax = VMW_BACK_MAGIC;
	regs->esi = param;
	regs->ecx = command;
	regs->edx = VMW_BACK_PORT;
	
	// The VMWare backdoor get/sets EBX, but this register
	// is used by PIC. So we use ESI instead.
#ifdef __i386__
	asm volatile(
		"pushl	%%ebx			\n\t" // Save ebx
		"movl	%%esi,	%%ebx	\n\t" // esi => ebx

		"inl	(%%dx)			\n\t" // Backdoor call
		
		"movl	%%ebx,	%%esi	\n\t" // ebx => esi
		"popl	%%ebx			\n\t" // Restore ebx
		:"=a"(regs->eax), "=S"(regs->esi), "=c"(regs->ecx), "=d"(regs->edx)
		:"a"(regs->eax), "S"(regs->esi), "c"(regs->ecx), "d"(regs->edx)
	);
#else
	asm volatile(
		"pushq	%%rbx			\n\t" // Save rbx
		"movl	%%esi,	%%ebx	\n\t" // esi => ebx

		"inl	(%%dx)			\n\t" // Backdoor call
		
		"movl	%%ebx,	%%esi	\n\t" // ebx => esi
		"popq	%%rbx			\n\t" // Restore rbx
		:"=a"(regs->eax), "=S"(regs->esi), "=c"(regs->ecx), "=d"(regs->edx)
		:"a"(regs->eax), "S"(regs->esi), "c"(regs->ecx), "d"(regs->edx)
	);
#endif
}

extern "C" void
backdoor_rpc_call(regs_t* regs, ulong command, ulong param, ulong port, ulong cookie1, ulong cookie2)
{
	// Same problem here, but worse because every register is used as input :(
	// Fortunately, the value returned in EAX is not useful.
	
	regs->eax = param;
	regs->ecx = command;
	regs->edx = port;
	regs->esi = cookie1;
	regs->edi = cookie2;
#ifdef __i386__	
	asm volatile(
		"pushl	%%ebx				\n\t" // Save rbx
		"movl	%%eax,	%%ebx		\n\t" // eax => ebx
		"movl	$0x564D5868, %%eax	\n\t" // Init eax
		
		"inl	(%%dx)				\n\t" // Backdoor call
		
		"movl	%%ebx,	%%eax		\n\t" // ebx => eax
		"popl	%%ebx				\n\t" // Restore rbx
		:"=a"(regs->eax), "=c"(regs->ecx), "=d"(regs->edx), "=S"(regs->esi), "=D"(regs->edi)
		:"a"(regs->eax), "c"(regs->ecx), "d"(regs->edx), "S"(regs->esi), "D"(regs->edi)
	);
#else
	asm volatile(
		"pushq	%%rbx				\n\t" // Save rbx
		"movl	%%eax,	%%ebx		\n\t" // eax => ebx
		"movl	$0x564D5868, %%eax	\n\t" // Init eax
		
		"inl	(%%dx)				\n\t" // Backdoor call
		
		"movl	%%ebx,	%%eax		\n\t" // ebx => eax
		"popq	%%rbx				\n\t" // Restore rbx
		:"=a"(regs->eax), "=c"(regs->ecx), "=d"(regs->edx), "=S"(regs->esi), "=D"(regs->edi)
		:"a"(regs->eax), "c"(regs->ecx), "d"(regs->edx), "S"(regs->esi), "D"(regs->edi)
	);
#endif
}

extern "C" void
backdoor_rpc_send(regs_t* regs, char* data, size_t length, ulong port, ulong cookie1, ulong cookie2)
{	
	regs->eax = cookie1;
	
	regs->ecx = length;
	regs->edx = port;
	regs->esi = (ulong)data;
	regs->edi = cookie2;
#ifdef __i386__	
	asm volatile(
		"pushl	%%ebx				\n\t" // Save rbx
		"pushl	%%ebp				\n\t" // and rbp
		"movl	%%eax,	%%ebp		\n\t" // eax => ebp
		"movl	$0x564D5868, %%eax	\n\t" // Init eax
		"movl	$0x00010000, %%ebx	\n\t" // Init ebx
		
		"cld						\n\t" // Backdoor call
		"rep outsb (%%esi), (%%dx)	\n\t"
		
		"movl	%%ebx,	%%eax		\n\t" // ebx => eax
		"popl	%%ebp				\n\t" // Restore rbp
		"popl	%%ebx				\n\t" // and rbx
		:"=a"(regs->eax), "=c"(regs->ecx), "=d"(regs->edx), "=S"(regs->esi), "=D"(regs->edi)
		:"a"(regs->eax), "c"(regs->ecx), "d"(regs->edx), "S"(regs->esi), "D"(regs->edi)
	);
#else
	asm volatile(
		"pushq	%%rbx				\n\t" // Save rbx
		"pushq	%%rbp				\n\t" // and rbp
		"movl	%%eax,	%%ebp		\n\t" // eax => ebp
		"movl	$0x564D5868, %%eax	\n\t" // Init eax
		"movl	$0x00010000, %%ebx	\n\t" // Init ebx
		
		"cld						\n\t" // Backdoor call
		"rep outsb (%%esi), (%%dx)	\n\t"
		
		"movl	%%ebx,	%%eax		\n\t" // ebx => eax
		"popq	%%rbp				\n\t" // Restore rbp
		"popq	%%rbx				\n\t" // and rbx
		:"=a"(regs->eax), "=c"(regs->ecx), "=d"(regs->edx), "=S"(regs->esi), "=D"(regs->edi)
		:"a"(regs->eax), "c"(regs->ecx), "d"(regs->edx), "S"(regs->esi), "D"(regs->edi)
	);
#endif
}

extern "C" void
backdoor_rpc_get(regs_t* regs, char* data, size_t length, ulong port, ulong cookie1, ulong cookie2)
{
	// TODO : if possible, combine this function with the previous one.

	regs->eax = cookie2;
	
	regs->ecx = length;
	regs->edx = port;
	regs->esi = cookie1;
	regs->edi = (ulong)data;
#ifdef __i386__	
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
#else
	asm volatile(
		"pushq	%%rbx				\n\t" // Save rbx
		"pushq	%%rbp				\n\t" // and rbp
		"movl	%%eax,	%%ebp		\n\t" // eax => ebp
		"movl	$0x564D5868, %%eax	\n\t" // Init eax
		"movl	$0x00010000, %%ebx	\n\t" // Init ebx
		
		"cld						\n\t" // Backdoor call
		"rep insb (%%dx), (%%edi)	\n\t"
		
		"movl	%%ebx,	%%eax		\n\t" // ebx => eax
		"popq	%%rbp				\n\t" // Restore rbp
		"popq	%%rbx				\n\t" // and rbx
		:"=a"(regs->eax), "=c"(regs->ecx), "=d"(regs->edx), "=S"(regs->esi), "=D"(regs->edi)
		:"a"(regs->eax), "c"(regs->ecx), "d"(regs->edx), "S"(regs->esi), "D"(regs->edi)
	);
#endif
}

// Avoid segfault when trying to access the backdoor on real hardware...
static sigjmp_buf sigbuffer;

static void sighandler(int sig)
{
	siglongjmp(sigbuffer, sig);
}

VMWBackdoor::VMWBackdoor()
{
	rpc_opened = false;
	
	// Are we running in a VMware virtual machine ?
	
	if (sigsetjmp(sigbuffer, true) != 0) {
		// We crashed during the backdoor access
		in_vmware = false;
		signal(SIGILL, SIG_DFL);
		
		return;
	}
	
	signal(SIGILL, sighandler);
	
	regs_t regs;
	backdoor_call(&regs, VMW_BACK_GET_VERSION, 0);	
	in_vmware = (regs.esi == VMW_BACK_MAGIC);
	
	signal(SIGILL, SIG_DFL);
	
	if (!in_vmware)
		return;
	backdoor_access = create_sem(1, "vmware backdoor lock");
}

VMWBackdoor::~VMWBackdoor()
{
	delete_sem(backdoor_access);
}

bool
VMWBackdoor::InVMware() const
{
	return in_vmware;
}

status_t
VMWBackdoor::SyncCursor(BMessage* cursor_message)
{	
	regs_t regs;
	backdoor_call(&regs, VMW_BACK_GET_CURSOR, 0);
	
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
	regs_t regs;
	backdoor_call(&regs, VMW_BACK_GET_CLIP_LENGTH, 0);
	
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
	
	char* buffer = (char*)malloc(length + 1);
	
	if (buffer == NULL)
		return B_NO_MEMORY;
	
	buffer[length] = '\0';
	
	*text = buffer;
	
	long i = length;
	
	while (i > 0) {
		regs.eax = 0;
		
		backdoor_call(&regs, VMW_BACK_GET_CLIP_DATA, 0);
		
		memcpy(buffer, &regs.eax, (i < (signed)sizeof(regs.eax) ? i : sizeof(regs.eax)));
		
		buffer += sizeof(regs.eax);
		i -= sizeof(regs.eax);
	}

	return B_OK;
}

status_t
VMWBackdoor::SetHostClipboard(char* text, size_t text_length)
{
	GetHostClipboard(NULL, NULL);
	
	regs_t regs;
	backdoor_call(&regs, VMW_BACK_SET_CLIP_LENGTH, text_length);
	
	if (regs.eax != 0) {
		printf("Error setting clipboard.\n");
		return B_ERROR;
	}
	
	long i = text_length;
	char* buffer = text;
	
	while (i > 0) {
		ulong data = 0;		
		memcpy(&data, buffer, (i < (signed)sizeof(data) ? i : sizeof(data)));
		
		backdoor_call(&regs, VMW_BACK_SET_CLIP_DATA, data);
		
		buffer += sizeof(data);
		i -= sizeof(data);
	}
	
	return B_OK;
}

status_t
VMWBackdoor::OpenRPCChannel()
{
	puts("Backdoor: Opening RPC channel...");
	if (rpc_opened)
		return B_ERROR;
	
	regs_t regs;	
	backdoor_rpc_call(&regs, VMW_BACK_RPC_OPEN, VMW_BACK_RPC_MAGIC, VMW_BACK_RPC_PORT, 0, 0);
	
	if (regs.ecx != VMW_BACK_RPC_OK)
		return B_ERROR;
	
	rpc_opened = true;
	rpc_channel = regs.edx; // in high bits
	rpc_cookie1 = regs.esi;
	rpc_cookie2 = regs.edi;
	
	return B_OK;
}

status_t
VMWBackdoor::SendMessage(const char* message)
{
	size_t length;
	
	if (message == NULL)
		length = 0;
	else
		length = strlen(message);
	
	if (acquire_sem(backdoor_access) != B_OK)
		return B_ERROR;

	regs_t regs;
	// Send command length
	backdoor_rpc_call(&regs, VMW_BACK_RPC_SEND_LENGTH, length,
		rpc_channel | VMW_BACK_RPC_PORT, rpc_cookie1, rpc_cookie2);
	
	if (regs.ecx != VMW_BACK_RPC_SEND_L_OK) {
		release_sem(backdoor_access);
		return B_ERROR;
	}
	
	if (length > 0) {
		// Send command data
		backdoor_rpc_send(&regs, (char*)message, length,
			rpc_channel | VMW_BACK_RPC_PORT2, rpc_cookie1, rpc_cookie2);
	
		if (regs.eax != VMW_BACK_RPC_OK) {
			release_sem(backdoor_access);
			return B_ERROR;
		}
	}
	
	release_sem(backdoor_access);
	
	printf("%s : sent “%s”\n", __FUNCTION__, message);
	
	return B_OK;
}

char* VMWBackdoor::GetMessage()
{
	if (acquire_sem(backdoor_access) != B_OK)
		return NULL;
	
	regs_t regs;
	
	// Get data length
	backdoor_rpc_call(&regs, VMW_BACK_RPC_GET_LENGTH, 0,
		rpc_channel | VMW_BACK_RPC_PORT, rpc_cookie1, rpc_cookie2);
	
	if (regs.ecx != VMW_BACK_RPC_GET_L_OK) {
		release_sem(backdoor_access);
		return NULL;
	}
	
	size_t length = regs.eax;
	char* data = (char*)malloc(length + 1);
	uint reply_id = HIGH_BITS(regs.edx);
	
	// Get data
	backdoor_rpc_get(&regs, data, length,
		rpc_channel | VMW_BACK_RPC_PORT2, rpc_cookie1, rpc_cookie2);
		
	if (regs.eax != VMW_BACK_RPC_OK) {
		release_sem(backdoor_access);
		free(data);
		return NULL;
	}
	
	// Confirm...
	backdoor_rpc_call(&regs, VMW_BACK_RPC_ACK, reply_id,
		rpc_channel | VMW_BACK_RPC_PORT, rpc_cookie1, rpc_cookie2);
	
	release_sem(backdoor_access);
	
	if (regs.ecx != VMW_BACK_RPC_OK) {
		free(data);
		return NULL;
	}
	
	data[length] = '\0';
	
	printf("%s : got “%s”\n", __FUNCTION__, data);
	
	return data;
}

status_t
VMWBackdoor::CloseRPCChannel()
{
	if (!rpc_opened)
		return B_ERROR;
	
	regs_t regs;
	backdoor_rpc_call(&regs, VMW_BACK_RPC_CLOSE, 0, rpc_channel | VMW_BACK_RPC_PORT, rpc_cookie1, rpc_cookie2);
	
	if (regs.ecx != VMW_BACK_RPC_OK)
		return B_ERROR;
	
	rpc_opened = false;
	
	return B_OK;
}
