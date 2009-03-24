/*
	vmwbackdoor.c

	From the VMW Tools,	Copyright (c) 2006 Ken Kato
	http://chitchat.at.infoseek.co.jp/vmware/vmtools.html
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef _VMW_H
#define _VMW_H 1

#include <SupportDefs.h>
#include <stdint.h>

#define VMWARE_MAGIC		0x564D5868UL	/* Backdoor magic number (VMXh)	*/
#define VMWARE_CMD_PORT		0x5658			/* Backdoor command port (VX)	*/
#define VMCMD_GET_CURSOR	0x04			/* Get cursor position			*/
#define VMCMD_GET_CLIPLEN	0x06			/* Get host->guest copy length	*/
#define VMCMD_GET_CLIPDATA	0x07			/* Get host->guest copy data	*/
#define VMCMD_SET_CLIPLEN	0x08			/* Set guest->host copy length	*/
#define VMCMD_SET_CLIPDATA	0x09			/* Set guest->host copy data	*/
#define VMCMD_GET_VERSION	0x0a			/* Get version number			*/

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

#ifdef __cplusplus
extern "C" {
#endif

status_t VMCheckVirtual(void);
status_t VMGetCursorPos(int16_t *xpos, int16_t *ypos);
status_t VMClipboardPaste(char **data, ssize_t *length);
status_t VMClipboardCopy(const char *data, ssize_t length);
void vmcall_cmd(reg_t *reg);

#ifdef __cplusplus
}
#endif

#endif
