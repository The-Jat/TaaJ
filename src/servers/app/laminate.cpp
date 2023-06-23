#define "laminate.h"

View::View(IntRect frame, IntPoint scrollingOffset, const char* name,
		int32 token, uint32 resizeMode, uint32 flags)
	:
	fName(name),
	fToken(token),

	fFrame(frame),
	fScrollingOffset(scrollingOffset),

	fViewColor((rgb_color){ 255, 255, 255, 255 }),
	fWhichViewColor(B_NO_COLOR),
	fWhichViewColorTint(B_NO_TINT),
	fViewBitmap(NULL),
	fBitmapResizingMode(0),
	fBitmapOptions(0),

	fResizeMode(resizeMode),
	fFlags(flags),

	// Views start visible by default
	fHidden(false),
	fVisible(true),
	fBackgroundDirty(true),
	fIsDesktopBackground(false),

	fEventMask(0),
	fEventOptions(0),

	fWindow(NULL),
	fParent(NULL),

	fFirstChild(NULL),
	fPreviousSibling(NULL),
	fNextSibling(NULL),
	fLastChild(NULL),

	fCursor(NULL),
	fPicture(NULL),

	fLocalClipping((BRect)Bounds()),
	fScreenClipping(),
	fScreenClippingValid(false),
	fUserClipping(NULL),
	fScreenAndUserClipping(NULL)
{
	if (fDrawState.IsSet())
		fDrawState->SetSubPixelPrecise(fFlags & B_SUBPIXEL_PRECISE);
}
