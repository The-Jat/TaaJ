/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "DesktopListener.h"

//khidki code
//start
//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TTRACE(x) debug_printf x
#else
#	define TTRACE(x) ;
#endif
//end

DesktopListener::~DesktopListener()
{

}


DesktopObservable::DesktopObservable()
	:
	fWeAreInvoking(false)
{

}


void
DesktopObservable::RegisterListener(DesktopListener* listener, Desktop* desktop)
{
debug_printf("[DesktopObservable] { RegisterListener }....\n");

	fDesktopListenerList.Add(listener);
	listener->ListenerRegistered(desktop);
}


void
DesktopObservable::UnregisterListener(DesktopListener* listener)
{
	fDesktopListenerList.Remove(listener);
	listener->ListenerUnregistered();
}


const DesktopListenerDLList&
DesktopObservable::GetDesktopListenerList()
{
	return fDesktopListenerList;
}


///khidki start

void
DesktopObservable::K_RegisterListener(DesktopListener* listener, Desktop* desktop)
{
debug_printf("[DesktopObservable] { K_RegisterListener }....\n");
	k_fDesktopListenerList.Add(listener);
	listener->ListenerRegistered(desktop);
}


void
DesktopObservable::K_UnregisterListener(DesktopListener* listener)
{
debug_printf("[DesktopObservable] { K_UnregisterListener }....\n");
	k_fDesktopListenerList.Remove(listener);
	listener->ListenerUnregistered();
}


const DesktopListenerDLList&
DesktopObservable::K_GetDesktopListenerList()
{
debug_printf("[DesktopObservable] { K_GetDesktopListener }....\n");
	return k_fDesktopListenerList;
}
//end


bool
DesktopObservable::MessageForListener(Window* sender,
	BPrivate::LinkReceiver& link, BPrivate::LinkSender& reply)
{
debug_printf("[DesktopObservable] { MessageForListener }....\n");
	int32 identifier;
	link.Read<int32>(&identifier);
	for (DesktopListener* listener = fDesktopListenerList.First();
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
bool
DesktopObservable::K_MessageForListener(K_Window* sender,
	BPrivate::LinkReceiver& link, BPrivate::LinkSender& reply)
{
debug_printf("[DesktopObservable] { K_MessageForListener }....\n");
	int32 identifier;
	link.Read<int32>(&identifier);
	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener)) {
		if (listener->Identifier() == identifier) {
			if (!listener->K_HandleMessage(sender, link, reply))
				break;
			return true;
		}
	}
	return false;
}

//end


void
DesktopObservable::NotifyWindowAdded(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowAdded(window);
}


void
DesktopObservable::NotifyWindowRemoved(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowRemoved(window);
}

//khidki start

void
DesktopObservable::K_NotifyWindowAdded(K_Window* window)
{
debug_printf("[DesktopObservable] { K_NotifyWindowAdded }....\n");
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_WindowAdded(window);
}

void
DesktopObservable::K_NotifyWindowRemoved(K_Window* window)
{
debug_printf("[DesktopObservable] { K_NotifyWindowRemoved }....\n");
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_WindowRemoved(window);
}

//end

bool
DesktopObservable::NotifyKeyPressed(uint32 what, int32 key, int32 modifiers)
{
	if (fWeAreInvoking)
		return false;
	InvokeGuard invokeGuard(fWeAreInvoking);

	bool skipEvent = false;
	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener)) {
		if (listener->KeyPressed(what, key, modifiers))
			skipEvent = true;
	}
	return skipEvent;
}


void
DesktopObservable::NotifyMouseEvent(BMessage* message)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseEvent(message);
}


void
DesktopObservable::NotifyMouseDown(Window* window, BMessage* message,
	const BPoint& where)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseDown(window, message, where);
}


void
DesktopObservable::NotifyMouseUp(Window* window, BMessage* message,
	const BPoint& where)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseUp(window, message, where);
}


void
DesktopObservable::NotifyMouseMoved(Window* window, BMessage* message,
	const BPoint& where)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->MouseMoved(window, message, where);
}


void
DesktopObservable::NotifyWindowMoved(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowMoved(window);
}

//khidki start

void
DesktopObservable::K_NotifyWindowMoved(K_Window* window)
{
debug_printf("[DesktopObservable] { K_NotifyWindowMoved }....\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_WindowMoved(window);
}

//end


void
DesktopObservable::NotifyWindowResized(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowResized(window);
}


//khidki start

void
DesktopObservable::K_NotifyWindowResized(K_Window* window)
{
debug_printf("[DesktopObservable] { K_NotifyWindowResized}....\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_WindowResized(window);
}

//end


void
DesktopObservable::NotifyWindowActivated(Window* window)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowActivated(window);
}


//khidki start

void
DesktopObservable::K_NotifyWindowActivated(K_Window* window)
{
debug_printf("[DesktopObservable] { K_NotifyWindowActivated }....\n");
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_WindowActivated(window);
}
//end


void
DesktopObservable::NotifyWindowSentBehind(Window* window, Window* behindOf)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowSentBehind(window, behindOf);
}


//khidki start

void
DesktopObservable::K_NotifyWindowSentBehind(K_Window* window, K_Window* behindOf)
{
debug_printf("[DesktopObservable] { K_NotifyWindowSentBehind }....\n");
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_WindowSentBehind(window, behindOf);
}
//end


void
DesktopObservable::NotifyWindowWorkspacesChanged(Window* window,
	uint32 workspaces)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowWorkspacesChanged(window, workspaces);
}


void
DesktopObservable::NotifyWindowHidden(Window* window, bool fromMinimize)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowHidden(window, fromMinimize);
}

//khidki start

void
DesktopObservable::K_NotifyWindowHidden(K_Window* window, bool fromMinimize)
{
debug_printf("[DesktopObservable] { K_NotifyWindowHidden }....\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_WindowHidden(window, fromMinimize);
}

//end


void
DesktopObservable::NotifyWindowMinimized(Window* window, bool minimize)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowMinimized(window, minimize);
}


void
DesktopObservable::NotifyWindowTabLocationChanged(Window* window,
	float location, bool isShifting)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowTabLocationChanged(window, location, isShifting);
}


void
DesktopObservable::NotifySizeLimitsChanged(Window* window, int32 minWidth,
	int32 maxWidth, int32 minHeight, int32 maxHeight)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->SizeLimitsChanged(window, minWidth, maxWidth, minHeight,
			maxHeight);
}

//khidki start

void
DesktopObservable::K_NotifyWindowTabLocationChanged(K_Window* window,
	float location, bool isShifting)
{
debug_printf("[DesktopObservable] { K_NotifyWindowTabLocationChanged }....\n");
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_WindowTabLocationChanged(window, location, isShifting);
}


void
DesktopObservable::K_NotifySizeLimitsChanged(K_Window* window, int32 minWidth,
	int32 maxWidth, int32 minHeight, int32 maxHeight)
{
debug_printf("[DesktopObservable] { K_NotifySizeLimitsChanged }....\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_SizeLimitsChanged(window, minWidth, maxWidth, minHeight,
			maxHeight);
}

//end


void
DesktopObservable::NotifyWindowLookChanged(Window* window, window_look look)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowLookChanged(window, look);
}


void
DesktopObservable::NotifyWindowFeelChanged(Window* window, window_feel feel)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->WindowFeelChanged(window, feel);
}


//khidki start

void
DesktopObservable::K_NotifyWindowLookChanged(K_Window* window, window_look look)
{
debug_printf("[DesktopObservable] { K_NotifyWindowLookChanged }....\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_WindowLookChanged(window, look);
}


void
DesktopObservable::K_NotifyWindowFeelChanged(K_Window* window, window_feel feel)
{

debug_printf("[DesktopObservable] { K_NotifyWindowFeelChanged }....\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_WindowFeelChanged(window, feel);
}

//end


bool
DesktopObservable::SetDecoratorSettings(Window* window,
	const BMessage& settings)
{
	if (fWeAreInvoking)
		return false;
	InvokeGuard invokeGuard(fWeAreInvoking);

	bool changed = false;
	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		changed = changed | listener->SetDecoratorSettings(window, settings);

	return changed;
}


void
DesktopObservable::GetDecoratorSettings(Window* window, BMessage& settings)
{
	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = fDesktopListenerList.First();
		listener != NULL; listener = fDesktopListenerList.GetNext(listener))
		listener->GetDecoratorSettings(window, settings);
}

//khidki start

bool
DesktopObservable::K_SetDecoratorSettings(K_Window* window,
	const BMessage& settings)
{
debug_printf("[DesktopObservable] { K_SetDecoratorSettings }....\n");

	if (fWeAreInvoking)
		return false;
	InvokeGuard invokeGuard(fWeAreInvoking);

	bool changed = false;
	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		changed = changed | listener->K_SetDecoratorSettings(window, settings);

	return changed;
}


void
DesktopObservable::K_GetDecoratorSettings(K_Window* window, BMessage& settings)
{
debug_printf("[DesktopObservable] { K_GetDecoratorSettings }....\n");

	if (fWeAreInvoking)
		return;
	InvokeGuard invokeGuard(fWeAreInvoking);

	for (DesktopListener* listener = k_fDesktopListenerList.First();
		listener != NULL; listener = k_fDesktopListenerList.GetNext(listener))
		listener->K_GetDecoratorSettings(window, settings);
}

//end


DesktopObservable::InvokeGuard::InvokeGuard(bool& invoking)
	:
	fInvoking(invoking)
{
	fInvoking = true;
}


DesktopObservable::InvokeGuard::~InvokeGuard()
{
	fInvoking = false;
}
