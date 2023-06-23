/*
 * Copyright (c) 2005-2008, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "DesktopSettings.h"
#include "KWindow.h"

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

const BPoint k_kInvalidWindowPosition = BPoint(INFINITY, INFINITY);


k_window_anchor::k_window_anchor()
	 :
	 next(NULL),
	 previous(NULL),
	 position(k_kInvalidWindowPosition)
{
}


//	#pragma mark -


K_WindowList::K_WindowList(int32 index)
	:
	fIndex(index),
	fFirstWindow(NULL),
	fLastWindow(NULL)
{
}


K_WindowList::~K_WindowList()
{
}


void
K_WindowList::SetIndex(int32 index)
{
	fIndex = index;
}


/*!
	Adds the \a window to the end of the list. If \a before is
	given, it will be inserted right before that window.
*/
void
K_WindowList::AddWindow(K_Window* window, K_Window* before)
{
#ifdef TESTING_K_WINDOW
debug_printf("[K_WindowList]{AddWindow}\n");
debug_printf("[K_WindowList]{AddWindow}Count=%d\n",Count());
#endif

	k_window_anchor& windowAnchor = window->Anchor(fIndex);

	if (before != NULL) {
		#ifdef TESTING_K_WINDOW
			debug_printf("[K_WindowList]{AddWindow}before!=NULL\n");
		#endif
		k_window_anchor& beforeAnchor = before->Anchor(fIndex);

		// add view before this one
		windowAnchor.next = before;
		windowAnchor.previous = beforeAnchor.previous;
		if (windowAnchor.previous != NULL)
			windowAnchor.previous->Anchor(fIndex).next = window;

		beforeAnchor.previous = window;
		if (fFirstWindow == before)
			fFirstWindow = window;
	} else {
		#ifdef TESTING_K_WINDOW
			debug_printf("[K_WindowList]{AddWindow}Before!=Null Else\n");
		#endif
		// add view to the end of the list
		if (fLastWindow != NULL) {
		#ifdef TESTING_K_WINDOW
			debug_printf("[K_WindowList]{AddWindow}fLastWindow != null\n");
		#endif
			fLastWindow->Anchor(fIndex).next = window;
			windowAnchor.previous = fLastWindow;
		} else {
			#ifdef TESTING_K_WINDOW
				debug_printf("[K_WindowList]{AddWindow}else\n");
			#endif
			fFirstWindow = window;
			windowAnchor.previous = NULL;
		}

		windowAnchor.next = NULL;
		fLastWindow = window;
	}

	if (fIndex < kMaxWorkspaces)
		window->SetWorkspaces(window->Workspaces() | (1UL << fIndex));

#ifdef TESTING_K_WINDOW
debug_printf("[K_WindowList]{AddWindow}ends\n");
debug_printf("[K_WindowList]{AddWindow}Count after adding=%d\n",Count());
#endif
}


void
K_WindowList::RemoveWindow(K_Window* window)
{
	k_window_anchor& windowAnchor = window->Anchor(fIndex);

	if (fFirstWindow == window) {
		// it's the first child
		fFirstWindow = windowAnchor.next;
	} else {
		// it must have a previous sibling, then
		windowAnchor.previous->Anchor(fIndex).next = windowAnchor.next;
	}

	if (fLastWindow == window) {
		// it's the last child
		fLastWindow = windowAnchor.previous;
	} else {
		// then it must have a next sibling
		windowAnchor.next->Anchor(fIndex).previous = windowAnchor.previous;
	}

	if (fIndex < kMaxWorkspaces)
		window->SetWorkspaces(window->Workspaces() & ~(1UL << fIndex));

	windowAnchor.previous = NULL;
	windowAnchor.next = NULL;
}


bool
K_WindowList::HasWindow(K_Window* window) const
{
	if (window == NULL)
		return false;

	return window->Anchor(fIndex).next != NULL
		|| window->Anchor(fIndex).previous != NULL
		|| fFirstWindow == window
		|| fLastWindow == window;
}


/*!	Unlike HasWindow(), this will not reference the window pointer. You
	can use this method to check whether or not a window is still part
	of a list (when it's possible that the window is already gone).
*/
bool
K_WindowList::ValidateWindow(K_Window* validateWindow) const
{
	for (K_Window *window = FirstWindow(); window != NULL;
			window = window->NextWindow(fIndex)) {
		if (window == validateWindow)
			return true;
	}

	return false;
}


int32
K_WindowList::Count() const
{
	int32 count = 0;

	for (K_Window *window = FirstWindow(); window != NULL;
			window = window->NextWindow(fIndex)) {
		count++;
	}

	return count;
}
