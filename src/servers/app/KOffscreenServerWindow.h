/*
 * Copyright 2005-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef K__OFFSCREEN_SERVER_WINDOW_H
#define K__OFFSCREEN_SERVER_WINDOW_H


#include "KServerWindow.h"


class K_OffscreenServerWindow : public KServerWindow {
public:
						K_OffscreenServerWindow(const char *title, ServerApp *app,
							port_id clientPort, port_id looperPort,
							int32 handlerID, ServerBitmap* bitmap);
	virtual				~K_OffscreenServerWindow();

			// util methods.
	virtual	void		SendMessageToClient(const BMessage* msg,
							int32 target = B_NULL_TOKEN,
							bool usePreferred = false) const;

	virtual	::K_Window*	MakeWindow(BRect frame, const char* name,
							window_look look, window_feel feel, uint32 flags,
							uint32 workspace);

private:
	BReference<ServerBitmap> fBitmap;
};

#endif	// OFFSCREEN_SERVER_WINDOW_H
