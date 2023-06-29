/*
 * Copyright 2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */

#include "KTiling.h"


#include "KSATWindow.h"
#include "StackAndTile.h"
#include "KWindow.h"


using namespace std;


//#define DEBUG_TILING

#ifdef DEBUG_TILING
#	define STRACE_TILING(x...) debug_printf("SAT Tiling: "x)
#else
#	define STRACE_TILING(x...) ;
#endif


K_SATTiling::K_SATTiling(K_SATWindow* window)
	:
	fSATWindow(window),
	fFreeAreaGroup(NULL)
{
	_ResetSearchResults();
}


K_SATTiling::~K_SATTiling()
{

}


bool
K_SATTiling::FindSnappingCandidates(K_SATGroup* group)
{
	_ResetSearchResults();

	if (_IsTileableWindow(fSATWindow->GetWindow()) == false
		|| (group->CountItems() == 1
			&& _IsTileableWindow(group->WindowAt(0)->GetWindow()) == false))
		return false;
	if (fSATWindow->GetGroup() == group)
		return false;

	if (_FindFreeAreaInGroup(group)) {
		fFreeAreaGroup = group;
		_HighlightWindows(fFreeAreaGroup, true);
		return true;
	}

	return false;
}

//total virtual start
/*bool
K_SATTiling::K_FindSnappingCandidates(K_SATGroup* group)
{
	_ResetSearchResults();

	if (_IsTileableWindow(fSATWindow->GetWindow()) == false
		|| (group->CountItems() == 1
			&& _IsTileableWindow(group->WindowAt(0)->GetWindow()) == false))
		return false;
	if (fSATWindow->GetGroup() == group)
		return false;

	if (_FindFreeAreaInGroup(group)) {
		fFreeAreaGroup = group;
		_HighlightWindows(fFreeAreaGroup, true);
		return true;
	}

	return false;
}*/
//end


bool
K_SATTiling::JoinCandidates()
{
	if (!fFreeAreaGroup)
		return false;

	if (!fFreeAreaGroup->AddWindow(fSATWindow, fFreeAreaLeft, fFreeAreaTop,
		fFreeAreaRight, fFreeAreaBottom)) {
		_ResetSearchResults();
		return false;
	}

	fFreeAreaGroup->WindowAt(0)->DoGroupLayout();

	_ResetSearchResults();
	return true;
}


void
K_SATTiling::WindowLookChanged(window_look look)
{
	K_SATGroup* group = fSATWindow->GetGroup();
	if (group == NULL)
		return;
	if (_IsTileableWindow(fSATWindow->GetWindow()) == false)
		group->RemoveWindow(fSATWindow);
}


bool
K_SATTiling::_IsTileableWindow(K_Window* window)
{
	if (window->Look() == B_DOCUMENT_WINDOW_LOOK)
		return true;
	if (window->Look() == B_TITLED_WINDOW_LOOK)
		return true;
	if (window->Look() == B_FLOATING_WINDOW_LOOK)
		return true;
	if (window->Look() == B_MODAL_WINDOW_LOOK)
		return true;
	if (window->Look() == B_BORDERED_WINDOW_LOOK)
		return true;
	return false;	
}


bool
K_SATTiling::_FindFreeAreaInGroup(K_SATGroup* group)
{
	if (_FindFreeAreaInGroup(group, K_Corner::kLeftTop))
		return true;
	if (_FindFreeAreaInGroup(group, K_Corner::kRightTop))
		return true;
	if (_FindFreeAreaInGroup(group, K_Corner::kLeftBottom))
		return true;
	if (_FindFreeAreaInGroup(group, K_Corner::kRightBottom))
		return true;

	return false;
}


bool
K_SATTiling::_FindFreeAreaInGroup(K_SATGroup* group, K_Corner::position_t cor)
{
	BRect windowFrame = fSATWindow->CompleteWindowFrame();

	const K_TabList* verticalTabs = group->VerticalTabs();
	for (int i = 0; i < verticalTabs->CountItems(); i++) {
		K_Tab* tab = verticalTabs->ItemAt(i);
		const K_CrossingList* K_crossingList = tab->GetCrossingList();
		for (int c = 0; c < K_crossingList->CountItems(); c++) {
			K_Crossing* crossing = K_crossingList->ItemAt(c);
			if (_InteresstingCrossing(crossing, cor, windowFrame)) {
				if (_FindFreeArea(group, crossing, cor, windowFrame)) {
					STRACE_TILING("K_SATTiling: free area found; corner %i\n",
						cor);
					return true;
				}
			}
		}
	}

	return false;
}


const float kNoMatch = 999.f;
const float kMaxMatchingDistance = 12.f;


bool
K_SATTiling::_InteresstingCrossing(K_Crossing* crossing,
	K_Corner::position_t cor, BRect& windowFrame)
{
	const K_Corner* corner = crossing->GetOppositeCorner(cor);
	if (corner->status != K_Corner::kFree)
		return false;

	float hTabPosition = crossing->HorizontalTab()->Position();
	float vTabPosition = crossing->VerticalTab()->Position();
	float hBorder = 0., vBorder = 0.;
	float vDistance = -1., hDistance = -1.;
	bool windowAtH = false, windowAtV = false;
	switch (cor) {
		case K_Corner::kLeftTop:
			if (crossing->RightBottomCorner()->status == K_Corner::kUsed)
				return false;
			vBorder = windowFrame.left;
			hBorder = windowFrame.top;
			if (crossing->LeftBottomCorner()->status == K_Corner::kUsed)
				windowAtV = true;
			if (crossing->RightTopCorner()->status == K_Corner::kUsed)
				windowAtH = true;
			vDistance = vTabPosition - vBorder;
			hDistance = hTabPosition - hBorder;
			break;
		case K_Corner::kRightTop:
			if (crossing->LeftBottomCorner()->status == K_Corner::kUsed)
				return false;
			vBorder = windowFrame.right;
			hBorder = windowFrame.top;
			if (crossing->RightBottomCorner()->status == K_Corner::kUsed)
				windowAtV = true;
			if (crossing->LeftTopCorner()->status == K_Corner::kUsed)
				windowAtH = true;
			vDistance = vBorder - vTabPosition;
			hDistance = hTabPosition - hBorder;
			break;
		case K_Corner::kLeftBottom:
			if (crossing->RightTopCorner()->status == K_Corner::kUsed)
				return false;
			vBorder = windowFrame.left;
			hBorder = windowFrame.bottom;
			if (crossing->LeftTopCorner()->status == K_Corner::kUsed)
				windowAtV = true;
			if (crossing->RightBottomCorner()->status == K_Corner::kUsed)
				windowAtH = true;
			vDistance = vTabPosition - vBorder;
			hDistance = hBorder - hTabPosition;
			break;
		case K_Corner::kRightBottom:
			if (crossing->LeftTopCorner()->status == K_Corner::kUsed)
				return false;
			vBorder = windowFrame.right;
			hBorder = windowFrame.bottom;
			if (crossing->RightTopCorner()->status == K_Corner::kUsed)
				windowAtV = true;
			if (crossing->LeftBottomCorner()->status == K_Corner::kUsed)
				windowAtH = true;
			vDistance = vBorder - vTabPosition;
			hDistance = hBorder - hTabPosition;
			break;
	};

	bool hValid = false;
	if (windowAtH && fabs(hDistance) < kMaxMatchingDistance
		&& vDistance  < kMaxMatchingDistance)
		hValid = true;
	bool vValid = false;
	if (windowAtV && fabs(vDistance) < kMaxMatchingDistance
		&& hDistance  < kMaxMatchingDistance)
		vValid = true;
	if (!hValid && !vValid)
		return false;

	return true;
};


const float kBigAreaError = 1E+17;


bool
K_SATTiling::_FindFreeArea(K_SATGroup* group, const K_Crossing* crossing,
	K_Corner::position_t corner, BRect& windowFrame)
{
	fFreeAreaLeft = fFreeAreaRight = fFreeAreaTop = fFreeAreaBottom = NULL;

	const K_TabList* hTabs = group->HorizontalTabs();
	const K_TabList* vTabs = group->VerticalTabs();
	int32 hIndex = hTabs->IndexOf(crossing->HorizontalTab());
	if (hIndex < 0)
		return false;
	int32 vIndex = vTabs->IndexOf(crossing->VerticalTab());
	if (vIndex < 0)
		return false;

	K_Tab** endHTab = NULL, **endVTab = NULL;
	int8 vSearchDirection = 1, hSearchDirection = 1;
	switch (corner) {
		case K_Corner::kLeftTop:
			fFreeAreaLeft = crossing->VerticalTab();
			fFreeAreaTop = crossing->HorizontalTab();
			endHTab = &fFreeAreaBottom;
			endVTab = &fFreeAreaRight;
			vSearchDirection = 1;
			hSearchDirection = 1;
			break;
		case K_Corner::kRightTop:
			fFreeAreaRight = crossing->VerticalTab();
			fFreeAreaTop = crossing->HorizontalTab();
			endHTab = &fFreeAreaBottom;
			endVTab = &fFreeAreaLeft;
			vSearchDirection = -1;
			hSearchDirection = 1;
			break;
		case K_Corner::kLeftBottom:
			fFreeAreaLeft = crossing->VerticalTab();
			fFreeAreaBottom = crossing->HorizontalTab();
			endHTab = &fFreeAreaTop;
			endVTab = &fFreeAreaRight;
			vSearchDirection = 1;
			hSearchDirection = -1;
			break;
		case K_Corner::kRightBottom:
			fFreeAreaRight = crossing->VerticalTab();
			fFreeAreaBottom = crossing->HorizontalTab();
			endHTab = &fFreeAreaTop;
			endVTab = &fFreeAreaLeft;
			vSearchDirection = -1;
			hSearchDirection = -1;
			break;
	};

	K_Tab* bestLeftTab = NULL, *bestRightTab = NULL, *bestTopTab = NULL,
		*bestBottomTab = NULL;
	float bestError = kBigAreaError;
	float error;
	bool stop = false;
	bool found = false;
	int32 v = vIndex;
	do {
		v += vSearchDirection;
		*endVTab = vTabs->ItemAt(v);
		int32 h = hIndex;
		do {
			h += hSearchDirection;
			*endHTab = hTabs->ItemAt(h);
			if (!_CheckArea(group, corner, windowFrame, error)) {
				if (h == hIndex + hSearchDirection)
					stop = true;
				break;
			}
			found = true;
			if (error < bestError) {
				bestError = error;
				bestLeftTab = fFreeAreaLeft;
				bestRightTab = fFreeAreaRight;
				bestTopTab = fFreeAreaTop;
				bestBottomTab = fFreeAreaBottom;
			}
		} while (*endHTab);
		if (stop)
			break;
	} while (*endVTab);
	if (!found)
		return false;

	fFreeAreaLeft = bestLeftTab;
	fFreeAreaRight = bestRightTab;
	fFreeAreaTop = bestTopTab;
	fFreeAreaBottom = bestBottomTab;

	return true;
}


bool
K_SATTiling::_HasOverlapp(K_SATGroup* group)
{
	BRect areaRect = _FreeAreaSize();
	areaRect.InsetBy(1., 1.);

	const K_TabList* hTabs = group->HorizontalTabs();
	for (int32 h = 0; h < hTabs->CountItems(); h++) {
		K_Tab* hTab = hTabs->ItemAt(h);
		if (hTab->Position() >= areaRect.bottom)
			return false;
		const K_CrossingList* crossings = hTab->GetCrossingList();
		for (int32 i = 0; i <  crossings->CountItems(); i++) {
			K_Crossing* leftTopCrossing = crossings->ItemAt(i);
			K_Tab* vTab = leftTopCrossing->VerticalTab();
			if (vTab->Position() > areaRect.right)
				continue;
			K_Corner* corner = leftTopCrossing->RightBottomCorner();
			if (corner->status != K_Corner::kUsed)
				continue;
			BRect rect = corner->windowArea->Frame();
			if (areaRect.Intersects(rect))
				return true;
		}
	}
	return false;
}


bool
K_SATTiling::_CheckArea(K_SATGroup* group, K_Corner::position_t corner,
	BRect& windowFrame, float& error)
{
	error = kBigAreaError;
	if (!_CheckMinFreeAreaSize())
		return false;
	// check if corner is in the free area
	if (!_IsCornerInFreeArea(corner, windowFrame))
		return false;
	
	error = _FreeAreaError(windowFrame);
	if (!_HasOverlapp(group))
		return true;
	return false;
}


bool
K_SATTiling::_CheckMinFreeAreaSize()
{
	// check if the area has a minimum size
	if (fFreeAreaLeft && fFreeAreaRight
		&& fFreeAreaRight->Position() - fFreeAreaLeft->Position()
			< 2 * kMaxMatchingDistance)
		return false;
	if (fFreeAreaBottom && fFreeAreaTop
		&& fFreeAreaBottom->Position() - fFreeAreaTop->Position()
			< 2 * kMaxMatchingDistance)
		return false;
	return true;
}


float
K_SATTiling::_FreeAreaError(BRect& windowFrame)
{
	const float kEndTabError = 9999999;
	float error = 0.;
	if (fFreeAreaLeft && fFreeAreaRight)
		error += pow(fFreeAreaRight->Position() - fFreeAreaLeft->Position()
			- windowFrame.Width(), 2);
	else
		error += kEndTabError;
	if (fFreeAreaBottom && fFreeAreaTop)
		error += pow(fFreeAreaBottom->Position() - fFreeAreaTop->Position()
			- windowFrame.Height(), 2);
	else
		error += kEndTabError;
	return error;
}


bool
K_SATTiling::_IsCornerInFreeArea(K_Corner::position_t corner, BRect& frame)
{
	BRect freeArea = _FreeAreaSize();

	switch (corner) {
		case K_Corner::kLeftTop:
			if (freeArea.bottom - kMaxMatchingDistance > frame.top
				&& freeArea.right - kMaxMatchingDistance > frame.left)
				return true;
			break;
		case K_Corner::kRightTop:
			if (freeArea.bottom - kMaxMatchingDistance > frame.top
				&& freeArea.left + kMaxMatchingDistance < frame.right)
				return true;
			break;
		case K_Corner::kLeftBottom:
			if (freeArea.top + kMaxMatchingDistance < frame.bottom
				&& freeArea.right - kMaxMatchingDistance > frame.left)
				return true;
			break;
		case K_Corner::kRightBottom:
			if (freeArea.top + kMaxMatchingDistance < frame.bottom
				&& freeArea.left + kMaxMatchingDistance < frame.right)
				return true;
			break;
	}

	return false;
}


BRect
K_SATTiling::_FreeAreaSize()
{
	// not to big to be be able to add/sub small float values
	const float kBigValue = 9999999.;
	float left = fFreeAreaLeft ? fFreeAreaLeft->Position() : -kBigValue;
	float right = fFreeAreaRight ? fFreeAreaRight->Position() : kBigValue;
	float top = fFreeAreaTop ? fFreeAreaTop->Position() : -kBigValue;
	float bottom = fFreeAreaBottom ? fFreeAreaBottom->Position() : kBigValue;
	return BRect(left, top, right, bottom);
}


void
K_SATTiling::_HighlightWindows(K_SATGroup* group, bool highlight)
{
	const K_TabList* hTabs = group->HorizontalTabs();
	const K_TabList* vTabs = group->VerticalTabs();
	// height light windows at all four sites
	bool leftWindowsFound = _SearchHighlightWindow(fFreeAreaLeft, fFreeAreaTop, fFreeAreaBottom, hTabs,
		fFreeAreaTop ? K_Corner::kLeftBottom : K_Corner::kLeftTop,
		K_Decorator::REGION_RIGHT_BORDER, highlight);

	bool topWindowsFound = _SearchHighlightWindow(fFreeAreaTop, fFreeAreaLeft, fFreeAreaRight, vTabs,
		fFreeAreaLeft ? K_Corner::kRightTop : K_Corner::kLeftTop,
		K_Decorator::REGION_BOTTOM_BORDER, highlight);

	bool rightWindowsFound = _SearchHighlightWindow(fFreeAreaRight, fFreeAreaTop, fFreeAreaBottom, hTabs,
		fFreeAreaTop ? K_Corner::kRightBottom : K_Corner::kRightTop,
		K_Decorator::REGION_LEFT_BORDER, highlight);

	bool bottomWindowsFound = _SearchHighlightWindow(fFreeAreaBottom, fFreeAreaLeft, fFreeAreaRight,
		vTabs, fFreeAreaLeft ? K_Corner::kRightBottom : K_Corner::kLeftBottom,
		K_Decorator::REGION_TOP_BORDER, highlight);

	if (leftWindowsFound)
		fSATWindow->HighlightBorders(K_Decorator::REGION_LEFT_BORDER, highlight);
	if (topWindowsFound)
		fSATWindow->HighlightBorders(K_Decorator::REGION_TOP_BORDER, highlight);
	if (rightWindowsFound)
		fSATWindow->HighlightBorders(K_Decorator::REGION_RIGHT_BORDER, highlight);
	if (bottomWindowsFound) {
		fSATWindow->HighlightBorders(K_Decorator::REGION_BOTTOM_BORDER,
			highlight);
	}
}


bool
K_SATTiling::_SearchHighlightWindow(K_Tab* tab, K_Tab* firstOrthTab,
	K_Tab* secondOrthTab, const K_TabList* orthTabs, K_Corner::position_t areaCorner,
	K_Decorator::Region region, bool highlight)
{
	bool windowsFound = false;

	if (!tab)
		return false;

	int8 searchDir = 1;
	K_Tab* startOrthTab = NULL;
	K_Tab* endOrthTab = NULL;
	if (firstOrthTab) {
		searchDir = 1;
		startOrthTab = firstOrthTab;
		endOrthTab = secondOrthTab;
	}
	else if (secondOrthTab) {
		searchDir = -1;
		startOrthTab = secondOrthTab;
		endOrthTab = firstOrthTab;
	}
	else
		return false;

	int32 index = orthTabs->IndexOf(startOrthTab);
	if (index < 0)
		return false;

	for (; index < orthTabs->CountItems() && index >= 0; index += searchDir) {
		K_Tab* orthTab = orthTabs->ItemAt(index);
		if (orthTab == endOrthTab)
 			break;
		K_Crossing* crossing = tab->FindCrossing(orthTab);
		if (!crossing)
			continue;
		K_Corner* corner = crossing->GetCorner(areaCorner);
		if (corner->windowArea) {
			_HighlightWindows(corner->windowArea, region,  highlight);
			windowsFound = true;
		}
	}
	return windowsFound;
}


void
K_SATTiling::_HighlightWindows(K_WindowArea* area, K_Decorator::Region region,
	bool highlight)
{
	const K_SATWindowList& list = area->LayerOrder();
	K_SATWindow* topWindow = list.ItemAt(list.CountItems() - 1);
	if (topWindow == NULL)
		return;
	topWindow->HighlightBorders(region, highlight);
}


void
K_SATTiling::_ResetSearchResults()
{
	if (!fFreeAreaGroup)
		return;

	_HighlightWindows(fFreeAreaGroup, false);
	fFreeAreaGroup = NULL;
}
