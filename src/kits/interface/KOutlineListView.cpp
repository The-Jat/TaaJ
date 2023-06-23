/*
 * Copyright 2001-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Axel Dörfler, axeld@pinc-software.de
 *		Rene Gollent (rene@gollent.com)
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		John Scipione, jscipione@gmail.com
 */


//! KOutlineListView represents a "nestable" list view.


#include <KOutlineListView.h>

#include <algorithm>

#include <stdio.h>
#include <stdlib.h>

#include <KControlLook.h>
#include <Khidki.h>

#include<KLayout.h>//khidki
#include <binary_compatibility/Interface.h>


typedef int (*compare_func)(const KListItem* a, const KListItem* b);


struct ListItemComparator {
	ListItemComparator(compare_func compareFunc)
		:
		fCompareFunc(compareFunc)
	{
	}

	bool operator()(const KListItem* a, const KListItem* b) const
	{
		return fCompareFunc(a, b) < 0;
	}

private:
	compare_func	fCompareFunc;
};


static void
_GetSubItems(BList& sourceList, BList& destList, KListItem* parent, int32 start)
{
	for (int32 i = start; i < sourceList.CountItems(); i++) {
		KListItem* item = (KListItem*)sourceList.ItemAt(i);
		if (item->OutlineLevel() <= parent->OutlineLevel())
			break;
		destList.AddItem(item);
	}
}


static void
_DoSwap(BList& list, int32 firstIndex, int32 secondIndex, BList* firstItems,
	BList* secondItems)
{
	KListItem* item = (KListItem*)list.ItemAt(firstIndex);
	list.SwapItems(firstIndex, secondIndex);
	list.RemoveItems(secondIndex + 1, secondItems->CountItems());
	list.RemoveItems(firstIndex + 1, firstItems->CountItems());
	list.AddList(secondItems, firstIndex + 1);
	int32 newIndex = list.IndexOf(item);
	if (newIndex + 1 < list.CountItems())
		list.AddList(firstItems, newIndex + 1);
	else
		list.AddList(firstItems);
}


//	#pragma mark - KOutlineListView


KOutlineListView::KOutlineListView(BRect frame, const char* name,
	list_view_type type, uint32 resizingMode, uint32 flags)
	:
	KListView(frame, name, type, resizingMode, flags)
{
}


KOutlineListView::KOutlineListView(const char* name, list_view_type type,
	uint32 flags)
	:
	KListView(name, type, flags)
{
}


KOutlineListView::KOutlineListView(BMessage* archive)
	:
	KListView(archive)
{
	int32 i = 0;
	BMessage subData;
	while (archive->FindMessage("_l_full_items", i++, &subData) == B_OK) {
		BArchivable* object = instantiate_object(&subData);
		if (!object)
			continue;

		KListItem* item = dynamic_cast<KListItem*>(object);
		if (item)
			AddItem(item);
	}
}


KOutlineListView::~KOutlineListView()
{
	fFullList.MakeEmpty();
}


BArchivable*
KOutlineListView::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "KOutlineListView"))
		return new KOutlineListView(archive);

	return NULL;
}


status_t
KOutlineListView::Archive(BMessage* archive, bool deep) const
{
	// Note: We can't call the KListView Archive function here, as we are also
	// interested in subitems KOutlineListView can have. They are even stored
	// with a different field name (_l_full_items vs. _l_items).

	status_t status = KView::Archive(archive, deep);
	if (status != B_OK)
		return status;

	status = archive->AddInt32("_lv_type", fListType);
	if (status == B_OK && deep) {
		int32 i = 0;
		KListItem* item = NULL;
		while ((item = static_cast<KListItem*>(fFullList.ItemAt(i++)))) {
			BMessage subData;
			status = item->Archive(&subData, true);
			if (status >= B_OK)
				status = archive->AddMessage("_l_full_items", &subData);

			if (status < B_OK)
				break;
		}
	}

	if (status >= B_OK && InvocationMessage() != NULL)
		status = archive->AddMessage("_msg", InvocationMessage());

	if (status == B_OK && fSelectMessage != NULL)
		status = archive->AddMessage("_2nd_msg", fSelectMessage);

	return status;
}


void
KOutlineListView::MouseDown(BPoint where)
{
	MakeFocus();

	int32 index = IndexOf(where);

	if (index != -1) {
		KListItem* item = ItemAt(index);

		if (item->fHasSubitems
			&& LatchRect(ItemFrame(index), item->fLevel).Contains(where)) {
			if (item->IsExpanded())
				Collapse(item);
			else
				Expand(item);
		} else
			KListView::MouseDown(where);
	}
}


void
KOutlineListView::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1) {
		int32 currentSel = CurrentSelection();
		switch (bytes[0]) {
			case B_RIGHT_ARROW:
			{
				KListItem* item = ItemAt(currentSel);
				if (item && item->fHasSubitems) {
					if (!item->IsExpanded())
						Expand(item);
					else {
						Select(currentSel + 1);
						ScrollToSelection();
					}
				}
				return;
			}

			case B_LEFT_ARROW:
			{
				KListItem* item = ItemAt(currentSel);
				if (item) {
 					if (item->fHasSubitems && item->IsExpanded())
						Collapse(item);
					else {
						item = Superitem(item);
 						if (item) {
							Select(IndexOf(item));
 							ScrollToSelection();
 							}
					}
				}
				return;
			}
		}
	}

	KListView::KeyDown(bytes, numBytes);
}


void
KOutlineListView::FrameMoved(BPoint newPosition)
{
	KListView::FrameMoved(newPosition);
}


void
KOutlineListView::FrameResized(float newWidth, float newHeight)
{
	KListView::FrameResized(newWidth, newHeight);
}


void
KOutlineListView::MouseUp(BPoint where)
{
	KListView::MouseUp(where);
}


bool
KOutlineListView::AddUnder(KListItem* item, KListItem* superItem)
{
	if (superItem == NULL)
		return AddItem(item);

	fFullList.AddItem(item, FullListIndexOf(superItem) + 1);

	item->fLevel = superItem->OutlineLevel() + 1;
	superItem->fHasSubitems = true;

	if (superItem->IsItemVisible() && superItem->IsExpanded()) {
		item->SetItemVisible(true);

		int32 index = KListView::IndexOf(superItem);

		KListView::AddItem(item, index + 1);
		Invalidate(LatchRect(ItemFrame(index), superItem->OutlineLevel()));
	} else
		item->SetItemVisible(false);

	return true;
}


bool
KOutlineListView::AddItem(KListItem* item)
{
	return AddItem(item, FullListCountItems());
}


bool
KOutlineListView::AddItem(KListItem* item, int32 fullListIndex)
{
	if (fullListIndex < 0)
		fullListIndex = 0;
	else if (fullListIndex > FullListCountItems())
		fullListIndex = FullListCountItems();

	if (!fFullList.AddItem(item, fullListIndex))
		return false;

	// Check if this item is visible, and if it is, add it to the
	// other list, too

	if (item->fLevel > 0) {
		KListItem* super = _SuperitemForIndex(fullListIndex, item->fLevel);
		if (super == NULL)
			return true;

		bool hadSubitems = super->fHasSubitems;
		super->fHasSubitems = true;

		if (!super->IsItemVisible() || !super->IsExpanded()) {
			item->SetItemVisible(false);
			return true;
		}

		if (!hadSubitems) {
			Invalidate(LatchRect(ItemFrame(IndexOf(super)),
				super->OutlineLevel()));
		}
	}

	int32 listIndex = _FindPreviousVisibleIndex(fullListIndex);

	if (!KListView::AddItem(item, IndexOf(FullListItemAt(listIndex)) + 1)) {
		// adding didn't work out, we need to remove it from the main list again
		fFullList.RemoveItem(fullListIndex);
		return false;
	}

	return true;
}


bool
KOutlineListView::AddList(BList* newItems)
{
	return AddList(newItems, FullListCountItems());
}


bool
KOutlineListView::AddList(BList* newItems, int32 fullListIndex)
{
	if ((newItems == NULL) || (newItems->CountItems() == 0))
		return false;

	for (int32 i = 0; i < newItems->CountItems(); i++)
		AddItem((KListItem*)newItems->ItemAt(i), fullListIndex + i);

	return true;
}


bool
KOutlineListView::RemoveItem(KListItem* item)
{
	return _RemoveItem(item, FullListIndexOf(item)) != NULL;
}


KListItem*
KOutlineListView::RemoveItem(int32 fullListIndex)
{
	return _RemoveItem(FullListItemAt(fullListIndex), fullListIndex);
}


bool
KOutlineListView::RemoveItems(int32 fullListIndex, int32 count)
{
	if (fullListIndex >= FullListCountItems())
		fullListIndex = -1;
	if (fullListIndex < 0)
		return false;

	// TODO: very bad for performance!!
	while (count--)
		KOutlineListView::RemoveItem(fullListIndex);

	return true;
}


KListItem*
KOutlineListView::FullListItemAt(int32 fullListIndex) const
{
	return (KListItem*)fFullList.ItemAt(fullListIndex);
}


int32
KOutlineListView::FullListIndexOf(BPoint where) const
{
	int32 index = KListView::IndexOf(where);

	if (index > 0)
		index = _FullListIndex(index);

	return index;
}


int32
KOutlineListView::FullListIndexOf(KListItem* item) const
{
	return fFullList.IndexOf(item);
}


KListItem*
KOutlineListView::FullListFirstItem() const
{
	return (KListItem*)fFullList.FirstItem();
}


KListItem*
KOutlineListView::FullListLastItem() const
{
	return (KListItem*)fFullList.LastItem();
}


bool
KOutlineListView::FullListHasItem(KListItem* item) const
{
	return fFullList.HasItem(item);
}


int32
KOutlineListView::FullListCountItems() const
{
	return fFullList.CountItems();
}


int32
KOutlineListView::FullListCurrentSelection(int32 index) const
{
	int32 i = KListView::CurrentSelection(index);

	KListItem* item = KListView::ItemAt(i);
	if (item)
		return fFullList.IndexOf(item);

	return -1;
}


void
KOutlineListView::MakeEmpty()
{
	fFullList.MakeEmpty();
	KListView::MakeEmpty();
}


bool
KOutlineListView::FullListIsEmpty() const
{
	return fFullList.IsEmpty();
}


void
KOutlineListView::FullListDoForEach(bool(*func)(KListItem* item))
{
	fFullList.DoForEach(reinterpret_cast<bool (*)(void*)>(func));
}


void
KOutlineListView::FullListDoForEach(bool (*func)(KListItem* item, void* arg),
	void* arg)
{
	fFullList.DoForEach(reinterpret_cast<bool (*)(void*, void*)>(func), arg);
}


KListItem*
KOutlineListView::Superitem(const KListItem* item)
{
	int32 index = FullListIndexOf((KListItem*)item);
	if (index == -1)
		return NULL;

	return _SuperitemForIndex(index, item->OutlineLevel());
}


void
KOutlineListView::Expand(KListItem* item)
{
	ExpandOrCollapse(item, true);
}


void
KOutlineListView::Collapse(KListItem* item)
{
	ExpandOrCollapse(item, false);
}


bool
KOutlineListView::IsExpanded(int32 fullListIndex)
{
	KListItem* item = FullListItemAt(fullListIndex);
	if (!item)
		return false;

	return item->IsExpanded();
}


BHandler*
KOutlineListView::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	return KListView::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
KOutlineListView::GetSupportedSuites(BMessage* data)
{
	return KListView::GetSupportedSuites(data);
}


status_t
KOutlineListView::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= KOutlineListView::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= KOutlineListView::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= KOutlineListView::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= KOutlineListView::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= KOutlineListView::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			KOutlineListView::GetHeightForWidth(data->width, &data->min,
				&data->max, &data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KOutlineListView::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			KOutlineListView::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			KOutlineListView::DoLayout();
			return B_OK;
		}
	}

	return KListView::Perform(code, _data);
}


void
KOutlineListView::ResizeToPreferred()
{
	KListView::ResizeToPreferred();
}


void
KOutlineListView::GetPreferredSize(float* _width, float* _height)
{
	int32 count = CountItems();

	if (count > 0) {
		float maxWidth = 0.0;
		for (int32 i = 0; i < count; i++) {
			// The item itself does not take his OutlineLevel into account, so
			// we must make up for that. Also add space for the latch.
			float itemWidth = ItemAt(i)->Width() + be_plain_font->Size()
				+ (ItemAt(i)->OutlineLevel() + 1)
					* k_be_control_look->DefaultItemSpacing();
			if (itemWidth > maxWidth)
				maxWidth = itemWidth;
		}

		if (_width != NULL)
			*_width = maxWidth;
		if (_height != NULL)
			*_height = ItemAt(count - 1)->Bottom();
	} else
		KView::GetPreferredSize(_width, _height);
}


void
KOutlineListView::MakeFocus(bool state)
{
	KListView::MakeFocus(state);
}


void
KOutlineListView::AllAttached()
{
	KListView::AllAttached();
}


void
KOutlineListView::AllDetached()
{
	KListView::AllDetached();
}


void
KOutlineListView::DetachedFromWindow()
{
	KListView::DetachedFromWindow();
}


void
KOutlineListView::FullListSortItems(int (*compareFunc)(const KListItem* a,
	const KListItem* b))
{
	SortItemsUnder(NULL, false, compareFunc);
}


void
KOutlineListView::SortItemsUnder(KListItem* superItem, bool oneLevelOnly,
	int (*compareFunc)(const KListItem* a, const KListItem* b))
{
	// This method is quite complicated: basically, it creates a real tree
	// from the items of the full list, sorts them as needed, and then
	// populates the entries back into the full and display lists

	int32 firstIndex = FullListIndexOf(superItem) + 1;
	int32 lastIndex = firstIndex;
	BList* tree = _BuildTree(superItem, lastIndex);

	_SortTree(tree, oneLevelOnly, compareFunc);

	// Populate to the full list
	_PopulateTree(tree, fFullList, firstIndex, false);

	if (superItem == NULL
		|| (superItem->IsItemVisible() && superItem->IsExpanded())) {
		// Populate to KListView's list
		firstIndex = fList.IndexOf(superItem) + 1;
		lastIndex = firstIndex;
		_PopulateTree(tree, fList, lastIndex, true);

		if (fFirstSelected != -1) {
			// update selection hints
			fFirstSelected = _CalcFirstSelected(0);
			fLastSelected = _CalcLastSelected(CountItems());
		}

		// only invalidate what may have changed
		_RecalcItemTops(firstIndex);
		BRect top = ItemFrame(firstIndex);
		BRect bottom = ItemFrame(lastIndex - 1);
		BRect update(top.left, top.top, bottom.right, bottom.bottom);
		Invalidate(update);
	}

	_DestructTree(tree);
}


int32
KOutlineListView::CountItemsUnder(KListItem* superItem, bool oneLevelOnly) const
{
	int32 i = FullListIndexOf(superItem);
	if (i == -1)
		return 0;

	++i;
	int32 count = 0;
	uint32 baseLevel = superItem->OutlineLevel();

	for (; i < FullListCountItems(); i++) {
		KListItem* item = FullListItemAt(i);

		// If we jump out of the subtree, return count
		if (item->fLevel <= baseLevel)
			return count;

		// If the level matches, increase count
		if (!oneLevelOnly || item->fLevel == baseLevel + 1)
			count++;
	}

	return count;
}


KListItem*
KOutlineListView::EachItemUnder(KListItem* superItem, bool oneLevelOnly,
	KListItem* (*eachFunc)(KListItem* item, void* arg), void* arg)
{
	int32 i = FullListIndexOf(superItem);
	if (i == -1)
		return NULL;

	i++; // skip the superitem
	while (i < FullListCountItems()) {
		KListItem* item = FullListItemAt(i);

		// If we jump out of the subtree, return NULL
		if (item->fLevel <= superItem->OutlineLevel())
			return NULL;

		// If the level matches, check the index
		if (!oneLevelOnly || item->fLevel == superItem->OutlineLevel() + 1) {
			item = eachFunc(item, arg);
			if (item != NULL)
				return item;
		}

		i++;
	}

	return NULL;
}


KListItem*
KOutlineListView::ItemUnderAt(KListItem* superItem, bool oneLevelOnly,
	int32 index) const
{
	int32 i = FullListIndexOf(superItem);
	if (i == -1)
		return NULL;

	while (i < FullListCountItems()) {
		KListItem* item = FullListItemAt(i);

		// If we jump out of the subtree, return NULL
		if (item->fLevel < superItem->OutlineLevel())
			return NULL;

		// If the level matches, check the index
		if (!oneLevelOnly || item->fLevel == superItem->OutlineLevel() + 1) {
			if (index == 0)
				return item;

			index--;
		}

		i++;
	}

	return NULL;
}


bool
KOutlineListView::DoMiscellaneous(MiscCode code, MiscData* data)
{
	if (code == B_SWAP_OP)
		return _SwapItems(data->swap.a, data->swap.b);

	return KListView::DoMiscellaneous(code, data);
}


void
KOutlineListView::MessageReceived(BMessage* msg)
{
	KListView::MessageReceived(msg);
}


void KOutlineListView::_ReservedOutlineListView1() {}
void KOutlineListView::_ReservedOutlineListView2() {}
void KOutlineListView::_ReservedOutlineListView3() {}
void KOutlineListView::_ReservedOutlineListView4() {}


void
KOutlineListView::ExpandOrCollapse(KListItem* item, bool expand)
{
	if (item->IsExpanded() == expand || !FullListHasItem(item))
		return;

	item->fExpanded = expand;

	// TODO: merge these cases together, they are pretty similar

	if (expand) {
		uint32 level = item->fLevel;
		int32 fullListIndex = FullListIndexOf(item);
		int32 index = IndexOf(item) + 1;
		int32 startIndex = index;
		int32 count = FullListCountItems() - fullListIndex - 1;
		KListItem** items = (KListItem**)fFullList.Items() + fullListIndex + 1;

		BFont font;
		GetFont(&font);
		while (count-- > 0) {
			item = items[0];
			if (item->fLevel <= level)
				break;

			if (!item->IsItemVisible()) {
				// fix selection hints
				if (index <= fFirstSelected)
					fFirstSelected++;
				if (index <= fLastSelected)
					fLastSelected++;

				fList.AddItem(item, index++);
				item->Update(this, &font);
				item->SetItemVisible(true);
			}

			if (item->HasSubitems() && !item->IsExpanded()) {
				// Skip hidden children
				uint32 subLevel = item->fLevel;
				items++;

				while (--count > 0 && items[0]->fLevel > subLevel)
					items++;
			} else
				items++;
		}
		_RecalcItemTops(startIndex);
	} else {
		// collapse
		uint32 level = item->fLevel;
		int32 fullListIndex = FullListIndexOf(item);
		int32 index = IndexOf(item);
		int32 startIndex = index;
		int32 max = FullListCountItems() - fullListIndex - 1;
		int32 count = 0;
		bool selectionChanged = false;

		KListItem** items = (KListItem**)fFullList.Items() + fullListIndex + 1;

		while (max-- > 0) {
			item = items[0];
			if (item->fLevel <= level)
				break;

			if (item->IsItemVisible()) {
				fList.RemoveItem(item);
				item->SetItemVisible(false);
				if (item->IsSelected()) {
					selectionChanged = true;
					item->Deselect();
				}
				count++;
			}

			items++;
		}

		_RecalcItemTops(startIndex);
		// fix selection hints
		// if the selected item was just removed by collapsing, select its
		// parent
		if (ListType() == B_SINGLE_SELECTION_LIST && selectionChanged)
			fFirstSelected = fLastSelected = index;

		if (index < fFirstSelected && index + count < fFirstSelected) {
				// all items removed were higher than the selection range,
				// adjust the indexes to correspond to their new visible positions
				fFirstSelected -= count;
				fLastSelected -= count;
		}

		int32 maxIndex = fList.CountItems() - 1;
		if (fFirstSelected > maxIndex)
			fFirstSelected = maxIndex;

		if (fLastSelected > maxIndex)
			fLastSelected = maxIndex;

		if (selectionChanged)
			SelectionChanged();
	}

	_FixupScrollBar();
	Invalidate();
}


BRect
KOutlineListView::LatchRect(BRect itemRect, int32 level) const
{
	float latchWidth = be_plain_font->Size();
	float latchHeight = be_plain_font->Size();
	float indentOffset = level * k_be_control_look->DefaultItemSpacing();
	float heightOffset = itemRect.Height() / 2 - latchHeight / 2;

	return BRect(0, 0, latchWidth, latchHeight)
		.OffsetBySelf(itemRect.left, itemRect.top)
		.OffsetBySelf(indentOffset, heightOffset);
}


void
KOutlineListView::DrawLatch(BRect itemRect, int32 level, bool collapsed,
	bool highlighted, bool misTracked)
{
	BRect latchRect(LatchRect(itemRect, level));
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	int32 arrowDirection = collapsed ? KControlLook::B_RIGHT_ARROW
		: KControlLook::B_DOWN_ARROW;

	float tintColor = B_DARKEN_4_TINT;
	if (base.red + base.green + base.blue <= 128 * 3) {
		tintColor = B_LIGHTEN_2_TINT;
	}

	k_be_control_look->DrawArrowShape(this, latchRect, itemRect, base,
		arrowDirection, 0, tintColor);
}


void
KOutlineListView::DrawItem(KListItem* item, BRect itemRect, bool complete)
{
	if (item->fHasSubitems) {
		DrawLatch(itemRect, item->fLevel, !item->IsExpanded(),
			item->IsSelected() || complete, false);
	}

	itemRect.left += LatchRect(itemRect, item->fLevel).right;
	KListView::DrawItem(item, itemRect, complete);
}


int32
KOutlineListView::_FullListIndex(int32 index) const
{
	KListItem* item = ItemAt(index);

	if (item == NULL)
		return -1;

	return FullListIndexOf(item);
}


void
KOutlineListView::_PopulateTree(BList* tree, BList& target,
	int32& firstIndex, bool onlyVisible)
{
	KListItem** items = (KListItem**)target.Items();
	int32 count = tree->CountItems();

	for (int32 index = 0; index < count; index++) {
		KListItem* item = (KListItem*)tree->ItemAtFast(index);

		items[firstIndex++] = item;

		if (item->HasSubitems() && (!onlyVisible || item->IsExpanded())) {
			_PopulateTree(item->fTemporaryList, target, firstIndex,
				onlyVisible);
		}
	}
}


void
KOutlineListView::_SortTree(BList* tree, bool oneLevelOnly,
	int (*compareFunc)(const KListItem* a, const KListItem* b))
{
	KListItem** items = (KListItem**)tree->Items();
	std::sort(items, items + tree->CountItems(),
		ListItemComparator(compareFunc));

	if (oneLevelOnly)
		return;

	for (int32 index = tree->CountItems(); index-- > 0;) {
		KListItem* item = (KListItem*)tree->ItemAt(index);

		if (item->HasSubitems())
			_SortTree(item->fTemporaryList, false, compareFunc);
	}
}


void
KOutlineListView::_DestructTree(BList* tree)
{
	for (int32 index = tree->CountItems(); index-- > 0;) {
		KListItem* item = (KListItem*)tree->ItemAt(index);

		if (item->HasSubitems())
			_DestructTree(item->fTemporaryList);
	}

	delete tree;
}


BList*
KOutlineListView::_BuildTree(KListItem* superItem, int32& fullListIndex)
{
	int32 fullCount = FullListCountItems();
	uint32 level = superItem != NULL ? superItem->OutlineLevel() + 1 : 0;
	BList* list = new BList;
	if (superItem != NULL)
		superItem->fTemporaryList = list;

	while (fullListIndex < fullCount) {
		KListItem* item = FullListItemAt(fullListIndex);

		// If we jump out of the subtree, break out
		if (item->fLevel < level)
			break;

		// If the level matches, put them into the list
		// (we handle the case of a missing sublevel gracefully)
		list->AddItem(item);
		fullListIndex++;

		if (item->HasSubitems()) {
			// we're going deeper
			_BuildTree(item, fullListIndex);
		}
	}

	return list;
}


void
KOutlineListView::_CullInvisibleItems(BList& list)
{
	int32 index = 0;
	while (index < list.CountItems()) {
		if (reinterpret_cast<KListItem*>(list.ItemAt(index))->IsItemVisible())
			++index;
		else
			list.RemoveItem(index);
	}
}


bool
KOutlineListView::_SwapItems(int32 first, int32 second)
{
	// same item, do nothing
	if (first == second)
		return true;

	// fail, first item out of bounds
	if ((first < 0) || (first >= CountItems()))
		return false;

	// fail, second item out of bounds
	if ((second < 0) || (second >= CountItems()))
		return false;

	int32 firstIndex = min_c(first, second);
	int32 secondIndex = max_c(first, second);
	KListItem* firstItem = ItemAt(firstIndex);
	KListItem* secondItem = ItemAt(secondIndex);
	BList firstSubItems, secondSubItems;

	if (Superitem(firstItem) != Superitem(secondItem))
		return false;

	if (!firstItem->IsItemVisible() || !secondItem->IsItemVisible())
		return false;

	int32 fullFirstIndex = _FullListIndex(firstIndex);
	int32 fullSecondIndex = _FullListIndex(secondIndex);
	_GetSubItems(fFullList, firstSubItems, firstItem, fullFirstIndex + 1);
	_GetSubItems(fFullList, secondSubItems, secondItem, fullSecondIndex + 1);
	_DoSwap(fFullList, fullFirstIndex, fullSecondIndex, &firstSubItems,
		&secondSubItems);

	_CullInvisibleItems(firstSubItems);
	_CullInvisibleItems(secondSubItems);
	_DoSwap(fList, firstIndex, secondIndex, &firstSubItems,
		&secondSubItems);

	_RecalcItemTops(firstIndex);
	_RescanSelection(firstIndex, secondIndex + secondSubItems.CountItems());
	Invalidate(Bounds());

	return true;
}


/*!	\brief Removes a single item from the list and all of its children.

	Unlike the BeOS version, this one will actually delete the children, too,
	as there should be no reference left to them. This may cause problems for
	applications that actually take the misbehaviour of the Be classes into
	account.
*/
KListItem*
KOutlineListView::_RemoveItem(KListItem* item, int32 fullListIndex)
{
	if (item == NULL || fullListIndex < 0
		|| fullListIndex >= FullListCountItems()) {
		return NULL;
	}

	uint32 level = item->OutlineLevel();
	int32 superIndex;
	KListItem* super = _SuperitemForIndex(fullListIndex, level, &superIndex);

	if (item->IsItemVisible()) {
		// remove children, too
		while (fullListIndex + 1 < FullListCountItems()) {
			KListItem* subItem = FullListItemAt(fullListIndex + 1);

			if (subItem->OutlineLevel() <= level)
				break;

			if (subItem->IsItemVisible())
				KListView::RemoveItem(subItem);

			fFullList.RemoveItem(fullListIndex + 1);
			delete subItem;
		}
		KListView::RemoveItem(item);
	}

	fFullList.RemoveItem(fullListIndex);

	if (super != NULL) {
		// we might need to change the fHasSubitems field of the parent
		KListItem* child = FullListItemAt(superIndex + 1);
		if (child == NULL || child->OutlineLevel() <= super->OutlineLevel())
			super->fHasSubitems = false;
	}

	return item;
}


/*!	Returns the super item before the item specified by \a fullListIndex
	and \a level.
*/
KListItem*
KOutlineListView::_SuperitemForIndex(int32 fullListIndex, int32 level,
	int32* _superIndex)
{
	KListItem* item;
	fullListIndex--;

	while (fullListIndex >= 0) {
		if ((item = FullListItemAt(fullListIndex))->OutlineLevel()
				< (uint32)level) {
			if (_superIndex != NULL)
				*_superIndex = fullListIndex;
			return item;
		}

		fullListIndex--;
	}

	return NULL;
}


int32
KOutlineListView::_FindPreviousVisibleIndex(int32 fullListIndex)
{
	fullListIndex--;

	while (fullListIndex >= 0) {
		if (FullListItemAt(fullListIndex)->fVisible)
			return fullListIndex;

		fullListIndex--;
	}

	return -1;
}
