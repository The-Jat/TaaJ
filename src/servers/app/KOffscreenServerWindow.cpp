/*
 * Copyright 2005-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "KOffscreenWindow.h"
#include "ServerBitmap.h"

#include "KOffscreenServerWindow.h"


K_OffscreenServerWindow::K_OffscreenServerWindow(const char *title, ServerApp *app,
		port_id clientPort, port_id looperPort, int32 handlerID,
		ServerBitmap* bitmap)
	: KServerWindow(title, app, clientPort, looperPort, handlerID),
	fBitmap(bitmap, true)
{
}


K_OffscreenServerWindow::~K_OffscreenServerWindow()
{
}


void
K_OffscreenServerWindow::SendMessageToClient(const BMessage* msg, int32 target,
	bool usePreferred) const
{
	// We're a special kind of window. The client BWindow thread is not running,
	// so we cannot post messages to the client. In order to not mess arround
	// with all the other code, we simply make this function virtual and
	// don't do anything in this implementation.
}


K_Window*
K_OffscreenServerWindow::MakeWindow(BRect frame, const char* name,
	window_look look, window_feel feel, uint32 flags, uint32 workspace)
{
	return new K_OffscreenWindow(fBitmap, name, this);
}
