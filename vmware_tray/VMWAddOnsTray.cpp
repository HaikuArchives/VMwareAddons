/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "VMWAddOnsTray.h"

#include <AboutWindow.h>
#include <Alert.h>
#include <Catalog.h>
#include <Deskbar.h>
#include <MenuItem.h>
#include <Mime.h>

#include "VMWAddOns.h"
#include "VMWAddOnsCleanup.h"
#include "VMWAddOnsSettings.h"

#include "icons.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Deskbar tray icon"

#define CLIP_POLL_DELAY 1000000
#define CLOCK_POLL_DELAY 60000000


VMWAddOnsSettings gSettings;


extern "C" _EXPORT BView*
instantiate_deskbar_item(float /*maxWidth*/, float maxHeight)
{
	return new VMWAddOnsTray(maxHeight);
}


// #pragma mark -


VMWAddOnsTray::VMWAddOnsTray(float maxHeight)
	:
	BView(BRect(0, 0, maxHeight - 1, maxHeight - 1), TRAY_NAME, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		  B_WILL_DRAW)
{
	Init();
}


VMWAddOnsTray::VMWAddOnsTray(BMessage* message)
	:
	BView(message)
{
	Init();
}


void
VMWAddOnsTray::Init()
{
	fSystemClipboard = new BClipboard("system");
	fClipboardPoller = NULL;
	fClockSynchonizer = NULL;
	fCleanupInProgress = false;

	fIconAll = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_CMAP8);
	fIconAll->SetBits(pic_act_yy, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	fIconMouse = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_CMAP8);
	fIconMouse->SetBits(pic_act_ny, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	fIconClipboard = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_CMAP8);
	fIconClipboard->SetBits(pic_act_yn, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	fIconNone = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_CMAP8);
	fIconNone->SetBits(pic_act_nn, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	fIconDisabled = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_CMAP8);
	fIconDisabled->SetBits(pic_disabled, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	SetDrawingMode(B_OP_ALPHA);
	SetFlags(Flags() | B_WILL_DRAW);
}


VMWAddOnsTray::~VMWAddOnsTray()
{
	delete fIconAll;
	delete fIconMouse;
	delete fIconClipboard;
	delete fIconNone;
	delete fIconDisabled;

	delete fSystemClipboard;
	delete fClipboardPoller;
	delete fClockSynchonizer;
}


status_t
VMWAddOnsTray::Archive(BMessage* data, bool deep = true) const
{
	data->AddString("add_on", APP_SIG);

	return BView::Archive(data, deep);
}


VMWAddOnsTray*
VMWAddOnsTray::Instantiate(BMessage* message)
{
	return new VMWAddOnsTray(message);
}


void
VMWAddOnsTray::Draw(BRect /*update_rect*/)
{
	BRect trayBounds(Bounds());

	if (Parent())
		SetHighColor(Parent()->ViewColor());
	else
		SetHighColor(189, 186, 189, 255);

	FillRect(trayBounds);

	if (!fBackdoor.InVMware()) {
		DrawBitmap(fIconDisabled);
		return;
	}

	if (gSettings.GetBool("mouse_enabled", true))
		if (gSettings.GetBool("clip_enabled", true))
			DrawBitmap(fIconAll, fIconAll->Bounds(), trayBounds);
		else
			DrawBitmap(fIconMouse, fIconMouse->Bounds(), trayBounds);
	else if (gSettings.GetBool("clip_enabled", true))
		DrawBitmap(fIconClipboard, fIconClipboard->Bounds(), trayBounds);
	else
		DrawBitmap(fIconNone, fIconNone->Bounds(), trayBounds);
}


void
VMWAddOnsTray::MouseDown(BPoint where)
{
	ConvertToScreen(&where);
	VMWAddOnsMenu* menu = new VMWAddOnsMenu(this, fBackdoor);
	menu->Go(where, true, true, ConvertToScreen(Bounds()), true);
}


void
VMWAddOnsTray::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MOUSE_SHARING:
		{
			bool value = gSettings.GetBool("mouse_enabled", true);
			value = !value;
			// Only enable mouse sharing if both guest and host have it enabled.
			if (!fBackdoor.GetGUISetting(VMWBackdoor::POINTER_GRAB_UNGRAB))
				value = false;

			gSettings.SetBool("mouse_enabled", value);
			this->Invalidate();
		} break;

		case CLIPBOARD_SHARING:
		{
			bool value = gSettings.GetBool("clip_enabled", true);
			value = !value;
			// Only enable clipboard sharing if both guest and host have it enabled.
			if (!fBackdoor.GetGUISetting(VMWBackdoor::CLIP_BOARD_SHARING))
				value = false;

			gSettings.SetBool("clip_enabled", value);
			SetClipboardSharing(value);
			this->Invalidate();
		} break;

		case TIMESYNC_HOST:
		{
			bool value = gSettings.GetBool("timesync_enabled", true);
			value = !value;
			// Only enable timesync if both guest and host have it enabled.
			if (!fBackdoor.GetGUISetting(VMWBackdoor::TIME_SYNC))
				value = false;
			gSettings.SetBool("timesync_enabled", value);
			SetTimeSynchronization(value);
		} break;

		case REMOVE_FROM_DESKBAR:
			RemoveMyself(true);
			break;

		case B_ABOUT_REQUESTED:
		{
			BAboutWindow* window = new BAboutWindow("VMware Add-ons",
				"application/x-vnd.VinDuv.VMwareAddOns");
			window->AddCopyright(2009, "Vincent Duvert");
			window->Show();
		} break;

		case B_CLIPBOARD_CHANGED:
		{
			if (!fSystemClipboard->Lock())
				return;

			BMessage* clipMessage = fSystemClipboard->Data();
			fSystemClipboard->Unlock();

			if (clipMessage == NULL)
				return;

			char* data;
			ssize_t len;
			clipMessage->FindData("text/plain", B_MIME_TYPE, (const void**)&data, &len);
			if (data == NULL)
				return;

			// Send data to the host clipboard
			fBackdoor.SetHostClipboard(data, len);
		} break;

		case CLIPBOARD_POLL:
		{
			char* data;
			size_t len;

			if (fBackdoor.GetHostClipboard(&data, &len) != B_OK)
				return;

			if (!fSystemClipboard->Lock()) {
				free(data);
				return;
			}
			fSystemClipboard->Clear();

			BMessage* clipMessage = fSystemClipboard->Data();
			if (clipMessage == NULL) {
				fSystemClipboard->Unlock();
				free(data);
				return;
			}

			clipMessage->AddData("text/plain", B_MIME_TYPE, data, len);
			fSystemClipboard->Commit();
			fSystemClipboard->Unlock();
			free(data);
		} break;

		case CLOCK_POLL:
		{
			ulong clockValue = fBackdoor.GetHostClock();
			if (clockValue > 0)
				set_real_time_clock(clockValue);
		} break;

		case SHRINK_DISKS:
		{
			thread_id thread
				= spawn_thread(VMWAddOnsCleanup::Start, "vmw cleanup", B_LOW_PRIORITY, this);
			if (thread > 0) {
				fCleanupInProgress = true;
				resume_thread(thread);
			}
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
VMWAddOnsTray::AttachedToWindow()
{
	if (Parent() != NULL)
		if ((Parent()->Flags() & B_DRAW_ON_CHILDREN) != 0)
			SetViewColor(B_TRANSPARENT_COLOR);
		else
			SetViewColor(Parent()->ViewColor());
	else
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	SetLowColor(ViewColor());

	if (fBackdoor.InVMware()) {
		SetClipboardSharing(gSettings.GetBool("clip_enabled", true));
		SetTimeSynchronization(gSettings.GetBool("timesync_enabled", true));
	}
}


void
VMWAddOnsTray::SetClipboardSharing(bool enable)
{
	delete fClipboardPoller;
	fClipboardPoller = NULL;

	if (enable) {
		fSystemClipboard->StartWatching(this);
		fClipboardPoller = new BMessageRunner(this, new BMessage(CLIPBOARD_POLL), CLIP_POLL_DELAY);
	} else {
		fSystemClipboard->StopWatching(this);
	}
}


void
VMWAddOnsTray::SetTimeSynchronization(bool enable)
{
	delete fClockSynchonizer;
	fClockSynchonizer = NULL;

	if (enable)
		fClockSynchonizer = new BMessageRunner(this, new BMessage(CLOCK_POLL), CLOCK_POLL_DELAY);
}


int32
removeFromDeskbar(void*)
{
	BDeskbar db;
	db.RemoveItem(TRAY_NAME);

	return 0;
}


void
VMWAddOnsTray::StartShrink()
{
	if (fBackdoor.OpenRPCChannel() == B_OK) {
		fBackdoor.SendMessage("disk.shrink", true);
		fBackdoor.CloseRPCChannel();
	} else {
		(new BAlert(TRAY_NAME,
			B_TRANSLATE("Unable to communicate with VMware. Your VMware version may be too old. "
				"Please start the process manually if your VMware version allows it."),
			 B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))
			->Go();
	}
}


void
VMWAddOnsTray::RemoveMyself(bool askUser)
{
	// From the BeBook : A BView sitting on the Deskbar cannot remove itself (or another view)
	// So we start another thread to do the dirty work

	int32 result = 1;
	if (askUser) {
		result = (new BAlert(TRAY_NAME,
					  B_TRANSLATE("Are you sure you want to quit?\n"
						"This will stop clipboard sharing, and time synchronization "
						"(but not the mouse sharing, if it is currently enabled)."),
					  B_TRANSLATE("Cancel"),
					  B_TRANSLATE("Quit"),
					  NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT))
					 ->Go();
	}

	if (result != 0) {
		thread_id thread
			= spawn_thread(removeFromDeskbar, "goodbye cruel world", B_NORMAL_PRIORITY, NULL);
		if (thread)
			resume_thread(thread);
	}
}


// #pragma mark -


VMWAddOnsMenu::VMWAddOnsMenu(VMWAddOnsTray* tray, VMWBackdoor& fBackdoor)
	:
	BPopUpMenu("tray_menu", false, false)
{
	BMenuItem* menuItem;

	SetFont(be_plain_font);

	if (fBackdoor.InVMware()) {
		menuItem = new BMenuItem(B_TRANSLATE("Enable mouse sharing"), new BMessage(MOUSE_SHARING));
		if (fBackdoor.GetGUISetting(VMWBackdoor::POINTER_GRAB_UNGRAB))
			menuItem->SetMarked(gSettings.GetBool("mouse_enabled", true));
		else
			menuItem->SetMarked(false);
		AddItem(menuItem);

		menuItem = new BMenuItem(B_TRANSLATE("Enable clipboard sharing"),
			new BMessage(CLIPBOARD_SHARING));
		if (fBackdoor.GetGUISetting(VMWBackdoor::CLIP_BOARD_SHARING))
			menuItem->SetMarked(gSettings.GetBool("clip_enabled", true));
		else
			menuItem->SetMarked(false);
		AddItem(menuItem);

		menuItem = new BMenuItem(B_TRANSLATE_COMMENT("Enable time synchronization",
			"Alternatively: \"Synchronize clock\"."),
			new BMessage(TIMESYNC_HOST));
		if (fBackdoor.GetGUISetting(VMWBackdoor::TIME_SYNC))
			menuItem->SetMarked(gSettings.GetBool("timesync_enabled", true));
		else
			menuItem->SetMarked(false);
		AddItem(menuItem);

		if (!tray->fCleanupInProgress) {
			AddItem(
				new BMenuItem(B_TRANSLATE("Shrink virtual disks" B_UTF8_ELLIPSIS),
				new BMessage(SHRINK_DISKS))
			);
		}
	} else {
		menuItem = new BMenuItem(B_TRANSLATE_COMMENT("Not running in VMware",
			"Alternatively: \"VMware not detected\"."), NULL);
		menuItem->SetEnabled(false);
		AddItem(menuItem);
	}

	AddSeparatorItem();

	AddItem(new BMenuItem(B_TRANSLATE("About " APP_NAME B_UTF8_ELLIPSIS),
		new BMessage(B_ABOUT_REQUESTED)));
	AddItem(new BMenuItem(B_TRANSLATE("Quit" B_UTF8_ELLIPSIS), new BMessage(REMOVE_FROM_DESKBAR)));

	SetTargetForItems(tray);
	SetAsyncAutoDestruct(true);
}
