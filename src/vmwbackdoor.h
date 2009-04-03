/*
	vmwbackdoor.h

	From the VMW Tools,	Copyright (c) 2006 Ken Kato
	http://chitchat.at.infoseek.co.jp/vmware/vmtools.html
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef _VMW_H
#define _VMW_H 1

#include <stdint.h>

#include <SupportDefs.h>

typedef struct _rpc {
	uint32_t channel;						/* channel number				*/
	uint32_t cookie1;						/* cookie#1 for enhanced rpc	*/
	uint32_t cookie2;						/* cookie#2 for enhanced rpc	*/
} rpc_t;

#ifdef __cplusplus
extern "C" {
#endif

status_t VMCheckVirtual(void);
status_t VMGetCursorPos(int16_t *xpos, int16_t *ypos);
status_t VMClipboardPaste(char **data, ssize_t *length);
status_t VMClipboardCopy(const char *data, ssize_t length);

#ifdef __cplusplus
}
#endif

#endif
