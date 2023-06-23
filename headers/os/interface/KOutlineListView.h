/*
 * Copyright 2006-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_OUTLINE_LIST_VIEW_H
#define _K_OUTLINE_LIST_VIEW_H


#include <KListView.h>


class KOutlineListView : public KListView {
public:
								KOutlineListView(BRect frame, const char* name,
									list_view_type type
										= B_SINGLE_SELECTION_LIST,
									uint32 resizingMode = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_WILL_DRAW
										| B_FRAME_EVENTS | B_NAVIGABLE);
								KOutlineListView(const char* name,
									list_view_type type
										= B_SINGLE_SELECTION_LIST,
									uint32 flags = B_WILL_DRAW
										| B_FRAME_EVENTS | B_NAVIGABLE);
								KOutlineListView(BMessage* archive);
	virtual						~KOutlineListView();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				MouseDown(BPoint where);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				MouseUp(BPoint where);

	virtual	bool				AddUnder(KListItem* item, KListItem* superItem);

	virtual	bool				AddItem(KListItem* item);
	virtual	bool				AddItem(KListItem* item, int32 fullListIndex);
	virtual	bool				AddList(BList* newItems);
	virtual	bool				AddList(BList* newItems, int32 fullListIndex);

	virtual	bool				RemoveItem(KListItem* item);
	virtual	KListItem*			RemoveItem(int32 fullListIndex);
	virtual	bool				RemoveItems(int32 fullListIndex, int32 count);

			KListItem*			FullListItemAt(int32 fullListIndex) const;
			int32				FullListIndexOf(BPoint where) const;
			int32				FullListIndexOf(KListItem* item) const;
			KListItem*			FullListFirstItem() const;
			KListItem*			FullListLastItem() const;
			bool				FullListHasItem(KListItem* item) const;
			int32				FullListCountItems() const;
			int32				FullListCurrentSelection(
									int32 index = 0) const;

	virtual	void				MakeEmpty();
			bool				FullListIsEmpty() const;
			void				FullListDoForEach(bool (*func)(KListItem* item));
			void				FullListDoForEach(bool (*func)(KListItem* item, void* arg),
									void* arg);

			KListItem*			Superitem(const KListItem* item);

			void				Expand(KListItem* item);
			void				Collapse(KListItem* item);

			bool				IsExpanded(int32 fullListIndex);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);
	virtual	status_t			Perform(perform_code code, void* data);

	virtual	void				ResizeToPreferred();
	virtual	void				GetPreferredSize(float* _width,
									float* _height);
	virtual	void				MakeFocus(bool focus = true);
	virtual	void				AllAttached();
	virtual	void				AllDetached();
	virtual	void				DetachedFromWindow();

			void				FullListSortItems(int (*compareFunc)(
										const KListItem* first,
										const KListItem* second));
			void				SortItemsUnder(KListItem* superItem,
									bool oneLevelOnly, int (*compareFunc)(
										const KListItem* first,
										const KListItem* second));
			int32				CountItemsUnder(KListItem* superItem,
									bool oneLevelOnly) const;
			KListItem*			EachItemUnder(KListItem* superItem,
									bool oneLevelOnly, KListItem* (*eachFunc)(
										KListItem* item, void* arg),
									void* arg);
			KListItem*			ItemUnderAt(KListItem* superItem,
									bool oneLevelOnly, int32 index) const;

protected:
	virtual	bool				DoMiscellaneous(MiscCode code, MiscData* data);
	virtual void				MessageReceived(BMessage* message);

private:
	virtual	void				_ReservedOutlineListView1();
	virtual	void				_ReservedOutlineListView2();
	virtual	void				_ReservedOutlineListView3();
	virtual	void				_ReservedOutlineListView4();

protected:
	virtual	void				ExpandOrCollapse(KListItem* superItem,
									bool expand);
	virtual BRect				LatchRect(BRect itemRect, int32 level) const;
	virtual void				DrawLatch(BRect itemRect, int32 level,
									bool collapsed, bool highlighted,
									bool misTracked);
	virtual	void				DrawItem(KListItem* item, BRect itemRect,
									bool complete = false);

private:
			int32				_FullListIndex(int32 index) const;

			void				_PopulateTree(BList* tree, BList& target,
									int32& firstIndex, bool onlyVisible);
			void				_SortTree(BList* tree, bool oneLevelOnly,
									int (*compareFunc)(const KListItem* a,
										const KListItem* b));
			void				_DestructTree(BList* tree);
			BList*				_BuildTree(KListItem* superItem, int32& index);

			void				_CullInvisibleItems(BList &list);
			bool				_SwapItems(int32 first, int32 second);
			KListItem*			_RemoveItem(KListItem* item,
									int32 fullListIndex);

			KListItem*			_SuperitemForIndex(int32 fullListIndex,
									int32 level, int32* _superIndex = NULL);
			int32				_FindPreviousVisibleIndex(int32 fullListIndex);

private:
			BList				fFullList;

			uint32				_reserved[2];
};

#endif // _OUTLINE_LIST_VIEW_H
