/*
	Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
	All rights reserved. Distributed under the terms of the MIT License.
*/

#include "VMWAddOnsTray.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Deskbar.h>
#include <Roster.h>
#include <MenuItem.h>
#include <Mime.h>

#include "VMWAddOns.h"
#include "VMWAddOnsSettings.h"
#include "VMWAddOnsCleanup.h"
#include "icons.h"

VMWAddOnsSettings settings;

#define CLIP_POLL_DELAY 1000000
#define CLOCK_POLL_DELAY 30000000

extern "C" _EXPORT BView* instantiate_deskbar_item(void)
{
	return (new VMWAddOnsTray);
}

VMWAddOnsTray::VMWAddOnsTray()
	: BView(BRect(0, 0, B_MINI_ICON, B_MINI_ICON),
		TRAY_NAME, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW)
{
		init();
}

VMWAddOnsTray::VMWAddOnsTray(BMessage* mdArchive)
	:BView(mdArchive)
{
	init();
}

void
VMWAddOnsTray::init()
{
	system_clipboard = new BClipboard("system");
	clipboard_poller = NULL;
	clock_sync = NULL;
	cleanup_in_process = false;

	icon_all = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_CMAP8);
	icon_all->SetBits(pic_act_yy, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	icon_mouse = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_CMAP8);
	icon_mouse->SetBits(pic_act_ny, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	icon_clipboard = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_CMAP8);
	icon_clipboard->SetBits(pic_act_yn, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	icon_none = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_CMAP8);
	icon_none->SetBits(pic_act_nn, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	icon_disabled = new BBitmap(BRect(0, 0, B_MINI_ICON - 1, B_MINI_ICON - 1), B_CMAP8);
	icon_disabled->SetBits(pic_disabled, B_MINI_ICON * B_MINI_ICON, 0, B_CMAP8);

	SetDrawingMode(B_OP_ALPHA);
	SetFlags(Flags() | B_WILL_DRAW);
}

VMWAddOnsTray::~VMWAddOnsTray()
{
	delete icon_all;
	delete icon_mouse;
	delete icon_clipboard;
	delete icon_none;
	delete icon_disabled;

	delete system_clipboard;
	delete clipboard_poller;
	delete clock_sync;
}

void
VMWAddOnsTray::GetPreferredSize(float *w, float *h)
{
	if (w == NULL)
		return;
	*w = B_MINI_ICON;
	*h = B_MINI_ICON;
}

status_t
VMWAddOnsTray::Archive(BMessage *data, bool deep = true) const
{
	data->AddString("add_on", APP_SIG);

	return BView::Archive(data, deep);
}

VMWAddOnsTray*
VMWAddOnsTray::Instantiate(BMessage *data)
{
	return (new VMWAddOnsTray(data));
}

void
VMWAddOnsTray::Draw(BRect /*update_rect*/) {
	BRect tray_bounds(Bounds());

	if (Parent()) SetHighColor(Parent()->ViewColor());
	else SetHighColor(189, 186, 189, 255);
	FillRect(tray_bounds);

	if (!backdoor.InVMware()) {
		DrawBitmap(icon_disabled);
		return;
	}

	if (settings.GetBool("mouse_enabled", true)) {
		if (settings.GetBool("clip_enabled", true))
			DrawBitmap(icon_all);
		else
			DrawBitmap(icon_mouse);
	} else {
		if (settings.GetBool("clip_enabled", true))
			DrawBitmap(icon_clipboard);
		else
			DrawBitmap(icon_none);
	}
}

void
VMWAddOnsTray::MouseDown(BPoint where)
{
	ConvertToScreen(&where);
	VMWAddOnsMenu* menu = new VMWAddOnsMenu(this, backdoor.InVMware());
	menu->Go(where, true, true, ConvertToScreen(Bounds()), true);
}

void
VMWAddOnsTray::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case MOUSE_SHARING:
		{
			bool sharing_enabled = settings.GetBool("mouse_enabled", true);
			settings.SetBool("mouse_enabled", !sharing_enabled);
			this->Invalidate();
		}
		break;

		case CLIPBOARD_SHARING:
		{
			bool sharing_enabled = settings.GetBool("clip_enabled", true);
			settings.SetBool("clip_enabled", !sharing_enabled);
			SetClipboardSharing(!sharing_enabled);
			this->Invalidate();
		}
		break;

		case REMOVE_FROM_DESKBAR:
			RemoveMyself(true);
		break;

		case B_ABOUT_REQUESTED:
		{
			BAlert* alert = new BAlert("about",
				APP_NAME ", version " APP_VERSION "\n"
				"© 2009, Vincent Duvert\n"
				"Distributed under the terms of the MIT License.", "OK", NULL, NULL,
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_INFO_ALERT);
			alert->SetShortcut(0, B_ENTER);
			alert->Go(NULL);
		}
		break;

		case B_CLIPBOARD_CHANGED:
		{
			char* data;
			ssize_t len;
			BMessage* clip_message = NULL;

			if (!system_clipboard->Lock())
				return;

			clip_message = system_clipboard->Data();
			if (clip_message == NULL) {
				system_clipboard->Unlock();
				return;
			}

			clip_message->FindData("text/plain", B_MIME_TYPE, (const void**)&data, &len);
			if(data == NULL) {
				system_clipboard->Unlock();
				return;
			}

			// Clear the host clipboard
			backdoor.SetHostClipboard(data, len);

			system_clipboard->Unlock();
		}
		break;

		case CLIPBOARD_POLL:
		{
			char* data;
			size_t len;

			if (backdoor.GetHostClipboard(&data, &len) != B_OK)
				return;

			BMessage* clip_message = NULL;
			if (!system_clipboard->Lock()) {
				free(data);
				return;
			}
			system_clipboard->Clear();

			if (len > 0) {
				clip_message = system_clipboard->Data();
				if (clip_message == NULL) {
					free(data);
					system_clipboard->Unlock();
					return;
				}

				clip_message->AddData("text/plain", B_MIME_TYPE, data, len);
				system_clipboard->Commit();
				system_clipboard->Unlock();
			}
		}

		break;

		case CLOCK_POLL:
		{
			int32 clock_value = backdoor.GetHostClock();
			if (clock_value > 0)
				set_real_time_clock(clock_value);
		}
		break;

		case SHRINK_DISKS:
		{
			thread_id th = spawn_thread(VMWAddOnsCleanup::Start,
				"vmw cleanup", B_LOW_PRIORITY, this);
			if (th > 0) {
				cleanup_in_process = true;
				resume_thread(th);
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
	if (backdoor.InVMware())
		SetClipboardSharing(settings.GetBool("clip_enabled", true));
	
	clock_sync = new BMessageRunner(this, new BMessage(CLOCK_POLL), CLOCK_POLL_DELAY);
}

void
VMWAddOnsTray::SetClipboardSharing(bool enable)
{
	delete clipboard_poller;
	clipboard_poller = NULL;

	if (enable) {
		system_clipboard->StartWatching(this);
		clipboard_poller = new BMessageRunner(this, new BMessage(CLIPBOARD_POLL), CLIP_POLL_DELAY);
	} else {
		system_clipboard->StopWatching(this);
	}
}

int32
removeFromDeskbar(void *)
{
	BDeskbar db;
	db.RemoveItem(TRAY_NAME);

	return 0;
}

void
VMWAddOnsTray::StartShrink()
{
	if (backdoor.OpenRPCChannel() == B_OK) {
		backdoor.SendMessage("disk.shrink", true);
		backdoor.CloseRPCChannel();
	} else {
		(new BAlert(TRAY_NAME,
			"Unable to communicate with VMWare. Your VMWare version may be too old. Please start the process manually if your VMware version allows it.",
			"Cancel", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
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
		"Are you sure you want to quit ?\n"
		"This will stop clipboard sharing (but not mouse sharing if it is running).",
		"Cancel", "Quit", NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT))->Go();
	}

	if (result != 0) {
		thread_id th = spawn_thread(removeFromDeskbar, "goodbye cruel world", B_NORMAL_PRIORITY, NULL);
		if (th) resume_thread(th);
	}
}

VMWAddOnsMenu::VMWAddOnsMenu(VMWAddOnsTray* tray, bool in_vmware)
	:BPopUpMenu("tray_menu", false, false)
{
	BMenuItem* menu_item;

	SetFont(be_plain_font);

	if (in_vmware) {
		menu_item = new BMenuItem("Enable mouse sharing", new BMessage(MOUSE_SHARING));
		menu_item->SetMarked(settings.GetBool("mouse_enabled", true));;
		AddItem(menu_item);

		menu_item = new BMenuItem("Enable clipboard sharing", new BMessage(CLIPBOARD_SHARING));
		menu_item->SetMarked(settings.GetBool("clip_enabled", true));
		AddItem(menu_item);

		if (!tray->cleanup_in_process)
			AddItem(new BMenuItem("Shrink virtual disks " B_UTF8_ELLIPSIS, new BMessage(SHRINK_DISKS)));
	} else {
		menu_item = new BMenuItem("Not running in VMware", NULL);
		menu_item->SetEnabled(false);
		AddItem(menu_item);
	}

	AddSeparatorItem();

	AddItem(new BMenuItem("About "APP_NAME B_UTF8_ELLIPSIS, new BMessage(B_ABOUT_REQUESTED)));
	AddItem(new BMenuItem("Quit" B_UTF8_ELLIPSIS, new BMessage(REMOVE_FROM_DESKBAR)));

	SetTargetForItems(tray);
	SetAsyncAutoDestruct(true);
}

VMWAddOnsMenu::~VMWAddOnsMenu()
{
}
