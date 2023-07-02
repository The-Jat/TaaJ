/*
 * Copyright 2001-2011, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Brecht Machiels <brecht@mos6581.org>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>


 */


#include "KWindow.h"

#include <new>
#include <stdio.h>

#include <Debug.h>

#include <DirectWindow.h>
#include <PortLink.h>
#include <View.h>
#include <ViewPrivate.h>
#include <WindowPrivate.h>

#include "ClickTarget.h"
#include "KDecorator.h"
#include "KDecorManager.h"
#include "Desktop.h"
#include "DrawingEngine.h"
#include "HWInterface.h"
#include "MessagePrivate.h"
#include "PortLink.h"
#include "ServerApp.h"
#include "KServerWindow.h"
#include "KWindowBehaviour.h"
#include "Workspace.h"
#include "KWorkspacesView.h"


// Toggle debug output
//#define DEBUG_WINDOW

#ifdef DEBUG_WINDOW
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif
//khidki code
//start
//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TTRACE(x) debug_printf x
#else
#	define TTRACE(x) ;
#endif

#define TESTING_K_WINDOW
//end

// IMPORTANT: nested LockSingleWindow()s are not supported (by MultiLocker)

using std::nothrow;

// if the background clearing is delayed until
// the client draws the view, we have less flickering
// when contents have to be redrawn because of resizing
// a window or because the client invalidates parts.
// when redrawing something that has been exposed from underneath
// other windows, the other window will be seen longer at
// its previous position though if the exposed parts are not
// cleared right away. maybe there ought to be a flag in
// the update session, which tells us the cause of the update


//static rgb_color sPendingColor = (rgb_color){ 255, 255, 0, 255 };
//static rgb_color sCurrentColor = (rgb_color){ 255, 0, 255, 255 };


K_Window::K_Window(const BRect& frame, const char *name,
		window_look look, window_feel feel, uint32 flags, uint32 workspaces,
		::KServerWindow* window, DrawingEngine* drawingEngine)
	:
	fTitle(name),
	fFrame(frame),
	fScreen(NULL),

	fVisibleRegion(),
	fVisibleContentRegion(),
	fDirtyRegion(),
	fDirtyCause(0),

	fContentRegion(),
	fEffectiveDrawingRegion(),

	fVisibleContentRegionValid(false),
	fContentRegionValid(false),
	fEffectiveDrawingRegionValid(false),

	fRegionPool(),

	fWindow(window),
	fDrawingEngine(drawingEngine),
	fDesktop(window->Desktop()),

	fCurrentUpdateSession(&fUpdateSessions[0]),
	fPendingUpdateSession(&fUpdateSessions[1]),
	fUpdateRequested(false),
	fInUpdate(false),
	fUpdatesEnabled(true),

	// Windows start hidden
	fHidden(true),
	// Hidden is 1 or more
	fShowLevel(1),
	fMinimized(false),
	fIsFocus(false),

	fLook(look),
	fFeel(feel),
	fWorkspaces(workspaces),
	fCurrentWorkspace(-1),

	fMinWidth(1),
	fMaxWidth(32768),
	fMinHeight(1),
	fMaxHeight(32768),

	fWorkspacesViewCount(0)
{
debug_printf("[K_Window]{K_Window constructor} into the window constructor of the servers /app...\n");
	_InitWindowStack();
debug_printf("[K_Window]{K_Window constructor} after _InitWindowStack...\n");
	// make sure our arguments are valid
	if (!IsValidLook(fLook))
		fLook = B_TITLED_WINDOW_LOOK;
	if (!IsValidFeel(fFeel))
		fFeel = B_NORMAL_WINDOW_FEEL;

	SetFlags(flags, NULL);

	if (fLook != B_NO_BORDER_WINDOW_LOOK && fCurrentStack.IsSet()) {
		debug_printf("[K_Window]{K_Window constructor} fLook != B_NO_BORDER_WINDOW_LOOK\n");
		// allocates a decorator
		::K_Decorator* decorator = Decorator();
		if (decorator != NULL) {
			decorator->GetSizeLimits(&fMinWidth, &fMinHeight, &fMaxWidth,
				&fMaxHeight);
		}
	}
	if (fFeel != kOffscreenWindowFeel){
	debug_printf("[K_Window]{K_Window constructor} fFeel != kOffScreenWindowFeel\n");
		fWindowBehaviour.SetTo(k_gDecorManager.AllocateWindowBehaviour(this));
	}

	// do we need to change our size to let the decorator fit?
	// _ResizeBy() will adapt the frame for validity before resizing
	if (feel == kDesktopWindowFeel) {
	debug_printf("[K_Window]{K_Window constructor} feel == kDesktopWindowFeel\n");
		// the desktop window spans over the whole screen
		// TODO: this functionality should be moved somewhere else
		//  (so that it is always used when the workspace is changed)
		uint16 width, height;
		uint32 colorSpace;
		float frequency;
		if (Screen() != NULL) {
			Screen()->GetMode(width, height, colorSpace, frequency);
// TODO: MOVE THIS AWAY!!! ResizeBy contains calls to virtual methods!
// Also, there is no TopView()!
			fFrame.OffsetTo(B_ORIGIN);
//			ResizeBy(width - frame.Width(), height - frame.Height(), NULL);
		}
	}

	STRACE(("K_Window %p, %s:\n", this, Name()));
	STRACE(("\tFrame: (%.1f, %.1f, %.1f, %.1f)\n", fFrame.left, fFrame.top,
		fFrame.right, fFrame.bottom));
	STRACE(("\tWindow %s\n", window ? window->Title() : "NULL"));
	
	//debug_printf("K_Window %p, %s:\n", this, Name());
	debug_printf("\tFrame: (%.1f, %.1f, %.1f, %.1f)\n", fFrame.left, fFrame.top,
		fFrame.right, fFrame.bottom);
	debug_printf("\tWindow %s\n", window ? window->Title() : "NULL");

debug_printf("[K_Window]{K_Window constructor} end\n");
}


K_Window::~K_Window()
{
	if (fTopView.IsSet()) {
		fTopView->DetachedFromWindow();
	}

	DetachFromWindowStack(false);

	k_gDecorManager.CleanupForWindow(this);
}


status_t
K_Window::InitCheck() const
{
debug_printf("[K_Window]{InitCheck} ...\n");
	if (GetDrawingEngine() == NULL
		|| (fFeel != kOffscreenWindowFeel && !fWindowBehaviour.IsSet()))
		return B_NO_MEMORY;
	// TODO: anything else?
	return B_OK;
}


void
K_Window::SetClipping(BRegion* stillAvailableOnScreen)
{
	// this function is only called from the Desktop thread

//this function receives the stillAvailableOnScreen from K_RebuilClippingForAllWindows()
//stillAvailableOnScreen has only one rectangle which is complete screen
//0,0,1023,767


//fVisibleRegion is empty initailly
	// start from full region (as if the window was fully visible)
	GetFullRegion(&fVisibleRegion);

//fVisibleRegion has two rect now,
//1=95,71,223,94
//2=95,95,505,405

	// clip to region still available on screen
	fVisibleRegion.IntersectWith(stillAvailableOnScreen);

//still fVisibleRegion has two rect now,
//1=95,71,223,94
//2=95,95,505,405

//khidki start
	debug_printf("[K_Window]{SetClipping}debug after fVisibleRegion.IntersectWith(stillAvailableOnScreen)\n");
	debug_printf("fVisibleRegion.fCount=%d\n",fVisibleRegion.FCount());
	debug_printf("fVisibleRegion.fDataSize=%d\n",fVisibleRegion.FDataSize());
	for (int32 i = 0; i < fVisibleRegion.FCount(); i++) {
	debug_printf("[K_Window]{SetClipping} inside loop i=%d\n",i);
		clipping_rect *rect = fVisibleRegion.get_DataArray(i);//fData[i];
		debug_printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
	}
//end

	fVisibleContentRegionValid = false;
	fEffectiveDrawingRegionValid = false;


//stillAvailableOnScreen still have one rect which is complete screen
//0,0,1023,767
//khidki start
	debug_printf("[K_Window]{SetClipping}just before existing the SetClipping\n");
	debug_printf("stillAvailableOnScreen->fCount=%d\n",stillAvailableOnScreen->FCount());
	debug_printf("stillAvailableOnScreen->fDataSize=%d\n",stillAvailableOnScreen->FDataSize());
	for (int32 i = 0; i < stillAvailableOnScreen->FCount(); i++) {
	debug_printf("[K_Window]{SetClipping} inside loop i=%d\n",i);
		clipping_rect *rect = stillAvailableOnScreen->get_DataArray(i);//fData[i];
		debug_printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
	}
//end

//stillAvailableOnScreen is unchanged on overall operations..
}


void
K_Window::GetFullRegion(BRegion* region)
{//the region is fVisibleRegion here
debug_printf("[K_Window]{GetFullRegion}\n");
	// TODO: if someone needs to call this from
	// the outside, the clipping needs to be readlocked!

	// start from the decorator border, extend to use the frame
	GetBorderRegion(region);
	//khidki start
	debug_printf("[K_Window]{GetFullRegion}debug after GetBorderRegion\n");
	debug_printf("region->fCount=%d\n",region->FCount());
	debug_printf("region->fDataSize=%d\n",region->FDataSize());
	for (int32 i = 0; i < region->FCount(); i++) {
	debug_printf("[K_Window]{GetFullRegion} inside loop i=%d\n",i);
		clipping_rect *rect = region->get_DataArray(i);//fData[i];
		debug_printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
	}
	//end
//here region has 5 rect
//1=(95,71,223,94)
//2=(95,95,505,99[405])
//3=(95,100,99,400)
//4=(501,100,505,400)
//5=(95,401,505,405)
	
	//khidki start
	debug_printf("[K_Window]{GetFullRegion}debug before region->Include(fFrame)\n");
	/*debug_printf("fFrame.fCount=%d\n",fFrame.FCount());
	debug_printf("fFrame.fDataSize=%d\n",fFrame.FDataSize());
	for (int32 i = 0; i < region.FCount(); i++) {
	debug_printf("[K_Window]{GetFullRegion} inside loop i=%d\n",i);
		clipping_rect *rect = fFrame.get_DataArray(i);//fData[i];
		debug_printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
	}*/
	debug_printf(" BRect(l:%f , t:%f, r:%f , b:%f \n",
			 fFrame.left, fFrame.top, fFrame.right - 1, fFrame.bottom - 1);
	//end

//now fFrame = 100,100,499,399 it is an rect {this is the rect which we pass when we create app}

	region->Include(fFrame);

//now region has two rect
//1=95,71,223,94
//2=95,95,505,405

	//khidki start
	debug_printf("[K_Window]{GetFullRegion}debug after region->Include(fFrame)\n");
	debug_printf("region->fCount=%d\n",region->FCount());
	debug_printf("region->fDataSize=%d\n",region->FDataSize());
	for (int32 i = 0; i < region->FCount(); i++) {
	debug_printf("[K_Window]{GetFullRegion} inside loop i=%d\n",i);
		clipping_rect *rect = region->get_DataArray(i);//fData[i];
		debug_printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
	}
	//end

debug_printf("[K_Window]{GetFullRegion}ends\n");
}


void
K_Window::GetBorderRegion(BRegion* region)
{//the region here is fVisibleRegion
debug_printf("[K_Window]{GetBorderRegion}\n");
	// TODO: if someone needs to call this from
	// the outside, the clipping needs to be readlocked!



//khidki
debug_printf("region CountRects() = %d\n", region->CountRects());
//region->PrintToStream();

debug_printf("region->fCount=%d\n",region->FCount());
debug_printf("region->fDataSize=%d\n",region->FDataSize());
for (int32 i = 0; i < region->FCount(); i++) {
debug_printf("[K_Window]{GetBorderRegion} inside loop i=%d\n",i);
		clipping_rect *rect = region->get_DataArray(i);//fData[i];
		debug_printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
	}
//region->DebugBRegion();//khidki
//region has two rect
//1=(95,71,223,94)
//2=(95,95,505,405)

	::K_Decorator* decorator = Decorator();
	if (decorator)
		*region = decorator->GetFootprint();
	else
		region->MakeEmpty();



//after this step here the region that is fVisibleRegion has 5 rects
//1=(95,71,223,94)
//2=(95,95,505,405)
//3=(95,100,99,400)
//4=(501,100,505,400)
//5=(95,401,505,405)
debug_printf("[K_Window]{GetBorderRegion}ends\n");
}


void
K_Window::GetContentRegion(BRegion* region)
{
	// TODO: if someone needs to call this from
	// the outside, the clipping needs to be readlocked!

	if (!fContentRegionValid) {
		_UpdateContentRegion();
	}

	*region = fContentRegion;
}


BRegion&
K_Window::VisibleContentRegion()
{
	// TODO: if someone needs to call this from
	// the outside, the clipping needs to be readlocked!

	// regions expected to be locked
	if (!fVisibleContentRegionValid) {
		GetContentRegion(&fVisibleContentRegion);
		fVisibleContentRegion.IntersectWith(&fVisibleRegion);
	}
	return fVisibleContentRegion;
}


// #pragma mark -


void
K_Window::_PropagatePosition()
{
	if ((fFlags & B_SAME_POSITION_IN_ALL_WORKSPACES) == 0)
		return;

	for (int32 i = 0; i < kListCount; i++) {
		Anchor(i).position = fFrame.LeftTop();
	}
}


void
K_Window::MoveBy(int32 x, int32 y, bool moveStack)
{
	// this function is only called from the desktop thread

	if (x == 0 && y == 0)
		return;

	fFrame.OffsetBy(x, y);
	_PropagatePosition();

	// take along the dirty region which is not
	// processed yet
	fDirtyRegion.OffsetBy(x, y);

	if (fContentRegionValid)
		fContentRegion.OffsetBy(x, y);

	if (fCurrentUpdateSession->IsUsed())
		fCurrentUpdateSession->MoveBy(x, y);
	if (fPendingUpdateSession->IsUsed())
		fPendingUpdateSession->MoveBy(x, y);

	fEffectiveDrawingRegionValid = false;

	if (fTopView.IsSet()) {
		fTopView->MoveBy(x, y, NULL);
		fTopView->UpdateOverlay();
	}

	::K_Decorator* decorator = Decorator();
	if (moveStack && decorator)
		decorator->MoveBy(x, y);

	K_WindowStack* stack = GetWindowStack();
	if (moveStack && stack) {
		for (int32 i = 0; i < stack->CountWindows(); i++) {
			K_Window* window = stack->WindowList().ItemAt(i);
			if (window == this)
				continue;
			window->MoveBy(x, y, false);
		}
	}

	// the desktop will take care of dirty regions

	// dispatch a message to the client informing about the changed size
	BMessage msg(B_WINDOW_MOVED);
	msg.AddInt64("when", system_time());
	msg.AddPoint("where", fFrame.LeftTop());
	fWindow->SendMessageToClient(&msg);
}


void
K_Window::ResizeBy(int32 x, int32 y, BRegion* dirtyRegion, bool resizeStack)
{
	// this function is only called from the desktop thread

	int32 wantWidth = fFrame.IntegerWidth() + x;
	int32 wantHeight = fFrame.IntegerHeight() + y;

	// enforce size limits
	K_WindowStack* stack = GetWindowStack();
	if (resizeStack && stack) {
		for (int32 i = 0; i < stack->CountWindows(); i++) {
			K_Window* window = stack->WindowList().ItemAt(i);

			if (wantWidth < window->fMinWidth)
				wantWidth = window->fMinWidth;
			if (wantWidth > window->fMaxWidth)
				wantWidth = window->fMaxWidth;

			if (wantHeight < window->fMinHeight)
				wantHeight = window->fMinHeight;
			if (wantHeight > window->fMaxHeight)
				wantHeight = window->fMaxHeight;
		}
	}

	x = wantWidth - fFrame.IntegerWidth();
	y = wantHeight - fFrame.IntegerHeight();

	if (x == 0 && y == 0)
		return;

	fFrame.right += x;
	fFrame.bottom += y;

	fContentRegionValid = false;
	fEffectiveDrawingRegionValid = false;

	if (fTopView.IsSet()) {
		fTopView->ResizeBy(x, y, dirtyRegion);
		fTopView->UpdateOverlay();
	}

	::K_Decorator* decorator = Decorator();
	if (decorator && resizeStack)
		decorator->ResizeBy(x, y, dirtyRegion);

	if (resizeStack && stack) {
		for (int32 i = 0; i < stack->CountWindows(); i++) {
			K_Window* window = stack->WindowList().ItemAt(i);
			if (window == this)
				continue;
			window->ResizeBy(x, y, dirtyRegion, false);
		}
	}

	// send a message to the client informing about the changed size
	BRect frame(Frame());
	BMessage msg(B_WINDOW_RESIZED);
	msg.AddInt64("when", system_time());
	msg.AddInt32("width", frame.IntegerWidth());
	msg.AddInt32("height", frame.IntegerHeight());
	fWindow->SendMessageToClient(&msg);
}


void
K_Window::SetOutlinesDelta(BPoint delta, BRegion* dirtyRegion)
{
	float wantWidth = fFrame.IntegerWidth() + delta.x;
	float wantHeight = fFrame.IntegerHeight() + delta.y;

	// enforce size limits
	K_WindowStack* stack = GetWindowStack();
	if (stack != NULL) {
		for (int32 i = 0; i < stack->CountWindows(); i++) {
			K_Window* window = stack->WindowList().ItemAt(i);

			if (wantWidth < window->fMinWidth)
				wantWidth = window->fMinWidth;
			if (wantWidth > window->fMaxWidth)
				wantWidth = window->fMaxWidth;

			if (wantHeight < window->fMinHeight)
				wantHeight = window->fMinHeight;
			if (wantHeight > window->fMaxHeight)
				wantHeight = window->fMaxHeight;
		}

		delta.x = wantWidth - fFrame.IntegerWidth();
		delta.y = wantHeight - fFrame.IntegerHeight();
	}

	::K_Decorator* decorator = Decorator();

	if (decorator != NULL)
		decorator->SetOutlinesDelta(delta, dirtyRegion);

	_UpdateContentRegion();
}


void
K_Window::ScrollViewBy(K_View* view, int32 dx, int32 dy)
{
	// this is executed in KServerWindow with the Readlock
	// held

	if (!view || view == fTopView.Get() || (dx == 0 && dy == 0))
		return;

	BRegion* dirty = fRegionPool.GetRegion();
	if (!dirty)
		return;

	view->ScrollBy(dx, dy, dirty);

//fDrawingEngine->FillRegion(*dirty, (rgb_color){ 255, 0, 255, 255 });
//snooze(20000);

	if (!IsOffscreenWindow() && IsVisible() && view->IsVisible()) {
		dirty->IntersectWith(&VisibleContentRegion());
		_TriggerContentRedraw(*dirty);
	}

	fRegionPool.Recycle(dirty);
}


//! Takes care of invalidating parts that could not be copied
void
K_Window::CopyContents(BRegion* region, int32 xOffset, int32 yOffset)
{
	// executed in KServerWindow thread with the read lock held
	if (!IsVisible())
		return;

	BRegion* newDirty = fRegionPool.GetRegion(*region);

	// clip the region to the visible contents at the
	// source and destination location (note that VisibleContentRegion()
	// is used once to make sure it is valid, then fVisibleContentRegion
	// is used directly)
	region->IntersectWith(&VisibleContentRegion());
	if (region->CountRects() > 0) {
		// Constrain to content region at destination
		region->OffsetBy(xOffset, yOffset);
		region->IntersectWith(&fVisibleContentRegion);
		if (region->CountRects() > 0) {
			// if the region still contains any rects
			// offset to source location again
			region->OffsetBy(-xOffset, -yOffset);

			BRegion* allDirtyRegions = fRegionPool.GetRegion(fDirtyRegion);
			if (allDirtyRegions != NULL) {
				if (fPendingUpdateSession->IsUsed()) {
					allDirtyRegions->Include(
						&fPendingUpdateSession->DirtyRegion());
				}
				if (fCurrentUpdateSession->IsUsed()) {
					allDirtyRegions->Include(
						&fCurrentUpdateSession->DirtyRegion());
				}
				// Get just the part of the dirty regions which is semantically
				// copied along
				allDirtyRegions->IntersectWith(region);
			}

			BRegion* copyRegion = fRegionPool.GetRegion(*region);
			if (copyRegion != NULL) {
				// never copy what's already dirty
				if (allDirtyRegions != NULL)
					copyRegion->Exclude(allDirtyRegions);

				if (fDrawingEngine->LockParallelAccess()) {
					fDrawingEngine->CopyRegion(copyRegion, xOffset, yOffset);
					fDrawingEngine->UnlockParallelAccess();

					// Prevent those parts from being added to the dirty region...
					newDirty->Exclude(copyRegion);

					// The parts that could be copied are not dirty (at the
					// target location!)
					copyRegion->OffsetBy(xOffset, yOffset);
					// ... and even exclude them from the pending dirty region!
					if (fPendingUpdateSession->IsUsed())
						fPendingUpdateSession->DirtyRegion().Exclude(copyRegion);
				}

				fRegionPool.Recycle(copyRegion);
			} else {
				// Fallback, should never be here.
				if (fDrawingEngine->LockParallelAccess()) {
					fDrawingEngine->CopyRegion(region, xOffset, yOffset);
					fDrawingEngine->UnlockParallelAccess();
				}
			}

			if (allDirtyRegions != NULL)
				fRegionPool.Recycle(allDirtyRegions);
		}
	}
	// what is left visible from the original region
	// at the destination after the region which could be
	// copied has been excluded, is considered dirty
	// NOTE: it may look like dirty regions are not moved
	// if no region could be copied, but that's alright,
	// since these parts will now be in newDirty anyways
	// (with the right offset)
	newDirty->OffsetBy(xOffset, yOffset);
	newDirty->IntersectWith(&fVisibleContentRegion);
	if (newDirty->CountRects() > 0)
		ProcessDirtyRegion(*newDirty);

	fRegionPool.Recycle(newDirty);
}


// #pragma mark -


void
K_Window::SetTopView(K_View* topView)
{
debug_printf("[K_Window] {SetTopView}...\n");
	if (fTopView.IsSet()) {
		fTopView->DetachedFromWindow();
	}

	fTopView.SetTo(topView);

	if (fTopView.IsSet()) {
	debug_printf("[K_Window] {SetTopView} fTopView.IsSet()...\n");
		// the top view is special, it has a coordinate system
		// as if it was attached directly to the desktop, therefor,
		// the coordinate conversion through the view tree works
		// as expected, since the top view has no "parent" but has
		// fFrame as if it had

		// make sure the location of the top view on screen matches ours
		fTopView->MoveBy((int32)(fFrame.left - fTopView->Frame().left),
			(int32)(fFrame.top - fTopView->Frame().top), NULL);

		// make sure the size of the top view matches ours
		fTopView->ResizeBy((int32)(fFrame.Width() - fTopView->Frame().Width()),
			(int32)(fFrame.Height() - fTopView->Frame().Height()), NULL);

		fTopView->AttachedToWindow(this);
	}

debug_printf("[K_Window] {SetTopView}end\n");
}


K_View*
K_Window::ViewAt(const BPoint& where)
{
	return fTopView->ViewAt(where);
}


k_window_anchor&
K_Window::Anchor(int32 index)
{
	return fAnchor[index];
}


K_Window*
K_Window::NextWindow(int32 index) const
{
	return fAnchor[index].next;
}


K_Window*
K_Window::PreviousWindow(int32 index) const
{
#ifdef TESTING_K_WINDOW
	debug_printf("[K_Window]{PreviousWindow}\n");
#endif
	return fAnchor[index].previous;
}


::K_Decorator*
K_Window::Decorator() const
{
debug_printf("[K_Window]{Decorator}\n");

	if (!fCurrentStack.IsSet())
		return NULL;
	return fCurrentStack->Decorator();
}


bool
K_Window::ReloadDecor()
{
	::K_Decorator* decorator = NULL;
	K_WindowBehaviour* windowBehaviour = NULL;
	K_WindowStack* stack = GetWindowStack();
	if (stack == NULL)
		return false;

	// only reload the window at the first position
	if (stack->WindowAt(0) != this)
		return true;

	if (fLook != B_NO_BORDER_WINDOW_LOOK) {
		// we need a new decorator
		decorator = k_gDecorManager.AllocateDecorator(this);
		if (decorator == NULL)
			return false;

		// add all tabs to the decorator
		for (int32 i = 1; i < stack->CountWindows(); i++) {
			K_Window* window = stack->WindowAt(i);
			BRegion dirty;
			K_DesktopSettings settings(fDesktop);
			if (decorator->AddTab(settings, window->Title(), window->Look(),
				window->Flags(), -1, &dirty) == NULL) {
				delete decorator;
				return false;
			}
		}
	} else
		return true;

	windowBehaviour = k_gDecorManager.AllocateWindowBehaviour(this);
	if (windowBehaviour == NULL) {
		delete decorator;
		return false;
	}

	stack->SetDecorator(decorator);

	fWindowBehaviour.SetTo(windowBehaviour);

	// set the correct focus and top layer tab
	for (int32 i = 0; i < stack->CountWindows(); i++) {
		K_Window* window = stack->WindowAt(i);
		if (window->IsFocus())
			decorator->SetFocus(i, true);
		if (window == stack->TopLayerWindow())
			decorator->SetTopTab(i);
	}

	return true;
}


void
K_Window::SetScreen(const ::Screen* screen)
{
	// TODO this assert fails in Desktop::ShowWindow
	//ASSERT_MULTI_WRITE_LOCKED(fDesktop->ScreenLocker());
	fScreen = screen;
}


const ::Screen*
K_Window::Screen() const
{
	// TODO this assert also fails
	//ASSERT_MULTI_READ_LOCKED(fDesktop->ScreenLocker());
	return fScreen;
}


// #pragma mark -


void
K_Window::GetEffectiveDrawingRegion(K_View* view, BRegion& region)
{
	if (!fEffectiveDrawingRegionValid) {
		fEffectiveDrawingRegion = VisibleContentRegion();
		if (fUpdateRequested && !fInUpdate) {
			// We requested an update, but the client has not started it yet,
			// so it is only allowed to draw outside the pending update sessions
			// region
			fEffectiveDrawingRegion.Exclude(
				&fPendingUpdateSession->DirtyRegion());
		} else if (fInUpdate) {
			// enforce the dirty region of the update session
			fEffectiveDrawingRegion.IntersectWith(
				&fCurrentUpdateSession->DirtyRegion());
		} else {
			// not in update, the view can draw everywhere
//printf("K_Window(%s)::GetEffectiveDrawingRegion(for %s) - outside update\n", Title(), view->Name());
		}

		fEffectiveDrawingRegionValid = true;
	}

	// TODO: this is a region that needs to be cached later in the server
	// when the current view in KServerWindow is set, and we are currently
	// in an update (fInUpdate), than we can set this region and remember
	// it for the comming drawing commands until the current view changes
	// again or fEffectiveDrawingRegionValid is suddenly false.
	region = fEffectiveDrawingRegion;
	if (!fContentRegionValid)
		_UpdateContentRegion();

	region.IntersectWith(&view->ScreenAndUserClipping(&fContentRegion));
}


bool
K_Window::DrawingRegionChanged(K_View* view) const
{
	return !fEffectiveDrawingRegionValid || !view->IsScreenClippingValid();
}


void
K_Window::ProcessDirtyRegion(BRegion& region)
{
debug_printf("[K_Window]{ProcessDirtyRegion}\n");
	// if this is executed in the desktop thread,
	// it means that the window thread currently
	// blocks to get the read lock, if it is
	// executed from the window thread, it should
	// have the read lock and the desktop thread
	// is blocking to get the write lock. IAW, this
	// is only executed in one thread.
	if (fDirtyRegion.CountRects() == 0) {
	debug_printf("[K_Window]{ProcessDirtyRegion} fDirtyRegion.CountRects()==0\n");
		// the window needs to be informed
		// when the dirty region was empty.
		// NOTE: when the window thread has processed
		// the dirty region in MessageReceived(),
		// it will make the region empty again,
		// when it is empty here, we need to send
		// the message to initiate the next update round.
		// Until the message is processed in the window
		// thread, the desktop thread can add parts to
		// the region as it likes.
		KServerWindow()->RequestRedraw();
	}

	fDirtyRegion.Include(&region);
	fDirtyCause |= K_UPDATE_EXPOSE;


//khidki start
debug_printf("[K_Window]{ProcessDirtyRegion}debug fDirtyRegion\n");
debug_printf("fDirtyRegion.fCount=%d\n",fDirtyRegion.FCount());
debug_printf("fDirtyRegion.fDataSize=%d\n",fDirtyRegion.FDataSize());
for (int32 i = 0; i < fDirtyRegion.FCount(); i++) {
	debug_printf("[K_Window]{ProcessDirtyRegion} inside loop i=%d\n",i);
	clipping_rect *rect = fDirtyRegion.get_DataArray(i);//fData[i];
	debug_printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
}
//end
//fDirtyRegion has the visible region
//it has two rects
//1=(95,71,223,94)
//2=(95,95,505,405)

}


void
K_Window::RedrawDirtyRegion()
{
	if (TopLayerStackWindow() != this) {
		fDirtyRegion.MakeEmpty();
		fDirtyCause = 0;
		return;
	}

	// executed from KServerWindow with the read lock held
	if (IsVisible()) {
		_DrawBorder();

		BRegion* dirtyContentRegion =
			fRegionPool.GetRegion(VisibleContentRegion());
		dirtyContentRegion->IntersectWith(&fDirtyRegion);

		_TriggerContentRedraw(*dirtyContentRegion);

		fRegionPool.Recycle(dirtyContentRegion);
	}

	// reset the dirty region, since
	// we're fully clean. If the desktop
	// thread wanted to mark something
	// dirty in the mean time, it was
	// blocking on the global region lock to
	// get write access, since we're holding
	// the read lock for the whole time.
	fDirtyRegion.MakeEmpty();
	fDirtyCause = 0;
}


void
K_Window::MarkDirty(BRegion& regionOnScreen)
{
debug_printf("[K_Window]{MarkDirty} TODODO\n");

	// for marking any part of the desktop dirty
	// this will get write access to the global
	// region lock, and result in ProcessDirtyRegion()
	// to be called for any windows affected
	if (fDesktop)
	{debug_printf("[K_Window]{MarkDirty} yes there is desktop TODODO\n");
		fDesktop->K_MarkDirty(regionOnScreen);
	}
}


void
K_Window::MarkContentDirty(BRegion& regionOnScreen)
{
	// for triggering AS_REDRAW
	// since this won't affect other windows, read locking
	// is sufficient. If there was no dirty region before,
	// an update message is triggered
	if (fHidden || IsOffscreenWindow())
		return;

	regionOnScreen.IntersectWith(&VisibleContentRegion());
	fDirtyCause |= K_UPDATE_REQUEST;
	_TriggerContentRedraw(regionOnScreen);
}


void
K_Window::MarkContentDirtyAsync(BRegion& regionOnScreen)
{
	// NOTE: see comments in ProcessDirtyRegion()
	if (fHidden || IsOffscreenWindow())
		return;

	regionOnScreen.IntersectWith(&VisibleContentRegion());

	if (fDirtyRegion.CountRects() == 0) {
		KServerWindow()->RequestRedraw();
	}

	fDirtyRegion.Include(&regionOnScreen);
	fDirtyCause |= K_UPDATE_REQUEST;
}


void
K_Window::InvalidateView(K_View* view, BRegion& viewRegion)
{
	if (view && IsVisible() && view->IsVisible()) {
		if (!fContentRegionValid)
			_UpdateContentRegion();

		view->LocalToScreenTransform().Apply(&viewRegion);
		viewRegion.IntersectWith(&VisibleContentRegion());
		if (viewRegion.CountRects() > 0) {
			viewRegion.IntersectWith(
				&view->ScreenAndUserClipping(&fContentRegion));

//fDrawingEngine->FillRegion(viewRegion, rgb_color{ 0, 255, 0, 255 });
//snooze(10000);
			fDirtyCause |= K_UPDATE_REQUEST;
			_TriggerContentRedraw(viewRegion);
		}
	}
}

// DisableUpdateRequests
void
K_Window::DisableUpdateRequests()
{
	fUpdatesEnabled = false;
}


// EnableUpdateRequests
void
K_Window::EnableUpdateRequests()
{
	fUpdatesEnabled = true;
	if (!fUpdateRequested && fPendingUpdateSession->IsUsed())
		_SendUpdateMessage();
}

// #pragma mark -


/*!	\brief Handles a mouse-down message for the window.

	\param message The message.
	\param where The point where the mouse click happened.
	\param lastClickTarget The target of the previous click.
	\param clickCount The number of subsequent, no longer than double-click
		interval separated clicks that have happened so far. This number doesn't
		necessarily match the value in the message. It has already been
		pre-processed in order to avoid erroneous multi-clicks (e.g. when a
		different button has been used or a different window was targeted). This
		is an in-out variable. The method can reset the value to 1, if it
		doesn't want this event handled as a multi-click. Returning a different
		click target will also make the caller reset the click count.
	\param _clickTarget Set by the method to a value identifying the clicked
		element. If not explicitly set, an invalid click target is assumed.
*/
void
K_Window::MouseDown(BMessage* message, BPoint where,
	const ClickTarget& lastClickTarget, int32& clickCount,
	ClickTarget& _clickTarget)
{
debug_printf("[K_Window]{MouseDown}\n");

	// If the previous click hit our decorator, get the hit region.
	int32 windowToken = fWindow->ServerToken();// tododo
debug_printf("[K_Window]{MouseDown} after windowToken\n");
	int32 lastHitRegion = 0;
	if (lastClickTarget.GetType() == ClickTarget::TYPE_WINDOW_DECORATOR
		&& lastClickTarget.WindowToken() == windowToken) {
		debug_printf("[K_Window]{MouseDown} into the if\n");
		lastHitRegion = lastClickTarget.WindowElement();
	}

debug_printf("[K_Window]{MouseDown} out of if\n");

	// Let the window behavior process the mouse event.
	int32 hitRegion = 0;
	bool eventEaten = fWindowBehaviour->MouseDown(message, where, lastHitRegion,
		clickCount, hitRegion);

debug_printf("[K_Window]{MouseDown} after MouseDown\n");

	if (eventEaten) {
	debug_printf("[K_Window]{MouseDown} evenEaten\n");
		// click on the decorator (or equivalent)
		_clickTarget = ClickTarget(ClickTarget::TYPE_WINDOW_DECORATOR,
			windowToken, (int32)hitRegion);
	} else {
	debug_printf("[K_Window]{MouseDown} else eventEaten\n");
		// click was inside the window contents
		int32 viewToken = B_NULL_TOKEN;
		if (K_View* view = ViewAt(where)) {
			debug_printf("[K_Window]{MouseDown} loop\n");
			if (HasModal())
				return;

			// clicking a simple K_View
			if (!IsFocus()) {
			debug_printf("[K_Window]{MouseDown} !IsFocus\n");
				bool acceptFirstClick
					= (Flags() & B_WILL_ACCEPT_FIRST_CLICK) != 0;

				// Activate or focus the window in case it doesn't accept first
				// click, depending on the mouse mode
				if (!acceptFirstClick) {
				debug_printf("[K_Window]{MouseDown} !acceptFirstClick\n");
					bool avoidFocus = (Flags() & B_AVOID_FOCUS) != 0;
					K_DesktopSettings desktopSettings(fDesktop);
					if (desktopSettings.MouseMode() == B_NORMAL_MOUSE)
					{
						fDesktop->K_ActivateWindow(this);
					debug_printf("[K_Window]{MouseDown} after K_ActivateWindow\n");
					}
					else if (!avoidFocus)
					{
						fDesktop->K_SetFocusWindow(this);
					debug_printf("[K_Window]{MouseDown} after K_SetFocusWindow\n");
					}

					// Eat the click if we don't accept first click
					// (B_AVOID_FOCUS never gets the focus, so they always accept
					// the first click)
					// TODO: the latter is unlike BeOS - if we really wanted to
					// imitate this behaviour, we would need to check if we're
					// the front window instead of the focus window
					if (!desktopSettings.AcceptFirstClick() && !avoidFocus)
						return;
				}
			}

			// fill out view token for the view under the mouse
			viewToken = view->Token();
			debug_printf("[K_Window]{MouseDown} after viewToke\n");
			view->MouseDown(message, where);
			debug_printf("[K_Window]{MouseDown} after view->MouseDown\n");
		}

		_clickTarget = ClickTarget(ClickTarget::TYPE_WINDOW_CONTENTS,
			windowToken, viewToken);
			debug_printf("[K_Window]{MouseDown} after ClickTarget\n");
	}


debug_printf("[K_Window]{MouseDown}end\n");
}


void
K_Window::MouseUp(BMessage* message, BPoint where, int32* _viewToken)
{
debug_printf("[K_Window]{MouseUp}\n");

	fWindowBehaviour->MouseUp(message, where);

	if (K_View* view = ViewAt(where)) {
		if (HasModal())
			return;

		*_viewToken = view->Token();
		view->MouseUp(message, where);
	}

debug_printf("[K_Window]{MouseUp}end\n");
}


void
K_Window::MouseMoved(BMessage *message, BPoint where, int32* _viewToken,
	bool isLatestMouseMoved, bool isFake)
{
debug_printf("[K_Window]{MouseMoved}\n");

	K_View* view = ViewAt(where);
	if (view != NULL)
		*_viewToken = view->Token();

	// ignore pointer history
	if (!isLatestMouseMoved)
		return;

	fWindowBehaviour->MouseMoved(message, where, isFake);

	// mouse cursor

	if (view != NULL) {
		view->MouseMoved(message, where);

		// TODO: there is more for real cursor support, ie. if a window is closed,
		//		new app cursor shouldn't override view cursor, ...
		KServerWindow()->App()->SetCurrentCursor(view->Cursor());
	}

debug_printf("[K_Window]{MouseMoved}end\n");
}


void
K_Window::ModifiersChanged(int32 modifiers)
{
	fWindowBehaviour->ModifiersChanged(modifiers);
}


// #pragma mark -


void
K_Window::WorkspaceActivated(int32 index, bool active)
{
debug_printf("[K_Window]{WorkspaceActivated} TODODO\n");

	BMessage activatedMsg(B_WORKSPACE_ACTIVATED);
	activatedMsg.AddInt64("when", system_time());
	activatedMsg.AddInt32("workspace", index);
	activatedMsg.AddBool("active", active);

	KServerWindow()->SendMessageToClient(&activatedMsg);
}


void
K_Window::WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces)
{
debug_printf("[K_Window]{WorkspacesChanged} TODODO\n");

	fWorkspaces = newWorkspaces;

	BMessage changedMsg(B_WORKSPACES_CHANGED);
	changedMsg.AddInt64("when", system_time());
	changedMsg.AddInt32("old", oldWorkspaces);
	changedMsg.AddInt32("new", newWorkspaces);

	KServerWindow()->SendMessageToClient(&changedMsg);
}


void
K_Window::Activated(bool active)
{
debug_printf("[K_Window] {Activated} Sending K_WINDOW_ACTIVATED msg with the help KServerWIndow to client\n");
	BMessage msg(K_WINDOW_ACTIVATED);
	msg.AddBool("active", active);
	KServerWindow()->SendMessageToClient(&msg);
}


//# pragma mark -


void
K_Window::SetTitle(const char* name, BRegion& dirty)
{
	// rebuild the clipping for the title area
	// and redraw it.

	fTitle = name;

	::K_Decorator* decorator = Decorator();
	if (decorator) {
		int32 index = PositionInStack();
		decorator->SetTitle(index, name, &dirty);
	}
}


void
K_Window::SetFocus(bool focus)
{
debug_printf("[K_Window] {SetFocus}\n");

	::K_Decorator* decorator = Decorator();

	// executed from Desktop thread
	// it holds the clipping write lock,
	// so the window thread cannot be
	// accessing fIsFocus

	BRegion* dirty = NULL;
	if (decorator)
	{
		dirty = fRegionPool.GetRegion(decorator->GetFootprint());
	//khidki start
		debug_printf("[K_Window]{SetFocus}debug dirty\n");
		debug_printf("dirty->fCount=%d\n",dirty->FCount());
		debug_printf("dirty->fDataSize=%d\n",dirty->FDataSize());
		for (int32 i = 0; i < dirty->FCount(); i++) {
			debug_printf("[K_Window]{SetFocus} inside loop i=%d\n",i);
			clipping_rect *rect = dirty->get_DataArray(i);//fData[i];
			debug_printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
		}
//end
	}
	if (dirty) {
		dirty->IntersectWith(&fVisibleRegion);
		//khidki start
		debug_printf("[K_Window]{SetFocus}debug dirty after IntersectWith\n");
		debug_printf("dirty->fCount=%d\n",dirty->FCount());
		debug_printf("dirty->fDataSize=%d\n",dirty->FDataSize());
		for (int32 i = 0; i < dirty->FCount(); i++) {
			debug_printf("[K_Window]{SetFocus} inside loop i=%d\n",i);
			clipping_rect *rect = dirty->get_DataArray(i);//fData[i];
			debug_printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
		}
//end
		fDesktop->K_MarkDirty(*dirty);
		//khidki start
		debug_printf("[K_Window]{SetFocus}debug dirty after K_MarkDirty\n");
		debug_printf("dirty->fCount=%d\n",dirty->FCount());
		debug_printf("dirty->fDataSize=%d\n",dirty->FDataSize());
		for (int32 i = 0; i < dirty->FCount(); i++) {
			debug_printf("[K_Window]{SetFocus} inside loop i=%d\n",i);
			clipping_rect *rect = dirty->get_DataArray(i);//fData[i];
			debug_printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
		}
//end


		fRegionPool.Recycle(dirty);

//khidki start
		debug_printf("[K_Window]{SetFocus}debug dirty after K_MarkDirty\n");
		debug_printf("dirty->fCount=%d\n",dirty->FCount());
		debug_printf("dirty->fDataSize=%d\n",dirty->FDataSize());
		for (int32 i = 0; i < dirty->FCount(); i++) {
			debug_printf("[K_Window]{SetFocus} inside loop i=%d\n",i);
			clipping_rect *rect = dirty->get_DataArray(i);//fData[i];
			debug_printf("data[%" B_PRId32 "] = BRect(l:%" B_PRId32 ".0, t:%" B_PRId32
			".0, r:%" B_PRId32 ".0, b:%" B_PRId32 ".0)\n",
			i, rect->left, rect->top, rect->right - 1, rect->bottom - 1);
		}
//end


	}

	fIsFocus = focus;
	if (decorator) {
		int32 index = PositionInStack();
		debug_printf("index=%d\n",index);
		decorator->SetFocus(index, focus);
	}

	Activated(focus);

debug_printf("[K_Window] {SetFocus} end\n");
}


void
K_Window::SetHidden(bool hidden)
{
debug_printf("[K_Window]{SetHidden}\n");

	// the desktop takes care of dirty regions
	if (fHidden != hidden) {
		fHidden = hidden;

		fTopView->SetHidden(hidden);

		// TODO: anything else?
	}

debug_printf("[K_Window]{SetHidden}end\n");
}


void
K_Window::SetShowLevel(int32 showLevel)
{
debug_printf("[K_Window]{SetShowLevel}\n");

	if (showLevel == fShowLevel)
		return;

	fShowLevel = showLevel;

debug_printf("[K_Window]{SetShowLevel}end\n");
}


void
K_Window::SetMinimized(bool minimized)
{
	if (minimized == fMinimized)
		return;

	fMinimized = minimized;
}


bool
K_Window::IsVisible() const
{
	if (IsOffscreenWindow())
		return true;

	if (IsHidden())
		return false;

/*
	if (fVisibleRegion.CountRects() == 0)
		return false;
*/
	return fCurrentWorkspace >= 0 && fCurrentWorkspace < kWorkingList;
}


bool
K_Window::IsDragging() const
{
	if (!fWindowBehaviour.IsSet())
		return false;
	return fWindowBehaviour->IsDragging();
}


bool
K_Window::IsResizing() const
{
	if (!fWindowBehaviour.IsSet())
		return false;
	return fWindowBehaviour->IsResizing();
}


void
K_Window::SetSizeLimits(int32 minWidth, int32 maxWidth, int32 minHeight,
	int32 maxHeight)
{
debug_printf("[K_Window]{SetSizeLimits}\n");
	if (minWidth < 0)
		minWidth = 0;

	if (minHeight < 0)
		minHeight = 0;

	fMinWidth = minWidth;
	fMaxWidth = maxWidth;
	fMinHeight = minHeight;
	fMaxHeight = maxHeight;

	// give the Decorator a say in this too
	::K_Decorator* decorator = Decorator();
	if (decorator) {
		decorator->GetSizeLimits(&fMinWidth, &fMinHeight, &fMaxWidth,
			&fMaxHeight);
	}

	_ObeySizeLimits();

debug_printf("[K_Window]{SetSizeLimits}end\n");
}


void
K_Window::GetSizeLimits(int32* minWidth, int32* maxWidth,
	int32* minHeight, int32* maxHeight) const
{
	*minWidth = fMinWidth;
	*maxWidth = fMaxWidth;
	*minHeight = fMinHeight;
	*maxHeight = fMaxHeight;
}


bool
K_Window::SetTabLocation(float location, bool isShifting, BRegion& dirty)
{
	::K_Decorator* decorator = Decorator();
	if (decorator) {
		int32 index = PositionInStack();
		return decorator->SetTabLocation(index, location, isShifting, &dirty);
	}

	return false;
}


float
K_Window::TabLocation() const
{
	::K_Decorator* decorator = Decorator();
	if (decorator) {
		int32 index = PositionInStack();
		return decorator->TabLocation(index);
	}
	return 0.0;
}


bool
K_Window::SetDecoratorSettings(const BMessage& settings, BRegion& dirty)
{
	if (settings.what == 'prVu') {
		// 'prVu' == preview a decorator!
		BString path;
		if (settings.FindString("preview", &path) == B_OK)
			return k_gDecorManager.PreviewDecorator(path, this) == B_OK;
		return false;
	}

	::K_Decorator* decorator = Decorator();
	if (decorator)
		return decorator->SetSettings(settings, &dirty);

	return false;
}


bool
K_Window::GetDecoratorSettings(BMessage* settings)
{
	if (fDesktop)
		fDesktop->K_GetDecoratorSettings(this, *settings);

	::K_Decorator* decorator = Decorator();
	if (decorator)
		return decorator->GetSettings(settings);

	return false;
}


void
K_Window::FontsChanged(BRegion* updateRegion)
{
	::K_Decorator* decorator = Decorator();
	if (decorator != NULL) {
		K_DesktopSettings settings(fDesktop);
		decorator->FontsChanged(settings, updateRegion);
	}
}


void
K_Window::ColorsChanged(BRegion* updateRegion)
{
	::K_Decorator* decorator = Decorator();
	if (decorator != NULL) {
		K_DesktopSettings settings(fDesktop);
		decorator->ColorsChanged(settings, updateRegion);
	}
}


void
K_Window::SetLook(window_look look, BRegion* updateRegion)
{
	fLook = look;

	fContentRegionValid = false;
		// mabye a resize handle was added...
	fEffectiveDrawingRegionValid = false;
		// ...and therefor the drawing region is
		// likely not valid anymore either

	if (!fCurrentStack.IsSet())
		return;

	int32 stackPosition = PositionInStack();

	::K_Decorator* decorator = Decorator();
	if (decorator == NULL && look != B_NO_BORDER_WINDOW_LOOK) {
		// we need a new decorator
		decorator = k_gDecorManager.AllocateDecorator(this);
		fCurrentStack->SetDecorator(decorator);
		if (IsFocus())
			decorator->SetFocus(stackPosition, true);
	}

	if (decorator != NULL) {
		K_DesktopSettings settings(fDesktop);
		decorator->SetLook(stackPosition, settings, look, updateRegion);

		// we might need to resize the window!
		decorator->GetSizeLimits(&fMinWidth, &fMinHeight, &fMaxWidth,
			&fMaxHeight);
		_ObeySizeLimits();
	}

	if (look == B_NO_BORDER_WINDOW_LOOK) {
		// we don't need a decorator for this window
		fCurrentStack->SetDecorator(NULL);
	}
}


void
K_Window::SetFeel(window_feel feel)
{
	// if the subset list is no longer needed, clear it
	if ((fFeel == B_MODAL_SUBSET_WINDOW_FEEL
			|| fFeel == B_FLOATING_SUBSET_WINDOW_FEEL)
		&& feel != B_MODAL_SUBSET_WINDOW_FEEL
		&& feel != B_FLOATING_SUBSET_WINDOW_FEEL)
		fSubsets.MakeEmpty();

	fFeel = feel;

	// having modal windows with B_AVOID_FRONT or B_AVOID_FOCUS doesn't
	// make that much sense, so we filter those flags out on demand
	fFlags = fOriginalFlags;
	fFlags &= ValidWindowFlags(fFeel);

	if (!IsNormal()) {
		fFlags |= B_SAME_POSITION_IN_ALL_WORKSPACES;
		_PropagatePosition();
	}
}


void
K_Window::SetFlags(uint32 flags, BRegion* updateRegion)
{
debug_printf("[K_Window] {SetFlags} begins\n");
	fOriginalFlags = flags;
	fFlags = flags & ValidWindowFlags(fFeel);
	debug_printf("[K_Window] {SetFlags} fFlags = %d\n", fFlags);
	if (!IsNormal())
	{
	debug_printf("[K_Window] {SetFlags} not IsNormal()\n");
		fFlags |= B_SAME_POSITION_IN_ALL_WORKSPACES;
	}

	if ((fFlags & B_SAME_POSITION_IN_ALL_WORKSPACES) != 0)
	{
	debug_printf("[K_Window] {SetFlags} fLags & B_SAME_POSITION_IN_ALL_WORKSPACES != 0 \n");
		_PropagatePosition();
	}

	::K_Decorator* decorator = Decorator();
	if (decorator == NULL)
		return;

	int32 stackPosition = PositionInStack();
	decorator->SetFlags(stackPosition, flags, updateRegion);

	// we might need to resize the window!
	decorator->GetSizeLimits(&fMinWidth, &fMinHeight, &fMaxWidth, &fMaxHeight);
	_ObeySizeLimits();

// TODO: not sure if we want to do this
#if 0
	if ((fOriginalFlags & kWindowScreenFlag) != (flags & kWindowScreenFlag)) {
		// TODO: disabling needs to be nestable (or we might lose the previous
		// update state)
		if ((flags & kWindowScreenFlag) != 0)
			DisableUpdateRequests();
		else
			EnableUpdateRequests();
	}
#endif
}


/*!	Returns whether or not a window is in the workspace list with the
	specified \a index.
*/
bool
K_Window::InWorkspace(int32 index) const
{
	return (fWorkspaces & (1UL << index)) != 0;
}


bool
K_Window::SupportsFront()
{
	if (fFeel == kDesktopWindowFeel
		|| fFeel == kMenuWindowFeel
		|| (fFlags & B_AVOID_FRONT) != 0)
		return false;

	return true;
}


bool
K_Window::IsModal() const
{
	return IsModalFeel(fFeel);
}


bool
K_Window::IsFloating() const
{
	return IsFloatingFeel(fFeel);
}


bool
K_Window::IsNormal() const
{
	return !IsFloatingFeel(fFeel) && !IsModalFeel(fFeel);
}


bool
K_Window::HasModal() const
{
	for (K_Window* window = NextWindow(fCurrentWorkspace); window != NULL;
			window = window->NextWindow(fCurrentWorkspace)) {
		if (window->IsHidden() || !window->IsModal())
			continue;

		if (window->HasInSubset(this))
			return true;
	}

	return false;
}


/*!	\brief Returns the windows that's in behind of the backmost position
		this window can get.
	Returns NULL is this window can be the backmost window.

	\param workspace the workspace on which this check should be made. If
		the value is -1, the window's current workspace will be used.
*/
K_Window*
K_Window::Backmost(K_Window* window, int32 workspace)
{
#ifdef TESTING_K_WINDOW
debug_printf("[K_Window]{Backmost}\n");
#endif
	if (workspace == -1)
	{
	#ifdef TESTING_K_WINDOW
		debug_printf("[K_Window]{Backmost}workspace == -1\n");//means current workspace
	#endif
		workspace = fCurrentWorkspace;
	}

	ASSERT(workspace != -1);
	if (workspace == -1)
	{
	#ifdef TESTING_K_WINDOW
		debug_printf("[K_Window]{Backmost} returning NULL\n");
	#endif
		return NULL;
	}

	// Desktop windows are always backmost
	if (fFeel == kDesktopWindowFeel)
	{
	#ifdef TESTING_K_WINDOW
		debug_printf("[K_Window]{Backmost}kDesktopWindowFeel\n");
	#endif
		return NULL;
	}

	if (window == NULL)
	{
	#ifdef TESTING_K_WINDOW
		debug_printf("[K_Window]{Backmost}window==NULL\n");
	#endif
		window = PreviousWindow(workspace);
	}

	for (; window != NULL; window = window->PreviousWindow(workspace)) {
		if (window->IsHidden() || window == this)
			continue;

		if (HasInSubset(window))
			return window;
	}

#ifdef TESTING_K_WINDOW
debug_printf("[K_Window]{Backmost} about to return NUll\n");
#endif
	return NULL;
}


/*!	\brief Returns the window that's in front of the frontmost position
		this window can get.
	Returns NULL if this window can be the frontmost window.

	\param workspace the workspace on which this check should be made. If
		the value is -1, the window's current workspace will be used.
*/
K_Window*
K_Window::Frontmost(K_Window* first, int32 workspace)
{
#ifdef TESTING_K_WINDOW
debug_printf("[K_Window]{Frontmost}\n");
debug_printf("[K_Window]{Frontmost}workspace=%d\n",workspace);
#endif
	if (workspace == -1)
		workspace = fCurrentWorkspace;

	ASSERT(workspace != -1);
	if (workspace == -1)
		return NULL;

	if (fFeel == kDesktopWindowFeel)
		return first ? first : NextWindow(workspace);

	if (first == NULL)
	{
	#ifdef TESTING_K_WINDOW
		debug_printf("[K_Window]{Frontmost}first == NULL\n");
	#endif
		first = NextWindow(workspace);
	}

	for (K_Window* window = first; window != NULL;
			window = window->NextWindow(workspace)) {
		if (window->IsHidden() || window == this)
			continue;

		if (window->HasInSubset(this))
			return window;
	}

#ifdef TESTING_K_WINDOW
debug_printf("[K_Window]{Frontmost}about to return NULL\n");
#endif
	return NULL;
}


bool
K_Window::AddToSubset(K_Window* window)
{
	return fSubsets.AddItem(window);
}


void
K_Window::RemoveFromSubset(K_Window* window)
{
	fSubsets.RemoveItem(window);
}


/*!	Returns whether or not a window is in the subset of this window.
	If a window is in the subset of this window, it means it should always
	appear behind this window.
*/
bool
K_Window::HasInSubset(const K_Window* window) const
{
	if (window == NULL || fFeel == window->Feel()
		|| fFeel == B_NORMAL_WINDOW_FEEL)
		return false;

	// Menus are a special case: they will always be on-top of every window
	// of their application
	if (fFeel == kMenuWindowFeel)
		return window->KServerWindow()->App() == KServerWindow()->App();
	if (window->Feel() == kMenuWindowFeel)
		return false;

	// we have a few special feels that have a fixed order

	const int32 kFeels[] = {kPasswordWindowFeel, kWindowScreenFeel,
		B_MODAL_ALL_WINDOW_FEEL, B_FLOATING_ALL_WINDOW_FEEL};

	for (uint32 order = 0;
			order < sizeof(kFeels) / sizeof(kFeels[0]); order++) {
		if (fFeel == kFeels[order])
			return true;
		if (window->Feel() == kFeels[order])
			return false;
	}

	if ((fFeel == B_FLOATING_APP_WINDOW_FEEL
			&& window->Feel() != B_MODAL_APP_WINDOW_FEEL)
		|| fFeel == B_MODAL_APP_WINDOW_FEEL)
		return window->KServerWindow()->App() == KServerWindow()->App();

	return fSubsets.HasItem(window);
}


/*!	\brief Collects all workspaces views in this window and puts it into \a list
*/
void
K_Window::FindWorkspacesViews(BObjectList<K_WorkspacesView>& list) const
{
	int32 count = fWorkspacesViewCount;
	fTopView->FindViews(kWorkspacesViewFlag, (BObjectList<K_View>&)list, count);
}


/*!	\brief Returns on which workspaces the window should be visible.

	A modal or floating window may be visible on a workspace if one
	of its subset windows is visible there. Floating windows also need
	to have a subset as front window to be visible.
*/
uint32
K_Window::SubsetWorkspaces() const
{
	if (fFeel == B_MODAL_ALL_WINDOW_FEEL
		|| fFeel == B_FLOATING_ALL_WINDOW_FEEL)
		return B_ALL_WORKSPACES;

	if (fFeel == B_FLOATING_APP_WINDOW_FEEL) {
		K_Window* front = fDesktop->K_FrontWindow();
		if (front != NULL && front->IsNormal()
			&& front->KServerWindow()->App() == KServerWindow()->App())
			return KServerWindow()->App()->Workspaces();

		return 0;
	}

	if (fFeel == B_MODAL_APP_WINDOW_FEEL) {
		uint32 workspaces = KServerWindow()->App()->Workspaces();
		if (workspaces == 0) {
			// The application doesn't seem to have any other windows
			// open or visible - but we'd like to see modal windows
			// anyway, at least when they are first opened.
			return 1UL << fDesktop->CurrentWorkspace();
		}
		return workspaces;
	}

	if (fFeel == B_MODAL_SUBSET_WINDOW_FEEL
		|| fFeel == B_FLOATING_SUBSET_WINDOW_FEEL) {
		uint32 workspaces = 0;
		bool hasNormalFront = false;
		for (int32 i = 0; i < fSubsets.CountItems(); i++) {
			K_Window* window = fSubsets.ItemAt(i);

			if (!window->IsHidden())
				workspaces |= window->Workspaces();
			if (window == fDesktop->K_FrontWindow() && window->IsNormal())
				hasNormalFront = true;
		}

		if (fFeel == B_FLOATING_SUBSET_WINDOW_FEEL && !hasNormalFront)
			return 0;

		return workspaces;
	}

	return 0;
}


/*!	Returns whether or not a window is in the subset workspace list with the
	specified \a index.
	See SubsetWorkspaces().
*/
bool
K_Window::InSubsetWorkspace(int32 index) const
{
	return (SubsetWorkspaces() & (1UL << index)) != 0;
}


// #pragma mark - static


/*static*/ bool
K_Window::IsValidLook(window_look look)
{
	return look == B_TITLED_WINDOW_LOOK
		|| look == B_DOCUMENT_WINDOW_LOOK
		|| look == B_MODAL_WINDOW_LOOK
		|| look == B_FLOATING_WINDOW_LOOK
		|| look == B_BORDERED_WINDOW_LOOK
		|| look == B_NO_BORDER_WINDOW_LOOK
		|| look == kDesktopWindowLook
		|| look == kLeftTitledWindowLook;
}


/*static*/ bool
K_Window::IsValidFeel(window_feel feel)
{
	return feel == B_NORMAL_WINDOW_FEEL
		|| feel == B_MODAL_SUBSET_WINDOW_FEEL
		|| feel == B_MODAL_APP_WINDOW_FEEL
		|| feel == B_MODAL_ALL_WINDOW_FEEL
		|| feel == B_FLOATING_SUBSET_WINDOW_FEEL
		|| feel == B_FLOATING_APP_WINDOW_FEEL
		|| feel == B_FLOATING_ALL_WINDOW_FEEL
		|| feel == kDesktopWindowFeel
		|| feel == kMenuWindowFeel
		|| feel == kWindowScreenFeel
		|| feel == kPasswordWindowFeel
		|| feel == kOffscreenWindowFeel;
}


/*static*/ bool
K_Window::IsModalFeel(window_feel feel)
{
	return feel == B_MODAL_SUBSET_WINDOW_FEEL
		|| feel == B_MODAL_APP_WINDOW_FEEL
		|| feel == B_MODAL_ALL_WINDOW_FEEL;
}


/*static*/ bool
K_Window::IsFloatingFeel(window_feel feel)
{
	return feel == B_FLOATING_SUBSET_WINDOW_FEEL
		|| feel == B_FLOATING_APP_WINDOW_FEEL
		|| feel == B_FLOATING_ALL_WINDOW_FEEL;
}


/*static*/ uint32
K_Window::ValidWindowFlags()
{
	return B_NOT_MOVABLE
		| B_NOT_CLOSABLE
		| B_NOT_ZOOMABLE
		| B_NOT_MINIMIZABLE
		| B_NOT_RESIZABLE
		| B_NOT_H_RESIZABLE
		| B_NOT_V_RESIZABLE
		| B_AVOID_FRONT
		| B_AVOID_FOCUS
		| B_WILL_ACCEPT_FIRST_CLICK
		| B_OUTLINE_RESIZE
		| B_NO_WORKSPACE_ACTIVATION
		| B_NOT_ANCHORED_ON_ACTIVATE
		| B_ASYNCHRONOUS_CONTROLS
		| B_QUIT_ON_WINDOW_CLOSE
		| B_SAME_POSITION_IN_ALL_WORKSPACES
		| B_AUTO_UPDATE_SIZE_LIMITS
		| B_CLOSE_ON_ESCAPE
		| B_NO_SERVER_SIDE_WINDOW_MODIFIERS
		| kWindowScreenFlag
		| kAcceptKeyboardFocusFlag;
}


/*static*/ uint32
K_Window::ValidWindowFlags(window_feel feel)
{
	uint32 flags = ValidWindowFlags();
	if (IsModalFeel(feel))
		return flags & ~(B_AVOID_FOCUS | B_AVOID_FRONT);

	return flags;
}


// #pragma mark - private


void
K_Window::_ShiftPartOfRegion(BRegion* region, BRegion* regionToShift,
	int32 xOffset, int32 yOffset)
{
	BRegion* common = fRegionPool.GetRegion(*regionToShift);
	if (!common)
		return;
	// see if there is a common part at all
	common->IntersectWith(region);
	if (common->CountRects() > 0) {
		// cut the common part from the region,
		// offset that to destination and include again
		region->Exclude(common);
		common->OffsetBy(xOffset, yOffset);
		region->Include(common);
	}
	fRegionPool.Recycle(common);
}


void
K_Window::_TriggerContentRedraw(BRegion& dirtyContentRegion)
{
	if (!IsVisible() || dirtyContentRegion.CountRects() == 0
		|| (fFlags & kWindowScreenFlag) != 0)
		return;

	// put this into the pending dirty region
	// to eventually trigger a client redraw

	_TransferToUpdateSession(&dirtyContentRegion);
}


void
K_Window::_DrawBorder()
{
	// this is executed in the window thread, but only
	// in respond to a REDRAW message having been received, the
	// clipping lock is held for reading
	::K_Decorator* decorator = Decorator();
	if (!decorator)
		return;

	// construct the region of the border that needs redrawing
	BRegion* dirtyBorderRegion = fRegionPool.GetRegion();
	if (!dirtyBorderRegion)
		return;
	GetBorderRegion(dirtyBorderRegion);
	// intersect with our visible region
	dirtyBorderRegion->IntersectWith(&fVisibleRegion);
	// intersect with the dirty region
	dirtyBorderRegion->IntersectWith(&fDirtyRegion);

	DrawingEngine* engine = decorator->GetDrawingEngine();
	if (dirtyBorderRegion->CountRects() > 0 && engine->LockParallelAccess()) {
		engine->ConstrainClippingRegion(dirtyBorderRegion);
		bool copyToFrontEnabled = engine->CopyToFrontEnabled();
		engine->SetCopyToFrontEnabled(false);

		decorator->Draw(dirtyBorderRegion->Frame());

		engine->SetCopyToFrontEnabled(copyToFrontEnabled);
		engine->CopyToFront(*dirtyBorderRegion);

// TODO: remove this once the DrawState stuff is handled
// more cleanly. The reason why this is needed is that
// when the decorator draws strings, a draw state is set
// on the Painter object, and this is were it might get
// out of sync with what the KServerWindow things is the
// current DrawState set on the Painter
fWindow->ResyncDrawState();

		engine->UnlockParallelAccess();
	}
	fRegionPool.Recycle(dirtyBorderRegion);
}


/*!	pre: the clipping is readlocked (this function is
	only called from _TriggerContentRedraw()), which
	in turn is only called from MessageReceived() with
	the clipping lock held
*/
void
K_Window::_TransferToUpdateSession(BRegion* contentDirtyRegion)
{
	if (contentDirtyRegion->CountRects() <= 0)
		return;

//fDrawingEngine->FillRegion(*contentDirtyRegion, sPendingColor);
//snooze(20000);

	// add to pending
	fPendingUpdateSession->SetUsed(true);
//	if (!fPendingUpdateSession->IsExpose())
	fPendingUpdateSession->AddCause(fDirtyCause);
	fPendingUpdateSession->Include(contentDirtyRegion);

	if (!fUpdateRequested) {
		// send this to client
		_SendUpdateMessage();
		// the pending region is now the current,
		// though the update does not start until
		// we received BEGIN_UPDATE from the client
	}
}


void
K_Window::_SendUpdateMessage()
{
debug_printf("[K_Window]{_SendUpdateMessage}\n");

	if (!fUpdatesEnabled)
		return;

	BMessage message(_UPDATE_);
	if (KServerWindow()->SendMessageToClient(&message) != B_OK) {
		// If sending the message failed, we'll just keep adding to the dirty
		// region until sending was successful.
		// TODO: we might want to automatically resend this message in this case
		return;
	}

	fUpdateRequested = true;
	fEffectiveDrawingRegionValid = false;
}


void
K_Window::BeginUpdate(BPrivate::PortLink& link)
{

debug_printf("[K_Window]{ BeginUpdate } Into the BeginUpdate...\n");

	// NOTE: since we might "shift" parts of the
	// internal dirty regions from the desktop thread
	// in response to K_Window::ResizeBy(), which
	// might move arround views, the user of this function
	// needs to hold the global clipping lock so that the internal
	// dirty regions are not messed with from the Desktop thread
	// and KServerWindow thread at the same time.

	if (!fUpdateRequested) {
		link.StartMessage(B_ERROR);
		link.Flush();
		fprintf(stderr, "K_Window::BeginUpdate() - no update requested!\n");
		return;
	}

	// make the pending update session the current update session
	// (toggle the pointers)
	UpdateSession* temp = fCurrentUpdateSession;
	fCurrentUpdateSession = fPendingUpdateSession;
	fPendingUpdateSession = temp;
	fPendingUpdateSession->SetUsed(false);
	// all drawing command from the client
	// will have the dirty region from the update
	// session enforced
	fInUpdate = true;
	fEffectiveDrawingRegionValid = false;

	// TODO: each view could be drawn individually
	// right before carrying out the first drawing
	// command from the client during an update
	// (K_View::IsBackgroundDirty() can be used
	// for this)
	if (!fContentRegionValid)
		_UpdateContentRegion();

	BRegion* dirty = fRegionPool.GetRegion(
		fCurrentUpdateSession->DirtyRegion());
	if (!dirty) {
		link.StartMessage(B_ERROR);
		link.Flush();
		return;
	}

	dirty->IntersectWith(&VisibleContentRegion());

//if (!fCurrentUpdateSession->IsExpose()) {
////sCurrentColor.red = rand() % 255;
////sCurrentColor.green = rand() % 255;
////sCurrentColor.blue = rand() % 255;
////sPendingColor.red = rand() % 255;
////sPendingColor.green = rand() % 255;
////sPendingColor.blue = rand() % 255;
//fDrawingEngine->FillRegion(*dirty, sCurrentColor);
//snooze(10000);
//}

	link.StartMessage(B_OK);
	// append the current window geometry to the
	// message, the client will need it
	link.Attach<BPoint>(fFrame.LeftTop());
	link.Attach<float>(fFrame.Width());
	link.Attach<float>(fFrame.Height());
	// find and attach all views that intersect with
	// the dirty region
	fTopView->AddTokensForViewsInRegion(link, *dirty, &fContentRegion);
	// mark the end of the token "list"
	link.Attach<int32>(B_NULL_TOKEN);
	link.Flush();

	// supress back to front buffer copies in the drawing engine
	fDrawingEngine->SetCopyToFrontEnabled(false);

	if (fDrawingEngine->LockParallelAccess()) {
		fDrawingEngine->SuspendAutoSync();

		fTopView->Draw(GetDrawingEngine(), dirty, &fContentRegion, true);

		fDrawingEngine->Sync();
		fDrawingEngine->UnlockParallelAccess();
	} // else the background was cleared already

	fRegionPool.Recycle(dirty);
}


void
K_Window::EndUpdate()
{
debug_printf("[K_Window]{EndUpdate}\n");

	// NOTE: see comment in _BeginUpdate()

	if (fInUpdate) {
		// reenable copy to front
		fDrawingEngine->SetCopyToFrontEnabled(true);

		BRegion* dirty = fRegionPool.GetRegion(
			fCurrentUpdateSession->DirtyRegion());

		if (dirty) {
			dirty->IntersectWith(&VisibleContentRegion());

			fDrawingEngine->CopyToFront(*dirty);
			fRegionPool.Recycle(dirty);
		}

		fCurrentUpdateSession->SetUsed(false);

		fInUpdate = false;
		fEffectiveDrawingRegionValid = false;
	}
	if (fPendingUpdateSession->IsUsed()) {
		// send this to client
		_SendUpdateMessage();
	} else {
		fUpdateRequested = false;
	}
}


void
K_Window::_UpdateContentRegion()
{
	fContentRegion.Set(fFrame);

	// resize handle
	::K_Decorator* decorator = Decorator();
	if (decorator)
		fContentRegion.Exclude(&decorator->GetFootprint());

	fContentRegionValid = true;
}


void
K_Window::_ObeySizeLimits()
{
debug_printf("[K_Window]{_ObeySizeLimits}\n");
	// make sure we even have valid size limits
	if (fMaxWidth < fMinWidth)
		fMaxWidth = fMinWidth;

	if (fMaxHeight < fMinHeight)
		fMaxHeight = fMinHeight;

	// Automatically resize the window to fit these new limits
	// if it does not already.

	// On R5, Windows don't automatically resize, but since
	// BWindow::ResizeTo() even honors the limits, I would guess
	// this is a bug that we don't have to adopt.
	// Note that most current apps will do unnecessary resizing
	// after having set the limits, but the overhead is neglible.

	float minWidthDiff = fMinWidth - fFrame.Width();
	float minHeightDiff = fMinHeight - fFrame.Height();
	float maxWidthDiff = fMaxWidth - fFrame.Width();
	float maxHeightDiff = fMaxHeight - fFrame.Height();

	float xDiff = 0.0;
	if (minWidthDiff > 0.0)	// we're currently smaller than minWidth
		xDiff = minWidthDiff;
	else if (maxWidthDiff < 0.0) // we're currently larger than maxWidth
		xDiff = maxWidthDiff;

	float yDiff = 0.0;
	if (minHeightDiff > 0.0) // we're currently smaller than minHeight
		yDiff = minHeightDiff;
	else if (maxHeightDiff < 0.0) // we're currently larger than maxHeight
		yDiff = maxHeightDiff;

	if (fDesktop)
		fDesktop->K_ResizeWindowBy(this, xDiff, yDiff);
	else
		ResizeBy((int32)xDiff, (int32)yDiff, NULL);

debug_printf("[K_Window]{_ObeySizeLimits}ends\n");
}


// #pragma mark - UpdateSession


K_Window::UpdateSession::UpdateSession()
	:
	fDirtyRegion(),
	fInUse(false),
	fCause(0)
{
}


K_Window::UpdateSession::~UpdateSession()
{
}


void
K_Window::UpdateSession::Include(BRegion* additionalDirty)
{
	fDirtyRegion.Include(additionalDirty);
}


void
K_Window::UpdateSession::Exclude(BRegion* dirtyInNextSession)
{
	fDirtyRegion.Exclude(dirtyInNextSession);
}


void
K_Window::UpdateSession::MoveBy(int32 x, int32 y)
{
	fDirtyRegion.OffsetBy(x, y);
}


void
K_Window::UpdateSession::SetUsed(bool used)
{
	fInUse = used;
	if (!fInUse) {
		fDirtyRegion.MakeEmpty();
		fCause = 0;
	}
}


void
K_Window::UpdateSession::AddCause(uint8 cause)
{
	fCause |= cause;
}


int32
K_Window::PositionInStack() const
{
	if (!fCurrentStack.IsSet())
		return -1;
	return fCurrentStack->WindowList().IndexOf(this);
}


bool
K_Window::DetachFromWindowStack(bool ownStackNeeded)
{
	// The lock must normally be held but is not held when closing the window.
	//ASSERT_MULTI_WRITE_LOCKED(fDesktop->WindowLocker());

	if (!fCurrentStack.IsSet())
		return false;
	if (fCurrentStack->CountWindows() == 1)
		return true;

	int32 index = PositionInStack();

	if (fCurrentStack->RemoveWindow(this) == false)
		return false;

	BRegion invalidatedRegion;
	::K_Decorator* decorator = fCurrentStack->Decorator();
	if (decorator != NULL) {
		decorator->RemoveTab(index, &invalidatedRegion);
		decorator->SetTopTab(fCurrentStack->LayerOrder().CountItems() - 1);
	}

	K_Window* remainingTop = fCurrentStack->TopLayerWindow();
	if (remainingTop != NULL) {
		if (decorator != NULL)
			decorator->SetDrawingEngine(remainingTop->GetDrawingEngine());
		// propagate focus to the decorator
		remainingTop->SetFocus(remainingTop->IsFocus());
		remainingTop->SetLook(remainingTop->Look(), NULL);
	}

	fCurrentStack = NULL;
	if (ownStackNeeded == true)
		_InitWindowStack();
	// propagate focus to the new decorator
	SetFocus(IsFocus());

	if (remainingTop != NULL) {
		invalidatedRegion.Include(&remainingTop->VisibleRegion());
		fDesktop->K_RebuildAndRedrawAfterWindowChange(remainingTop,
			invalidatedRegion);
	}
	return true;
}


bool
K_Window::AddWindowToStack(K_Window* window)
{
debug_printf("[K_Window]{AddWindowToStack}\n");

	ASSERT_MULTI_WRITE_LOCKED(fDesktop->WindowLocker());

	K_WindowStack* stack = GetWindowStack();
	if (stack == NULL)
		return false;

	BRegion dirty;
	// move window to the own position
	BRect ownFrame = Frame();
	BRect frame = window->Frame();
	float deltaToX = round(ownFrame.left - frame.left);
	float deltaToY = round(ownFrame.top - frame.top);
	frame.OffsetBy(deltaToX, deltaToY);
	float deltaByX = round(ownFrame.right - frame.right);
	float deltaByY = round(ownFrame.bottom - frame.bottom);
	dirty.Include(&window->VisibleRegion());
	window->MoveBy(deltaToX, deltaToY, false);
	window->ResizeBy(deltaByX, deltaByY, &dirty, false);

	// first collect dirt from the window to add
	::K_Decorator* otherDecorator = window->Decorator();
	if (otherDecorator != NULL)
		dirty.Include(otherDecorator->TitleBarRect());
	::K_Decorator* decorator = stack->Decorator();
	if (decorator != NULL)
		dirty.Include(decorator->TitleBarRect());

	int32 position = PositionInStack() + 1;
	if (position >= stack->CountWindows())
		position = -1;
	if (stack->AddWindow(window, position) == false)
		return false;
	window->DetachFromWindowStack(false);
	window->fCurrentStack.SetTo(stack);

	if (decorator != NULL) {
		K_DesktopSettings settings(fDesktop);
		decorator->AddTab(settings, window->Title(), window->Look(),
			window->Flags(), position, &dirty);
	}

	window->SetLook(window->Look(), &dirty);
	fDesktop->K_RebuildAndRedrawAfterWindowChange(TopLayerStackWindow(), dirty);
	window->SetFocus(window->IsFocus());

debug_printf("[K_Window]{AddWindowToStack} end\n");
	return true;
}


K_Window*
K_Window::StackedWindowAt(const BPoint& where)
{
	::K_Decorator* decorator = Decorator();
	if (decorator == NULL)
		return this;

	int tab = decorator->TabAt(where);
	// if we have a decorator we also have a stack
	K_Window* window = fCurrentStack->WindowAt(tab);
	if (window != NULL)
		return window;
	return this;
}


K_Window*
K_Window::TopLayerStackWindow()
{
	if (!fCurrentStack.IsSet())
		return this;
	return fCurrentStack->TopLayerWindow();
}


K_WindowStack*
K_Window::GetWindowStack()
{
	if (!fCurrentStack.IsSet())
		return _InitWindowStack();
	return fCurrentStack;
}


bool
K_Window::MoveToTopStackLayer()
{
	::K_Decorator* decorator = Decorator();
	if (decorator == NULL)
		return false;
	decorator->SetDrawingEngine(GetDrawingEngine());
	SetLook(Look(), NULL);
	decorator->SetTopTab(PositionInStack());
	return fCurrentStack->MoveToTopLayer(this);
}


bool
K_Window::MoveToStackPosition(int32 to, bool isMoving)
{
	if (!fCurrentStack.IsSet())
		return false;
	int32 index = PositionInStack();
	if (fCurrentStack->Move(index, to) == false)
		return false;

	BRegion dirty;
	::K_Decorator* decorator = Decorator();
	if (decorator && decorator->MoveTab(index, to, isMoving, &dirty) == false)
		return false;

	fDesktop->K_RebuildAndRedrawAfterWindowChange(this, dirty);
	return true;
}


K_WindowStack*
K_Window::_InitWindowStack()
{
debug_printf("[K_Window]{_InitWindowStack}\n");
	fCurrentStack = NULL;
	::K_Decorator* decorator = NULL;
	if (fLook != B_NO_BORDER_WINDOW_LOOK)
	{
	debug_printf("[K_Window]{_InitWindowStack} fLook != B_NO_BORDER_WINDOW_LOOK...\n");
		decorator = k_gDecorManager.AllocateDecorator(this);
	}

	K_WindowStack* stack = new(std::nothrow) K_WindowStack(decorator);
	if (stack == NULL)
	{
	debug_printf("[K_Window]{_InitWindowStack} stack == NULL...\n");
		return NULL;
	}

	if (stack->AddWindow(this) != true) {
	debug_printf("[K_Window]{_InitWindowStack} stack->AddWindow(this) != true ...\n");
		delete stack;
		return NULL;
	}
	fCurrentStack.SetTo(stack, true);

debug_printf("[K_Window]{_InitWindowStack} end\n");
	return stack;


}


K_WindowStack::K_WindowStack(::K_Decorator* decorator)
	:
	fDecorator(decorator)
{
	debug_printf("[K_WindowStack]{K_WindowStack constuctor}\n");

}


K_WindowStack::~K_WindowStack()
{
}


void
K_WindowStack::SetDecorator(::K_Decorator* decorator)
{
	fDecorator.SetTo(decorator);
}


::K_Decorator*
K_WindowStack::Decorator()
{
debug_printf("[K_WindowStack]{Decorator}\n");

	return fDecorator.Get();
}


K_Window*
K_WindowStack::TopLayerWindow() const
{
	return fWindowLayerOrder.ItemAt(fWindowLayerOrder.CountItems() - 1);
}


int32
K_WindowStack::CountWindows()
{
	return fWindowList.CountItems();
}


K_Window*
K_WindowStack::WindowAt(int32 index)
{
	return fWindowList.ItemAt(index);
}


bool
K_WindowStack::AddWindow(K_Window* window, int32 position)
{
	debug_printf("[K_WindowStack]{AddWindow} \n");
	debug_printf("[K_WindowStack]{AddWindow} position=%d\n",position);

	if (position >= 0) {
		if (fWindowList.AddItem(window, position) == false)
			return false;
	} else if (fWindowList.AddItem(window) == false)
		return false;

	if (fWindowLayerOrder.AddItem(window) == false) {
		fWindowList.RemoveItem(window);
		return false;
	}

	debug_printf("[K_WindowStack]{AddWindow}end\n");
	return true;
}


bool
K_WindowStack::RemoveWindow(K_Window* window)
{
debug_printf("[K_WindowStack]{RemoveWindow}\n");

	if (fWindowList.RemoveItem(window) == false)
		return false;

	fWindowLayerOrder.RemoveItem(window);
	return true;
}


bool
K_WindowStack::MoveToTopLayer(K_Window* window)
{
	int32 index = fWindowLayerOrder.IndexOf(window);
	return fWindowLayerOrder.MoveItem(index,
		fWindowLayerOrder.CountItems() - 1);
}


bool
K_WindowStack::Move(int32 from, int32 to)
{
	return fWindowList.MoveItem(from, to);
}
