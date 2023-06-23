/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef __K_DESKTOP_LISTENER_H
#define __K_DESKTOP_LISTENER_H


#include <util/DoublyLinkedList.h>

#include <Point.h>

#include <ServerLink.h>
//#include "Window.h"
#include "KWindow.h"//khdiki


class BMessage;
class Desktop;
//class Window;
class K_Window;//khdiki


class K_DesktopListener : public DoublyLinkedListLinkImpl<K_DesktopListener> {
public:
	virtual						~K_DesktopListener();

	virtual int32				Identifier() = 0;

	virtual	void				ListenerRegistered(Desktop* desktop) = 0;
	virtual	void				ListenerUnregistered() = 0;

	virtual bool				HandleMessage(K_Window* sender,
									BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply) = 0;

//khidki start
/*virtual bool	K_HandleMessage(K_Window* sender,
									BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply) = 0;
*///end


/*	virtual void				WindowAdded(K_Window* window) = 0;
	virtual void				WindowRemoved(K_Window* window) = 0;
*/
virtual void				K_WindowAdded(K_Window* window) = 0;//khidki
virtual void				K_WindowRemoved(K_Window* window) = 0;//khidki

	virtual bool				KeyPressed(uint32 what, int32 key,
									int32 modifiers) = 0;
	virtual void				MouseEvent(BMessage* message) = 0;
	virtual void				MouseDown(K_Window* window, BMessage* message,
									const BPoint& where) = 0;
	virtual void				MouseUp(K_Window* window, BMessage* message,
									const BPoint& where) = 0;
	virtual void				MouseMoved(K_Window* window, BMessage* message,
									const BPoint& where) = 0;

/*	virtual void				WindowMoved(K_Window* window) = 0;
	virtual void				WindowResized(K_Window* window) = 0;
	virtual void				WindowActivated(K_Window* window) = 0;
*/
virtual void	K_WindowMoved(K_Window* window) = 0;//khidki
virtual void	K_WindowResized(K_Window* window) = 0;//khidki
virtual void	K_WindowActivated(K_Window* window) = 0;//khidki

/*	virtual void				WindowSentBehind(K_Window* window,
									K_Window* behindOf) = 0;
*/
virtual void	K_WindowSentBehind(K_Window* window, K_Window* behindOf) = 0;//khidki


	virtual void				WindowWorkspacesChanged(K_Window* window,
									uint32 workspaces) = 0;
/*	virtual void				WindowHidden(K_Window* window,
									bool fromMinimize) = 0;
*/
//khidki start
virtual void			K_WindowHidden(K_Window* window,
							bool fromMinimize) = 0;
//end
	virtual void				WindowMinimized(K_Window* window,
									bool minimize) = 0;

/*	virtual void				WindowTabLocationChanged(K_Window* window,
									float location, bool isShifting) = 0;
	virtual void				SizeLimitsChanged(K_Window* window,
									int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight) = 0;
	virtual void				WindowLookChanged(K_Window* window,
									window_look look) = 0;
	virtual void				WindowFeelChanged(K_Window* window,
									window_feel feel) = 0;
*/

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

/*
	virtual bool				SetDecoratorSettings(K_Window* window,
									const BMessage& settings) = 0;
	virtual void				GetDecoratorSettings(K_Window* window,
									BMessage& settings) = 0;
*/
//khidki start

virtual bool	K_SetDecoratorSettings(K_Window* window,
									const BMessage& settings) = 0;
virtual void	K_GetDecoratorSettings(K_Window* window,
									BMessage& settings) = 0;

//end

};


typedef DoublyLinkedList<K_DesktopListener> K_DesktopListenerDLList;


class K_DesktopObservable {
public:
								K_DesktopObservable();

			void				K_RegisterListener(K_DesktopListener* listener,
									Desktop* desktop);
			void				K_UnregisterListener(K_DesktopListener* listener);
	const K_DesktopListenerDLList&	K_GetDesktopListenerList();

			bool				K_MessageForListener(K_Window* sender,
									BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply);

//khidki start
/*
bool	K_MessageForListener(K_Window* sender,
									BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply);
*///end


//			void				NotifyWindowAdded(K_Window* window);
//			void				NotifyWindowRemoved(K_Window* window);
//khidki start
void	K_NotifyWindowAdded(K_Window* window);
void	K_NotifyWindowRemoved(K_Window* window);

//khidki end
			bool				K_NotifyKeyPressed(uint32 what, int32 key,
									int32 modifiers);
			void				K_NotifyMouseEvent(BMessage* message);
			void				K_NotifyMouseDown(K_Window* window,
									BMessage* message, const BPoint& where);
			void				K_NotifyMouseUp(K_Window* window, BMessage* message,
										const BPoint& where);
			void				K_NotifyMouseMoved(K_Window* window,
									BMessage* message, const BPoint& where);

//			void				NotifyWindowMoved(K_Window* window);
//			void				NotifyWindowResized(K_Window* window);
//			void				NotifyWindowActivated(K_Window* window);

void		K_NotifyWindowMoved(K_Window* window);//khidki
void		K_NotifyWindowResized(K_Window* window);//khidki
void		K_NotifyWindowActivated(K_Window* window);//khidki

/*			void				NotifyWindowSentBehind(K_Window* window,
									K_Window* behindOf);
*/
void	K_NotifyWindowSentBehind(K_Window* window, K_Window* behindOf);//khidki

			void				K_NotifyWindowWorkspacesChanged(K_Window* window,
									uint32 workspaces);
/*			void				NotifyWindowHidden(K_Window* window,
									bool fromMinimize);
*/
//khdiki start
void	K_NotifyWindowHidden(K_Window* window,
		bool fromMinimize);
//end
			void				K_NotifyWindowMinimized(K_Window* window,
									bool minimize);

/*			void				NotifyWindowTabLocationChanged(K_Window* window,
									float location, bool isShifting);
			void				NotifySizeLimitsChanged(K_Window* window,
									int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight);
			void				NotifyWindowLookChanged(K_Window* window,
									window_look look);
			void				NotifyWindowFeelChanged(K_Window* window,
									window_feel feel);
*/
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

/*
			bool				SetDecoratorSettings(K_Window* window,
									const BMessage& settings);
			void				GetDecoratorSettings(K_Window* window,
									BMessage& settings);
*/
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

		K_DesktopListenerDLList	fDesktopListenerList;

		// prevent recursive invokes
		bool					fWeAreInvoking;
};

#endif
