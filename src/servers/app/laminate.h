#ifndef _LAMINATE_H
#define _LAMINATE_H

#include "Canvas.h"
#include "IntRect.h"

#include <AutoDeleter.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>

class View: public Canvas {
public:
							View(IntRect frame, IntPoint scrollingOffset,
								const char* name, int32 token,
								uint32 resizeMode, uint32 flags);

protected:


			BString			fName;
			int32			fToken;
			// area within parent coordinate space
			IntRect			fFrame;
			// offset of the local area (bounds)
			IntPoint		fScrollingOffset;
			rgb_color		fViewColor;
			color_which		fWhichViewColor;
			float			fWhichViewColorTint;
			BReference<ServerBitmap>
							fViewBitmap;
			IntRect			fBitmapSource;
			IntRect			fBitmapDestination;
			int32			fBitmapResizingMode;
			int32			fBitmapOptions;

			uint32			fResizeMode;
			uint32			fFlags;
			bool			fHidden : 1;
			bool			fVisible : 1;
			bool			fBackgroundDirty : 1;
			bool			fIsDesktopBackground : 1;

			uint32			fEventMask;
			uint32			fEventOptions;
			::Window*		fWindow;
};
#endif
