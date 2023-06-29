/*
 * Copyright 2010-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "KSATWindow.h"

#include <Debug.h>

#include "StackAndTilePrivate.h"

#include "Desktop.h"
#include "KSATGroup.h"
#include "ServerApp.h"
//#include "Window.h"
#include "KWindow.h"//khidki


using namespace BPrivate;


// #pragma mark -


/*K_SATWindow::K_SATWindow(K_StackAndTile* sat, Window* window)
	:
	fWindow(window),
	fStackAndTile(sat),

	fWindowArea(NULL),

	fOngoingSnapping(NULL),
	fSATStacking(this),
	fSATTiling(this)
{
	fId = _GenerateId();

	fDesktop = fWindow->Desktop();

	// read initial limit values
	fWindow->GetSizeLimits(&fOriginalMinWidth, &fOriginalMaxWidth,
		&fOriginalMinHeight, &fOriginalMaxHeight);
	BRect frame = fWindow->Frame();
	fOriginalWidth = frame.Width();
	fOriginalHeight = frame.Height();

	fSATSnappingBehaviourList.AddItem(&fSATStacking);
	fSATSnappingBehaviourList.AddItem(&fSATTiling);
}
*/
//khidki start
K_SATWindow::K_SATWindow(K_StackAndTile* sat, K_Window* window)
	:
	fWindow(window),
	fStackAndTile(sat),

	fWindowArea(NULL),

	fOngoingSnapping(NULL),
	fSATStacking(this),
	fSATTiling(this)
{
	fId = _GenerateId();

	fDesktop = fWindow->Desktop();

	// read initial limit values
	fWindow->GetSizeLimits(&fOriginalMinWidth, &fOriginalMaxWidth,
		&fOriginalMinHeight, &fOriginalMaxHeight);
	BRect frame = fWindow->Frame();
	fOriginalWidth = frame.Width();
	fOriginalHeight = frame.Height();

	fSATSnappingBehaviourList.AddItem(&fSATStacking);
	fSATSnappingBehaviourList.AddItem(&fSATTiling);
}

//end


K_SATWindow::~K_SATWindow()
{
	if (fWindowArea != NULL)
		fWindowArea->Group()->RemoveWindow(this);
}


K_SATDecorator*
K_SATWindow::GetDecorator() const
{
	return static_cast<K_SATDecorator*>(fWindow->Decorator());
}


K_SATGroup*
K_SATWindow::GetGroup()
{
	if (fWindowArea == NULL) {
		K_SATGroup* group = new (std::nothrow)K_SATGroup;
		if (group == NULL)
			return group;
		BReference<K_SATGroup> groupRef;
		groupRef.SetTo(group, true);

		/* AddWindow also will trigger the window to hold a reference on the new
		group. */
		if (group->AddWindow(this, NULL, NULL, NULL, NULL) == false)
			return NULL;
	}

	ASSERT(fWindowArea != NULL);

	 // manually set the tabs of the single window
	if (PositionManagedBySAT() == false) {
		BRect frame = CompleteWindowFrame();
		fWindowArea->LeftTopCrossing()->VerticalTab()->SetPosition(frame.left);
		fWindowArea->LeftTopCrossing()->HorizontalTab()->SetPosition(frame.top);
		fWindowArea->RightBottomCrossing()->VerticalTab()->SetPosition(
			frame.right);
		fWindowArea->RightBottomCrossing()->HorizontalTab()->SetPosition(
			frame.bottom);
	}

	return fWindowArea->Group();
}


bool
K_SATWindow::HandleMessage(K_SATWindow* sender, BPrivate::LinkReceiver& link,
	BPrivate::LinkSender& reply)
{
	int32 target;
	link.Read<int32>(&target);
	if (target == kStacking)
		return K_StackingEventHandler::HandleMessage(sender, link, reply);

	return false;
}


bool
K_SATWindow::PropagateToGroup(K_SATGroup* group)
{
	if (fWindowArea == NULL)
		return false;
	return fWindowArea->PropagateToGroup(group);
}


bool
K_SATWindow::AddedToGroup(K_SATGroup* group, K_WindowArea* area)
{
	STRACE_SAT("K_SATWindow::AddedToGroup group: %p window %s\n", group,
		fWindow->Title());
	fWindowArea = area;
	return true;
}


bool
K_SATWindow::RemovedFromGroup(K_SATGroup* group, bool stayBelowMouse)
{
	STRACE_SAT("K_SATWindow::RemovedFromGroup group: %p window %s\n", group,
		fWindow->Title());

	_RestoreOriginalSize(stayBelowMouse);
	if (group->CountItems() == 1)
		group->WindowAt(0)->_RestoreOriginalSize(false);

	return true;
}


bool
K_SATWindow::StackWindow(K_SATWindow* child)
{
	K_SATGroup* group = GetGroup();
	K_WindowArea* area = GetWindowArea();
	if (!group || !area)
		return false;

	if (group->AddWindow(child, area, this) == false)
		return false;

	DoGroupLayout();

	if (fWindow->AddWindowToStack(child->GetWindow()) == false) {
		group->RemoveWindow(child);
		DoGroupLayout();
		return false;
	}

	return true;
}


void
K_SATWindow::RemovedFromArea(K_WindowArea* area)
{
	K_SATDecorator* decorator = GetDecorator();
	if (decorator != NULL)
		fOldTabLocatiom = decorator->TabRect(fWindow->PositionInStack()).left;

	fWindow->DetachFromWindowStack(true);
	for (int i = 0; i < fSATSnappingBehaviourList.CountItems(); i++)
		fSATSnappingBehaviourList.ItemAt(i)->RemovedFromArea(area);

	fWindowArea = NULL;
}

//khidki start
/*void
K_SATWindow::K_RemovedFromArea(K_WindowArea* area)
{
	K_SATDecorator* decorator = GetDecorator();
	if (decorator != NULL)
		fOldTabLocatiom = decorator->TabRect(fWindow->PositionInStack()).left;

	fWindow->DetachFromWindowStack(true);
	for (int i = 0; i < fSATSnappingBehaviourList.CountItems(); i++)
		fSATSnappingBehaviourList.ItemAt(i)->RemovedFromArea(area);

	fWindowArea = NULL;
}
*/
//end


void
K_SATWindow::WindowLookChanged(window_look look)
{
	for (int i = 0; i < fSATSnappingBehaviourList.CountItems(); i++)
		fSATSnappingBehaviourList.ItemAt(i)->WindowLookChanged(look);
}


void
K_SATWindow::FindSnappingCandidates()
{
	fOngoingSnapping = NULL;

	if (fWindow->Feel() != B_NORMAL_WINDOW_FEEL)
		return;

	K_GroupIterator groupIterator(fStackAndTile, GetWindow()->Desktop());
	for (K_SATGroup* group = groupIterator.NextGroup(); group;
		group = groupIterator.NextGroup()) {
		if (group->CountItems() == 1
			&& group->WindowAt(0)->GetWindow()->Feel() != B_NORMAL_WINDOW_FEEL)
			continue;
		for (int i = 0; i < fSATSnappingBehaviourList.CountItems(); i++) {
			if (fSATSnappingBehaviourList.ItemAt(i)->FindSnappingCandidates(
				group)) {
				fOngoingSnapping = fSATSnappingBehaviourList.ItemAt(i);
				return;
			}
		}
	}
}


bool
K_SATWindow::JoinCandidates()
{
	if (!fOngoingSnapping)
		return false;
	bool status = fOngoingSnapping->JoinCandidates();
	fOngoingSnapping = NULL;

	return status;
}


void
K_SATWindow::DoGroupLayout()
{
	if (!PositionManagedBySAT())
		return;

	if (fWindowArea != NULL)
		fWindowArea->DoGroupLayout();
}


void
K_SATWindow::AdjustSizeLimits(BRect targetFrame)
{
	K_SATDecorator* decorator = GetDecorator();
	if (decorator == NULL)
		return;

	targetFrame.right -= 2 * (int32)decorator->BorderWidth();
	targetFrame.bottom -= 2 * (int32)decorator->BorderWidth()
		+ (int32)decorator->TabHeight() + 1;

	int32 minWidth, maxWidth;
	int32 minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);

	if (maxWidth < targetFrame.Width())
		maxWidth = targetFrame.IntegerWidth();
	if (maxHeight < targetFrame.Height())
		maxHeight = targetFrame.IntegerHeight();

	fWindow->SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);
}


void
K_SATWindow::GetSizeLimits(int32* minWidth, int32* maxWidth, int32* minHeight,
	int32* maxHeight) const
{
	*minWidth = fOriginalMinWidth;
	*minHeight = fOriginalMinHeight;
	*maxWidth = fOriginalMaxWidth;
	*maxHeight = fOriginalMaxHeight;

	K_SATDecorator* decorator = GetDecorator();
	if (decorator == NULL)
		return;

	int32 minDecorWidth = 1, maxDecorWidth = 1;
	int32 minDecorHeight = 1, maxDecorHeight = 1;
	decorator->GetSizeLimits(&minDecorWidth, &minDecorHeight,
		&maxDecorWidth, &maxDecorHeight);

	// if no size limit is set but the window is not resizeable choose the
	// current size as limit
	if (IsHResizeable() == false && fOriginalMinWidth <= minDecorWidth)
		*minWidth = (int32)fOriginalWidth;
	if (IsVResizeable() == false && fOriginalMinHeight <= minDecorHeight)
		*minHeight = (int32)fOriginalHeight;

	if (*minWidth > *maxWidth)
		*maxWidth = *minWidth;
	if (*minHeight > *maxHeight)
		*maxHeight = *minHeight;
}


void
K_SATWindow::AddDecorator(int32* minWidth, int32* maxWidth, int32* minHeight,
	int32* maxHeight)
{
	K_SATDecorator* decorator = GetDecorator();
	if (decorator == NULL)
		return;

	*minWidth += 2 * (int32)decorator->BorderWidth();
	*minHeight += 2 * (int32)decorator->BorderWidth()
		+ (int32)decorator->TabHeight() + 1;
	*maxWidth += 2 * (int32)decorator->BorderWidth();
	*maxHeight += 2 * (int32)decorator->BorderWidth()
		+ (int32)decorator->TabHeight() + 1;
}


void
K_SATWindow::AddDecorator(BRect& frame)
{
	K_SATDecorator* decorator = GetDecorator();
	if (!decorator)
		return;
	frame.left -= decorator->BorderWidth();
	frame.right += decorator->BorderWidth() + 1;
	frame.top -= decorator->BorderWidth() + decorator->TabHeight() + 1;
	frame.bottom += decorator->BorderWidth();
}


void
K_SATWindow::SetOriginalSizeLimits(int32 minWidth, int32 maxWidth,
	int32 minHeight, int32 maxHeight)
{
	fOriginalMinWidth = minWidth;
	fOriginalMaxWidth = maxWidth;
	fOriginalMinHeight = minHeight;
	fOriginalMaxHeight = maxHeight;

	if (fWindowArea != NULL)
		fWindowArea->UpdateSizeLimits();
}


void
K_SATWindow::Resized()
{
	bool hResizeable = IsHResizeable();
	bool vResizeable = IsVResizeable();
	if (hResizeable == false && vResizeable == false)
		return;

	BRect frame = fWindow->Frame();
	if (hResizeable)
		fOriginalWidth = frame.Width();
	if (vResizeable)
		fOriginalHeight = frame.Height();

	if (fWindowArea != NULL)
		fWindowArea->UpdateSizeConstaints(CompleteWindowFrame());
}


bool
K_SATWindow::IsHResizeable() const
{
	if (fWindow->Look() == B_MODAL_WINDOW_LOOK
		|| fWindow->Look() == B_BORDERED_WINDOW_LOOK
		|| fWindow->Look() == B_NO_BORDER_WINDOW_LOOK
		|| (fWindow->Flags() & B_NOT_RESIZABLE) != 0
		|| (fWindow->Flags() & B_NOT_H_RESIZABLE) != 0)
		return false;
	return true;
}


bool
K_SATWindow::IsVResizeable() const
{
	if (fWindow->Look() == B_MODAL_WINDOW_LOOK
		|| fWindow->Look() == B_BORDERED_WINDOW_LOOK
		|| fWindow->Look() == B_NO_BORDER_WINDOW_LOOK
		|| (fWindow->Flags() & B_NOT_RESIZABLE) != 0
		|| (fWindow->Flags() & B_NOT_V_RESIZABLE) != 0)
		return false;
	return true;
}


BRect
K_SATWindow::CompleteWindowFrame()
{
	BRect frame = fWindow->Frame();
	if (fDesktop
		&& fDesktop->CurrentWorkspace() != fWindow->CurrentWorkspace()) {
		k_window_anchor& anchor = fWindow->Anchor(fWindow->CurrentWorkspace());
		if (anchor.position != kInvalidWindowPosition)
			frame.OffsetTo(anchor.position);
	}

	AddDecorator(frame);
	return frame;
}


bool
K_SATWindow::PositionManagedBySAT()
{
	if (fWindowArea == NULL || fWindowArea->Group()->CountItems() == 1)
		return false;

	return true;
}


bool
K_SATWindow::HighlightTab(bool active)
{
	K_SATDecorator* decorator = GetDecorator();
	if (!decorator)
		return false;

	int32 tabIndex = fWindow->PositionInStack();
	BRegion dirty;
	uint8 highlight = active ?  K_SATDecorator::HIGHLIGHT_STACK_AND_TILE : 0;
	decorator->SetRegionHighlight(K_Decorator::REGION_TAB, highlight, &dirty,
		tabIndex);
	decorator->SetRegionHighlight(K_Decorator::REGION_CLOSE_BUTTON, highlight,
		&dirty, tabIndex);
	decorator->SetRegionHighlight(K_Decorator::REGION_ZOOM_BUTTON, highlight,
		&dirty, tabIndex);

	fWindow->TopLayerStackWindow()->ProcessDirtyRegion(dirty);
	return true;
}


bool
K_SATWindow::HighlightBorders(K_Decorator::Region region, bool active)
{
	K_SATDecorator* decorator = GetDecorator();
	if (!decorator)
		return false;

	BRegion dirty;
	uint8 highlight = active ? K_SATDecorator::HIGHLIGHT_STACK_AND_TILE : 0;
	decorator->SetRegionHighlight(region, highlight, &dirty);

	fWindow->ProcessDirtyRegion(dirty);
	return true;
}


uint64
K_SATWindow::Id()
{
	return fId;
}


bool
K_SATWindow::SetSettings(const BMessage& message)
{
	uint64 id;
	if (message.FindInt64("window_id", (int64*)&id) != B_OK)
		return false;
	fId = id;
	return true;
}


void
K_SATWindow::GetSettings(BMessage& message)
{
	message.AddInt64("window_id", fId);
}


uint64
K_SATWindow::_GenerateId()
{
	bigtime_t time = real_time_clock_usecs();
	srand(time);
	int16 randNumber = rand();
	return (time & ~0xFFFF) | randNumber;
}


void
K_SATWindow::_RestoreOriginalSize(bool stayBelowMouse)
{
	// restore size
	fWindow->SetSizeLimits(fOriginalMinWidth, fOriginalMaxWidth,
		fOriginalMinHeight, fOriginalMaxHeight);
	BRect frame = fWindow->Frame();
	float x = 0, y = 0;
	if (IsHResizeable() == false)
		x = fOriginalWidth - frame.Width();
	if (IsVResizeable() == false)
		y = fOriginalHeight - frame.Height();
	fDesktop->K_ResizeWindowBy(fWindow, x, y);

	if (!stayBelowMouse)
		return;
	// verify that the window stays below the mouse
	BPoint mousePosition;
	int32 buttons;
	fDesktop->GetLastMouseState(&mousePosition, &buttons);
	K_SATDecorator* decorator = GetDecorator();
	if (decorator == NULL)
		return;
	BRect tabRect = decorator->TitleBarRect();
	if (mousePosition.y < tabRect.bottom && mousePosition.y > tabRect.top
		&& mousePosition.x <= frame.right + decorator->BorderWidth() +1
		&& mousePosition.x >= frame.left + decorator->BorderWidth()) {
		// verify mouse stays on the tab
		float oldOffset = mousePosition.x - fOldTabLocatiom;
		float deltaX = mousePosition.x - (tabRect.left + oldOffset);
		fDesktop->K_MoveWindowBy(fWindow, deltaX, 0);
	} else {
		// verify mouse stays on the border
		float deltaX = 0;
		float deltaY = 0;
		BRect newFrame = fWindow->Frame();
		if (x != 0 && mousePosition.x > frame.left
			&& mousePosition.x > newFrame.right) {
			deltaX = mousePosition.x - newFrame.right;
			if (mousePosition.x > frame.right)
				deltaX -= mousePosition.x - frame.right;
		}
		if (y != 0 && mousePosition.y > frame.top
			&& mousePosition.y > newFrame.bottom) {
			deltaY = mousePosition.y - newFrame.bottom;
			if (mousePosition.y > frame.bottom)
				deltaY -= mousePosition.y - frame.bottom;
		}
			fDesktop->K_MoveWindowBy(fWindow, deltaX, deltaY);
	}
}
