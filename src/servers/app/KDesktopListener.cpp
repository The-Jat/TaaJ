/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "KDesktopListener.h"

//khidki code
//start
//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TTRACE(x) debug_printf x
#else
#	define TTRACE(x) ;
#endif
//end

K_DesktopListener::~K_DesktopListener()
{
debug_printf("[K_DesktopObservable]{destructor}\n");
}


K_DesktopObservable::K_DesktopObservable()
	:
	fWeAreInvoking(false)
{
debug_printf("[K_DesktopObservable]\n");
}


void
K_DesktopObservable::K_RegisterListener(K_DesktopListener* listener, Desktop* desktop)
{
debug_printf("[K_DesktopObservable]{K_RegisterListener}\n");

	fDesktopListenerList.Add(listener);
	listener->ListenerRegistered(desktop);
}


void
K_DesktopObservable::K_UnregisterListener(K_DesktopListener* listener)
{
debug_printf("[K_DesktopObservable]{K_UnregisterListener}\n");

	fDesktopListenerList.Remove(listener);
	listener->ListenerUnregistered();
}


const K_DesktopListenerDLList&
K_DesktopObservable::K_GetDesktopListenerList()
{
debug_printf("[K_DesktopObservable]{K_GetDesktopListenerList}\n");

	return fDesktopListenerList;
}


bool
K_DesktopObservable::K_MessageForListener(K_Window* sender,
	BPrivate::LinkReceiver& link, BPrivate::LinkSender& reply)
{
debug_printf("[K_DesktopObservable]{K_MEssageForListener}\n");

	int32 identifier;
	link.Read<int32>(&identifier);
	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener)) {
		if (listener->Identifier() == identifier) {
			if (!listener->HandleMessage(sender, link, reply))
				break;
			return true;
		}
	}
	return false;
}


//khidki start
/*
bool
K_DesktopObservable::K_MessageForListener(K_Window* sender,
	BPrivate::LinkReceiver& link, BPrivate::LinkSender& reply)
{
	int32 identifier;
	link.Read<int32>(&identifier);
	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener)) {
		if (listener->Identifier() == identifier) {
			if (!listener->K_HandleMessage(sender, link, reply))
				break;
			return true;
		}
	}
	return false;
}
*/
//end

/*
void
K_DesktopObservable::NotifyWindowAdded(K_Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowAdded(window);
}


void
K_DesktopObservable::NotifyWindowRemoved(K_Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowRemoved(window);
}
*/
//khidki start

void
K_DesktopObservable::K_NotifyWindowAdded(K_Window* window)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowAdded}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_WindowAdded(window);

debug_printf("[K_DesktopObservable]{K_NotifyWindowAdded}ended\n");
}

void
K_DesktopObservable::K_NotifyWindowRemoved(K_Window* window)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowRemoved}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_WindowRemoved(window);
}

//end

bool
K_DesktopObservable::K_NotifyKeyPressed(uint32 what, int32 key, int32 modifiers)
{
debug_printf("[K_DesktopObservable]{K_NotifyKeyPressed}\n");

	if (fWeAreInvoking)
		return false;
	InvokeGuard invokeGuard(fWeAreInvoking);

	bool skipEvent = false;
	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener)) {
		if (listener->KeyPressed(what, key, modifiers))
			skipEvent = true;
	}
	return skipEvent;
}


void
K_DesktopObservable::K_NotifyMouseEvent(BMessage* message)
{
debug_printf("[K_DesktopObservable]{K_NotifyMouseEvent}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseEvent(message);
}


void
K_DesktopObservable::K_NotifyMouseDown(K_Window* window, BMessage* message,
	const BPoint& where)
{
debug_printf("[K_DesktopObservable]{K_NotifyMouseDown}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseDown(window, message, where);
}


void
K_DesktopObservable::K_NotifyMouseUp(K_Window* window, BMessage* message,
	const BPoint& where)
{
debug_printf("[K_DesktopObservable]{K_NotifyMouseUp}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseUp(window, message, where);
}


void
K_DesktopObservable::K_NotifyMouseMoved(K_Window* window, BMessage* message,
	const BPoint& where)
{
debug_printf("[K_DesktopObservable]{K_NotifyMouseMoved}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseMoved(window, message, where);
}

/*
void
K_DesktopObservable::NotifyWindowMoved(K_Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowMoved(window);
}*/

//khidki start
void
K_DesktopObservable::K_NotifyWindowMoved(K_Window* window)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowMoved}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_WindowMoved(window);
}

//end

/*
void
K_DesktopObservable::NotifyWindowResized(K_Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowResized(window);
}
*/

//khidki start

void
K_DesktopObservable::K_NotifyWindowResized(K_Window* window)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowResized}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_WindowResized(window);
}

//end

/*
void
K_DesktopObservable::NotifyWindowActivated(K_Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowActivated(window);
}
*/

//khidki start

void
K_DesktopObservable::K_NotifyWindowActivated(K_Window* window)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowActivated}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_WindowActivated(window);
}

//end

/*
void
K_DesktopObservable::NotifyWindowSentBehind(K_Window* window, K_Window* behindOf)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowSentBehind(window, behindOf);
}
*/

//khidki start

void
K_DesktopObservable::K_NotifyWindowSentBehind(K_Window* window, K_Window* behindOf)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowSentBehind}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_WindowSentBehind(window, behindOf);
}

//end


void
K_DesktopObservable::K_NotifyWindowWorkspacesChanged(K_Window* window,
	uint32 workspaces)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowWorkspacesChanged}\n");
//debug_printf("[K_DesktopObservable]{K_NotifyWindowWorkspacesChanged}workspaces=%d,workspaces\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
	{
	debug_printf("[K_DesktopObservable]{K_NotifyWindowWorkspacesChanged} loop\n");
		listener->WindowWorkspacesChanged(window, workspaces);
	}

debug_printf("[K_DesktopObservable]{K_NotifyWindowWorkspacesChanged}ended\n");
}

/*
void
K_DesktopObservable::NotifyWindowHidden(K_Window* window, bool fromMinimize)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowHidden(window, fromMinimize);
}*/

//khidki start

void
K_DesktopObservable::K_NotifyWindowHidden(K_Window* window, bool fromMinimize)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowHidden}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_WindowHidden(window, fromMinimize);
}
//end


void
K_DesktopObservable::K_NotifyWindowMinimized(K_Window* window, bool minimize)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowMinimized}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowMinimized(window, minimize);
}

/*
void
K_DesktopObservable::NotifyWindowTabLocationChanged(K_Window* window,
	float location, bool isShifting)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowTabLocationChanged(window, location, isShifting);
}


void
K_DesktopObservable::NotifySizeLimitsChanged(K_Window* window, int32 minWidth,
	int32 maxWidth, int32 minHeight, int32 maxHeight)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->SizeLimitsChanged(window, minWidth, maxWidth, minHeight,
			maxHeight);
}*/

//khidki start

void
K_DesktopObservable::K_NotifyWindowTabLocationChanged(K_Window* window,
	float location, bool isShifting)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowTabLocationChanged}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_WindowTabLocationChanged(window, location, isShifting);
}


void
K_DesktopObservable::K_NotifySizeLimitsChanged(K_Window* window, int32 minWidth,
	int32 maxWidth, int32 minHeight, int32 maxHeight)
{
debug_printf("[K_DesktopObservable]{K_NotifySizeLimitsChanged}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_SizeLimitsChanged(window, minWidth, maxWidth, minHeight,
			maxHeight);
}

//end

/*
void
K_DesktopObservable::NotifyWindowLookChanged(K_Window* window, window_look look)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowLookChanged(window, look);
}


void
K_DesktopObservable::NotifyWindowFeelChanged(K_Window* window, window_feel feel)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowFeelChanged(window, feel);
}

*/
//khidki start

void
K_DesktopObservable::K_NotifyWindowLookChanged(K_Window* window, window_look look)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowLookChanged}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_WindowLookChanged(window, look);
}


void
K_DesktopObservable::K_NotifyWindowFeelChanged(K_Window* window, window_feel feel)
{
debug_printf("[K_DesktopObservable]{K_NotifyWindowFeelChanged}\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_WindowFeelChanged(window, feel);
}

//end

/*
bool
K_DesktopObservable::SetDecoratorSettings(K_Window* window,
	const BMessage& settings)
{
	if (fWeAreInvoking)
		return false;
	InvokeGuard invokeGuard(fWeAreInvoking);

	bool changed = false;
	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		changed = changed | listener->SetDecoratorSettings(window, settings);

	return changed;
}


void
K_DesktopObservable::GetDecoratorSettings(K_Window* window, BMessage& settings)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->GetDecoratorSettings(window, settings);
}
*/
//khidki start

bool
K_DesktopObservable::K_SetDecoratorSettings(K_Window* window,
	const BMessage& settings)
{
	if (fWeAreInvoking)
		return false;
	InvokeGuard invokeGuard(fWeAreInvoking);

	bool changed = false;
	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		changed = changed | listener->K_SetDecoratorSettings(window, settings);

	return changed;
}


void
K_DesktopObservable::K_GetDecoratorSettings(K_Window* window, BMessage& settings)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (K_DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->K_GetDecoratorSettings(window, settings);
}

//end


K_DesktopObservable::InvokeGuard::InvokeGuard(bool& invoking)
	:
	fInvoking(invoking)
{
	fInvoking = true;
}


K_DesktopObservable::InvokeGuard::~InvokeGuard()
{
	fInvoking = false;
}
