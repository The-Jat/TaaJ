/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef K__STACKING_H
#define K__STACKING_H

#include "ObjectList.h"
#include "KStackAndTile.h"


class KSATWindow;


class K_StackingEventHandler
{
public:
	static bool				HandleMessage(K_SATWindow* sender,
								BPrivate::LinkReceiver& link,
								BPrivate::LinkSender& reply);
};


class K_SATStacking : public K_SATSnappingBehaviour {
public:
							K_SATStacking(K_SATWindow* window);
							~K_SATStacking();

		bool				FindSnappingCandidates(K_SATGroup* group);
//bool	K_FindSnappingCandidates(K_SATGroup* group);//total virtual
		bool				JoinCandidates();
		void				DoWindowLayout();

		void				RemovedFromArea(K_WindowArea* area);
		void				WindowLookChanged(window_look look);
private:
		bool				_IsStackableWindow(K_Window* window);
		void				_ClearSearchResult();
		void				_HighlightWindows(bool highlight = true);

		K_SATWindow*			fSATWindow;

		K_SATWindow*			fStackingParent;
};

#endif
