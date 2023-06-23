/*
 * Copyright 2010, Haiku Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <KLayout.h>

#include <algorithm>
#include <new>
#include <syslog.h>

#include <AutoDeleter.h>
#include <LayoutContext.h>
#include <Message.h>
#include <Laminate.h>
#include <ViewPrivate.h>

#include "KViewLayoutItem.h"


using BPrivate::AutoDeleter;

using std::nothrow;
using std::swap;


//khidki code
//start
//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TTRACE(x) debug_printf x
#else
#	define TTRACE(x) ;
#endif
//end

namespace {
	// flags for our state
	const uint32 B_LAYOUT_INVALID = 0x80000000UL; // needs layout
	const uint32 B_LAYOUT_CACHE_INVALID = 0x40000000UL; // needs recalculation
	const uint32 B_LAYOUT_REQUIRED = 0x20000000UL; // needs layout
	const uint32 B_LAYOUT_IN_PROGRESS = 0x10000000UL;
	const uint32 B_LAYOUT_ALL_CLEAR = 0UL;

	// handy masks to check various states
	const uint32 B_LAYOUT_INVALIDATION_ILLEGAL
		= B_LAYOUT_CACHE_INVALID | B_LAYOUT_IN_PROGRESS;
	const uint32 B_LAYOUT_NECESSARY
		= B_LAYOUT_INVALID | B_LAYOUT_REQUIRED | B_LAYOUT_CACHE_INVALID;
	const uint32 B_RELAYOUT_NOT_OK
		= B_LAYOUT_INVALID | B_LAYOUT_IN_PROGRESS;

	const char* const kLayoutItemField = "KLayout:items";


	struct ViewRemover {
		inline void operator()(KView* view) {
			if (view)
				KView::KPrivate(view).RemoveSelf();
		}
	};
}


KLayout::KLayout()
	:
	fState(B_LAYOUT_ALL_CLEAR),
	fAncestorsVisible(true),
	fInvalidationDisabled(0),
	fContext(NULL),
	fOwner(NULL),
	fTarget(NULL),
	fItems(20)
{
debug_printf("[KLayout]{KLayout}\n");
}


KLayout::KLayout(BMessage* from)
	:
	KLayoutItem(BUnarchiver::PrepareArchive(from)),
	fState(B_LAYOUT_ALL_CLEAR),
	fAncestorsVisible(true),
	fInvalidationDisabled(0),
	fContext(NULL),
	fOwner(NULL),
	fTarget(NULL),
	fItems(20)
{
	BUnarchiver unarchiver(from);

	int32 i = 0;
	while (unarchiver.EnsureUnarchived(kLayoutItemField, i++) == B_OK)
		;
}


KLayout::~KLayout()
{
	// in case we have a view, but have been added to a layout as a KLayoutItem
	// we will get deleted before our view, so we should tell it that we're
	// going, so that we aren't double-freed.
	if (fOwner && this == fOwner->GetLayout())
		fOwner->_LayoutLeft(this);

	if (CountItems() > 0) {
		debugger("Deleting a KLayout that still has items. Subclass hooks "
			"will not be called");
	}
}


KView*
KLayout::Owner() const
{
	return fOwner;
}


KView*
KLayout::TargetView() const
{
	return fTarget;
}


KView*
KLayout::View()
{
	return fOwner;
}


KLayoutItem*
KLayout::AddView(KView* child)
{
	return AddView(-1, child);
}


KLayoutItem*
KLayout::AddView(int32 index, KView* child)
{
	KLayoutItem* item = child->GetLayout();
	ObjectDeleter<KLayoutItem> itemDeleter(NULL);
	if (!item) {
		item = new(nothrow) KViewLayoutItem(child);
		itemDeleter.SetTo(item);
	}

	if (item && AddItem(index, item)) {
		itemDeleter.Detach();
		return item;
	}

	return NULL;
}


bool
KLayout::AddItem(KLayoutItem* item)
{
	return AddItem(-1, item);
}


bool
KLayout::AddItem(int32 index, KLayoutItem* item)
{
	if (!fTarget || !item || fItems.HasItem(item))
		return false;

	// if the item refers to a KView, we make sure it is added to the parent
	// view
	KView* view = item->View();
	AutoDeleter<KView, ViewRemover> remover(NULL);
		// In case of errors, we don't want to leave this view added where it
		// shouldn't be.
	if (view && view->fParent != fTarget) {
		if (!fTarget->_AddChild(view, NULL))
			return false;
		else
			remover.SetTo(view);
	}

	// validate the index
	if (index < 0 || index > fItems.CountItems())
		index = fItems.CountItems();

	if (!fItems.AddItem(item, index))
		return false;

	if (!ItemAdded(item, index)) {
		fItems.RemoveItem(index);
		return false;
	}

	item->SetLayout(this);
	if (!fAncestorsVisible)
		item->AncestorVisibilityChanged(fAncestorsVisible);
	InvalidateLayout();
	remover.Detach();
	return true;
}


bool
KLayout::RemoveView(KView* child)
{
	bool removed = false;

	// a view can have any number of layout items - we need to remove them all
	int32 remaining = KView::KPrivate(child).CountLayoutItems();
	for (int32 i = CountItems() - 1; i >= 0 && remaining > 0; i--) {
		KLayoutItem* item = ItemAt(i);

		if (item->View() != child)
			continue;

		RemoveItem(i);
		if (item != child->GetLayout())
			delete item;

		remaining--;
		removed = true;
	}

	return removed;
}


bool
KLayout::RemoveItem(KLayoutItem* item)
{
	int32 index = IndexOfItem(item);
	return (index >= 0 ? RemoveItem(index) != NULL : false);
}


KLayoutItem*
KLayout::RemoveItem(int32 index)
{
	if (index < 0 || index >= fItems.CountItems())
		return NULL;

	KLayoutItem* item = (KLayoutItem*)fItems.RemoveItem(index);
	ItemRemoved(item, index);
	item->SetLayout(NULL);

	// If this is the last item in use that refers to its KView,
	// that KView now needs to be removed. UNLESS fTarget is NULL,
	// in which case we leave the view as is. (See SetTarget() for more info)
	KView* view = item->View();
	if (fTarget && view && KView::KPrivate(view).CountLayoutItems() == 0)
		view->_RemoveSelf();

	InvalidateLayout();
	return item;
}


KLayoutItem*
KLayout::ItemAt(int32 index) const
{
	return (KLayoutItem*)fItems.ItemAt(index);
}


int32
KLayout::CountItems() const
{
	return fItems.CountItems();
}


int32
KLayout::IndexOfItem(const KLayoutItem* item) const
{
	return fItems.IndexOf(item);
}


int32
KLayout::IndexOfView(KView* child) const
{
	if (child == NULL)
		return -1;

	// A KView can have many items, so we just do our best and return the
	// index of the first one in this layout.
	KView::KPrivate viewPrivate(child);
	int32 itemCount = viewPrivate.CountLayoutItems();
	for (int32 i = 0; i < itemCount; i++) {
		KLayoutItem* item = viewPrivate.LayoutItemAt(i);
		if (item->Layout() == this)
			return IndexOfItem(item);
	}
	return -1;
}


bool
KLayout::AncestorsVisible() const
{
	return fAncestorsVisible;
}


void
KLayout::InvalidateLayout(bool children)
{
debug_printf("[KLayout]{InvalidateLayout} start\n");
	// printf("KLayout(%p)::InvalidateLayout(%i) : state %x, disabled %li\n",
	// this, children, (unsigned int)fState, fInvalidationDisabled);

	if (fTarget && fTarget->IsLayoutInvalidationDisabled())
		return;
	if (fInvalidationDisabled > 0
		|| (fState & B_LAYOUT_INVALIDATION_ILLEGAL) != 0) {
		return;
	}

	fState |= B_LAYOUT_NECESSARY;
	LayoutInvalidated(children);

	if (children) {
		for (int32 i = CountItems() - 1; i >= 0; i--)
			ItemAt(i)->InvalidateLayout(children);
	}

	if (fOwner)
		fOwner->InvalidateLayout(children);

	if (KLayout* nestedIn = Layout()) {
		nestedIn->InvalidateLayout();
	} else if (fOwner) {
		// If we weren't added as a KLayoutItem, we still have to invalidate
		// whatever layout our owner is in.
		fOwner->_InvalidateParentLayout();
	}
debug_printf("[KLayout]{InvalidateLayout}ends\n");
}


void
KLayout::RequireLayout()
{
	fState |= B_LAYOUT_REQUIRED;
}


bool
KLayout::IsValid()
{
	return (fState & B_LAYOUT_INVALID) == 0;
}


void
KLayout::DisableLayoutInvalidation()
{
	fInvalidationDisabled++;
}


void
KLayout::EnableLayoutInvalidation()
{
	if (fInvalidationDisabled > 0)
		fInvalidationDisabled--;
}


void
KLayout::LayoutItems(bool force)
{
	if ((fState & B_LAYOUT_NECESSARY) == 0 && !force)
		return;

	if (Layout() && (Layout()->fState & B_LAYOUT_IN_PROGRESS) != 0)
		return; // wait for parent layout to lay us out.

	if (fTarget && fTarget->LayoutContext())
		return;

	BLayoutContext context;
	_LayoutWithinContext(force, &context);
}


void
KLayout::Relayout(bool immediate)
{
	if ((fState & B_RELAYOUT_NOT_OK) == 0 || immediate) {
		fState |= B_LAYOUT_REQUIRED;
		LayoutItems(false);
	}
}


void
KLayout::_LayoutWithinContext(bool force, BLayoutContext* context)
{
// printf("KLayout(%p)::_LayoutWithinContext(%i, %p), state %x, fContext %p\n",
// this, force, context, (unsigned int)fState, fContext);

	if ((fState & B_LAYOUT_NECESSARY) == 0 && !force)
		return;

	BLayoutContext* oldContext = fContext;
	fContext = context;

	if (fOwner && KView::KPrivate(fOwner).WillLayout()) {
		// in this case, let our owner decide whether or not to have us
		// do our layout, if they do, we won't end up here again.
		fOwner->_Layout(force, context);
	} else {
		fState |= B_LAYOUT_IN_PROGRESS;
		DoLayout();
		// we must ensure that all items are laid out, layouts with a view will
		// have their layout process triggered by their view, but nested
		// view-less layouts must have their layout triggered here (if it hasn't
		// already been triggered).
		int32 nestedLayoutCount = fNestedLayouts.CountItems();
		for (int32 i = 0; i < nestedLayoutCount; i++) {
			KLayout* layout = (KLayout*)fNestedLayouts.ItemAt(i);
			if ((layout->fState & B_LAYOUT_NECESSARY) != 0)
				layout->_LayoutWithinContext(force, context);
		}
		fState = B_LAYOUT_ALL_CLEAR;
	}

	fContext = oldContext;
}


BRect
KLayout::LayoutArea()
{
	BRect area(Frame());
	if (fOwner)
		area.OffsetTo(B_ORIGIN);
	return area;
}


status_t
KLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = KLayoutItem::Archive(into, deep);

	if (deep) {
		int32 count = CountItems();
		for (int32 i = 0; i < count && err == B_OK; i++) {
			KLayoutItem* item = ItemAt(i);
			err = archiver.AddArchivable(kLayoutItemField, item, deep);

			if (err == B_OK) {
				err = ItemArchived(into, item, i);
				if (err != B_OK)
					syslog(LOG_ERR, "ItemArchived() failed at index: %d.", i);
			}
		}
	}

	return archiver.Finish(err);
}


status_t
KLayout::AllArchived(BMessage* archive) const
{
	return KLayoutItem::AllArchived(archive);
}


status_t
KLayout::AllUnarchived(const BMessage* from)
{
	BUnarchiver unarchiver(from);
	status_t err = KLayoutItem::AllUnarchived(from);
	if (err != B_OK)
		return err;

	int32 itemCount = 0;
	unarchiver.ArchiveMessage()->GetInfo(kLayoutItemField, NULL, &itemCount);
	for (int32 i = 0; i < itemCount && err == B_OK; i++) {
		KLayoutItem* item;
		err = unarchiver.FindObject(kLayoutItemField,
			i, BUnarchiver::B_DONT_ASSUME_OWNERSHIP, item);
		if (err != B_OK)
			return err;

		if (!fItems.AddItem(item, i) || !ItemAdded(item, i)) {
			fItems.RemoveItem(i);
			return B_ERROR;
		}

		err = ItemUnarchived(from, item, i);
		if (err != B_OK) {
			fItems.RemoveItem(i);
			ItemRemoved(item, i);
			return err;
		}

		item->SetLayout(this);
		unarchiver.AssumeOwnership(item);
	}

	InvalidateLayout();
	return err;
}


status_t
KLayout::ItemArchived(BMessage* into, KLayoutItem* item, int32 index) const
{
	return B_OK;
}


status_t
KLayout::ItemUnarchived(const BMessage* from, KLayoutItem* item, int32 index)
{
	return B_OK;
}


bool
KLayout::ItemAdded(KLayoutItem* item, int32 atIndex)
{
	return true;
}


void
KLayout::ItemRemoved(KLayoutItem* item, int32 fromIndex)
{
}


void
KLayout::LayoutInvalidated(bool children)
{
}


void
KLayout::OwnerChanged(KView* was)
{
}


void
KLayout::AttachedToLayout()
{
	if (!fOwner) {
		Layout()->fNestedLayouts.AddItem(this);
		SetTarget(Layout()->TargetView());
	}
}


void
KLayout::DetachedFromLayout(KLayout* from)
{
	if (!fOwner) {
		from->fNestedLayouts.RemoveItem(this);
		SetTarget(NULL);
	}
}


void
KLayout::AncestorVisibilityChanged(bool shown)
{
	if (fAncestorsVisible == shown)
		return;

	fAncestorsVisible = shown;
	VisibilityChanged(shown);
}


void
KLayout::VisibilityChanged(bool show)
{
	if (fOwner)
		return;

	for (int32 i = CountItems() - 1; i >= 0; i--)
		ItemAt(i)->AncestorVisibilityChanged(show);
}


void
KLayout::ResetLayoutInvalidation()
{
	fState &= ~B_LAYOUT_CACHE_INVALID;
}


BLayoutContext*
KLayout::LayoutContext() const
{
	return fContext;
}


void
KLayout::SetOwner(KView* owner)
{
	if (fOwner == owner)
		return;

	SetTarget(owner);
	swap(fOwner, owner);

	OwnerChanged(owner);
		// call hook
}


void
KLayout::SetTarget(KView* target)
{
	if (fTarget != target) {
		/* With fTarget NULL, RemoveItem() will not remove the views from their
		 * parent. This ensures that the views are not lost to the void.
		 */
		fTarget = NULL;

		// remove and delete all items
		for (int32 i = CountItems() - 1; i >= 0; i--)
			delete RemoveItem(i);

		fTarget = target;

		InvalidateLayout();
	}
}


// Binary compatibility stuff


status_t
KLayout::Perform(perform_code code, void* _data)
{
	return KLayoutItem::Perform(code, _data);
}


void KLayout::_ReservedLayout1() {}
void KLayout::_ReservedLayout2() {}
void KLayout::_ReservedLayout3() {}
void KLayout::_ReservedLayout4() {}
void KLayout::_ReservedLayout5() {}
void KLayout::_ReservedLayout6() {}
void KLayout::_ReservedLayout7() {}
void KLayout::_ReservedLayout8() {}
void KLayout::_ReservedLayout9() {}
void KLayout::_ReservedLayout10() {}

