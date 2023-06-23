/*
 * Copyright 2010-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */


#include "KSATGroup.h"

#include <vector>

#include <Debug.h>
#include <Message.h>

#include "Desktop.h"

#include "KSATWindow.h"
#include "KStackAndTile.h"
#include "Window.h"


using namespace std;
using namespace LinearProgramming;

//khidki code
//start
//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TTRACE(x) debug_printf x
#else
#	define TTRACE(x) ;
#endif
//end

const float kExtentPenalty = 1;
const float kHighPenalty = 100;
const float kInequalityPenalty = 10000;


K_WindowArea::K_WindowArea(K_Crossing* leftTop, K_Crossing* rightTop,
	K_Crossing* leftBottom, K_Crossing* rightBottom)
	:
	fGroup(NULL),

	fLeftTopCrossing(leftTop),
	fRightTopCrossing(rightTop),
	fLeftBottomCrossing(leftBottom),
	fRightBottomCrossing(rightBottom),

	fMinWidthConstraint(NULL),
	fMinHeightConstraint(NULL),
	fMaxWidthConstraint(NULL),
	fMaxHeightConstraint(NULL),
	fWidthConstraint(NULL),
	fHeightConstraint(NULL)
{
}


K_WindowArea::~K_WindowArea()
{
	if (fGroup)
		fGroup->WindowAreaRemoved(this);

	_CleanupCorners();
	fGroup->fWindowAreaList.RemoveItem(this);

	_UninitConstraints();
}


bool
K_WindowArea::Init(K_SATGroup* group)
{
	_UninitConstraints();

	if (group == NULL || group->fWindowAreaList.AddItem(this) == false)
		return false;

	fGroup = group;

	LinearSpec* linearSpec = fGroup->GetLinearSpec();

	fMinWidthConstraint = linearSpec->AddConstraint(1.0, RightVar(), -1.0,
		LeftVar(), kGE, 0);
	fMinHeightConstraint = linearSpec->AddConstraint(1.0, BottomVar(), -1.0,
		TopVar(), kGE, 0);

	fMaxWidthConstraint = linearSpec->AddConstraint(1.0, RightVar(), -1.0,
		LeftVar(), kLE, 0, kInequalityPenalty, kInequalityPenalty);
	fMaxHeightConstraint = linearSpec->AddConstraint(1.0, BottomVar(), -1.0,
		TopVar(), kLE, 0, kInequalityPenalty, kInequalityPenalty);

	// Width and height have soft constraints
	fWidthConstraint = linearSpec->AddConstraint(1.0, RightVar(), -1.0,
		LeftVar(), kEQ, 0, kExtentPenalty,
		kExtentPenalty);
	fHeightConstraint = linearSpec->AddConstraint(-1.0, TopVar(), 1.0,
		BottomVar(), kEQ, 0, kExtentPenalty,
		kExtentPenalty);

	if (!fMinWidthConstraint || !fMinHeightConstraint || !fWidthConstraint
		|| !fHeightConstraint || !fMaxWidthConstraint
		|| !fMaxHeightConstraint)
		return false;

	return true;
}


void
K_WindowArea::DoGroupLayout()
{
	K_SATWindow* parentWindow = fWindowLayerOrder.ItemAt(0);
	if (parentWindow == NULL)
		return;

	BRect frame = parentWindow->CompleteWindowFrame();
	// Make it also work for solver which don't support negative variables
	frame.OffsetBy(k_kMakePositiveOffset, k_kMakePositiveOffset);

	// adjust window size soft constraints
	fWidthConstraint->SetRightSide(frame.Width());
	fHeightConstraint->SetRightSide(frame.Height());

	LinearSpec* linearSpec = fGroup->GetLinearSpec();
	Constraint* leftConstraint = linearSpec->AddConstraint(1.0, LeftVar(),
		kEQ, frame.left);
	Constraint* topConstraint = linearSpec->AddConstraint(1.0, TopVar(), kEQ,
		frame.top);

	// give soft constraints a high penalty
	fWidthConstraint->SetPenaltyNeg(kHighPenalty);
	fWidthConstraint->SetPenaltyPos(kHighPenalty);
	fHeightConstraint->SetPenaltyNeg(kHighPenalty);
	fHeightConstraint->SetPenaltyPos(kHighPenalty);

	// After we set the new parameter solve and apply the new layout.
	ResultType result;
	for (int32 tries = 0; tries < 15; tries++) {
		result = fGroup->GetLinearSpec()->Solve();
		if (result == kInfeasible) {
			debug_printf("can't solve constraints!\n");
			break;
		}
		if (result == kOptimal) {
			const K_WindowAreaList& areas = fGroup->GetAreaList();
			for (int32 i = 0; i < areas.CountItems(); i++) {
				K_WindowArea* area = areas.ItemAt(i);
				area->_MoveToSAT(parentWindow);
			}
			break;
		}
	}

	// set penalties back to normal
	fWidthConstraint->SetPenaltyNeg(kExtentPenalty);
	fWidthConstraint->SetPenaltyPos(kExtentPenalty);
	fHeightConstraint->SetPenaltyNeg(kExtentPenalty);
	fHeightConstraint->SetPenaltyPos(kExtentPenalty);

	linearSpec->RemoveConstraint(leftConstraint);
	linearSpec->RemoveConstraint(topConstraint);
}

//khidki start
/*void
K_WindowArea::K_DoGroupLayout()
{
	K_SATWindow* parentWindow = k_fWindowLayerOrder.ItemAt(0);
	if (parentWindow == NULL)
		return;

	BRect frame = parentWindow->CompleteWindowFrame();
	// Make it also work for solver which don't support negative variables
	frame.OffsetBy(k_kMakePositiveOffset, k_kMakePositiveOffset);

	// adjust window size soft constraints
	fWidthConstraint->SetRightSide(frame.Width());
	fHeightConstraint->SetRightSide(frame.Height());

	LinearSpec* linearSpec = fGroup->GetLinearSpec();
	Constraint* leftConstraint = linearSpec->AddConstraint(1.0, LeftVar(),
		kEQ, frame.left);
	Constraint* topConstraint = linearSpec->AddConstraint(1.0, TopVar(), kEQ,
		frame.top);

	// give soft constraints a high penalty
	fWidthConstraint->SetPenaltyNeg(kHighPenalty);
	fWidthConstraint->SetPenaltyPos(kHighPenalty);
	fHeightConstraint->SetPenaltyNeg(kHighPenalty);
	fHeightConstraint->SetPenaltyPos(kHighPenalty);

	// After we set the new parameter solve and apply the new layout.
	ResultType result;
	for (int32 tries = 0; tries < 15; tries++) {
		result = fGroup->GetLinearSpec()->Solve();
		if (result == kInfeasible) {
			debug_printf("can't solve constraints!\n");
			break;
		}
		if (result == kOptimal) {
			const K_WindowAreaList& areas = fGroup->GetAreaList();
			for (int32 i = 0; i < areas.CountItems(); i++) {
				K_WindowArea* area = areas.ItemAt(i);
				area->_MoveToSAT(parentWindow);
			}
			break;
		}
	}

	// set penalties back to normal
	fWidthConstraint->SetPenaltyNeg(kExtentPenalty);
	fWidthConstraint->SetPenaltyPos(kExtentPenalty);
	fHeightConstraint->SetPenaltyNeg(kExtentPenalty);
	fHeightConstraint->SetPenaltyPos(kExtentPenalty);

	linearSpec->RemoveConstraint(leftConstraint);
	linearSpec->RemoveConstraint(topConstraint);
}
*/
//end


void
K_WindowArea::UpdateSizeLimits()
{
	_UpdateConstraintValues();
}


void
K_WindowArea::UpdateSizeConstaints(const BRect& frame)
{
	// adjust window size soft constraints
	fWidthConstraint->SetRightSide(frame.Width());
	fHeightConstraint->SetRightSide(frame.Height());
}


bool
K_WindowArea::MoveWindowToPosition(K_SATWindow* window, int32 index)
{
	int32 oldIndex = fWindowList.IndexOf(window);
	ASSERT(oldIndex != index);
	return fWindowList.MoveItem(oldIndex, index);
}


K_SATWindow*
K_WindowArea::TopWindow()
{
debug_printf("[K_WindowArea] {TopWindow}\n");

	return fWindowLayerOrder.ItemAt(fWindowLayerOrder.CountItems() - 1);
}

//khidki start
/*K_SATWindow*
K_WindowArea::K_TopWindow()
{
debug_printf("[K_WindowArea] {K_TopWindow}\n");

	return k_fWindowLayerOrder.ItemAt(k_fWindowLayerOrder.CountItems() - 1);
}*/
//end

void
K_WindowArea::_UpdateConstraintValues()
{
debug_printf("[K_WindowArea] {_UpdateConstraintValues}\n");
	K_SATWindow* topWindow = TopWindow();
	if (topWindow == NULL)
		return;

	int32 minWidth, maxWidth;
	int32 minHeight, maxHeight;
	K_SATWindow* window = fWindowList.ItemAt(0);
	window->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	for (int32 i = 1; i < fWindowList.CountItems(); i++) {
		window = fWindowList.ItemAt(i);
		// size limit constraints
		int32 minW, maxW;
		int32 minH, maxH;
		window->GetSizeLimits(&minW, &maxW, &minH, &maxH);
		if (minWidth < minW)
			minWidth = minW;
		if (minHeight < minH)
			minHeight = minH;
		if (maxWidth < maxW)
			maxWidth = maxW;
		if (maxHeight < maxH)
			maxHeight = maxH;
	}
	// the current solver don't like big values
	const int32 kMaxSolverValue = 5000;
	if (minWidth > kMaxSolverValue)
		minWidth = kMaxSolverValue;
	if (minHeight > kMaxSolverValue)
		minHeight = kMaxSolverValue;
	if (maxWidth > kMaxSolverValue)
		maxWidth = kMaxSolverValue;
	if (maxHeight > kMaxSolverValue)
		maxHeight = kMaxSolverValue;

	topWindow->AddDecorator(&minWidth, &maxWidth, &minHeight, &maxHeight);
	fMinWidthConstraint->SetRightSide(minWidth);
	fMinHeightConstraint->SetRightSide(minHeight);

	fMaxWidthConstraint->SetRightSide(maxWidth);
	fMaxHeightConstraint->SetRightSide(maxHeight);

	BRect frame = topWindow->CompleteWindowFrame();
	fWidthConstraint->SetRightSide(frame.Width());
	fHeightConstraint->SetRightSide(frame.Height());
}

/*
//khidkki start
void
K_WindowArea::K__UpdateConstraintValues()
{
debug_printf("[K_WindowArea] {K__UpdateConstraintValues}\n");
	K_SATWindow* topWindow = K_TopWindow();
	if (topWindow == NULL)
		return;

	int32 minWidth, maxWidth;
	int32 minHeight, maxHeight;
	K_SATWindow* window = k_fWindowList.ItemAt(0);
	window->GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	for (int32 i = 1; i < k_fWindowList.CountItems(); i++) {
		window = k_fWindowList.ItemAt(i);
		// size limit constraints
		int32 minW, maxW;
		int32 minH, maxH;
		window->GetSizeLimits(&minW, &maxW, &minH, &maxH);
		if (minWidth < minW)
			minWidth = minW;
		if (minHeight < minH)
			minHeight = minH;
		if (maxWidth < maxW)
			maxWidth = maxW;
		if (maxHeight < maxH)
			maxHeight = maxH;
	}
	// the current solver don't like big values
	const int32 kMaxSolverValue = 5000;
	if (minWidth > kMaxSolverValue)
		minWidth = kMaxSolverValue;
	if (minHeight > kMaxSolverValue)
		minHeight = kMaxSolverValue;
	if (maxWidth > kMaxSolverValue)
		maxWidth = kMaxSolverValue;
	if (maxHeight > kMaxSolverValue)
		maxHeight = kMaxSolverValue;

	topWindow->AddDecorator(&minWidth, &maxWidth, &minHeight, &maxHeight);
	fMinWidthConstraint->SetRightSide(minWidth);
	fMinHeightConstraint->SetRightSide(minHeight);

	fMaxWidthConstraint->SetRightSide(maxWidth);
	fMaxHeightConstraint->SetRightSide(maxHeight);

	BRect frame = topWindow->CompleteWindowFrame();
	fWidthConstraint->SetRightSide(frame.Width());
	fHeightConstraint->SetRightSide(frame.Height());
}
*/
//end

bool
K_WindowArea::_AddWindow(K_SATWindow* window, K_SATWindow* after)
{
	if (after) {
		int32 indexAfter = fWindowList.IndexOf(after);
		if (!fWindowList.AddItem(window, indexAfter + 1))
			return false;
	} else if (fWindowList.AddItem(window) == false)
		return false;

	AcquireReference();

	if (fWindowList.CountItems() <= 1)
		_InitCorners();

	fWindowLayerOrder.AddItem(window);

	_UpdateConstraintValues();
	return true;
}


//khidki start
/*
bool
K_WindowArea::K__AddWindow(K_SATWindow* window, K_SATWindow* after)
{
	if (after) {
		int32 indexAfter = k_fWindowList.IndexOf(after);
		if (!k_fWindowList.AddItem(window, indexAfter + 1))
			return false;
	} else if (k_fWindowList.AddItem(window) == false)
		return false;

	AcquireReference();

	if (k_fWindowList.CountItems() <= 1)
		_InitCorners();

	fWindowLayerOrder.AddItem(window);

	_UpdateConstraintValues();
	return true;
}
*/
//end


bool
K_WindowArea::_RemoveWindow(K_SATWindow* window)
{
	if (!fWindowList.RemoveItem(window))
		return false;

	fWindowLayerOrder.RemoveItem(window);
	_UpdateConstraintValues();

	window->RemovedFromArea(this);
	ReleaseReference();
	return true;
}


//khidki start
/*
bool
K_WindowArea::K__RemoveWindow(K_SATWindow* window)
{
debug_printf("[K_SATGroup] {K__RemoveWindow}\n");
	if (!fWindowList.RemoveItem(window))
		return false;

	fWindowLayerOrder.RemoveItem(window);
	_UpdateConstraintValues();

	window->RemovedFromArea(this);
	ReleaseReference();
	return true;
}
*/

//end

K_Tab*
K_WindowArea::LeftTab()
{
	return fLeftTopCrossing->VerticalTab();
}


K_Tab*
K_WindowArea::RightTab()
{
	return fRightBottomCrossing->VerticalTab();
}


K_Tab*
K_WindowArea::TopTab()
{
	return fLeftTopCrossing->HorizontalTab();
}


K_Tab*
K_WindowArea::BottomTab()
{
	return fRightBottomCrossing->HorizontalTab();
}


BRect
K_WindowArea::Frame()
{
	return BRect(fLeftTopCrossing->VerticalTab()->Position(),
		fLeftTopCrossing->HorizontalTab()->Position(),
		fRightBottomCrossing->VerticalTab()->Position(),
		fRightBottomCrossing->HorizontalTab()->Position());
}


bool
K_WindowArea::PropagateToGroup(K_SATGroup* group)
{
	BReference<K_Crossing> newLeftTop = _CrossingByPosition(fLeftTopCrossing,
		group);
	BReference<K_Crossing> newRightTop = _CrossingByPosition(fRightTopCrossing,
		group);
	BReference<K_Crossing> newLeftBottom = _CrossingByPosition(
		fLeftBottomCrossing, group);
	BReference<K_Crossing> newRightBottom = _CrossingByPosition(
		fRightBottomCrossing, group);

	if (!newLeftTop || !newRightTop || !newLeftBottom || !newRightBottom)
		return false;

	// hold a ref to the crossings till we cleaned up everything
	BReference<K_Crossing> oldLeftTop = fLeftTopCrossing;
	BReference<K_Crossing> oldRightTop = fRightTopCrossing;
	BReference<K_Crossing> oldLeftBottom = fLeftBottomCrossing;
	BReference<K_Crossing> oldRightBottom = fRightBottomCrossing;

	fLeftTopCrossing = newLeftTop;
	fRightTopCrossing = newRightTop;
	fLeftBottomCrossing = newLeftBottom;
	fRightBottomCrossing = newRightBottom;

	_InitCorners();

	BReference<K_SATGroup> oldGroup = fGroup;
	// manage constraints
	if (Init(group) == false)
		return false;

	oldGroup->fWindowAreaList.RemoveItem(this);
	for (int32 i = 0; i < fWindowList.CountItems(); i++) {
		K_SATWindow* window = fWindowList.ItemAt(i);
		if (oldGroup->fSATWindowList.RemoveItem(window) == false)
			return false;
		if (group->fSATWindowList.AddItem(window) == false) {
			_UninitConstraints();
			return false;
		}
	}

	_UpdateConstraintValues();

	return true;
}


bool
K_WindowArea::MoveToTopLayer(K_SATWindow* window)
{
	if (!fWindowLayerOrder.RemoveItem(window))
		return false;
	return fWindowLayerOrder.AddItem(window);
}


void
K_WindowArea::_UninitConstraints()
{
	if (fGroup != NULL) {
		LinearSpec* linearSpec = fGroup->GetLinearSpec();

		if (linearSpec != NULL) {
			linearSpec->RemoveConstraint(fMinWidthConstraint, true);
			linearSpec->RemoveConstraint(fMinHeightConstraint, true);
			linearSpec->RemoveConstraint(fMaxWidthConstraint, true);
			linearSpec->RemoveConstraint(fMaxHeightConstraint, true);
			linearSpec->RemoveConstraint(fWidthConstraint, true);
			linearSpec->RemoveConstraint(fHeightConstraint, true);
		}
	}

	fMinWidthConstraint = NULL;
	fMinHeightConstraint = NULL;
	fMaxWidthConstraint = NULL;
	fMaxHeightConstraint = NULL;
	fWidthConstraint = NULL;
	fHeightConstraint = NULL;
}


BReference<K_Crossing>
K_WindowArea::_CrossingByPosition(K_Crossing* crossing, K_SATGroup* group)
{
	BReference<K_Crossing> crossRef = NULL;

	K_Tab* oldHTab = crossing->HorizontalTab();
	BReference<K_Tab> hTab = group->FindHorizontalTab(oldHTab->Position());
	if (!hTab)
		hTab = group->_AddHorizontalTab(oldHTab->Position());
	if (!hTab)
		return crossRef;

	K_Tab* oldVTab = crossing->VerticalTab();
	crossRef = hTab->FindCrossing(oldVTab->Position());
	if (crossRef)
		return crossRef;

	BReference<K_Tab> vTab = group->FindVerticalTab(oldVTab->Position());
	if (!vTab)
		vTab = group->_AddVerticalTab(oldVTab->Position());
	if (!vTab)
		return crossRef;

	return hTab->AddCrossing(vTab);
}


void
K_WindowArea::_InitCorners()
{
	_SetToWindowCorner(fLeftTopCrossing->RightBottomCorner());
	_SetToNeighbourCorner(fLeftTopCrossing->LeftBottomCorner());
	_SetToNeighbourCorner(fLeftTopCrossing->RightTopCorner());

	_SetToWindowCorner(fRightTopCrossing->LeftBottomCorner());
	_SetToNeighbourCorner(fRightTopCrossing->LeftTopCorner());
	_SetToNeighbourCorner(fRightTopCrossing->RightBottomCorner());

	_SetToWindowCorner(fLeftBottomCrossing->RightTopCorner());
	_SetToNeighbourCorner(fLeftBottomCrossing->LeftTopCorner());
	_SetToNeighbourCorner(fLeftBottomCrossing->RightBottomCorner());

	_SetToWindowCorner(fRightBottomCrossing->LeftTopCorner());
	_SetToNeighbourCorner(fRightBottomCrossing->LeftBottomCorner());
	_SetToNeighbourCorner(fRightBottomCrossing->RightTopCorner());
}


void
K_WindowArea::_CleanupCorners()
{
	_UnsetWindowCorner(fLeftTopCrossing->RightBottomCorner());
	_UnsetNeighbourCorner(fLeftTopCrossing->LeftBottomCorner(),
		fLeftBottomCrossing->LeftTopCorner());
	_UnsetNeighbourCorner(fLeftTopCrossing->RightTopCorner(),
		fLeftBottomCrossing->LeftTopCorner());

	_UnsetWindowCorner(fRightTopCrossing->LeftBottomCorner());
	_UnsetNeighbourCorner(fRightTopCrossing->LeftTopCorner(),
		fLeftBottomCrossing->RightTopCorner());
	_UnsetNeighbourCorner(fRightTopCrossing->RightBottomCorner(),
		fLeftBottomCrossing->RightTopCorner());

	_UnsetWindowCorner(fLeftBottomCrossing->RightTopCorner());
	_UnsetNeighbourCorner(fLeftBottomCrossing->LeftTopCorner(),
		fLeftBottomCrossing->LeftBottomCorner());
	_UnsetNeighbourCorner(fLeftBottomCrossing->RightBottomCorner(),
		fLeftBottomCrossing->LeftBottomCorner());

	_UnsetWindowCorner(fRightBottomCrossing->LeftTopCorner());
	_UnsetNeighbourCorner(fRightBottomCrossing->LeftBottomCorner(),
		fRightBottomCrossing->RightBottomCorner());
	_UnsetNeighbourCorner(fRightBottomCrossing->RightTopCorner(),
		fRightBottomCrossing->RightBottomCorner());
}


void
K_WindowArea::_SetToWindowCorner(K_Corner* corner)
{
	corner->status = K_Corner::kUsed;
	corner->windowArea = this;
}


void
K_WindowArea::_SetToNeighbourCorner(K_Corner* neighbour)
{
	if (neighbour->status == K_Corner::kNotDockable)
		neighbour->status = K_Corner::kFree;
}


void
K_WindowArea::_UnsetWindowCorner(K_Corner* corner)
{
	corner->status = K_Corner::kFree;
	corner->windowArea = NULL;
}


void
K_WindowArea::_UnsetNeighbourCorner(K_Corner* neighbour, K_Corner* opponent)
{
	if (neighbour->status == K_Corner::kFree && opponent->status != K_Corner::kUsed)
		neighbour->status = K_Corner::kNotDockable;
}


void
K_WindowArea::_MoveToSAT(K_SATWindow* triggerWindow)
{
	K_SATWindow* topWindow = TopWindow();
	// if there is no window in the group we are done
	if (topWindow == NULL)
		return;

	BRect frameSAT(LeftVar()->Value() - k_kMakePositiveOffset,
		TopVar()->Value() - k_kMakePositiveOffset,
		RightVar()->Value() - k_kMakePositiveOffset,
		BottomVar()->Value() - k_kMakePositiveOffset);
	topWindow->AdjustSizeLimits(frameSAT);

	BRect frame = topWindow->CompleteWindowFrame();
	float deltaToX = round(frameSAT.left - frame.left);
	float deltaToY = round(frameSAT.top - frame.top);
	frame.OffsetBy(deltaToX, deltaToY);
	float deltaByX = round(frameSAT.right - frame.right);
	float deltaByY = round(frameSAT.bottom - frame.bottom);

	int32 workspace = triggerWindow->GetWindow()->CurrentWorkspace();
	Desktop* desktop = triggerWindow->GetWindow()->Desktop();
	desktop->K_MoveWindowBy(topWindow->GetWindow(), deltaToX, deltaToY,
		workspace);
	// Update frame to the new position
	desktop->K_ResizeWindowBy(topWindow->GetWindow(), deltaByX, deltaByY);

	UpdateSizeConstaints(frameSAT);
}


K_Corner::K_Corner()
	:
	status(kNotDockable),
	windowArea(NULL)
{

}


void
K_Corner::Trace() const
{
	switch (status) {
		case kFree:
			debug_printf("free corner\n");
			break;

		case kUsed:
		{
			debug_printf("attached windows:\n");
			const K_SATWindowList& list = windowArea->WindowList();
			for (int i = 0; i < list.CountItems(); i++) {
				debug_printf("- %s\n", list.ItemAt(i)->GetWindow()->Title());
			}
			break;
		}

		case kNotDockable:
			debug_printf("not dockable\n");
			break;
	};
}


K_Crossing::K_Crossing(K_Tab* vertical, K_Tab* horizontal)
	:
	fVerticalTab(vertical),
	fHorizontalTab(horizontal)
{
}


K_Crossing::~K_Crossing()
{
	fVerticalTab->RemoveCrossing(this);
	fHorizontalTab->RemoveCrossing(this);
}


K_Corner*
K_Crossing::GetCorner(K_Corner::position_t corner) const
{
	return &const_cast<K_Corner*>(fCorners)[corner];
}


K_Corner*
K_Crossing::GetOppositeCorner(K_Corner::position_t corner) const
{
	return &const_cast<K_Corner*>(fCorners)[3 - corner];
}


K_Tab*
K_Crossing::VerticalTab() const
{
	return fVerticalTab;
}


K_Tab*
K_Crossing::HorizontalTab() const
{
	return fHorizontalTab;
}


void
K_Crossing::Trace() const
{
	debug_printf("left-top corner: ");
	fCorners[K_Corner::kLeftTop].Trace();
	debug_printf("right-top corner: ");
	fCorners[K_Corner::kRightTop].Trace();
	debug_printf("left-bottom corner: ");
	fCorners[K_Corner::kLeftBottom].Trace();
	debug_printf("right-bottom corner: ");
	fCorners[K_Corner::kRightBottom].Trace();
}


K_Tab::K_Tab(K_SATGroup* group, Variable* variable, orientation_t orientation)
	:
	fGroup(group),
	fVariable(variable),
	fOrientation(orientation)
{

}


K_Tab::~K_Tab()
{
	if (fOrientation == kVertical)
		fGroup->_RemoveVerticalTab(this);
	else
		fGroup->_RemoveHorizontalTab(this);
}


float
K_Tab::Position() const
{
	return (float)fVariable->Value() - k_kMakePositiveOffset;
}


void
K_Tab::SetPosition(float position)
{
	fVariable->SetValue(position + k_kMakePositiveOffset);
}


K_Tab::orientation_t
K_Tab::Orientation() const
{
	return fOrientation;
}


Constraint*
K_Tab::Connect(Variable* variable)
{
	return fVariable->IsEqual(variable);
}


BReference<K_Crossing>
K_Tab::AddCrossing(K_Tab* tab)
{
	if (tab->Orientation() == fOrientation)
		return NULL;

	K_Tab* vTab = (fOrientation == kVertical) ? this : tab;
	K_Tab* hTab = (fOrientation == kHorizontal) ? this : tab;

	K_Crossing* crossing = new (std::nothrow)K_Crossing(vTab, hTab);
	if (!crossing)
		return NULL;

	if (!fCrossingList.AddItem(crossing)) {
		return NULL;
	}
	if (!tab->fCrossingList.AddItem(crossing)) {
		fCrossingList.RemoveItem(crossing);
		return NULL;
	}

	BReference<K_Crossing> crossingRef(crossing, true);
	return crossingRef;
}


bool
K_Tab::RemoveCrossing(K_Crossing* crossing)
{
	K_Tab* vTab = crossing->VerticalTab();
	K_Tab* hTab = crossing->HorizontalTab();

	if (vTab != this && hTab != this)
		return false;
	fCrossingList.RemoveItem(crossing);

	return true;
}


int32
K_Tab::FindCrossingIndex(K_Tab* tab)
{
	if (fOrientation == kVertical) {
		for (int32 i = 0; i < fCrossingList.CountItems(); i++) {
			if (fCrossingList.ItemAt(i)->HorizontalTab() == tab)
				return i;
		}
	} else {
		for (int32 i = 0; i < fCrossingList.CountItems(); i++) {
			if (fCrossingList.ItemAt(i)->VerticalTab() == tab)
				return i;
		}
	}
	return -1;
}


int32
K_Tab::FindCrossingIndex(float pos)
{
	if (fOrientation == kVertical) {
		for (int32 i = 0; i < fCrossingList.CountItems(); i++) {
			if (fabs(fCrossingList.ItemAt(i)->HorizontalTab()->Position() - pos)
				< 0.0001)
				return i;
		}
	} else {
		for (int32 i = 0; i < fCrossingList.CountItems(); i++) {
			if (fabs(fCrossingList.ItemAt(i)->VerticalTab()->Position() - pos)
				< 0.0001)
				return i;
		}
	}
	return -1;
}


K_Crossing*
K_Tab::FindCrossing(K_Tab* tab)
{
	return fCrossingList.ItemAt(FindCrossingIndex(tab));
}


K_Crossing*
K_Tab::FindCrossing(float tabPosition)
{
	return fCrossingList.ItemAt(FindCrossingIndex(tabPosition));
}


const K_CrossingList*
K_Tab::GetCrossingList() const
{
	return &fCrossingList;
}


int
K_Tab::CompareFunction(const K_Tab* tab1, const K_Tab* tab2)
{
	if (tab1->Position() < tab2->Position())
		return -1;

	return 1;
}


K_SATGroup::K_SATGroup()
	:
	fLinearSpec(new(std::nothrow) LinearSpec()),
	fHorizontalTabsSorted(false),
	fVerticalTabsSorted(false),
	fActiveWindow(NULL)
{
debug_printf("[K_SATGroup] {K_SATGroup constructor}\n");
}


K_SATGroup::~K_SATGroup()
{
	// Should be empty
	if (fSATWindowList.CountItems() > 0)
		debugger("Deleting a K_SATGroup which is not empty");
	//while (fSATWindowList.CountItems() > 0)
	//	RemoveWindow(fSATWindowList.ItemAt(0));

	fLinearSpec->ReleaseReference();
}


bool
K_SATGroup::AddWindow(K_SATWindow* window, K_Tab* left, K_Tab* top, K_Tab* right,
	K_Tab* bottom)
{
debug_printf("[K_SATGroup] {AddWindow}\n");
	STRACE_SAT("K_SATGroup::AddWindow\n");

	// first check if we have to create tabs and missing corners.
	BReference<K_Tab> leftRef, rightRef, topRef, bottomRef;
	BReference<K_Crossing> leftTopRef, rightTopRef, leftBottomRef, rightBottomRef;

	if (left != NULL && top != NULL)
		leftTopRef = left->FindCrossing(top);
	if (right != NULL && top != NULL)
		rightTopRef = right->FindCrossing(top);
	if (left != NULL && bottom != NULL)
		leftBottomRef = left->FindCrossing(bottom);
	if (right != NULL && bottom != NULL)
		rightBottomRef = right->FindCrossing(bottom);

	if (left == NULL) {
		leftRef = _AddVerticalTab();
		left = leftRef.Get();
	}
	if (top == NULL) {
		topRef = _AddHorizontalTab();
		top = topRef.Get();
	}
	if (right == NULL) {
		rightRef = _AddVerticalTab();
		right = rightRef.Get();
	}
	if (bottom == NULL) {
		bottomRef = _AddHorizontalTab();
		bottom = bottomRef.Get();
	}
	if (left == NULL || top == NULL || right == NULL || bottom == NULL)
		return false;

	if (leftTopRef == NULL) {
		leftTopRef = left->AddCrossing(top);
		if (leftTopRef == NULL)
			return false;
	}
	if (!rightTopRef) {
		rightTopRef = right->AddCrossing(top);
		if (!rightTopRef)
			return false;
	}
	if (!leftBottomRef) {
		leftBottomRef = left->AddCrossing(bottom);
		if (!leftBottomRef)
			return false;
	}
	if (!rightBottomRef) {
		rightBottomRef = right->AddCrossing(bottom);
		if (!rightBottomRef)
			return false;
	}

	K_WindowArea* area = new(std::nothrow) K_WindowArea(leftTopRef, rightTopRef,
		leftBottomRef, rightBottomRef);
	if (area == NULL)
		return false;
	// the area register itself in our area list
	if (area->Init(this) == false) {
		delete area;
		return false;
	}
	// delete the area if AddWindow failed / release our reference on it
	BReference<K_WindowArea> areaRef(area, true);

	return AddWindow(window, area);
}


bool
K_SATGroup::AddWindow(K_SATWindow* window, K_WindowArea* area, K_SATWindow* after)
{
	if (!area->_AddWindow(window, after))
		return false;

	if (!fSATWindowList.AddItem(window)) {
		area->_RemoveWindow(window);
		return false;
	}

	if (!window->AddedToGroup(this, area)) {
		area->_RemoveWindow(window);
		fSATWindowList.RemoveItem(window);
		return false;
	}

	return true;
}


//khidki start
/*
bool
K_SATGroup::K_AddWindow(K_SATWindow* window, K_Tab* left, K_Tab* top, K_Tab* right,
	K_Tab* bottom)
{
debug_printf("[K_SATGroup] {K_AddWindow}\n");
	STRACE_SAT("K_SATGroup::K_AddWindow\n");

	// first check if we have to create tabs and missing corners.
	BReference<K_Tab> leftRef, rightRef, topRef, bottomRef;
	BReference<K_Crossing> leftTopRef, rightTopRef, leftBottomRef, rightBottomRef;

	if (left != NULL && top != NULL)
		leftTopRef = left->FindCrossing(top);
	if (right != NULL && top != NULL)
		rightTopRef = right->FindCrossing(top);
	if (left != NULL && bottom != NULL)
		leftBottomRef = left->FindCrossing(bottom);
	if (right != NULL && bottom != NULL)
		rightBottomRef = right->FindCrossing(bottom);

	if (left == NULL) {
		leftRef = _AddVerticalTab();
		left = leftRef.Get();
	}
	if (top == NULL) {
		topRef = _AddHorizontalTab();
		top = topRef.Get();
	}
	if (right == NULL) {
		rightRef = _AddVerticalTab();
		right = rightRef.Get();
	}
	if (bottom == NULL) {
		bottomRef = _AddHorizontalTab();
		bottom = bottomRef.Get();
	}
	if (left == NULL || top == NULL || right == NULL || bottom == NULL)
		return false;

	if (leftTopRef == NULL) {
		leftTopRef = left->AddCrossing(top);
		if (leftTopRef == NULL)
			return false;
	}
	if (!rightTopRef) {
		rightTopRef = right->AddCrossing(top);
		if (!rightTopRef)
			return false;
	}
	if (!leftBottomRef) {
		leftBottomRef = left->AddCrossing(bottom);
		if (!leftBottomRef)
			return false;
	}
	if (!rightBottomRef) {
		rightBottomRef = right->AddCrossing(bottom);
		if (!rightBottomRef)
			return false;
	}

	K_WindowArea* area = new(std::nothrow) K_WindowArea(leftTopRef, rightTopRef,
		leftBottomRef, rightBottomRef);
	if (area == NULL)
		return false;
	// the area register itself in our area list
	if (area->Init(this) == false) {
		delete area;
		return false;
	}
	// delete the area if AddWindow failed / release our reference on it
	BReference<K_WindowArea> areaRef(area, true);

	return K_AddWindow(window, area);
}


bool
K_SATGroup::K_AddWindow(K_SATWindow* window, K_WindowArea* area, K_SATWindow* after)
{
debug_printf("[K_SATGroup] {K_AddWindow}\n");
	if (!area->K__AddWindow(window, after))
		return false;

	if (!fSATWindowList.AddItem(window)) {
		area->K__RemoveWindow(window);
		return false;
	}

	if (!window->AddedToGroup(this, area)) {
		area->K__RemoveWindow(window);
		fSATWindowList.RemoveItem(window);
		return false;
	}

	return true;
}
*/
//end


bool
K_SATGroup::RemoveWindow(K_SATWindow* window, bool stayBelowMouse)
{
	if (!fSATWindowList.RemoveItem(window))
		return false;

	// We need the area a little bit longer because the area could hold the
	// last reference to the group.
	BReference<K_WindowArea> area = window->GetWindowArea();
	if (area.IsSet())
		area->_RemoveWindow(window);

	window->RemovedFromGroup(this, stayBelowMouse);

	if (CountItems() >= 2)
		WindowAt(0)->DoGroupLayout();

	return true;
}


int32
K_SATGroup::CountItems()
{
	return fSATWindowList.CountItems();
}


K_SATWindow*
K_SATGroup::WindowAt(int32 index)
{
	return fSATWindowList.ItemAt(index);
}


K_SATWindow*
K_SATGroup::ActiveWindow() const
{
	return fActiveWindow;
}


void
K_SATGroup::SetActiveWindow(K_SATWindow* window)
{
	fActiveWindow = window;
}


const K_TabList*
K_SATGroup::HorizontalTabs()
{
	if (!fHorizontalTabsSorted) {
		fHorizontalTabs.SortItems(K_Tab::CompareFunction);
		fHorizontalTabsSorted = true;
	}
	return &fHorizontalTabs;
}


const K_TabList*
K_SATGroup::VerticalTabs()
{
	if (!fVerticalTabsSorted) {
		fVerticalTabs.SortItems(K_Tab::CompareFunction);
		fVerticalTabsSorted = true;
	}
	return &fVerticalTabs;
}


K_Tab*
K_SATGroup::FindHorizontalTab(float position)
{
	return _FindTab(fHorizontalTabs, position);
}


K_Tab*
K_SATGroup::FindVerticalTab(float position)
{
	return _FindTab(fVerticalTabs, position);
}


void
K_SATGroup::WindowAreaRemoved(K_WindowArea* area)
{
	_SplitGroupIfNecessary(area);
}


status_t
K_SATGroup::RestoreGroup(const BMessage& archive, K_StackAndTile* sat)
{
	// create new group
	K_SATGroup* group = new (std::nothrow)K_SATGroup;
	if (group == NULL)
		return B_NO_MEMORY;
	BReference<K_SATGroup> groupRef;
	groupRef.SetTo(group, true);

	int32 nHTabs, nVTabs;
	status_t status;
	status = archive.FindInt32("htab_count", &nHTabs);
	if (status != B_OK)
		return status;
	status = archive.FindInt32("vtab_count", &nVTabs);
	if (status != B_OK)
		return status;

	vector<BReference<K_Tab> > tempHTabs;
	for (int i = 0; i < nHTabs; i++) {
		BReference<K_Tab> tab = group->_AddHorizontalTab();
		if (!tab)
			return B_NO_MEMORY;
		tempHTabs.push_back(tab);
	}
	vector<BReference<K_Tab> > tempVTabs;
	for (int i = 0; i < nVTabs; i++) {
		BReference<K_Tab> tab = group->_AddVerticalTab();
		if (!tab)
			return B_NO_MEMORY;
		tempVTabs.push_back(tab);
	}

	BMessage areaArchive;
	for (int32 i = 0; archive.FindMessage("area", i, &areaArchive) == B_OK;
		i++) {
		uint32 leftTab, rightTab, topTab, bottomTab;
		if (areaArchive.FindInt32("left_tab", (int32*)&leftTab) != B_OK
			|| areaArchive.FindInt32("right_tab", (int32*)&rightTab) != B_OK
			|| areaArchive.FindInt32("top_tab", (int32*)&topTab) != B_OK
			|| areaArchive.FindInt32("bottom_tab", (int32*)&bottomTab) != B_OK)
			return B_ERROR;

		if (leftTab >= tempVTabs.size() || rightTab >= tempVTabs.size())
			return B_BAD_VALUE;
		if (topTab >= tempHTabs.size() || bottomTab >= tempHTabs.size())
			return B_BAD_VALUE;

		K_Tab* left = tempVTabs[leftTab];
		K_Tab* right = tempVTabs[rightTab];
		K_Tab* top = tempHTabs[topTab];
		K_Tab* bottom = tempHTabs[bottomTab];

		// adding windows to area
		uint64 windowId;
		K_SATWindow* prevWindow = NULL;
		for (int32 i = 0; areaArchive.FindInt64("window", i,
			(int64*)&windowId) == B_OK; i++) {
			K_SATWindow* window = sat->FindSATWindow(windowId);
			if (!window)
				continue;

			if (prevWindow == NULL) {
				if (!group->AddWindow(window, left, top, right, bottom))
					continue;
				prevWindow = window;
			} else {
				if (!prevWindow->StackWindow(window))
					continue;
				prevWindow = window;
			}
		}
	}
	return B_OK;
}


status_t
K_SATGroup::ArchiveGroup(BMessage& archive)
{
	archive.AddInt32("htab_count", fHorizontalTabs.CountItems());
	archive.AddInt32("vtab_count", fVerticalTabs.CountItems());

	for (int i = 0; i < fWindowAreaList.CountItems(); i++) {
		K_WindowArea* area = fWindowAreaList.ItemAt(i);
		int32 leftTab = fVerticalTabs.IndexOf(area->LeftTab());
		int32 rightTab = fVerticalTabs.IndexOf(area->RightTab());
		int32 topTab = fHorizontalTabs.IndexOf(area->TopTab());
		int32 bottomTab = fHorizontalTabs.IndexOf(area->BottomTab());

		BMessage areaMessage;
		areaMessage.AddInt32("left_tab", leftTab);
		areaMessage.AddInt32("right_tab", rightTab);
		areaMessage.AddInt32("top_tab", topTab);
		areaMessage.AddInt32("bottom_tab", bottomTab);

		const K_SATWindowList& windowList = area->WindowList();
		for (int a = 0; a < windowList.CountItems(); a++)
			areaMessage.AddInt64("window", windowList.ItemAt(a)->Id());

		archive.AddMessage("area", &areaMessage);
	}
	return B_OK;
}


BReference<K_Tab>
K_SATGroup::_AddHorizontalTab(float position)
{
	if (fLinearSpec == NULL)
		return NULL;
	Variable* variable = fLinearSpec->AddVariable();
	if (variable == NULL)
		return NULL;

	K_Tab* tab = new (std::nothrow)K_Tab(this, variable, K_Tab::kHorizontal);
	if (tab == NULL)
		return NULL;
	BReference<K_Tab> tabRef(tab, true);

	if (!fHorizontalTabs.AddItem(tab))
		return NULL;

	fHorizontalTabsSorted = false;
	tabRef->SetPosition(position);
	return tabRef;
}


BReference<K_Tab>
K_SATGroup::_AddVerticalTab(float position)
{
	if (fLinearSpec == NULL)
		return NULL;
	Variable* variable = fLinearSpec->AddVariable();
	if (variable == NULL)
		return NULL;

	K_Tab* tab = new (std::nothrow)K_Tab(this, variable, K_Tab::kVertical);
	if (tab == NULL)
		return NULL;
	BReference<K_Tab> tabRef(tab, true);

	if (!fVerticalTabs.AddItem(tab))
		return NULL;

	fVerticalTabsSorted = false;
	tabRef->SetPosition(position);
	return tabRef;
}


bool
K_SATGroup::_RemoveHorizontalTab(K_Tab* tab)
{
	if (!fHorizontalTabs.RemoveItem(tab))
		return false;
	fHorizontalTabsSorted = false;
	// don't delete the tab it is reference counted
	return true;
}


bool
K_SATGroup::_RemoveVerticalTab(K_Tab* tab)
{
	if (!fVerticalTabs.RemoveItem(tab))
		return false;
	fVerticalTabsSorted = false;
	// don't delete the tab it is reference counted
	return true;
}


K_Tab*
K_SATGroup::_FindTab(const K_TabList& list, float position)
{
	for (int i = 0; i < list.CountItems(); i++)
		if (fabs(list.ItemAt(i)->Position() - position) < 0.00001)
			return list.ItemAt(i);

	return NULL;
}


void
K_SATGroup::_SplitGroupIfNecessary(K_WindowArea* removedArea)
{
	// if there are windows stacked in the area we don't need to split
	if (removedArea == NULL || removedArea->WindowList().CountItems() > 1)
		return;

	K_WindowAreaList neighbourWindows;

	_FillNeighbourList(neighbourWindows, removedArea);

	bool ownGroupProcessed = false;
	K_WindowAreaList newGroup;
	while (_FindConnectedGroup(neighbourWindows, removedArea, newGroup)) {
		STRACE_SAT("Connected group found; %i window(s)\n",
			(int)newGroup.CountItems());
		if (newGroup.CountItems() == 1
			&& newGroup.ItemAt(0)->WindowList().CountItems() == 1) {
			K_SATWindow* window = newGroup.ItemAt(0)->WindowList().ItemAt(0);
			RemoveWindow(window);
			_EnsureGroupIsOnScreen(window->GetGroup());
		} else if (ownGroupProcessed)
			_SpawnNewGroup(newGroup);
		else {
			_EnsureGroupIsOnScreen(this);
			ownGroupProcessed = true;
		}

		newGroup.MakeEmpty();
	}
}


void
K_SATGroup::_FillNeighbourList(K_WindowAreaList& neighbourWindows,
	K_WindowArea* area)
{
	_LeftNeighbours(neighbourWindows, area);
	_RightNeighbours(neighbourWindows, area);
	_TopNeighbours(neighbourWindows, area);
	_BottomNeighbours(neighbourWindows, area);
}


void
K_SATGroup::_LeftNeighbours(K_WindowAreaList& neighbourWindows, K_WindowArea* parent)
{
	float startPos = parent->LeftTopCrossing()->HorizontalTab()->Position();
	float endPos = parent->LeftBottomCrossing()->HorizontalTab()->Position();

	K_Tab* tab = parent->LeftTopCrossing()->VerticalTab();
	const K_CrossingList* crossingList = tab->GetCrossingList();
	for (int i = 0; i < crossingList->CountItems(); i++) {
		K_Corner* corner = crossingList->ItemAt(i)->LeftTopCorner();
		if (corner->status != K_Corner::kUsed)
			continue;

		K_WindowArea* area = corner->windowArea;
		float pos1 = area->LeftTopCrossing()->HorizontalTab()->Position();
		float pos2 = area->LeftBottomCrossing()->HorizontalTab()->Position();

		if (pos1 < endPos && pos2 > startPos)
			neighbourWindows.AddItem(area);

		if (pos2 > endPos)
			break;
	}
}


void
K_SATGroup::_TopNeighbours(K_WindowAreaList& neighbourWindows, K_WindowArea* parent)
{
	float startPos = parent->LeftTopCrossing()->VerticalTab()->Position();
	float endPos = parent->RightTopCrossing()->VerticalTab()->Position();

	K_Tab* tab = parent->LeftTopCrossing()->HorizontalTab();
	const K_CrossingList* crossingList = tab->GetCrossingList();
	for (int i = 0; i < crossingList->CountItems(); i++) {
		K_Corner* corner = crossingList->ItemAt(i)->LeftTopCorner();
		if (corner->status != K_Corner::kUsed)
			continue;

		K_WindowArea* area = corner->windowArea;
		float pos1 = area->LeftTopCrossing()->VerticalTab()->Position();
		float pos2 = area->RightTopCrossing()->VerticalTab()->Position();

		if (pos1 < endPos && pos2 > startPos)
			neighbourWindows.AddItem(area);

		if (pos2 > endPos)
			break;
	}
}


void
K_SATGroup::_RightNeighbours(K_WindowAreaList& neighbourWindows, K_WindowArea* parent)
{
	float startPos = parent->RightTopCrossing()->HorizontalTab()->Position();
	float endPos = parent->RightBottomCrossing()->HorizontalTab()->Position();

	K_Tab* tab = parent->RightTopCrossing()->VerticalTab();
	const K_CrossingList* crossingList = tab->GetCrossingList();
	for (int i = 0; i < crossingList->CountItems(); i++) {
		K_Corner* corner = crossingList->ItemAt(i)->RightTopCorner();
		if (corner->status != K_Corner::kUsed)
			continue;

		K_WindowArea* area = corner->windowArea;
		float pos1 = area->RightTopCrossing()->HorizontalTab()->Position();
		float pos2 = area->RightBottomCrossing()->HorizontalTab()->Position();

		if (pos1 < endPos && pos2 > startPos)
			neighbourWindows.AddItem(area);

		if (pos2 > endPos)
			break;
	}
}


void
K_SATGroup::_BottomNeighbours(K_WindowAreaList& neighbourWindows,
	K_WindowArea* parent)
{
	float startPos = parent->LeftBottomCrossing()->VerticalTab()->Position();
	float endPos = parent->RightBottomCrossing()->VerticalTab()->Position();

	K_Tab* tab = parent->LeftBottomCrossing()->HorizontalTab();
	const K_CrossingList* crossingList = tab->GetCrossingList();
	for (int i = 0; i < crossingList->CountItems(); i++) {
		K_Corner* corner = crossingList->ItemAt(i)->LeftBottomCorner();
		if (corner->status != K_Corner::kUsed)
			continue;

		K_WindowArea* area = corner->windowArea;
		float pos1 = area->LeftBottomCrossing()->VerticalTab()->Position();
		float pos2 = area->RightBottomCrossing()->VerticalTab()->Position();

		if (pos1 < endPos && pos2 > startPos)
			neighbourWindows.AddItem(area);

		if (pos2 > endPos)
			break;
	}
}


bool
K_SATGroup::_FindConnectedGroup(K_WindowAreaList& seedList, K_WindowArea* removedArea,
	K_WindowAreaList& newGroup)
{
	if (seedList.CountItems() == 0)
		return false;

	K_WindowArea* area = seedList.RemoveItemAt(0);
	newGroup.AddItem(area);

	_FollowSeed(area, removedArea, seedList, newGroup);
	return true;
}


void
K_SATGroup::_FollowSeed(K_WindowArea* area, K_WindowArea* veto,
	K_WindowAreaList& seedList, K_WindowAreaList& newGroup)
{
	K_WindowAreaList neighbours;
	_FillNeighbourList(neighbours, area);
	for (int i = 0; i < neighbours.CountItems(); i++) {
		K_WindowArea* currentArea = neighbours.ItemAt(i);
		if (currentArea != veto && !newGroup.HasItem(currentArea)) {
			newGroup.AddItem(currentArea);
			// if we get a area from the seed list it is not a seed any more
			seedList.RemoveItem(currentArea);
		} else {
			// don't _FollowSeed of invalid areas
			neighbours.RemoveItemAt(i);
			i--;
		}
	}

	for (int i = 0; i < neighbours.CountItems(); i++)
		_FollowSeed(neighbours.ItemAt(i), veto, seedList, newGroup);
}


void
K_SATGroup::_SpawnNewGroup(const K_WindowAreaList& newGroup)
{
	STRACE_SAT("K_SATGroup::_SpawnNewGroup\n");
	K_SATGroup* group = new (std::nothrow)K_SATGroup;
	if (group == NULL)
		return;
	BReference<K_SATGroup> groupRef;
	groupRef.SetTo(group, true);

	for (int i = 0; i < newGroup.CountItems(); i++)
		newGroup.ItemAt(i)->PropagateToGroup(group);

	_EnsureGroupIsOnScreen(group);
}


const float kMinOverlap = 50;
const float kMoveToScreen = 75;


void
K_SATGroup::_EnsureGroupIsOnScreen(K_SATGroup* group)
{
	STRACE_SAT("K_SATGroup::_EnsureGroupIsOnScreen\n");
	if (group == NULL || group->CountItems() < 1)
		return;

	K_SATWindow* window = group->WindowAt(0);
	Desktop* desktop = window->GetWindow()->Desktop();
	if (desktop == NULL)
		return;

	const float kBigDistance = 1E+10;

	float minLeftDistance = kBigDistance;
	BRect leftRect;
	float minTopDistance = kBigDistance;
	BRect topRect;
	float minRightDistance = kBigDistance;
	BRect rightRect;
	float minBottomDistance = kBigDistance;
	BRect bottomRect;

	BRect screen = window->GetWindow()->Screen()->Frame();
	BRect reducedScreen = screen;
	reducedScreen.InsetBy(kMinOverlap, kMinOverlap);

	for (int i = 0; i < group->CountItems(); i++) {
		K_SATWindow* window = group->WindowAt(i);
		BRect frame = window->CompleteWindowFrame();
		if (reducedScreen.Intersects(frame))
			return;

		if (frame.right < screen.left + kMinOverlap) {
			float dist = fabs(screen.left - frame.right);
			if (dist < minLeftDistance) {
				minLeftDistance = dist;
				leftRect = frame;
			} else if (dist == minLeftDistance)
				leftRect = leftRect | frame;
		}
		if (frame.top > screen.bottom - kMinOverlap) {
			float dist = fabs(frame.top - screen.bottom);
			if (dist < minBottomDistance) {
				minBottomDistance = dist;
				bottomRect = frame;
			} else if (dist == minBottomDistance)
				bottomRect = bottomRect | frame;
		}
		if (frame.left > screen.right - kMinOverlap) {
			float dist = fabs(frame.left - screen.right);
			if (dist < minRightDistance) {
				minRightDistance = dist;
				rightRect = frame;
			} else if (dist == minRightDistance)
				rightRect = rightRect | frame;
		}
		if (frame.bottom < screen.top + kMinOverlap) {
			float dist = fabs(frame.bottom - screen.top);
			if (dist < minTopDistance) {
				minTopDistance = dist;
				topRect = frame;
			} else if (dist == minTopDistance)
				topRect = topRect | frame;
		}
	}

	BPoint offset;
	if (minLeftDistance < kBigDistance) {
		offset.x = screen.left - leftRect.right + kMoveToScreen;
		_CallculateYOffset(offset, leftRect, screen);
	} else if (minTopDistance < kBigDistance) {
		offset.y = screen.top - topRect.bottom + kMoveToScreen;
		_CallculateXOffset(offset, topRect, screen);
	} else if (minRightDistance < kBigDistance) {
		offset.x = screen.right - rightRect.left - kMoveToScreen;
		_CallculateYOffset(offset, rightRect, screen);
	} else if (minBottomDistance < kBigDistance) {
		offset.y = screen.bottom - bottomRect.top - kMoveToScreen;
		_CallculateXOffset(offset, bottomRect, screen);
	}

	if (offset.x == 0. && offset.y == 0.)
		return;
	STRACE_SAT("move group back to screen: offset x: %f offset y: %f\n",
		offset.x, offset.y);

	desktop->K_MoveWindowBy(window->GetWindow(), offset.x, offset.y);
	window->DoGroupLayout();
}


void
K_SATGroup::_CallculateXOffset(BPoint& offset, BRect& frame, BRect& screen)
{
	if (frame.right < screen.left + kMinOverlap)
		offset.x = screen.left - frame.right + kMoveToScreen;
	else if (frame.left > screen.right - kMinOverlap)
		offset.x = screen.right - frame.left - kMoveToScreen;
}


void
K_SATGroup::_CallculateYOffset(BPoint& offset, BRect& frame, BRect& screen)
{
	if (frame.top > screen.bottom - kMinOverlap)
		offset.y = screen.bottom - frame.top - kMoveToScreen;
	else if (frame.bottom < screen.top + kMinOverlap)
		offset.y = screen.top - frame.bottom + kMoveToScreen;
}
