/*
 * Copyright 2010-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 */
#ifndef K__SAT_GROUP_H
#define K__SAT_GROUP_H


#include <Rect.h>

#include <AutoDeleter.h>
#include "ObjectList.h"
#include "Referenceable.h"

#include "MagneticBorder.h"

#include "LinearSpec.h"


class K_SATWindow;
class K_Tab;
class K_WindowArea;

typedef BObjectList<K_SATWindow> K_SATWindowList;


class K_Corner {
public:
		enum info_t
		{
			kFree,
			kUsed,
			kNotDockable
		};

		enum position_t
		{
			kLeftTop = 0,
			kRightTop = 1,
			kLeftBottom = 2,
			kRightBottom = 3
		};

						K_Corner();
		void			Trace() const;

		info_t			status;
		K_WindowArea*		windowArea;
};


class K_Crossing : public BReferenceable {
public:
								K_Crossing(K_Tab* vertical, K_Tab* horizontal);
								~K_Crossing();

			K_Corner*				GetCorner(K_Corner::position_t corner) const;
			K_Corner*				GetOppositeCorner(
									K_Corner::position_t corner) const;

			K_Corner*				LeftTopCorner()
									{ return &fCorners[K_Corner::kLeftTop]; }
			K_Corner*				RightTopCorner()
									{ return &fCorners[K_Corner::kRightTop]; }
			K_Corner*				LeftBottomCorner()
									{ return &fCorners[K_Corner::kLeftBottom]; }
			K_Corner*				RightBottomCorner()
									{ return &fCorners[K_Corner::kRightBottom]; }

			K_Tab*				VerticalTab() const;
			K_Tab*				HorizontalTab() const;

			void				Trace() const;
private:
			K_Corner				fCorners[4];

			BReference<K_Tab>		fVerticalTab;
			BReference<K_Tab>		fHorizontalTab;
};


typedef BObjectList<Constraint> ConstraintList;
class K_SATGroup;

typedef BObjectList<K_Crossing> K_CrossingList;


// make all coordinates positive needed for the solver
const float k_kMakePositiveOffset = 5000;


class K_Tab : public BReferenceable {
public:
		enum orientation_t
		{
			kVertical,
			kHorizontal
		};

								K_Tab(K_SATGroup* group, Variable* variable,
									orientation_t orientation);
								~K_Tab();

			float				Position() const;
			void				SetPosition(float position);
			orientation_t		Orientation() const;
			Variable*			Var() {	return fVariable.Get(); }

			//! Caller takes ownership of the constraint.
			Constraint*			Connect(Variable* variable);

			BReference<K_Crossing>	AddCrossing(K_Tab* tab);
			bool				RemoveCrossing(K_Crossing* crossing);
			int32				FindCrossingIndex(K_Tab* tab);
			int32				FindCrossingIndex(float tabPosition);
			K_Crossing*			FindCrossing(K_Tab* tab);
			K_Crossing*			FindCrossing(float tabPosition);

			const K_CrossingList*	GetCrossingList() const;

	static	int					CompareFunction(const K_Tab* tab1,
									const K_Tab* tab2);

private:
			K_SATGroup*			fGroup;
			ObjectDeleter<Variable>
								fVariable;
			orientation_t		fOrientation;

			K_CrossingList		fCrossingList;
};


class K_WindowArea : public BReferenceable {
public:
								K_WindowArea(K_Crossing* leftTop,
									K_Crossing* rightTop, K_Crossing* leftBottom,
									K_Crossing* rightBottom);
								~K_WindowArea();

			bool				Init(K_SATGroup* group);
			K_SATGroup*			Group() { return fGroup; }

			void				DoGroupLayout();

void	K_DoGroupLayout();//khidki

			void				UpdateSizeLimits();
			void				UpdateSizeConstaints(const BRect& frame);

	const	K_SATWindowList&		WindowList() { return fWindowList; }
	const	K_SATWindowList&		LayerOrder() { return fWindowLayerOrder; }
			bool				MoveWindowToPosition(K_SATWindow* window,
									int32 index);
			K_SATWindow*			TopWindow();

//K_SATWindow*	K_TopWindow();//khidki

			K_Crossing*			LeftTopCrossing()
									{ return fLeftTopCrossing.Get(); }
			K_Crossing*			RightTopCrossing()
									{ return fRightTopCrossing.Get(); }
			K_Crossing*			LeftBottomCrossing()
									{ return fLeftBottomCrossing.Get(); }
			K_Crossing*			RightBottomCrossing()
									{ return fRightBottomCrossing.Get(); }

			K_Tab*				LeftTab();
			K_Tab*				RightTab();
			K_Tab*				TopTab();
			K_Tab*				BottomTab();

			Variable*			LeftVar() { return LeftTab()->Var(); }
			Variable*			RightVar() { return RightTab()->Var(); }
			Variable*			TopVar() { return TopTab()->Var(); }
			Variable*			BottomVar() { return BottomTab()->Var(); }

			BRect				Frame();

			bool				PropagateToGroup(K_SATGroup* group);

			bool				MoveToTopLayer(K_SATWindow* window);

private:
		friend class K_SATGroup;
			void				_UninitConstraints();
			void				_UpdateConstraintValues();

//void	K__UpdateConstraintValues();//khidki

			/*! K_SATGroup adds new windows to the area. */
			bool				_AddWindow(K_SATWindow* window,
									K_SATWindow* after = NULL);
//khidki start
/*! K_SATGroup adds new windows to the area. */
/*bool	K__AddWindow(K_SATWindow* window,
									K_SATWindow* after = NULL);
//end*/

			/*! After the last window has been removed the K_WindowArea delete
			himself and clean up all crossings. */
			bool				_RemoveWindow(K_SATWindow* window);

//bool	K__RemoveWindow(K_SATWindow* window);//khidki

	inline	void				_InitCorners();
	inline	void				_CleanupCorners();
	inline	void				_SetToWindowCorner(K_Corner* corner);
	inline	void				_SetToNeighbourCorner(K_Corner* neighbour);
	inline	void				_UnsetWindowCorner(K_Corner* corner);
		//! opponent is the other neighbour of the neighbour
	inline	void				_UnsetNeighbourCorner(K_Corner* neighbour,
									K_Corner* opponent);

			// Find crossing by tab position in group and if not exist create
			// it.
			BReference<K_Crossing>	_CrossingByPosition(K_Crossing* crossing,
										K_SATGroup* group);

			void				_MoveToSAT(K_SATWindow* topWindow);

			BReference<K_SATGroup>	fGroup;

			K_SATWindowList		fWindowList;

//K_SATWindowList	k_fWindowList;//khidki

			K_SATWindowList		fWindowLayerOrder;

//K_SATWindowList	k_fWindowLayerOrder;//khidki

			BReference<K_Crossing>	fLeftTopCrossing;
			BReference<K_Crossing>	fRightTopCrossing;
			BReference<K_Crossing>	fLeftBottomCrossing;
			BReference<K_Crossing>	fRightBottomCrossing;

			Constraint*			fMinWidthConstraint;
			Constraint*			fMinHeightConstraint;
			Constraint*			fMaxWidthConstraint;
			Constraint*			fMaxHeightConstraint;
			Constraint*			fWidthConstraint;
			Constraint*			fHeightConstraint;

			MagneticBorder		fMagneticBorder;
};


typedef BObjectList<K_WindowArea> K_WindowAreaList;
typedef BObjectList<K_Tab> K_TabList;

class BMessage;
class K_StackAndTile;


class K_SATGroup : public BReferenceable {
public:
		friend class K_Tab;
		friend class K_WindowArea;
		friend class GroupCookie;

								K_SATGroup();
								~K_SATGroup();

			LinearSpec*			GetLinearSpec() { return fLinearSpec; }

			/*! Create a new K_WindowArea from the crossing and add the window. */
			bool				AddWindow(K_SATWindow* window, K_Tab* left,
									K_Tab* top, K_Tab* right, K_Tab* bottom);
			/*! Add a window to an existing window area. */
			bool				AddWindow(K_SATWindow* window, K_WindowArea* area,
									K_SATWindow* after = NULL);
//khidki start
/*! Create a new K_WindowArea from the crossing and add the window. */
/*bool	K_AddWindow(K_SATWindow* window, K_Tab* left,
									K_Tab* top, K_Tab* right, K_Tab* bottom);
*/			/*! Add a window to an existing window area. */
/*bool	K_AddWindow(K_SATWindow* window, K_WindowArea* area,
									K_SATWindow* after = NULL);
//end
*/
			/*! If stayBelowMouse is true move the removed window below the
			cursor if necessary. */
			bool				RemoveWindow(K_SATWindow* window,
									bool stayBelowMouse = true);
			int32				CountItems();
			K_SATWindow*			WindowAt(int32 index);

			K_SATWindow*			ActiveWindow() const;
			void				SetActiveWindow(K_SATWindow* window);

			const K_WindowAreaList&	GetAreaList() { return fWindowAreaList; }

			/*! \return a sorted tab list. */
			const K_TabList*		HorizontalTabs();
			const K_TabList*		VerticalTabs();

			K_Tab*				FindHorizontalTab(float position);
			K_Tab*				FindVerticalTab(float position);

			void				WindowAreaRemoved(K_WindowArea* area);

	static	status_t			RestoreGroup(const BMessage& archive,
									K_StackAndTile* sat);
			status_t			ArchiveGroup(BMessage& archive);

private:
			BReference<K_Tab>		_AddHorizontalTab(float position = 0);
			BReference<K_Tab>		_AddVerticalTab(float position = 0);

			bool				_RemoveHorizontalTab(K_Tab* tab);
			bool				_RemoveVerticalTab(K_Tab* tab);

			K_Tab*				_FindTab(const K_TabList& list, float position);

			void				_SplitGroupIfNecessary(
									K_WindowArea* removedArea);
			void				_FillNeighbourList(
									K_WindowAreaList& neighbourWindows,
									K_WindowArea* area);
			void				_LeftNeighbours(
									K_WindowAreaList& neighbourWindows,
									K_WindowArea* window);
			void				_TopNeighbours(
									K_WindowAreaList& neighbourWindows,
									K_WindowArea* window);
			void				_RightNeighbours(
									K_WindowAreaList& neighbourWindows,
									K_WindowArea* window);
			void				_BottomNeighbours(
									K_WindowAreaList& neighbourWindows,
									K_WindowArea* window);
			bool				_FindConnectedGroup(K_WindowAreaList& seedList,
									K_WindowArea* removedArea,
									K_WindowAreaList& newGroup);
			void				_FollowSeed(K_WindowArea* area, K_WindowArea* veto,
									K_WindowAreaList& seedList,
									K_WindowAreaList& newGroup);
			void				_SpawnNewGroup(const K_WindowAreaList& newGroup);

			void				_EnsureGroupIsOnScreen(K_SATGroup* group);
	inline	void				_CallculateXOffset(BPoint& offset, BRect& frame,
									BRect& screen);
	inline	void				_CallculateYOffset(BPoint& offset, BRect& frame,
									BRect& screen);

protected:
			K_WindowAreaList		fWindowAreaList;
			K_SATWindowList		fSATWindowList;

			LinearSpec*			fLinearSpec;

private:
			K_TabList				fHorizontalTabs;
			bool				fHorizontalTabsSorted;
			K_TabList				fVerticalTabs;
			bool				fVerticalTabsSorted;

			K_SATWindow*			fActiveWindow;
};


typedef BObjectList<K_SATGroup> K_SATGroupList;

#endif
