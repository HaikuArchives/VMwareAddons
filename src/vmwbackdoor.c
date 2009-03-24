/*
	vmwbackdoor.c

	From the VMW Tools,	Copyright (c) 2006 Ken Kato
	http://chitchat.at.infoseek.co.jp/vmware/vmtools.html
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include <setjmp.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>

#include <SupportDefs.h>
#include <Errors.h>

#include "vmwbackdoor.h"

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

	if (R_EAX(reg) == VMWARE_MAGIC ||
		R_EAX(reg) == 0xffffffffL) {

		return B_ERROR;
	}

	if (xpos) {
		*xpos = (int16_t)(R_EAX(reg) >> 16);
	}

	if (ypos) {
		*ypos = (int16_t)(R_EAX(reg) & 0xffff);
	}

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

	if (length <= 0) {
		length = strlen(data);
	}

	if (length == 0) {
		return B_ERROR;
	}

	if (length > 0xffff) {
		return B_ERROR;
	}

	/*	set text length */

	R_EAX(reg) = VMWARE_MAGIC;
	R_EBX(reg) = length;
	R_ECX(reg) = VMCMD_SET_CLIPLEN;
	R_EDX(reg) = VMWARE_CMD_PORT;

	vmcall_cmd(&reg);

	if (R_EAX(reg) != 0) {
		return B_ERROR;
	}

	/* set text data */

	for (;;) {
		memset(&reg, 0, sizeof(reg));

		R_EAX(reg) = VMWARE_MAGIC;
		memcpy(&R_EBX(reg), data, length > 4 ? 4 : length);
		R_ECX(reg) = VMCMD_SET_CLIPDATA;
		R_EDX(reg) = VMWARE_CMD_PORT;

		vmcall_cmd(&reg);

		if (length <= 4) {
			return B_OK;
		}

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

	if (R_EBX(reg) != VMWARE_MAGIC) {
		/* get version backdoor command failure */
		return B_ERROR;
	}

	return B_OK;
}


