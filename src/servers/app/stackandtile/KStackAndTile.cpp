/*
 * Copyright 2010-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "KStackAndTile.h"

#include <Debug.h>

#include "StackAndTilePrivate.h"

#include "Desktop.h"
#include "KSATWindow.h"
#include "KTiling.h"
//#include "Window.h"

#include "KWindow.h"//khidki

//khidki code
//start
//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TTRACE(x) debug_printf x
#else
#	define TTRACE(x) ;
#endif
//end



static const int32 kRightOptionKey	= 0x67;
static const int32 kTabKey			= 0x26;
static const int32 kPageUpKey		= 0x21;
static const int32 kPageDownKey		= 0x36;
static const int32 kLeftArrowKey	= 0x61;
static const int32 kUpArrowKey		= 0x57;
static const int32 kRightArrowKey	= 0x63;
static const int32 kDownArrowKey	= 0x62;

static const int32 kModifiers = B_SHIFT_KEY | B_COMMAND_KEY
	| B_CONTROL_KEY | B_OPTION_KEY | B_MENU_KEY;


using namespace std;


//	#pragma mark - K_StackAndTile


K_StackAndTile::K_StackAndTile()
	:
	fDesktop(NULL),
	fSATKeyPressed(false),
	fCurrentSATWindow(NULL)
{

}


K_StackAndTile::~K_StackAndTile()
{

}


int32
K_StackAndTile::Identifier()
{
	return BPrivate::kMagicSATIdentifier;
}


void
K_StackAndTile::ListenerRegistered(Desktop* desktop)
{
debug_printf("[K_StackAndTile]{ListenerRegistered}\n");

	fDesktop = desktop;

	K_WindowList& windows = desktop->K_AllWindows();
	for (K_Window *window = windows.FirstWindow(); window != NULL;
			window = window->NextWindow(kAllWindowList))
		K_WindowAdded(window);
}


void
K_StackAndTile::ListenerUnregistered()
{
debug_printf("[K_StackAndTile]{ListenerUnregistered}\n");

	for (K_SATWindowMap::iterator it = fSATWindowMap.begin();
		it != fSATWindowMap.end(); it++) {
		K_SATWindow* satWindow = it->second;
		delete satWindow;
	}
	fSATWindowMap.clear();
}


bool
K_StackAndTile::HandleMessage(K_Window* sender, BPrivate::LinkReceiver& link,
	BPrivate::LinkSender& reply)
{
debug_printf("[K_StackAndTile]{HandleMessage}\n");

	if (sender == NULL)
		return _HandleMessage(link, reply);

	K_SATWindow* satWindow = GetSATWindow(sender);
	if (!satWindow)
		return false;

	return satWindow->HandleMessage(satWindow, link, reply);
}


//khidki start
/*
bool
K_StackAndTile::K_HandleMessage(K_Window* sender, BPrivate::LinkReceiver& link,
	BPrivate::LinkSender& reply)
{
	if (sender == NULL)
		return _HandleMessage(link, reply);

	K_SATWindow* satWindow = K_GetSATWindow(sender);
	if (!satWindow)
		return false;

	return satWindow->HandleMessage(satWindow, link, reply);
}*/
//end

void
K_StackAndTile::K_WindowAdded(K_Window* window)
{
debug_printf("[K_StackAndTile]{K_WindowAdded}\n");

	K_SATWindow* satWindow = new (std::nothrow)K_SATWindow(this, window);
	if (!satWindow)
		return;

	ASSERT(fSATWindowMap.find(window) == fSATWindowMap.end());
	fSATWindowMap[window] = satWindow;

debug_printf("[K_StackAndTile]{K_WindowAdded}ended\n");
}


void
K_StackAndTile::K_WindowRemoved(K_Window* window)
{
debug_printf("[K_StackAndTile]{K_WindowRemoved}\n");

	STRACE_SAT("K_StackAndTile::WindowRemoved %s\n", window->Title());

	K_SATWindowMap::iterator it = fSATWindowMap.find(window);
	if (it == fSATWindowMap.end())
		return;

	K_SATWindow* satWindow = it->second;
	// delete K_SATWindow
	delete satWindow;
	fSATWindowMap.erase(it);
}


//khidki start
/*
void
K_StackAndTile::K_WindowAdded(K_Window* window)
{
	K_SATWindow* satWindow = new (std::nothrow) K_SATWindow(this, window);
	if (!satWindow)
		return;

	ASSERT(fSATWindowMap.find(window) == fSATWindowMap.end());
	k_fSATWindowMap[window] = satWindow;
}


void
K_StackAndTile::K_WindowRemoved(K_Window* window)
{
	STRACE_SAT("K_StackAndTile::WindowRemoved %s\n", window->Title());

	K_SATWindowMap::iterator it = k_fSATWindowMap.find(window);
	if (it == fSATWindowMap.end())
		return;

	K_SATWindow* satWindow = it->second;
	// delete K_SATWindow
	delete satWindow;
	k_fSATWindowMap.erase(it);
}
*/
//end


bool
K_StackAndTile::KeyPressed(uint32 what, int32 key, int32 modifiers)
{
	if (what == B_MODIFIERS_CHANGED
		|| (what == B_UNMAPPED_KEY_DOWN && key == kRightOptionKey)
		|| (what == B_UNMAPPED_KEY_UP && key == kRightOptionKey)) {
		// switch to and from stacking and snapping mode
		bool wasPressed = fSATKeyPressed;
		fSATKeyPressed = (what == B_MODIFIERS_CHANGED
				&& (modifiers & kModifiers) == B_OPTION_KEY)
			|| (what == B_UNMAPPED_KEY_DOWN && key == kRightOptionKey);
		if (wasPressed && !fSATKeyPressed)
			_StopSAT();
		if (!wasPressed && fSATKeyPressed)
			_StartSAT();
	}

	if (!SATKeyPressed() || what != B_KEY_DOWN)
		return false;

	K_SATWindow* frontWindow = GetSATWindow(fDesktop->K_FocusWindow());
	K_SATGroup* currentGroup = _GetSATGroup(frontWindow);

	switch (key) {
		case kLeftArrowKey:
		case kRightArrowKey:
		case kTabKey:
		{
			// go to previous or next window tab in current window group
			if (currentGroup == NULL)
				return false;

			int32 groupSize = currentGroup->CountItems();
			if (groupSize <= 1)
				return false;

			for (int32 i = 0; i < groupSize; i++) {
				K_SATWindow* targetWindow = currentGroup->WindowAt(i);
				if (targetWindow == frontWindow) {
					if (key == kLeftArrowKey
						|| (key == kTabKey && (modifiers & B_SHIFT_KEY) != 0)) {
						// Go to previous window tab (wrap around)
						int32 previousIndex = i > 0 ? i - 1 : groupSize - 1;
						targetWindow = currentGroup->WindowAt(previousIndex);
					} else {
						// Go to next window tab (wrap around)
						int32 nextIndex = i < groupSize - 1 ? i + 1 : 0;
						targetWindow = currentGroup->WindowAt(nextIndex);
					}

					_ActivateWindow(targetWindow);
					return true;
				}
			}
			break;
		}

		case kUpArrowKey:
		case kPageUpKey:
		{
			// go to previous window group
			K_GroupIterator groups(this, fDesktop);
			groups.SetCurrentGroup(currentGroup);
			K_SATGroup* backmostGroup = NULL;

			while (true) {
				K_SATGroup* group = groups.NextGroup();
				if (group == NULL || group == currentGroup)
					break;
				else if (group->CountItems() < 1)
					continue;

				if (currentGroup == NULL) {
					K_SATWindow* activeWindow = group->ActiveWindow();
					if (activeWindow != NULL)
						_ActivateWindow(activeWindow);
					else
						_ActivateWindow(group->WindowAt(0));

					return true;
				}
				backmostGroup = group;
			}
			if (backmostGroup != NULL && backmostGroup != currentGroup) {
				K_SATWindow* activeWindow = backmostGroup->ActiveWindow();
				if (activeWindow != NULL)
					_ActivateWindow(activeWindow);
				else
					_ActivateWindow(backmostGroup->WindowAt(0));

				return true;
			}

			break;
		}

		case kDownArrowKey:
		case kPageDownKey:
		{
			// go to next window group
			K_GroupIterator groups(this, fDesktop);
			groups.SetCurrentGroup(currentGroup);

			while (true) {
				K_SATGroup* group = groups.NextGroup();
				if (group == NULL || group == currentGroup)
					break;
				else if (group->CountItems() < 1)
					continue;

				K_SATWindow* activeWindow = group->ActiveWindow();
				if (activeWindow != NULL)
					_ActivateWindow(activeWindow);
				else
					_ActivateWindow(group->WindowAt(0));

				if (currentGroup != NULL && frontWindow != NULL) {
					K_Window* window = frontWindow->GetWindow();
					fDesktop->K_SendWindowBehind(window);
					K_WindowSentBehind(window, NULL);
				}
				return true;
			}
			break;
		}
	}

	return false;
}


void
K_StackAndTile::MouseDown(K_Window* window, BMessage* message, const BPoint& where)
{
debug_printf("[K_StackAndTile]{MouseDown}\n");
	K_SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow || !satWindow->GetDecorator())
		return;

	// fCurrentSATWindow is not zero if e.g. the secondary and the primary
	// mouse button are pressed at the same time
	if ((message->FindInt32("buttons") & B_PRIMARY_MOUSE_BUTTON) == 0 ||
		fCurrentSATWindow != NULL)
		return;

	// we are only interested in single clicks
	if (message->FindInt32("clicks") == 2)
		return;

	int32 tab;
	switch (satWindow->GetDecorator()->RegionAt(where, tab)) {
		case K_Decorator::REGION_TAB:
		case K_Decorator::REGION_LEFT_BORDER:
		case K_Decorator::REGION_RIGHT_BORDER:
		case K_Decorator::REGION_TOP_BORDER:
		case K_Decorator::REGION_BOTTOM_BORDER:
		case K_Decorator::REGION_LEFT_TOP_CORNER:
		case K_Decorator::REGION_LEFT_BOTTOM_CORNER:
		case K_Decorator::REGION_RIGHT_TOP_CORNER:
		case K_Decorator::REGION_RIGHT_BOTTOM_CORNER:
			break;

		default:
			return;
	}

	ASSERT(fCurrentSATWindow == NULL);
	fCurrentSATWindow = satWindow;

	if (!SATKeyPressed())
		return;

	_StartSAT();

debug_printf("[K_StackAndTile]{MouseDown}ends\n");
}


void
K_StackAndTile::MouseUp(K_Window* window, BMessage* message, const BPoint& where)
{
	if (fSATKeyPressed)
		_StopSAT();

	fCurrentSATWindow = NULL;
}


void
K_StackAndTile::K_WindowMoved(K_Window* window)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	if (SATKeyPressed() && fCurrentSATWindow)
		satWindow->FindSnappingCandidates();
	else
		satWindow->DoGroupLayout();
}


void
K_StackAndTile::K_WindowResized(K_Window* window)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;
	satWindow->Resized();

	if (SATKeyPressed() && fCurrentSATWindow)
		satWindow->FindSnappingCandidates();
	else
		satWindow->DoGroupLayout();
}


//khidki start
/*
void
K_StackAndTile::K_WindowMoved(K_Window* window)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	if (SATKeyPressed() && fCurrentSATWindow)
		satWindow->FindSnappingCandidates();
	else
		satWindow->DoGroupLayout();
}


void
K_StackAndTile::K_WindowResized(K_Window* window)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;
	satWindow->Resized();

	if (SATKeyPressed() && fCurrentSATWindow)
		satWindow->FindSnappingCandidates();
	else
		satWindow->DoGroupLayout();
}
*/
//end


void
K_StackAndTile::K_WindowActivated(K_Window* window)
{
debug_printf("[K_StackAndTile]{K_WindowActivated}\n");

	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	_ActivateWindow(satWindow);
}

//khidki start
/*
void
K_StackAndTile::K_WindowActivated(K_Window* window)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	_ActivateWindow(satWindow);
}
*/
//end


void
K_StackAndTile::K_WindowSentBehind(K_Window* window, K_Window* behindOf)
{
debug_printf("[K_StackAndTile]{K_WindowSentBehind}\n");

	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	K_SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;

	Desktop* desktop = satWindow->GetWindow()->Desktop();
	if (desktop == NULL)
		return;

	const K_WindowAreaList& areaList = group->GetAreaList();
	for (int32 i = 0; i < areaList.CountItems(); i++) {
		K_WindowArea* area = areaList.ItemAt(i);
		K_SATWindow* topWindow = area->TopWindow();
		if (topWindow == NULL || topWindow == satWindow)
			continue;
		desktop->K_SendWindowBehind(topWindow->GetWindow(), behindOf);
	}

debug_printf("[K_StackAndTile]{K_WindowSentBehind} ends\n");
}


//khidki start
/*
void
K_StackAndTile::K_WindowSentBehind(K_Window* window, K_Window* behindOf)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	K_SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;

	Desktop* desktop = satWindow->GetWindow()->Desktop();
	if (desktop == NULL)
		return;

	const K_WindowAreaList& areaList = group->GetAreaList();
	for (int32 i = 0; i < areaList.CountItems(); i++) {
		K_WindowArea* area = areaList.ItemAt(i);
		K_SATWindow* topWindow = area->TopWindow();
		if (topWindow == NULL || topWindow == satWindow)
			continue;
		desktop->K_SendWindowBehind(topWindow->GetWindow(), behindOf);
	}
}
*/
//end


void
K_StackAndTile::WindowWorkspacesChanged(K_Window* window, uint32 workspaces)
{
debug_printf("[K_StackAndTile]{WindowWorkspacesChanged}\n");

	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	K_SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;

	Desktop* desktop = satWindow->GetWindow()->Desktop();
	if (desktop == NULL)
		return;

	const K_WindowAreaList& areaList = group->GetAreaList();
	for (int32 i = 0; i < areaList.CountItems(); i++) {
		K_WindowArea* area = areaList.ItemAt(i);
		if (area->WindowList().HasItem(satWindow))
			continue;
		K_SATWindow* topWindow = area->TopWindow();
		desktop->K_SetWindowWorkspaces(topWindow->GetWindow(), workspaces);
	}
debug_printf("[K_StackAndTile]{WindowWorkspacesChanged}ended\n");
}


void
K_StackAndTile::K_WindowHidden(K_Window* window, bool fromMinimize)
{
debug_printf("[K_StackAndTile]{K_WindowHidden}\n");
	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	K_SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;

	if (fromMinimize == false && group->CountItems() > 1)
		group->RemoveWindow(satWindow, false);

debug_printf("[K_StackAndTile]{K_WindowHidden}ends\n");
}

//khidki start
/*
void
K_StackAndTile::K_WindowHidden(K_Window* window, bool fromMinimize)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	K_SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;

	if (fromMinimize == false && group->CountItems() > 1)
		group->RemoveWindow(satWindow, false);
}
*/
//end


void
K_StackAndTile::WindowMinimized(K_Window* window, bool minimize)
{
debug_printf("[K_StackAndTile]{WindowMinimized}\n");

	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	K_SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;

	Desktop* desktop = satWindow->GetWindow()->Desktop();
	if (desktop == NULL)
		return;

	for (int i = 0; i < group->CountItems(); i++) {
		K_SATWindow* listWindow = group->WindowAt(i);
		if (listWindow != satWindow)
			listWindow->GetWindow()->KServerWindow()->NotifyMinimize(minimize);
	}


debug_printf("[K_StackAndTile]{WindowMinimized} ends\n");
}


void
K_StackAndTile::K_WindowTabLocationChanged(K_Window* window, float location,
	bool isShifting)
{

}


void
K_StackAndTile::K_SizeLimitsChanged(K_Window* window, int32 minWidth, int32 maxWidth,
	int32 minHeight, int32 maxHeight)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	satWindow->SetOriginalSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	// trigger a relayout
	K_WindowMoved(window);
}


//khidki start
/*
void
K_StackAndTile::K_WindowTabLocationChanged(K_Window* window, float location,
	bool isShifting)
{

}

void
K_StackAndTile::K_SizeLimitsChanged(K_Window* window, int32 minWidth, int32 maxWidth,
	int32 minHeight, int32 maxHeight)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	satWindow->SetOriginalSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	// trigger a relayout
	WindowMoved(window);
}
*/
//end


void
K_StackAndTile::K_WindowLookChanged(K_Window* window, window_look look)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	satWindow->WindowLookChanged(look);
}


void
K_StackAndTile::K_WindowFeelChanged(K_Window* window, window_feel feel)
{
	// check if it is still a compatible feel
	if (feel == B_NORMAL_WINDOW_FEEL)
		return;
	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	K_SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;

	if (group->CountItems() > 1)
		group->RemoveWindow(satWindow, false);
}


//khidki start
/*
void
K_StackAndTile::K_WindowLookChanged(K_Window* window, window_look look)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;
	satWindow->WindowLookChanged(look);
}


void
K_StackAndTile::K_WindowFeelChanged(K_Window* window, window_feel feel)
{
	// check if it is still a compatible feel
	if (feel == B_NORMAL_WINDOW_FEEL)
		return;
	K_SATWindow* satWindow = GetSATWindow(window);
	if (satWindow == NULL)
		return;

	K_SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;

	if (group->CountItems() > 1)
		group->RemoveWindow(satWindow, false);
}
*/

//end


bool
K_StackAndTile::K_SetDecoratorSettings(K_Window* window, const BMessage& settings)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return false;

	return satWindow->SetSettings(settings);
}


void
K_StackAndTile::K_GetDecoratorSettings(K_Window* window, BMessage& settings)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;

	satWindow->GetSettings(settings);
}

//khidki start
/*
bool
K_StackAndTile::K_SetDecoratorSettings(K_Window* window, const BMessage& settings)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return false;

	return satWindow->SetSettings(settings);
}


void
K_StackAndTile::K_GetDecoratorSettings(K_Window* window, BMessage& settings)
{
	K_SATWindow* satWindow = GetSATWindow(window);
	if (!satWindow)
		return;

	satWindow->GetSettings(settings);
}
*/
//end


K_SATWindow*
K_StackAndTile::GetSATWindow(K_Window* window)
{
debug_printf("[K_StackAndTile]{GetSATWindow}\n");

	if (window == NULL)
		return NULL;

	K_SATWindowMap::const_iterator it = fSATWindowMap.find(
		window);
	if (it != fSATWindowMap.end())
		return it->second;

	// TODO fix race condition with WindowAdded this method is called before
	// WindowAdded and a K_SATWindow is created twice!
	return NULL;

	// If we don't know this window, memory allocation might has been failed
	// previously. Try to add the window now.
	K_SATWindow* satWindow = new (std::nothrow)K_SATWindow(this, window);
	if (satWindow)
		fSATWindowMap[window] = satWindow;

	return satWindow;
}


//khidki start
/*
K_SATWindow*
K_StackAndTile::K_GetSATWindow(K_Window* window)
{
	if (window == NULL)
		return NULL;

	K_SATWindowMap::const_iterator it = fSATWindowMap.find(
		window);
	if (it != fSATWindowMap.end())
		return it->second;

	// TODO fix race condition with WindowAdded this method is called before
	// WindowAdded and a K_SATWindow is created twice!
	return NULL;

	// If we don't know this window, memory allocation might has been failed
	// previously. Try to add the window now.
	K_SATWindow* satWindow = new (std::nothrow)K_SATWindow(this, window);
	if (satWindow)
		fSATWindowMap[window] = satWindow;

	return satWindow;
}
*/
//end


K_SATWindow*
K_StackAndTile::FindSATWindow(uint64 id)
{
	for (K_SATWindowMap::const_iterator it = fSATWindowMap.begin();
		it != fSATWindowMap.end(); it++) {
		K_SATWindow* window = it->second;
		if (window->Id() == id)
			return window;
	}

	return NULL;
}


//	#pragma mark - K_StackAndTile private methods


void
K_StackAndTile::_StartSAT()
{
	STRACE_SAT("K_StackAndTile::_StartSAT()\n");
	if (!fCurrentSATWindow)
		return;

	// Remove window from the group.
	K_SATGroup* group = fCurrentSATWindow->GetGroup();
	if (group == NULL)
		return;

	group->RemoveWindow(fCurrentSATWindow, false);
	// Bring window to the front. (in focus follow mouse this is not
	// automatically the case)
	_ActivateWindow(fCurrentSATWindow);

	fCurrentSATWindow->FindSnappingCandidates();
}


void
K_StackAndTile::_StopSAT()
{
	STRACE_SAT("K_StackAndTile::_StopSAT()\n");
	if (!fCurrentSATWindow)
		return;
	if (fCurrentSATWindow->JoinCandidates())
		_ActivateWindow(fCurrentSATWindow);
}


void
K_StackAndTile::_ActivateWindow(K_SATWindow* satWindow)
{
debug_printf("[K_StackAndTile]{_ActivateWindow}\n");

	if (satWindow == NULL)
		return;

	K_SATGroup* group = satWindow->GetGroup();
	if (group == NULL)
		return;

	Desktop* desktop = satWindow->GetWindow()->Desktop();
	if (desktop == NULL)
		return;

	K_WindowArea* area = satWindow->GetWindowArea();
	if (area == NULL)
		return;

	area->MoveToTopLayer(satWindow);

	// save the active window of the current group
	K_SATWindow* frontWindow = GetSATWindow(fDesktop->K_FocusWindow());
	K_SATGroup* currentGroup = _GetSATGroup(frontWindow);
	if (currentGroup != NULL && currentGroup != group && frontWindow != NULL)
		currentGroup->SetActiveWindow(frontWindow);
	else
		group->SetActiveWindow(satWindow);

	const K_WindowAreaList& areas = group->GetAreaList();
	int32 areasCount = areas.CountItems();
debug_printf("[K_StackAndTile]{_ActivateWindow} areasCount=%d\n",areasCount);
	for (int32 i = 0; i < areasCount; i++) {
		K_WindowArea* currentArea = areas.ItemAt(i);
		if (currentArea == area)
			continue;

		debug_printf("[K_StackAndTile]{_ActivateWindow} i=%d\n",i);
		desktop->K_ActivateWindow(currentArea->TopWindow()->GetWindow());
	}

	debug_printf("[K_StackAndTile]{_ActivateWindow} out of loop\n");
	desktop->K_ActivateWindow(satWindow->GetWindow());
}


bool
K_StackAndTile::_HandleMessage(BPrivate::LinkReceiver& link,
	BPrivate::LinkSender& reply)
{
debug_printf("[K_StackAndTile]{_HandleMessage}\n");

	int32 what;
	link.Read<int32>(&what);

	switch (what) {
		case BPrivate::kSaveAllGroups:
		{
			BMessage allGroupsArchive;
			K_GroupIterator groups(this, fDesktop);
			while (true) {
				K_SATGroup* group = groups.NextGroup();
				if (group == NULL)
					break;
				if (group->CountItems() <= 1)
					continue;
				BMessage groupArchive;
				if (group->ArchiveGroup(groupArchive) != B_OK)
					continue;
				allGroupsArchive.AddMessage("group", &groupArchive);
			}
			int32 size = allGroupsArchive.FlattenedSize();
			char buffer[size];
			if (allGroupsArchive.Flatten(buffer, size) == B_OK) {
				reply.StartMessage(B_OK);
				reply.Attach<int32>(size);
				reply.Attach(buffer, size);
			} else
				reply.StartMessage(B_ERROR);
			reply.Flush();
			break;
		}

		case BPrivate::kRestoreGroup:
		{
			int32 size;
			if (link.Read<int32>(&size) == B_OK) {
				char buffer[size];
				BMessage group;
				if (link.Read(buffer, size) == B_OK
					&& group.Unflatten(buffer) == B_OK) {
					status_t status = K_SATGroup::RestoreGroup(group, this);
					reply.StartMessage(status);
					reply.Flush();
				}
			}
			break;
		}

		default:
			return false;
	}

	return true;
}

//khidki start
/*bool
K_StackAndTile::K__HandleMessage(BPrivate::LinkReceiver& link,
	BPrivate::LinkSender& reply)
{
	int32 what;
	link.Read<int32>(&what);

	switch (what) {
		case BPrivate::kSaveAllGroups:
		{
			BMessage allGroupsArchive;
			K_GroupIterator groups(this, fDesktop);
			while (true) {
				K_SATGroup* group = groups.NextGroup();
				if (group == NULL)
					break;
				if (group->CountItems() <= 1)
					continue;
				BMessage groupArchive;
				if (group->ArchiveGroup(groupArchive) != B_OK)
					continue;
				allGroupsArchive.AddMessage("group", &groupArchive);
			}
			int32 size = allGroupsArchive.FlattenedSize();
			char buffer[size];
			if (allGroupsArchive.Flatten(buffer, size) == B_OK) {
				reply.StartMessage(B_OK);
				reply.Attach<int32>(size);
				reply.Attach(buffer, size);
			} else
				reply.StartMessage(B_ERROR);
			reply.Flush();
			break;
		}

		case BPrivate::kRestoreGroup:
		{
			int32 size;
			if (link.Read<int32>(&size) == B_OK) {
				char buffer[size];
				BMessage group;
				if (link.Read(buffer, size) == B_OK
					&& group.Unflatten(buffer) == B_OK) {
					status_t status = K_SATGroup::RestoreGroup(group, this);
					reply.StartMessage(status);
					reply.Flush();
				}
			}
			break;
		}

		default:
			return false;
	}

	return true;
}
*/
//end


K_SATGroup*
K_StackAndTile::_GetSATGroup(K_SATWindow* window)
{
	if (window == NULL)
		return NULL;

	K_SATGroup* group = window->GetGroup();
	if (group == NULL)
		return NULL;

	if (group->CountItems() < 1)
		return NULL;

	return group;
}


//	#pragma mark - K_GroupIterator


K_GroupIterator::K_GroupIterator(K_StackAndTile* sat, Desktop* desktop)
	:
	fStackAndTile(sat),
	fDesktop(desktop),
	fCurrentGroup(NULL)
{
	RewindToFront();
}


void
K_GroupIterator::RewindToFront()
{
	fCurrentWindow = fDesktop->K_CurrentWindows().LastWindow();
}


K_SATGroup*
K_GroupIterator::NextGroup()
{
	K_SATGroup* group = NULL;
	do {
		K_Window* window = fCurrentWindow;
		if (window == NULL) {
			group = NULL;
			break;
		}
		fCurrentWindow = fCurrentWindow->PreviousWindow(
			fCurrentWindow->CurrentWorkspace());
		if (window->IsHidden()
			|| strcmp(window->Title(), "Deskbar") == 0
			|| strcmp(window->Title(), "Desktop") == 0) {
			continue;
		}

		K_SATWindow* satWindow = fStackAndTile->GetSATWindow(window);
		group = satWindow->GetGroup();
	} while (group == NULL || fCurrentGroup == group);

	fCurrentGroup = group;
	return fCurrentGroup;
}


//	#pragma mark - K_WindowIterator


K_WindowIterator::K_WindowIterator(K_SATGroup* group, bool reverseLayerOrder)
	:
	fGroup(group),
	fReverseLayerOrder(reverseLayerOrder)
{
	if (fReverseLayerOrder)
		_ReverseRewind();
	else
		Rewind();
}


void
K_WindowIterator::Rewind()
{
	fAreaIndex = 0;
	fWindowIndex = 0;
	fCurrentArea = fGroup->GetAreaList().ItemAt(fAreaIndex);
}


K_SATWindow*
K_WindowIterator::NextWindow()
{
	if (fReverseLayerOrder)
		return _ReverseNextWindow();

	if (fWindowIndex == fCurrentArea->LayerOrder().CountItems()) {
		fAreaIndex++;
		fWindowIndex = 0;
		fCurrentArea = fGroup->GetAreaList().ItemAt(fAreaIndex);
		if (!fCurrentArea)
			return NULL;
	}
	K_SATWindow* window = fCurrentArea->LayerOrder().ItemAt(fWindowIndex);
	fWindowIndex++;
	return window;
}


//	#pragma mark - K_WindowIterator private methods


K_SATWindow*
K_WindowIterator::_ReverseNextWindow()
{
	if (fWindowIndex < 0) {
		fAreaIndex++;
		fCurrentArea = fGroup->GetAreaList().ItemAt(fAreaIndex);
		if (!fCurrentArea)
			return NULL;
		fWindowIndex = fCurrentArea->LayerOrder().CountItems() - 1;
	}
	K_SATWindow* window = fCurrentArea->LayerOrder().ItemAt(fWindowIndex);
	fWindowIndex--;
	return window;
}


void
K_WindowIterator::_ReverseRewind()
{
	Rewind();
	if (fCurrentArea)
		fWindowIndex = fCurrentArea->LayerOrder().CountItems() - 1;
}
