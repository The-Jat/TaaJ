/*
 * Copyright 2005-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef K__OFFSCREEN_WINDOW_H
#define K__OFFSCREEN_WINDOW_H


#include "KWindow.h"

#include <AutoDeleter.h>


class BitmapHWInterface;
class ServerBitmap;

class K_OffscreenWindow : public K_Window {
public:
							K_OffscreenWindow(ServerBitmap* bitmap,
								const char* name, ::KServerWindow* window);
	virtual					~K_OffscreenWindow();

	virtual	bool			IsOffscreenWindow() const
								{ return true; }

private:
	ServerBitmap*			fBitmap;
	ObjectDeleter<BitmapHWInterface>
							fHWInterface;
};

#endif	// OFFSCREEN_WINDOW_H
