/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "KStacking.h"

#include <Debug.h>

#include "StackAndTilePrivate.h"

#include "Desktop.h"
#include "KSATWindow.h"
#include "KWindow.h"


//#define DEBUG_STACKING

#ifdef DEBUG_STACKING
#	define STRACE_STACKING(x...) debug_printf("SAT Stacking: "x)
#else
#	define STRACE_STACKING(x...) ;
#endif


using namespace BPrivate;


bool
K_StackingEventHandler::HandleMessage(K_SATWindow* sender,
	BPrivate::LinkReceiver& link, BPrivate::LinkSender& reply)
{
	Desktop* desktop = sender->GetDesktop();
	K_StackAndTile* stackAndTile = sender->GetStackAndTile();

	int32 what;
	link.Read<int32>(&what);

	switch (what) {
		case kAddWindowToStack:
		{
			port_id port;
			int32 token;
			team_id team;
			link.Read<port_id>(&port);
			link.Read<int32>(&token);
			link.Read<team_id>(&team);
			int32 position;
			if (link.Read<int32>(&position) != B_OK)
				return false;

			K_WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			if (position < 0)
				position = area->WindowList().CountItems() - 1;

			K_SATWindow* parent = area->WindowList().ItemAt(position);
			K_Window* window = desktop->K_WindowForClientLooperPort(port);
			if (!parent || !window) {
				reply.StartMessage(B_BAD_VALUE);
				reply.Flush();
				break;
			}

			K_SATWindow* candidate = stackAndTile->GetSATWindow(window);
			if (!candidate)
				return false;
			if (!parent->StackWindow(candidate))
				return false;

			reply.StartMessage(B_OK);
			reply.Flush();
			break;
		}
		case kRemoveWindowFromStack:
		{
			port_id port;
			int32 token;
			team_id team;
			link.Read<port_id>(&port);
			link.Read<int32>(&token);
			if (link.Read<team_id>(&team) != B_OK)
				return false;

			K_SATGroup* group = sender->GetGroup();
			if (!group)
				return false;

			K_Window* window = desktop->K_WindowForClientLooperPort(port);
			if (!window) {
				reply.StartMessage(B_BAD_VALUE);
				reply.Flush();
				break;
			}
			K_SATWindow* candidate = stackAndTile->GetSATWindow(window);
			if (!candidate)
				return false;
			if (!group->RemoveWindow(candidate, false))
				return false;
			break;
		}
		case kRemoveWindowFromStackAt:
		{
			int32 position;
			if (link.Read<int32>(&position) != B_OK)
				return false;
			K_SATGroup* group = sender->GetGroup();
			K_WindowArea* area = sender->GetWindowArea();
			if (!area || !group)
				return false;
			K_SATWindow* removeWindow = area->WindowList().ItemAt(position);
			if (!removeWindow) {
				reply.StartMessage(B_BAD_VALUE);
				reply.Flush();
				break;
			}

			if (!group->RemoveWindow(removeWindow, false))
				return false;

			KServerWindow* window = removeWindow->GetWindow()->KServerWindow();
			reply.StartMessage(B_OK);
			reply.Attach<port_id>(window->ClientLooperPort());
			reply.Attach<int32>(window->ClientToken());
			reply.Attach<team_id>(window->ClientTeam());
			reply.Flush();
			break;
		}
		case kCountWindowsOnStack:
		{
			K_WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			reply.StartMessage(B_OK);
			reply.Attach<int32>(area->WindowList().CountItems());
			reply.Flush();
			break;
		}
		case kWindowOnStackAt:
		{
			int32 position;
			if (link.Read<int32>(&position) != B_OK)
				return false;
			K_WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			K_SATWindow* satWindow = area->WindowList().ItemAt(position);
			if (!satWindow) {
				reply.StartMessage(B_BAD_VALUE);
				reply.Flush();
				break;
			}

			KServerWindow* window = satWindow->GetWindow()->KServerWindow();
			reply.StartMessage(B_OK);
			reply.Attach<port_id>(window->ClientLooperPort());
			reply.Attach<int32>(window->ClientToken());
			reply.Attach<team_id>(window->ClientTeam());
			reply.Flush();
			break;
		}
		case kStackHasWindow:
		{
			port_id port;
			int32 token;
			team_id team;
			link.Read<port_id>(&port);
			link.Read<int32>(&token);
			if (link.Read<team_id>(&team) != B_OK)
				return false;

			K_Window* window = desktop->K_WindowForClientLooperPort(port);
			if (!window) {
				reply.StartMessage(B_BAD_VALUE);
				reply.Flush();
				break;
			}
			K_SATWindow* candidate = stackAndTile->GetSATWindow(window);
			if (!candidate)
				return false;

			K_WindowArea* area = sender->GetWindowArea();
			if (!area)
				return false;
			reply.StartMessage(B_OK);
			reply.Attach<bool>(area->WindowList().HasItem(candidate));
			reply.Flush();
			break;
		}
		default:
			return false;
	}
	return true;
}


K_SATStacking::K_SATStacking(K_SATWindow* window)
	:
	fSATWindow(window),
	fStackingParent(NULL)
{

}


K_SATStacking::~K_SATStacking()
{

}


bool
K_SATStacking::FindSnappingCandidates(K_SATGroup* group)
{
	_ClearSearchResult();

	K_Window* window = fSATWindow->GetWindow();
	if (!window->Decorator())
		return false;

	BPoint mousePosition;
	int32 buttons;
	fSATWindow->GetDesktop()->GetLastMouseState(&mousePosition, &buttons);
	if (!window->Decorator()->TitleBarRect().Contains(mousePosition))
		return false;

	// use the upper edge of the candidate window to find the parent window
	mousePosition.y = window->Decorator()->TitleBarRect().top;

	for (int i = 0; i < group->CountItems(); i++) {
		K_SATWindow* satWindow = group->WindowAt(i);
		// search for stacking parent
		K_Window* parentWindow = satWindow->GetWindow();
		if (parentWindow == window || parentWindow->Decorator() == NULL)
			continue;
		if (_IsStackableWindow(parentWindow) == false
			|| _IsStackableWindow(window) == false)
			continue;
		K_Decorator::Tab* tab = parentWindow->Decorator()->TabAt(
			parentWindow->PositionInStack());
		if (tab == NULL)
			continue;
		if (tab->tabRect.Contains(mousePosition)) {
			// remember window as the parent for stacking
			fStackingParent = satWindow;
			_HighlightWindows(true);
			return true;
		}
	}

	return false;
}


//total virtual
/*bool
K_SATStacking::K_FindSnappingCandidates(K_SATGroup* group)
{
	_ClearSearchResult();

	K_Window* window = fSATWindow->GetWindow();
	if (!window->Decorator())
		return false;

	BPoint mousePosition;
	int32 buttons;
	fSATWindow->GetDesktop()->GetLastMouseState(&mousePosition, &buttons);
	if (!window->Decorator()->TitleBarRect().Contains(mousePosition))
		return false;

	// use the upper edge of the candidate window to find the parent window
	mousePosition.y = window->Decorator()->TitleBarRect().top;

	for (int i = 0; i < group->CountItems(); i++) {
		K_SATWindow* satWindow = group->WindowAt(i);
		// search for stacking parent
		K_Window* parentWindow = satWindow->GetWindow();
		if (parentWindow == window || parentWindow->Decorator() == NULL)
			continue;
		if (_IsStackableWindow(parentWindow) == false
			|| _IsStackableWindow(window) == false)
			continue;
		Decorator::Tab* tab = parentWindow->Decorator()->TabAt(
			parentWindow->PositionInStack());
		if (tab == NULL)
			continue;
		if (tab->tabRect.Contains(mousePosition)) {
			// remember window as the parent for stacking
			fStackingParent = satWindow;
			_HighlightWindows(true);
			return true;
		}
	}

	return false;
}*/
//end

bool
K_SATStacking::JoinCandidates()
{
	if (!fStackingParent)
		return false;

	bool result = fStackingParent->StackWindow(fSATWindow);

	_ClearSearchResult();
	return result;
}


void
K_SATStacking::RemovedFromArea(K_WindowArea* area)
{
	const K_SATWindowList& list = area->WindowList();
	if (list.CountItems() > 0)
		list.ItemAt(0)->DoGroupLayout();
}


void
K_SATStacking::WindowLookChanged(window_look look)
{
	K_Window* window = fSATWindow->GetWindow();
	K_WindowStack* stack = window->GetWindowStack();
	if (stack == NULL)
		return;
	K_SATGroup* group = fSATWindow->GetGroup();
	if (group == NULL)
		return;
	if (stack->CountWindows() > 1 && _IsStackableWindow(window) == false)
		group->RemoveWindow(fSATWindow);
}


bool
K_SATStacking::_IsStackableWindow(K_Window* window)
{
	if (window->Look() == B_DOCUMENT_WINDOW_LOOK)
		return true;
	if (window->Look() == B_TITLED_WINDOW_LOOK)
		return true;
	return false;
}


void
K_SATStacking::_ClearSearchResult()
{
	if (!fStackingParent)
		return;

	_HighlightWindows(false);
	fStackingParent = NULL;
}


void
K_SATStacking::_HighlightWindows(bool highlight)
{
	Desktop* desktop = fSATWindow->GetWindow()->Desktop();
	if (!desktop)
		return;
	fStackingParent->HighlightTab(highlight);
	fSATWindow->HighlightTab(highlight);
}
