/*
	vmwbackdoor.c

	From the VMW Tools,	Copyright (c) 2006 Ken Kato
	http://chitchat.at.infoseek.co.jp/vmware/vmtools.html
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "vmwbackdoor.h"

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <Errors.h>

#define VMWARE_MAGIC		0x564D5868UL	/* Backdoor magic number (VMXh)	*/
#define VMWARE_CMD_PORT		0x5658			/* Backdoor command port (VX)	*/
#define VMCMD_GET_CURSOR	0x04			/* Get cursor position			*/
#define VMCMD_GET_CLIPLEN	0x06			/* Get host->guest copy length	*/
#define VMCMD_GET_CLIPDATA	0x07			/* Get host->guest copy data	*/
#define VMCMD_SET_CLIPLEN	0x08			/* Set guest->host copy length	*/
#define VMCMD_SET_CLIPDATA	0x09			/* Set guest->host copy data	*/
#define VMCMD_GET_VERSION	0x0a			/* Get version number			*/
#define VMCMD_INVOKE_RPC	0x1e			/* Invoke RPC command.			*/

#define VMRPC_OPEN			0x00000000UL	/* Open RPC channel				*/
#define VMRPC_SENDLEN		0x00010000UL	/* Send RPC command length		*/
#define VMRPC_SENDDAT		0x00020000UL	/* Send RPC command				*/
#define VMRPC_RECVLEN		0x00030000UL	/* Receive RPC reply length		*/
#define VMRPC_RECVDAT		0x00040000UL	/* Receive RPC reply			*/
#define VMRPC_RECVEND		0x00050000UL	/* Unknown backdoor command		*/
#define VMRPC_CLOSE			0x00060000UL	/* Close RPC channel			*/

#define VMRPC_OPEN_RPCI		0x49435052UL	/* 'RPCI' channel open magic	*/
#define VMRPC_OPEN_TCLO		0x4f4c4354UL	/* 'TCLO' channel open magic	*/

typedef struct _reg {
	uint8_t eax[4];
	uint8_t ebx[4];
	uint8_t ecx[4];
	uint8_t edx[4];
	uint8_t ebp[4];
	uint8_t edi[4];
	uint8_t esi[4];
} reg_t;

#define R_EAX(r)	*((uint32_t *)&((r).eax))
#define R_EBX(r)	*((uint32_t *)&((r).ebx))
#define R_ECX(r)	*((uint32_t *)&((r).ecx))
#define R_EDX(r)	*((uint32_t *)&((r).edx))
#define R_EBP(r)	*((uint32_t *)&((r).ebp))
#define R_EDI(r)	*((uint32_t *)&((r).edi))
#define R_ESI(r)	*((uint32_t *)&((r).esi))


#define asm_op(opcode, reg) 		\
	__asm__ __volatile__ (			\
		"pushal;"					\
		"pushl %%eax;"				\
		"movl 0x18(%%eax), %%esi;"	\
		"movl 0x14(%%eax), %%edi;"	\
		"movl 0x10(%%eax), %%ebp;"	\
		"movl 0x0c(%%eax), %%edx;"	\
		"movl 0x08(%%eax), %%ecx;"	\
		"movl 0x04(%%eax), %%ebx;"	\
		"movl 0x00(%%eax), %%eax;"	\
		opcode						\
		"xchgl %%eax, 0x00(%%esp);"	\
		"movl %%esi, 0x18(%%eax);"	\
		"movl %%edi, 0x14(%%eax);"	\
		"movl %%ebp, 0x10(%%eax);"	\
		"movl %%edx, 0x0c(%%eax);"	\
		"movl %%ecx, 0x08(%%eax);"	\
		"movl %%ebx, 0x04(%%eax);"	\
		"popl 0x00(%%eax);"			\
		"popal;"					\
		::"a"(reg)					\
	);

void vmcall_cmd(reg_t *reg)
{
	asm_op("inl (%%dx);", reg);
}

status_t VMGetCursorPos(int16_t *xpos, int16_t *ypos)
{
	reg_t reg;

	R_EAX(reg) = VMWARE_MAGIC;
	R_ECX(reg) = VMCMD_GET_CURSOR;
	R_EDX(reg) = VMWARE_CMD_PORT;

	vmcall_cmd(&reg);

	if (R_EAX(reg) == VMWARE_MAGIC || R_EAX(reg) == 0xffffffffL)
		return B_ERROR;

	if (xpos)
		*xpos = (int16_t)(R_EAX(reg) >> 16);

	if (ypos)
		*ypos = (int16_t)(R_EAX(reg) & 0xffff);

	return B_OK;
}

status_t VMClipboardPaste(char **data, ssize_t *length)
{
	reg_t reg;
	uint32_t total;
	char *buf = NULL;

	/*	get text length */

	R_EAX(reg) = VMWARE_MAGIC;
	R_ECX(reg) = VMCMD_GET_CLIPLEN;
	R_EDX(reg) = VMWARE_CMD_PORT;

	vmcall_cmd(&reg);

	total = R_EAX(reg);

	if (total == 0) {
		if (length != NULL)
			*length = 0;

		return B_OK;
	}
	
	if (total == 0xffffffffL) {
		/* No more data in the clipboard */
		if (length != NULL)
			*length = -1;
		return B_OK;
		
	} else if (total > 0xffff) {
		return B_ERROR;
	}

	/* allocate text buffer */

	if (data) {
		buf = malloc(total + 1);

		if (buf == NULL) {
			return B_NO_MEMORY;
		}

		*data = buf;
	}

	if (length) {
		*length = total;
	}

	/* get text data */

	for (;;) {
		memset(&reg, 0, sizeof(reg));

		R_EAX(reg) = VMWARE_MAGIC;
		R_ECX(reg) = VMCMD_GET_CLIPDATA;
		R_EDX(reg) = VMWARE_CMD_PORT;

		vmcall_cmd(&reg);

		if (buf) {
			memcpy(buf, &R_EAX(reg), total > 4 ? 4 : total);

			if (total <= 4) {
				*(buf + total) = '\0';
			}
			else {
				buf += 4;
			}
		}

		if (total <= 4) {
			return B_OK;
		}
		else {
			total -= 4;
		}
	}
}

status_t VMClipboardCopy(const char *data, ssize_t length)
{
	reg_t reg;
	
	 if (length <= 0)
		length = strlen(data);
	
	
	if (length == 0 || length > 0xffff)
		return B_ERROR;

	/*	set text length */

	R_EAX(reg) = VMWARE_MAGIC;
	R_EBX(reg) = length;
	R_ECX(reg) = VMCMD_SET_CLIPLEN;
	R_EDX(reg) = VMWARE_CMD_PORT;

	vmcall_cmd(&reg);

	if (R_EAX(reg) != 0)
		return B_ERROR;

	/* set text data */

	for (;;) {
		memset(&reg, 0, sizeof(reg));

		R_EAX(reg) = VMWARE_MAGIC;
		memcpy(&R_EBX(reg), data, length > 4 ? 4 : length);
		R_ECX(reg) = VMCMD_SET_CLIPDATA;
		R_EDX(reg) = VMWARE_CMD_PORT;

		vmcall_cmd(&reg);

		if (length <= 4)
			return B_OK;

		data += 4;
		length -= 4;
	}
}

static sigjmp_buf	sigillbuffer;

static void sigillhandler(int sig)
{
	siglongjmp(sigillbuffer, sig);
}

status_t VMCheckVirtual(void)
{
	reg_t reg;
	struct sigaction newaction, oldaction;

	R_EAX(reg) = VMWARE_MAGIC;
	R_EBX(reg) = ~VMWARE_MAGIC;
	R_ECX(reg) = VMCMD_GET_VERSION;
	R_EDX(reg) = VMWARE_CMD_PORT;

	sigemptyset(&newaction.sa_mask);
	newaction.sa_flags = SA_RESTART;
	newaction.sa_handler = sigillhandler;
	sigaction(SIGILL, &newaction, &oldaction);

	if (sigsetjmp(sigillbuffer, 1) != 0) {
		/* SIGILL is caught during the backdoor access */
		sigaction(SIGILL, &oldaction, 0);

		return B_ERROR;
	}

	vmcall_cmd(&reg);

	sigaction(SIGILL, &oldaction, 0);

	if (R_EBX(reg) != VMWARE_MAGIC)
		return B_ERROR;

	return B_OK;
}

status_t VMRpcOpen(rpc_t *rpc)
{
	reg_t reg;

	R_EAX(reg) = VMWARE_MAGIC;
	R_EBX(reg) = VMRPC_OPEN_RPCI;
	R_ECX(reg) = VMCMD_INVOKE_RPC | VMRPC_OPEN;
	R_EDX(reg) = VMWARE_CMD_PORT;

	vmcall_cmd(&reg);

	if (R_EAX(reg) || R_ECX(reg) != 0x00010000L || R_EDX(reg) & 0xffff)
		return B_ERROR;

	memset(rpc, 0, sizeof(rpc_t));

	rpc->channel = R_EDX(reg);

	return B_OK;
}

status_t VMRpcSend(const rpc_t *rpc, const unsigned char *command)
{
	reg_t reg;

	uint32_t length = strlen((const char *)command);

	R_EAX(reg) = VMWARE_MAGIC;
	R_EBX(reg) = length;
	R_ECX(reg) = VMCMD_INVOKE_RPC | VMRPC_SENDLEN;
	R_EDX(reg) = rpc->channel | VMWARE_CMD_PORT;
	R_ESI(reg) = rpc->cookie1;
	R_EDI(reg) = rpc->cookie2;

	vmcall_cmd(&reg);

	if (R_EAX(reg) || (R_ECX(reg) >> 16) == 0) {
		return B_ERROR;
	}

	if (length == 0) {
		return B_OK;
	}

	for (;;) {
		memset(&reg, 0, sizeof(reg));

		R_EAX(reg) = VMWARE_MAGIC;
		memcpy(&R_EBX(reg), command, length > 4 ? 4 : length);
		R_ECX(reg) = VMCMD_INVOKE_RPC | VMRPC_SENDDAT;
		R_EDX(reg) = rpc->channel | VMWARE_CMD_PORT;

		vmcall_cmd(&reg);

		if (R_EAX(reg) || (R_ECX(reg) >> 16) == 0) {
			return B_ERROR;
		}

		if (length <= 4) {
			break;
		}

		length -= 4;
		command += 4;
	}

	return B_OK;
}

status_t VMRpcRecvLen(const rpc_t *rpc, uint32_t *length, uint16_t *dataid)
{
	reg_t reg;

	R_EAX(reg) = VMWARE_MAGIC;
	R_ECX(reg) = VMCMD_INVOKE_RPC | VMRPC_RECVLEN;
	R_EDX(reg) = rpc->channel | VMWARE_CMD_PORT;
	R_ESI(reg) = rpc->cookie1;
	R_EDI(reg) = rpc->cookie2;

	vmcall_cmd(&reg);

	if (R_EAX(reg) || (R_ECX(reg) >> 16) == 0) {
		return B_ERROR;
	}

	*length = R_EBX(reg);
	*dataid = (uint16_t)(R_EDX(reg) >> 16);

	return B_OK;
}

status_t VMRpcRecvDat(const rpc_t *rpc, unsigned char *data, uint32_t length, uint16_t dataid)
{
	reg_t reg;

	uint32_t *p = (uint32_t *)data;

	for (;;) {
		memset(&reg, 0, sizeof(reg));

		R_EAX(reg) = VMWARE_MAGIC;
		R_EBX(reg) = dataid;
		R_ECX(reg) = VMCMD_INVOKE_RPC | VMRPC_RECVDAT;
		R_EDX(reg) = rpc->channel | VMWARE_CMD_PORT;

		vmcall_cmd(&reg);

		if (R_EAX(reg) || (R_ECX(reg) >> 16) == 0)
			return B_ERROR;

		*(p++) = R_EBX(reg);

		if (length <= 4) {
			break;
		}

		length -= 4;
	}

	/*
		I'm not sure what the following command is for.
		It's just the vmware-tools always uses this command in this place.
	*/
	memset(&reg, 0, sizeof(reg));

	R_EAX(reg) = VMWARE_MAGIC;
	R_EBX(reg) = dataid;
	R_ECX(reg) = VMCMD_INVOKE_RPC | VMRPC_RECVEND;
	R_EDX(reg) = rpc->channel | VMWARE_CMD_PORT;
	R_ESI(reg) = rpc->cookie1;
	R_EDI(reg) = rpc->cookie2;

	vmcall_cmd(&reg);

	if (R_EAX(reg) || (R_ECX(reg) >> 16) == 0)
		return B_ERROR;

	return B_OK;
}


status_t VMRpcRecv(const rpc_t *rpc, unsigned char **result)
{
	status_t ret;
	uint32_t length;
	uint16_t dataid;

	ret = VMRpcRecvLen(rpc, &length, &dataid);

	if (ret != B_OK || length == 0) {
		return ret;
	}

	/* returned length does not include terminating NULL character */

	*result = (unsigned char *)malloc(length + 4);

	if (!*result) {
		return B_NO_MEMORY;
	}

	memset(*result, 0, length + 4);

	return VMRpcRecvDat(rpc, *result, length, dataid);
}

status_t VMRpcClose(const rpc_t *rpc)
{
	reg_t reg;

	R_EAX(reg) = VMWARE_MAGIC;
	R_ECX(reg) = VMCMD_INVOKE_RPC | VMRPC_CLOSE;
	R_EDX(reg) = rpc->channel | VMWARE_CMD_PORT;
	R_ESI(reg) = rpc->cookie1;
	R_EDI(reg) = rpc->cookie2;

	vmcall_cmd(&reg);

	if (R_EAX(reg) || (R_ECX(reg) >> 16) == 0)
		return B_ERROR;

	return B_OK;
}


