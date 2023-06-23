/*
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <KCardLayout.h>

#include <KLayoutItem.h>
#include <Message.h>
#include <Laminate.h>


namespace {
	const char* kVisibleItemField = "KCardLayout:visibleItem";
}


KCardLayout::KCardLayout()
	:
	KAbstractLayout(),
	fMin(0, 0),
	fMax(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED),
	fPreferred(0, 0),
	fVisibleItem(NULL),
	fMinMaxValid(false)
{
}


KCardLayout::KCardLayout(BMessage* from)
	:
	KAbstractLayout(BUnarchiver::PrepareArchive(from)),
	fMin(0, 0),
	fMax(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED),
	fPreferred(0, 0),
	fVisibleItem(NULL),
	fMinMaxValid(false)
{
	BUnarchiver(from).Finish();
}


KCardLayout::~KCardLayout()
{
}


KLayoutItem*
KCardLayout::VisibleItem() const
{
	return fVisibleItem;
}


int32
KCardLayout::VisibleIndex() const
{
	return IndexOfItem(fVisibleItem);
}


void
KCardLayout::SetVisibleItem(int32 index)
{
	SetVisibleItem(ItemAt(index));
}


void
KCardLayout::SetVisibleItem(KLayoutItem* item)
{
	if (item == fVisibleItem)
		return;

	if (item != NULL && IndexOfItem(item) < 0) {
		debugger("KCardLayout::SetVisibleItem(KLayoutItem*): this item is not "
			"part of this layout, or the item does not exist.");
		return;
	}

	// Changing an item's visibility will invalidate its parent's layout (us),
	// which would normally cause the min-max to be re-computed. But in this
	// case, that is unnecessary, and so we can skip it.
	const bool minMaxValid = fMinMaxValid;

	if (fVisibleItem != NULL)
		fVisibleItem->SetVisible(false);

	fVisibleItem = item;

	if (fVisibleItem != NULL)
		fVisibleItem->SetVisible(true);

	fMinMaxValid = minMaxValid;
}


BSize
KCardLayout::BaseMinSize()
{
	_ValidateMinMax();
	return fMin;
}


BSize
KCardLayout::BaseMaxSize()
{
	_ValidateMinMax();
	return fMax;
}


BSize
KCardLayout::BasePreferredSize()
{
	_ValidateMinMax();
	return fPreferred;
}


BAlignment
KCardLayout::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


bool
KCardLayout::HasHeightForWidth()
{
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		if (ItemAt(i)->HasHeightForWidth())
			return true;
	}

	return false;
}


void
KCardLayout::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	_ValidateMinMax();

	// init with useful values
	float minHeight = fMin.height;
	float maxHeight = fMax.height;
	float preferredHeight = fPreferred.height;

	// apply the items' constraints
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		KLayoutItem* item = ItemAt(i);
		if (item->HasHeightForWidth()) {
			float itemMinHeight;
			float itemMaxHeight;
			float itemPreferredHeight;
			item->GetHeightForWidth(width, &itemMinHeight, &itemMaxHeight,
				&itemPreferredHeight);
			minHeight = max_c(minHeight, itemMinHeight);
			maxHeight = min_c(maxHeight, itemMaxHeight);
			preferredHeight = min_c(preferredHeight, itemPreferredHeight);
		}
	}

	// adjust max and preferred, if necessary
	maxHeight = max_c(maxHeight, minHeight);
	preferredHeight = max_c(preferredHeight, minHeight);
	preferredHeight = min_c(preferredHeight, maxHeight);

	if (min)
		*min = minHeight;
	if (max)
		*max = maxHeight;
	if (preferred)
		*preferred = preferredHeight;
}


void
KCardLayout::LayoutInvalidated(bool children)
{
	fMinMaxValid = false;
}


void
KCardLayout::DoLayout()
{
	_ValidateMinMax();

	BSize size(LayoutArea().Size());

	// this cannot be done when we are viewless, as our children
	// would not get cut off in the right place.
	if (Owner()) {
		size.width = max_c(size.width, fMin.width);
		size.height = max_c(size.height, fMin.height);
	}

	if (fVisibleItem != NULL)
		fVisibleItem->AlignInFrame(BRect(LayoutArea().LeftTop(), size));
}


status_t
KCardLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = KAbstractLayout::Archive(into, deep);

	if (err == B_OK && deep)
		err = into->AddInt32(kVisibleItemField, IndexOfItem(fVisibleItem));

	return archiver.Finish(err);
}


status_t
KCardLayout::AllArchived(BMessage* archive) const
{
	return KAbstractLayout::AllArchived(archive);
}


status_t
KCardLayout::AllUnarchived(const BMessage* from)
{
	status_t err = KLayout::AllUnarchived(from);
	if (err != B_OK)
		return err;

	int32 visibleIndex;
	err = from->FindInt32(kVisibleItemField, &visibleIndex);
	if (err == B_OK)
		SetVisibleItem(visibleIndex);

	return err;
}


status_t
KCardLayout::ItemArchived(BMessage* into, KLayoutItem* item, int32 index) const
{
	return KAbstractLayout::ItemArchived(into, item, index);
}


status_t
KCardLayout::ItemUnarchived(const BMessage* from, KLayoutItem* item,
	int32 index)
{
	return KAbstractLayout::ItemUnarchived(from, item, index);
}



BArchivable*
KCardLayout::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KCardLayout"))
		return new KCardLayout(from);
	return NULL;
}


bool
KCardLayout::ItemAdded(KLayoutItem* item, int32 atIndex)
{
	if (CountItems() <= 1)
		SetVisibleItem(item);
	else
		item->SetVisible(false);
	return true;
}


void
KCardLayout::ItemRemoved(KLayoutItem* item, int32 fromIndex)
{
	fMinMaxValid = false;

	if (fVisibleItem == item) {
		KLayoutItem* newVisibleItem = NULL;
		SetVisibleItem(newVisibleItem);
	}
}


void
KCardLayout::_ValidateMinMax()
{
	if (fMinMaxValid)
		return;

	fMin.width = 0;
	fMin.height = 0;
	fMax.width = B_SIZE_UNLIMITED;
	fMax.height = B_SIZE_UNLIMITED;
	fPreferred.width = 0;
	fPreferred.height = 0;

	int32 itemCount = CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		KLayoutItem* item = ItemAt(i);

		BSize min = item->MinSize();
		BSize max = item->MaxSize();
		BSize preferred = item->PreferredSize();

		fMin.width = max_c(fMin.width, min.width);
		fMin.height = max_c(fMin.height, min.height);

		fMax.width = min_c(fMax.width, max.width);
		fMax.height = min_c(fMax.height, max.height);

		fPreferred.width = max_c(fPreferred.width, preferred.width);
		fPreferred.height = max_c(fPreferred.height, preferred.height);
	}

	fMax.width = max_c(fMax.width, fMin.width);
	fMax.height = max_c(fMax.height, fMin.height);

	fPreferred.width = max_c(fPreferred.width, fMin.width);
	fPreferred.height = max_c(fPreferred.height, fMin.height);
	fPreferred.width = min_c(fPreferred.width, fMax.width);
	fPreferred.height = min_c(fPreferred.height, fMax.height);

	fMinMaxValid = true;
	ResetLayoutInvalidation();
}


status_t
KCardLayout::Perform(perform_code d, void* arg)
{
	return KAbstractLayout::Perform(d, arg);
}


void KCardLayout::_ReservedCardLayout1() {}
void KCardLayout::_ReservedCardLayout2() {}
void KCardLayout::_ReservedCardLayout3() {}
void KCardLayout::_ReservedCardLayout4() {}
void KCardLayout::_ReservedCardLayout5() {}
void KCardLayout::_ReservedCardLayout6() {}
void KCardLayout::_ReservedCardLayout7() {}
void KCardLayout::_ReservedCardLayout8() {}
void KCardLayout::_ReservedCardLayout9() {}
void KCardLayout::_ReservedCardLayout10() {}

