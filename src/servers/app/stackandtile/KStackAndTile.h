/*
 * Copyright 2010-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef K__STACK_AND_TILE_H
#define K__STACK_AND_TILE_H


#include <map>

#include <Message.h>
#include <MessageFilter.h>

#include "KDesktopListener.h"
#include "ObjectList.h"
#include "KWindowList.h"


//#define DEBUG_STACK_AND_TILE

#ifdef DEBUG_STACK_AND_TILE
#	define STRACE_SAT(x...) debug_printf("SAT: "x)
#else
#	define STRACE_SAT(x...) ;
#endif


class K_SATGroup;
class K_SATWindow;
//class Window;
class K_Window;//khidki
class K_WindowArea;


//typedef std::map<Window*, K_SATWindow*> SATWindowMap;

typedef std::map<K_Window*, K_SATWindow*> K_SATWindowMap;//khidki


class K_StackAndTile : public K_DesktopListener {
public:
								K_StackAndTile();
	virtual						~K_StackAndTile();

	virtual int32				Identifier();

	// DesktopListener hooks
	virtual void				ListenerRegistered(Desktop* desktop);
	virtual	void				ListenerUnregistered();

	virtual bool				HandleMessage(K_Window* sender,
									BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply);

	virtual void				K_WindowAdded(K_Window* window);
	virtual void				K_WindowRemoved(K_Window* window);

//khidki start
/*
virtual bool	K_HandleMessage(K_Window* sender,BPrivate::LinkReceiver& link,
			BPrivate::LinkSender& reply);
virtual void		K_WindowAdded(K_Window* window);//khidki
virtual void		K_WindowRemoved(K_Window* window);//khidki
*///end

	virtual bool				KeyPressed(uint32 what, int32 key,
									int32 modifiers);
	virtual void				MouseEvent(BMessage* message) {}
	virtual void				MouseDown(K_Window* window, BMessage* message,
									const BPoint& where);
	virtual void				MouseUp(K_Window* window, BMessage* message,
									const BPoint& where);
	virtual void				MouseMoved(K_Window* window, BMessage* message,
									const BPoint& where) {}

	virtual void				K_WindowMoved(K_Window* window);
	virtual void				K_WindowResized(K_Window* window);
	virtual void				K_WindowActivated(K_Window* window);
/*
virtual void		K_WindowMoved(K_Window* window);//khidki
virtual void		K_WindowResized(K_Window* window);//khidki
virtual void		K_WindowActivated(K_Window* window);//khidki
*/
	virtual void				K_WindowSentBehind(K_Window* window,
									K_Window* behindOf);

//virtual void	K_WindowSentBehind(K_Window* window, K_Window* behindOf);//khidki

	virtual void				WindowWorkspacesChanged(K_Window* window,
									uint32 workspaces);
	virtual void				K_WindowHidden(K_Window* window, bool fromMinimize);

//virtual void	K_WindowHidden(K_Window* window, bool fromMinimize);//khidki

	virtual void				WindowMinimized(K_Window* window, bool minimize);

	virtual void				K_WindowTabLocationChanged(K_Window* window,
									float location, bool isShifting);
	virtual void				K_SizeLimitsChanged(K_Window* window,
									int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight);
	virtual void				K_WindowLookChanged(K_Window* window,
									window_look look);
	virtual void				K_WindowFeelChanged(K_Window* window,
									window_feel feel);

//khidki start
/*
virtual void	K_WindowTabLocationChanged(K_Window* window,
									float location, bool isShifting);
virtual void	K_SizeLimitsChanged(K_Window* window,
									int32 minWidth, int32 maxWidth,
									int32 minHeight, int32 maxHeight);
virtual void	K_WindowLookChanged(K_Window* window,
									window_look look);
virtual void	K_WindowFeelChanged(K_Window* window,
									window_feel feel);
*///end

	virtual bool				K_SetDecoratorSettings(K_Window* window,
									const BMessage& settings);
	virtual void				K_GetDecoratorSettings(K_Window* window,
									BMessage& settings);

//khidki start
/*
virtual bool	K_SetDecoratorSettings(K_Window* window,
									const BMessage& settings);
virtual void	K_GetDecoratorSettings(K_Window* window,
									BMessage& settings);
*///end


			bool				SATKeyPressed()
									{ return fSATKeyPressed; }

			K_SATWindow*			GetSATWindow(K_Window* window);

//khidki start
//K_SATWindow*	K_GetSATWindow(K_Window* window);
//end
			K_SATWindow*			FindSATWindow(uint64 id);

private:
			void				_StartSAT();
			void				_StopSAT();
			void				_ActivateWindow(K_SATWindow* window);
			bool				_HandleMessage(BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply);
			K_SATGroup*			_GetSATGroup(K_SATWindow* window);

			Desktop*			fDesktop;

			bool				fSATKeyPressed;
		
			K_SATWindowMap		fSATWindowMap;

//K_SATWindowMap		k_fSATWindowMap;//khidki

			BObjectList<K_SATWindow>	fGrouplessWindows;

			K_SATWindow*			fCurrentSATWindow;
};


class K_GroupIterator {
public:
								K_GroupIterator(K_StackAndTile* sat,
									Desktop* desktop);

			K_SATGroup*			CurrentGroup(void) const
									{ return fCurrentGroup; };
			void				SetCurrentGroup(K_SATGroup* group)
									{ fCurrentGroup = group; };

			void				RewindToFront();
			K_SATGroup*			NextGroup();

private:
			K_StackAndTile*		fStackAndTile;
			Desktop*			fDesktop;
			K_Window*				fCurrentWindow;
			K_SATGroup*			fCurrentGroup;
};


class K_WindowIterator {
public:
								K_WindowIterator(K_SATGroup* group,
									bool reverseLayerOrder = false);

			void				Rewind();
			/*! Iterates over all areas in the group and return the windows in
			the areas. Within one area the windows are ordered by their layer
			position. If reverseLayerOrder is false the bottommost window comes
			first. */
			K_SATWindow*			NextWindow();

private:
			K_SATWindow*			_ReverseNextWindow();
			void				_ReverseRewind();

			K_SATGroup*			fGroup;
			bool				fReverseLayerOrder;

			K_WindowArea*			fCurrentArea;
			int32				fAreaIndex;
			int32				fWindowIndex;
};


class K_SATSnappingBehaviour {
public:
	virtual						~K_SATSnappingBehaviour() {};

	/*! Find all window candidates which possibly can join the group. Found
	candidates are marked here visual. */
	virtual bool				FindSnappingCandidates(K_SATGroup* group) = 0;
	/*! Join all candidates found in FindSnappingCandidates to the group.
	Previously visually mark should be removed here. \return true if
	integration has been succeed. */
	virtual bool				JoinCandidates() = 0;
	/*! Update the window tab values, solve the layout and move all windows in
	the group accordantly. */
	virtual void				RemovedFromArea(K_WindowArea* area) {}
	virtual void				WindowLookChanged(window_look look) {}
};


typedef BObjectList<K_SATSnappingBehaviour> K_SATSnappingBehaviourList;


#endif
