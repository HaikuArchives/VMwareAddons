/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	Copyright 2010-2011 Joshua Stein <jcs@jcs.org>
	Copyright 2018 Gerasim Troeglazov <3dEyes@gmail.com>
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMW_BACK_CORE_H
#define VMW_BACK_CORE_H

#include <Message.h>

#define VMW_BACK_MAGIC				0x564D5868UL
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


#define HIGH_BITS(x) (((x) >> 16) & 0xFFFF)
#define LOW_BITS(x) ((x) & 0xFFFF)
#define TO_HIGH(x) ((x) << 16)

union vm_reg {
	struct {
		uint16_t low;
		uint16_t high;
	} part;
	uint32_t word;
#ifdef __amd64__
	struct {
		uint32_t low;
		uint32_t high;
	} words;
	uint64_t quad;
#endif
};

struct vm_backdoor {
	union vm_reg eax;
	union vm_reg ebx;
	union vm_reg ecx;
	union vm_reg edx;
	union vm_reg esi;
	union vm_reg edi;
	union vm_reg ebp;
};

#define BACKDOOR_OP_I386(op, frame)		\
	__asm__ __volatile__ (			\
		"pushal;"			\
		"pushl %%eax;"			\
		"movl 0x18(%%eax), %%ebp;"	\
		"movl 0x14(%%eax), %%edi;"	\
		"movl 0x10(%%eax), %%esi;"	\
		"movl 0x0c(%%eax), %%edx;"	\
		"movl 0x08(%%eax), %%ecx;"	\
		"movl 0x04(%%eax), %%ebx;"	\
		"movl 0x00(%%eax), %%eax;"	\
		op				\
		"xchgl %%eax, 0x00(%%esp);"	\
		"movl %%ebp, 0x18(%%eax);"	\
		"movl %%edi, 0x14(%%eax);"	\
		"movl %%esi, 0x10(%%eax);"	\
		"movl %%edx, 0x0c(%%eax);"	\
		"movl %%ecx, 0x08(%%eax);"	\
		"movl %%ebx, 0x04(%%eax);"	\
		"popl 0x00(%%eax);"		\
		"popal;"			\
		::"a"(frame)			\
	)

#define BACKDOOR_OP_AMD64(op, frame)		\
	__asm__ __volatile__ (			\
		"pushq %%rbp;			\n\t" \
		"pushq %%rax;			\n\t" \
		"movq 0x30(%%rax), %%rbp;	\n\t" \
		"movq 0x28(%%rax), %%rdi;	\n\t" \
		"movq 0x20(%%rax), %%rsi;	\n\t" \
		"movq 0x18(%%rax), %%rdx;	\n\t" \
		"movq 0x10(%%rax), %%rcx;	\n\t" \
		"movq 0x08(%%rax), %%rbx;	\n\t" \
		"movq 0x00(%%rax), %%rax;	\n\t" \
		op				"\n\t" \
		"xchgq %%rax, 0x00(%%rsp);	\n\t" \
		"movq %%rbp, 0x30(%%rax);	\n\t" \
		"movq %%rdi, 0x28(%%rax);	\n\t" \
		"movq %%rsi, 0x20(%%rax);	\n\t" \
		"movq %%rdx, 0x18(%%rax);	\n\t" \
		"movq %%rcx, 0x10(%%rax);	\n\t" \
		"movq %%rbx, 0x08(%%rax);	\n\t" \
		"popq 0x00(%%rax);		\n\t" \
		"popq %%rbp;			\n\t" \
		:: "a" (frame) \
		: "rbx", "rcx", "rdx", "rdi", "rsi", "cc", "memory" \
	)

#ifdef __i386__
#define BACKDOOR_OP(op, frame) BACKDOOR_OP_I386(op, frame)
#else
#define BACKDOOR_OP(op, frame) BACKDOOR_OP_AMD64(op, frame)
#endif

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
