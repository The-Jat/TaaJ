/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef DESKTOP_LISTENER_H
#define DESKTOP_LISTENER_H


#include <util/DoublyLinkedList.h>

#include <Point.h>

#include <ServerLink.h>
#include "Window.h"
#include "KWindow.h"//khdiki


class BMessage;
class Desktop;
class Window;
class K_Window;//khdiki


class DesktopListener : public DoublyLinkedListLinkImpl<DesktopListener> {
public:
	virtual						~DesktopListener();

	virtual int32				Identifier() = 0;

	virtual	void				ListenerRegistered(Desktop* desktop) = 0;
	virtual	void				ListenerUnregistered() = 0;

	virtual bool				HandleMessage(Window* sender,
									BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply) = 0;

//khidki start
virtual bool	K_HandleMessage(K_Window* sender,
									BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply) = 0;
//end


	virtual void				WindowAdded(Window* window) = 0;
	virtual void				WindowRemoved(Window* window) = 0;

virtual void				K_WindowAdded(K_Window* window) = 0;//khidki
virtual void				K_WindowRemoved(K_Window* window) = 0;//khidki

	virtual bool				KeyPressed(uint32 what, int32 key,
									int32 modifiers) = 0;
	virtual void				MouseEvent(BMessage* message) = 0;
	virtual void				MouseDown(Window* window, BMessage* message,
									const BPoint& where) = 0;
	virtual void				MouseUp(Window* window, BMessage* message,
									const BPoint& where) = 0;
	virtual void				MouseMoved(Window* window, BMessage* message,
									const BPoint& where) = 0;

	virtual void				WindowMoved(Window* window) = 0;
	virtual void				WindowResized(Window* window) = 0;
	virtual void				WindowActivated(Window* window) = 0;

virtual void	K_WindowMoved(K_Window* window) = 0;//khidki
virtual void	K_WindowResized(K_Window* window) = 0;//khidki
virtual void	K_WindowActivated(K_Window* window) = 0;//khidki

	virtual void				WindowSentBehind(Window* window,
									Window* behindOf) = 0;

virtual void	K_WindowSentBehind(K_Window* window, K_Window* behindOf) = 0;//khidki


	virtual void				WindowWorkspacesChanged(Window* window,
									uint32 workspaces) = 0;
	virtual void				WindowHidden(Window* window,
									bool fromMinimize) = 0;
//khidki start
virtual void			K_WindowHidden(K_Window* window,
							bool fromMinimize) = 0;

//end
	virtual void				WindowMinimized(Window* window,
									bool minimize) = 0;

	virtual void				WindowTabLocationChanged(Window* window,
									float location, bool isShifting) = 0;
	virtual void				SizeLimitsChanged(Window* window,
									int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight) = 0;
	virtual void				WindowLookChanged(Window* window,
									window_look look) = 0;
	virtual void				WindowFeelChanged(Window* window,
									window_feel feel) = 0;


//khidki start

virtual void	K_WindowTabLocationChanged(K_Window* window,
									float location, bool isShifting) = 0;
virtual void	K_SizeLimitsChanged(K_Window* window,
									int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight) = 0;

virtual void	K_WindowLookChanged(K_Window* window,
									window_look look) = 0;
virtual void	K_WindowFeelChanged(K_Window* window,
									window_feel feel) = 0;


//end


	virtual bool				SetDecoratorSettings(Window* window,
									const BMessage& settings) = 0;
	virtual void				GetDecoratorSettings(Window* window,
									BMessage& settings) = 0;

//khidki start

virtual bool	K_SetDecoratorSettings(K_Window* window,
									const BMessage& settings) = 0;
virtual void	K_GetDecoratorSettings(K_Window* window,
									BMessage& settings) = 0;
//end

};


typedef DoublyLinkedList<DesktopListener> DesktopListenerDLList;


class DesktopObservable {
public:
								DesktopObservable();

			void				RegisterListener(DesktopListener* listener,
									Desktop* desktop);
			void				UnregisterListener(DesktopListener* listener);
	const DesktopListenerDLList&	GetDesktopListenerList();


//khidki start
void	K_RegisterListener(DesktopListener* listener,
									Desktop* desktop);
void	K_UnregisterListener(DesktopListener* listener);
const DesktopListenerDLList&	K_GetDesktopListenerList();
//end
			bool				MessageForListener(Window* sender,
									BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply);

//khidki start
bool	K_MessageForListener(K_Window* sender,
									BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply);

//end


			void				NotifyWindowAdded(Window* window);
			void				NotifyWindowRemoved(Window* window);
//khidki start
void	K_NotifyWindowAdded(K_Window* window);
void	K_NotifyWindowRemoved(K_Window* window);
//khidki end
			bool				NotifyKeyPressed(uint32 what, int32 key,
									int32 modifiers);
			void				NotifyMouseEvent(BMessage* message);
			void				NotifyMouseDown(Window* window,
									BMessage* message, const BPoint& where);
			void				NotifyMouseUp(Window* window, BMessage* message,
										const BPoint& where);
			void				NotifyMouseMoved(Window* window,
									BMessage* message, const BPoint& where);

			void				NotifyWindowMoved(Window* window);
			void				NotifyWindowResized(Window* window);
			void				NotifyWindowActivated(Window* window);

void		K_NotifyWindowMoved(K_Window* window);//khidki
void		K_NotifyWindowResized(K_Window* window);//khidki
void		K_NotifyWindowActivated(K_Window* window);//khidki

			void				NotifyWindowSentBehind(Window* window,
									Window* behindOf);

void	K_NotifyWindowSentBehind(K_Window* window, K_Window* behindOf);//khidki

			void				NotifyWindowWorkspacesChanged(Window* window,
									uint32 workspaces);
			void				NotifyWindowHidden(Window* window,
									bool fromMinimize);
//khdiki start

void	K_NotifyWindowHidden(K_Window* window,
		bool fromMinimize);
//end
			void				NotifyWindowMinimized(Window* window,
									bool minimize);

			void				NotifyWindowTabLocationChanged(Window* window,
									float location, bool isShifting);
			void				NotifySizeLimitsChanged(Window* window,
									int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight);
			void				NotifyWindowLookChanged(Window* window,
									window_look look);
			void				NotifyWindowFeelChanged(Window* window,
									window_feel feel);

//khidki start

void	K_NotifyWindowTabLocationChanged(K_Window* window,
									float location, bool isShifting);
void	K_NotifySizeLimitsChanged(K_Window* window,
									int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight);
void	K_NotifyWindowLookChanged(K_Window* window,
									window_look look);
void	K_NotifyWindowFeelChanged(K_Window* window,
									window_feel feel);

//end


			bool				SetDecoratorSettings(Window* window,
									const BMessage& settings);
			void				GetDecoratorSettings(Window* window,
									BMessage& settings);

//khidki start
bool	K_SetDecoratorSettings(K_Window* window,
									const BMessage& settings);
void	K_GetDecoratorSettings(K_Window* window,
									BMessage& settings);

//end

private:
		class InvokeGuard {
			public:
				InvokeGuard(bool& invoking);
				~InvokeGuard();
			private:
				bool&	fInvoking;
		};

		DesktopListenerDLList	fDesktopListenerList;

DesktopListenerDLList	k_fDesktopListenerList;//khidki

		// prevent recursive invokes
		bool					fWeAreInvoking;
};

#endif
