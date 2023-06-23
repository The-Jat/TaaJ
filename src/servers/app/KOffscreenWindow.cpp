/*
 * Copyright 2005-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		Stephan Aßmus <superstippi@gmx.de>
 */


#include "KOffscreenWindow.h"

#include <new>
#include <stdio.h>

#include <Debug.h>

#include <WindowPrivate.h>

#include "BitmapHWInterface.h"
#include "DrawingEngine.h"
#include "ServerBitmap.h"

using std::nothrow;


//khidki code
//start
//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TTRACE(x) debug_printf x
#else
#	define TTRACE(x) ;
#endif
//end


K_OffscreenWindow::K_OffscreenWindow(ServerBitmap* bitmap,
		const char* name, ::KServerWindow* window)
	: K_Window(bitmap->Bounds(), name,
			B_NO_BORDER_WINDOW_LOOK, kOffscreenWindowFeel,
			0, 0, window, new (nothrow) DrawingEngine()),
	fBitmap(bitmap),
	fHWInterface(new (nothrow) BitmapHWInterface(fBitmap))
{
debug_printf("[K_OffscreenWindow] constructor\n");

	if (!fHWInterface.IsSet() || !GetDrawingEngine())
		return;

	fHWInterface->Initialize();
	GetDrawingEngine()->SetHWInterface(fHWInterface.Get());

	fVisibleRegion.Set(fFrame);
	fVisibleContentRegion.Set(fFrame);
	fVisibleContentRegionValid = true;
	fContentRegion.Set(fFrame);
	fContentRegionValid = true;

debug_printf("[K_OffscreenWindow] constructor end\n");
}


K_OffscreenWindow::~K_OffscreenWindow()
{
	if (GetDrawingEngine())
		GetDrawingEngine()->SetHWInterface(NULL);

	if (fHWInterface.IsSet()) {
		fHWInterface->LockExclusiveAccess();
		fHWInterface->Shutdown();
		fHWInterface->UnlockExclusiveAccess();
	}
}

