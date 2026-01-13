/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef VMW_TRAY_H
#define VMW_TRAY_H

#include <Bitmap.h>
#include <Clipboard.h>
#include <MessageRunner.h>
#include <PopUpMenu.h>
#include <View.h>

#include "VMWBackdoor.h"


class VMWAddOnsTray : public BView {
public:
							VMWAddOnsTray(float maxHeight);
							VMWAddOnsTray(BMessage* message);
	virtual					~VMWAddOnsTray();

	virtual void 			AttachedToWindow();
	virtual status_t		Archive(BMessage* message, bool deep) const;
	static VMWAddOnsTray*	Instantiate(BMessage* message);
	virtual void			Draw(BRect /*update_rect*/);
	virtual void			MessageReceived(BMessage* message);
	virtual void			MouseDown(BPoint where);

	void					StartShrink();

public:
	bool					fCleanupInProgress;

private:
	void					Init();
	void					RemoveMyself(bool askUser);
	void					SetClipboardSharing(bool enable);
	void					SetTimeSynchronization(bool enable);

	BBitmap*				fIconAll;
	BBitmap*				fIconClipboard;
	BBitmap*				fIconDisabled;
	BBitmap*				fIconMouse;
	BBitmap*				fIconNone;

	BClipboard*				fSystemClipboard;

	BMessageRunner*			fClipboardPoller;
	BMessageRunner*			fClockSynchonizer;

	VMWBackdoor				fBackdoor;
};


class VMWAddOnsMenu : public BPopUpMenu {
public:
			VMWAddOnsMenu(VMWAddOnsTray* tray, VMWBackdoor& backdoor);
};


#endif
