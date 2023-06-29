/*
 * Copyright 2010-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef K__SAT_WINDOW_H
#define K__SAT_WINDOW_H


#include <Region.h>

#include "SATDecorator.h"
#include "KSATGroup.h"//khidki
#include "KStacking.h"//khidki
#include "KStacking.h"//khidki
#include "KTiling.h"


class Desktop;
class K_SATGroup;
class K_StackAndTile;
class Window;
class K_Window;//khidki


class K_SATWindow {
public:
								K_SATWindow(K_StackAndTile* sat, K_Window* window);

//khidki start
//K_SATWindow(K_StackAndTile* sat, K_Window* window);
//end

								~K_SATWindow();

			K_Window*				GetWindow() { return fWindow; }

//K_Window*	K_GetWindow() { return k_fWindow; }//khidki

			K_SATDecorator*		GetDecorator() const;
			K_StackAndTile*		GetStackAndTile() { return fStackAndTile; }
			Desktop*			GetDesktop() { return fDesktop; }
			//! Can be NULL if memory allocation failed!
			K_SATGroup*			GetGroup();
			K_WindowArea*			GetWindowArea() { return fWindowArea; }

			bool				HandleMessage(K_SATWindow* sender,
									BPrivate::LinkReceiver& link,
									BPrivate::LinkSender& reply);

			bool				PropagateToGroup(K_SATGroup* group);

			// hook function called from K_SATGroup
			bool				AddedToGroup(K_SATGroup* group, K_WindowArea* area);
			bool				RemovedFromGroup(K_SATGroup* group,
									bool stayBelowMouse);
			void				RemovedFromArea(K_WindowArea* area);
			void				WindowLookChanged(window_look look);

			bool				StackWindow(K_SATWindow* child);

			void				FindSnappingCandidates();
			bool				JoinCandidates();
			void				DoGroupLayout();

			void				AdjustSizeLimits(BRect targetFrame);
			void				SetOriginalSizeLimits(int32 minWidth,
									int32 maxWidth, int32 minHeight,
									int32 maxHeight);
			void				GetSizeLimits(int32* minWidth, int32* maxWidth,
									int32* minHeight, int32* maxHeight) const;
			void				AddDecorator(int32* minWidth, int32* maxWidth,
									int32* minHeight, int32* maxHeight);
			void				AddDecorator(BRect& frame);

			// hook called when window has been resized form the outside
			void				Resized();
			bool				IsHResizeable() const;
			bool				IsVResizeable() const;

			//! \return the complete window frame including the K_Decorator
			BRect				CompleteWindowFrame();

			//! \return true if window is in a group with a least another window
			bool				PositionManagedBySAT();

			bool				HighlightTab(bool active);
			bool				HighlightBorders(K_Decorator::Region region,
									bool active);
			bool				IsTabHighlighted();
			bool				IsBordersHighlighted();

			uint64				Id();

			bool				SetSettings(const BMessage& message);
			void				GetSettings(BMessage& message);
private:
			uint64				_GenerateId();

			void				_UpdateSizeLimits();
			void				_RestoreOriginalSize(
									bool stayBelowMouse = true);

			K_Window*				fWindow;

//khidki start
//K_Window*				k_fWindow;
//end

			K_StackAndTile*		fStackAndTile;
			Desktop*			fDesktop;

			//! Current group.
			K_WindowArea*			fWindowArea;

			K_SATSnappingBehaviour*	fOngoingSnapping;
			K_SATStacking			fSATStacking;
			K_SATTiling			fSATTiling;

			K_SATSnappingBehaviourList	fSATSnappingBehaviourList;

//K_SATSnappingBehaviourList	k_fSATSnappingBehaviourList;//khidki

			int32				fOriginalMinWidth;
			int32				fOriginalMaxWidth;
			int32				fOriginalMinHeight;
			int32				fOriginalMaxHeight;

			float				fOriginalWidth;
			float				fOriginalHeight;

			uint64				fId;

			float				fOldTabLocatiom;
};


#endif
