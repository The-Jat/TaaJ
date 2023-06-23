/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef K__TILING_H
#define K__TILING_H

#include "ObjectList.h"

#include "Decorator.h"
#include "KStackAndTile.h"
#include "KSATGroup.h"


class K_SATWindow;


class K_SATTiling : public K_SATSnappingBehaviour {
public:
							K_SATTiling(K_SATWindow* window);
							~K_SATTiling();

		bool				FindSnappingCandidates(K_SATGroup* group);

//bool	K_FindSnappingCandidates(K_SATGroup* group);//total virtual

		bool				JoinCandidates();

		void				WindowLookChanged(window_look look);
private:
		bool				_IsTileableWindow(K_Window* window);

		bool				_FindFreeAreaInGroup(K_SATGroup* group);
		bool				_FindFreeAreaInGroup(K_SATGroup* group,
								K_Corner::position_t corner);

		bool				_InteresstingCrossing(K_Crossing* crossing,
								K_Corner::position_t corner, BRect& windowFrame);
		bool				_FindFreeArea(K_SATGroup* group,
								const K_Crossing* crossing,
								K_Corner::position_t areaCorner,
								BRect& windowFrame);
		bool				_HasOverlapp(K_SATGroup* group);
		bool				_CheckArea(K_SATGroup* group,
								K_Corner::position_t corner, BRect& windowFrame,
								float& error);
		bool				_CheckMinFreeAreaSize();
		float				_FreeAreaError(BRect& windowFrame);
		bool				_IsCornerInFreeArea(K_Corner::position_t corner,
								BRect& windowFrame);

		BRect				_FreeAreaSize();

		void				_HighlightWindows(K_SATGroup* group,
								bool highlight = true);
		bool				_SearchHighlightWindow(K_Tab* tab, K_Tab* firstOrthTab,
								K_Tab* secondOrthTab, const K_TabList* orthTabs,
								K_Corner::position_t areaCorner,
								Decorator::Region region, bool highlight);
		void				_HighlightWindows(K_WindowArea* area,
								Decorator::Region region, bool highlight);

		void				_ResetSearchResults();

		K_SATWindow*			fSATWindow;

		K_SATGroup*			fFreeAreaGroup;
		K_Tab*				fFreeAreaLeft;
		K_Tab*				fFreeAreaRight;
		K_Tab*				fFreeAreaTop;
		K_Tab*				fFreeAreaBottom;
};

#endif
