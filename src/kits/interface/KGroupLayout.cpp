/*
 * Copyright 2010 Haiku, Inc. All rights reserved.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 *
 * Distributed under the terms of the MIT License.
 */


#include <KGroupLayout.h>

#include <KControlLook.h>
#include <KLayoutItem.h>
#include <Message.h>

#include <new>


using std::nothrow;


namespace {
	const char* const kItemWeightField = "KGroupLayout:item:weight";
	const char* const kVerticalField = "KGroupLayout:vertical";
}


struct KGroupLayout::ItemLayoutData {
	float	weight;

	ItemLayoutData()
		: weight(1)
	{
	}
};


KGroupLayout::KGroupLayout(orientation orientation, float spacing)
	:
	KTwoDimensionalLayout(),
	fOrientation(orientation)
{
	SetSpacing(spacing);
}


KGroupLayout::KGroupLayout(BMessage* from)
	:
	KTwoDimensionalLayout(from)
{
	bool isVertical;
	if (from->FindBool(kVerticalField, &isVertical) != B_OK)
		isVertical = false;
	fOrientation = isVertical ? B_VERTICAL : B_HORIZONTAL;
}


KGroupLayout::~KGroupLayout()
{
}


float
KGroupLayout::Spacing() const
{
	return fHSpacing;
}


void
KGroupLayout::SetSpacing(float spacing)
{
	spacing = KControlLook::ComposeSpacing(spacing);
	if (spacing != fHSpacing) {
		fHSpacing = spacing;
		fVSpacing = spacing;
		InvalidateLayout();
	}
}


orientation
KGroupLayout::Orientation() const
{
	return fOrientation;
}


void
KGroupLayout::SetOrientation(orientation orientation)
{
	if (orientation != fOrientation) {
		fOrientation = orientation;

		InvalidateLayout();
	}
}


float
KGroupLayout::ItemWeight(int32 index) const
{
	if (index < 0 || index >= CountItems())
		return 0;

	ItemLayoutData* data = _LayoutDataForItem(ItemAt(index));
	return (data ? data->weight : 0);
}


void
KGroupLayout::SetItemWeight(int32 index, float weight)
{
	if (index < 0 || index >= CountItems())
		return;

	if (ItemLayoutData* data = _LayoutDataForItem(ItemAt(index)))
		data->weight = weight;

	InvalidateLayout();
}


KLayoutItem*
KGroupLayout::AddView(KView* child)
{
	return KTwoDimensionalLayout::AddView(child);
}


KLayoutItem*
KGroupLayout::AddView(int32 index, KView* child)
{
	return KTwoDimensionalLayout::AddView(index, child);
}


KLayoutItem*
KGroupLayout::AddView(KView* child, float weight)
{
	return AddView(-1, child, weight);
}


KLayoutItem*
KGroupLayout::AddView(int32 index, KView* child, float weight)
{
	KLayoutItem* item = AddView(index, child);
	if (ItemLayoutData* data = _LayoutDataForItem(item))
		data->weight = weight;

	return item;
}


bool
KGroupLayout::AddItem(KLayoutItem* item)
{
	return KTwoDimensionalLayout::AddItem(item);
}


bool
KGroupLayout::AddItem(int32 index, KLayoutItem* item)
{
	return KTwoDimensionalLayout::AddItem(index, item);
}


bool
KGroupLayout::AddItem(KLayoutItem* item, float weight)
{
	return AddItem(-1, item, weight);
}


bool
KGroupLayout::AddItem(int32 index, KLayoutItem* item, float weight)
{
	bool success = AddItem(index, item);
	if (success) {
		if (ItemLayoutData* data = _LayoutDataForItem(item))
			data->weight = weight;
	}

	return success;
}


status_t
KGroupLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t result = KTwoDimensionalLayout::Archive(into, deep);

	if (result == B_OK)
		result = into->AddBool(kVerticalField, fOrientation == B_VERTICAL);

	return archiver.Finish(result);
}


status_t
KGroupLayout::AllArchived(BMessage* into) const
{
	return KTwoDimensionalLayout::AllArchived(into);
}


status_t
KGroupLayout::AllUnarchived(const BMessage* from)
{
	return KTwoDimensionalLayout::AllUnarchived(from);
}


BArchivable*
KGroupLayout::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KGroupLayout"))
		return new(nothrow) KGroupLayout(from);
	return NULL;
}


status_t
KGroupLayout::ItemArchived(BMessage* into,
	KLayoutItem* item, int32 index) const
{
	return into->AddFloat(kItemWeightField, _LayoutDataForItem(item)->weight);
}


status_t
KGroupLayout::ItemUnarchived(const BMessage* from,
	KLayoutItem* item, int32 index)
{
	float weight;
	status_t result = from->FindFloat(kItemWeightField, index, &weight);

	if (result == B_OK)
		_LayoutDataForItem(item)->weight = weight;

	return result;
}


bool
KGroupLayout::ItemAdded(KLayoutItem* item, int32 atIndex)
{
	item->SetLayoutData(new(nothrow) ItemLayoutData);
	return item->LayoutData() != NULL;
}


void
KGroupLayout::ItemRemoved(KLayoutItem* item, int32 fromIndex)
{
	if (ItemLayoutData* data = _LayoutDataForItem(item)) {
		item->SetLayoutData(NULL);
		delete data;
	}
}


void
KGroupLayout::PrepareItems(orientation orientation)
{
	// filter the visible items
	fVisibleItems.MakeEmpty();
	int32 itemCount = CountItems();
	for (int i = 0; i < itemCount; i++) {
		KLayoutItem* item = ItemAt(i);
		if (item->IsVisible())
			fVisibleItems.AddItem(item);
	}
}


int32
KGroupLayout::InternalCountColumns()
{
	return (fOrientation == B_HORIZONTAL ? fVisibleItems.CountItems() : 1);
}


int32
KGroupLayout::InternalCountRows()
{
	return (fOrientation == B_VERTICAL ? fVisibleItems.CountItems() : 1);
}


void
KGroupLayout::GetColumnRowConstraints(orientation orientation, int32 index,
	ColumnRowConstraints* constraints)
{
	if (index >= 0 && index < fVisibleItems.CountItems()) {
		KLayoutItem* item = (KLayoutItem*)fVisibleItems.ItemAt(index);
		constraints->min = -1;
		constraints->max = B_SIZE_UNLIMITED;
		if (ItemLayoutData* data = _LayoutDataForItem(item))
			constraints->weight = data->weight;
		else
			constraints->weight = 1;
	}
}


void
KGroupLayout::GetItemDimensions(KLayoutItem* item, Dimensions* dimensions)
{
	int32 index = fVisibleItems.IndexOf(item);
	if (index < 0)
		return;

	if (fOrientation == B_HORIZONTAL) {
		dimensions->x = index;
		dimensions->y = 0;
		dimensions->width = 1;
		dimensions->height = 1;
	} else {
		dimensions->x = 0;
		dimensions->y = index;
		dimensions->width = 1;
		dimensions->height = 1;
	}
}


KGroupLayout::ItemLayoutData*
KGroupLayout::_LayoutDataForItem(KLayoutItem* item) const
{
	return item == NULL ? NULL : (ItemLayoutData*)item->LayoutData();
}


status_t
KGroupLayout::Perform(perform_code code, void* _data)
{
	return KTwoDimensionalLayout::Perform(code, _data);
}


void KGroupLayout::_ReservedGroupLayout1() {}
void KGroupLayout::_ReservedGroupLayout2() {}
void KGroupLayout::_ReservedGroupLayout3() {}
void KGroupLayout::_ReservedGroupLayout4() {}
void KGroupLayout::_ReservedGroupLayout5() {}
void KGroupLayout::_ReservedGroupLayout6() {}
void KGroupLayout::_ReservedGroupLayout7() {}
void KGroupLayout::_ReservedGroupLayout8() {}
void KGroupLayout::_ReservedGroupLayout9() {}
void KGroupLayout::_ReservedGroupLayout10() {}
