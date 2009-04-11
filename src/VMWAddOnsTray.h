/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef VMW_TRAY_H
#define VMW_TRAY_H

#include <Bitmap.h>
#include <Clipboard.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <View.h>

#include "VMWAddOnsSettings.h"
#include "VMWAddOnsCleanupWindow.h"
#include "VMWBackdoor.h"

class VMWAddOnsTray: public BView {
public:
							VMWAddOnsTray();
							VMWAddOnsTray(BMessage* mdArchive);
	virtual					~VMWAddOnsTray();
	virtual void			GetPreferredSize(float *w, float* h);
	virtual void			Draw(BRect /*update_rect*/);
	virtual void			MessageReceived(BMessage* message);
	virtual void 			AttachedToWindow();	
	virtual void			MouseDown(BPoint where);

	virtual status_t		Archive(BMessage *data, bool deep) const;
	static VMWAddOnsTray*	Instantiate(BMessage *data);

	bool					window_open;

private:
	void					init();
	void					RemoveMyself(bool askUser);
	void					SetClipboardSharing(bool enable);
	
	BBitmap*				icon_all;
	BBitmap*				icon_mouse;
	BBitmap*				icon_clipboard;
	BBitmap*				icon_none;
	
	BClipboard*				system_clipboard;
	BMessageRunner*			clipboard_poller;
	
	VMWAddOnsCleanupWindow*	cleanup_window;	
	
	VMWBackdoor				backdoor;
};

class VMWAddOnsMenu: public BPopUpMenu {
public:
					VMWAddOnsMenu(VMWAddOnsTray* tray);
	virtual			~VMWAddOnsMenu();
};

#endif
