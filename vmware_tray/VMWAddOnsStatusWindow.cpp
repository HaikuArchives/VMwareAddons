/*
 * Copyright 2009 Vincent Duvert, vincent.duvert@free.fr
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "VMWAddOnsStatusWindow.h"

#include <Screen.h>
#include <String.h>


#define _H(x) static_cast<int>((x)->Frame().Height())
#define _W(x) static_cast<int>((x)->Frame().Width())

#define MB (1024 * 1024)
#define SPACING 8
#define STOP_OPERATION 'stOp'


VMWAddOnsStatusWindow::VMWAddOnsStatusWindow()
	:
	BWindow(BRect(0, 0, 300, 400), "Progress", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	fStatusView = new BView(Bounds(), "cleanup view", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	fStatusView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fProgressBar = new BStatusBar(BRect(SPACING, SPACING, 200, 45), NULL,
		"Starting cleanup process, please wait...");
	fProgressBar->ResizeToPreferred();

	fStatusView->AddChild(fProgressBar);

	fStopButton = new BButton(BRect(0, 0, 0, 0), NULL, "Stop", new BMessage(STOP_OPERATION));
	fStopButton->ResizeToPreferred();

	fStopButton->MoveTo(2 * SPACING + _W(fProgressBar),
		fProgressBar->Frame().bottom - _H(fStopButton));

	fStatusView->AddChild(fStopButton);

	fStatusView->ResizeTo(3 * SPACING + _W(fProgressBar) + _W(fStopButton),
		2 * SPACING + _H(fProgressBar));

	ResizeTo(_W(fStatusView), _H(fStatusView));
	AddChild(fStatusView);
	fCancelled = false;

	BScreen screen(this);
	if (screen.IsValid()) {
		MoveTo((screen.Frame().Width() - Frame().Width()) / 2,
			(screen.Frame().Height() - Frame().Height()) / 2);
	}
}


void
VMWAddOnsStatusWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case STOP_OPERATION:
			fCancelled = true;
			Hide();
			break;

		case RESET_PROGRESS:
		{
			const char* name;
			int64 sizeRead;

			message->FindString("volume_name", &name);
			message->FindInt64("max_size", &sizeRead);

			off_t size = sizeRead;

			fProgressBar->Reset((BString("Cleaning up “") << name << "”" B_UTF8_ELLIPSIS).String());
			fProgressBar->SetMaxValue(size);
		}

		case UPDATE_PROGRESS:
			fProgressBar->Update(MB);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}
