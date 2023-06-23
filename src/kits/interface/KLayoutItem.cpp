/*
 * Copyright 2010-2012, Haiku, Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <KLayoutItem.h>

#include <KLayout.h>
#include <LayoutUtils.h>
#include <Laminate.h>
#include <ViewPrivate.h>

#include <algorithm>


KLayoutItem::KLayoutItem()
	:
	fLayout(NULL),
	fLayoutData(NULL)
{
}


KLayoutItem::KLayoutItem(BMessage* from)
	:
	BArchivable(BUnarchiver::PrepareArchive(from)),
	fLayout(NULL),
	fLayoutData(NULL)
{
	BUnarchiver(from).Finish();
}


KLayoutItem::~KLayoutItem()
{
	if (fLayout != NULL) {
		debugger("Deleting a KLayoutItem that is still attached to a layout. "
			"Call RemoveSelf first.");
	}
}


KLayout*
KLayoutItem::Layout() const
{
	return fLayout;
}


bool
KLayoutItem::RemoveSelf()
{
	return Layout() != NULL && Layout()->RemoveItem(this);
}


void
KLayoutItem::SetExplicitSize(BSize size)
{
	SetExplicitMinSize(size);
	SetExplicitMaxSize(size);
	SetExplicitPreferredSize(size);
}


bool
KLayoutItem::HasHeightForWidth()
{
	// no "height for width" by default
	return false;
}


void
KLayoutItem::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	// no "height for width" by default
}


KView*
KLayoutItem::View()
{
	return NULL;
}


void
KLayoutItem::InvalidateLayout(bool children)
{
	LayoutInvalidated(children);
	if (fLayout)
		fLayout->InvalidateLayout(children);
}


void
KLayoutItem::Relayout(bool immediate)
{
	KView* view = View();
	if (view && !immediate)
		view->Relayout();
	else if (view && immediate)
		view->Layout(false);
}


void*
KLayoutItem::LayoutData() const
{
	return fLayoutData;
}


void
KLayoutItem::SetLayoutData(void* data)
{
	fLayoutData = data;
}


void
KLayoutItem::AlignInFrame(BRect frame)
{
	BSize maxSize = MaxSize();
	BAlignment alignment = Alignment();

	if (HasHeightForWidth()) {
		// The item has height for width, so we do the horizontal alignment
		// ourselves and restrict the height max constraint respectively.
		if (maxSize.width < frame.Width()
			&& alignment.horizontal != B_ALIGN_USE_FULL_WIDTH) {
			frame.left += (int)((frame.Width() - maxSize.width)
				* alignment.horizontal);
			frame.right = frame.left + maxSize.width;
		}
		alignment.horizontal = B_ALIGN_USE_FULL_WIDTH;

		float minHeight;
		GetHeightForWidth(frame.Width(), &minHeight, NULL, NULL);

		frame.bottom = frame.top + max_c(frame.Height(), minHeight);
		maxSize.height = minHeight;
	}

	SetFrame(BLayoutUtils::AlignInFrame(frame, maxSize, alignment));
}


status_t
KLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BArchivable::Archive(into, deep);

	if (err == B_OK)
		err = archiver.Finish();

	return err;
}


status_t
KLayoutItem::AllArchived(BMessage* into) const
{
	BArchiver archiver(into);
	return BArchivable::AllArchived(into);
}


status_t
KLayoutItem::AllUnarchived(const BMessage* from)
{
	return BArchivable::AllUnarchived(from);
}


void
KLayoutItem::SetLayout(KLayout* layout)
{
	if (layout == fLayout)
		return;

	KLayout* oldLayout = fLayout;
	fLayout = layout;

	if (oldLayout)
		DetachedFromLayout(oldLayout);

	if (KView* view = View()) {
		if (oldLayout && !fLayout) {
			KView::KPrivate(view).DeregisterLayoutItem(this);
		} else if (fLayout && !oldLayout) {
			KView::KPrivate(view).RegisterLayoutItem(this);
		}
	}

	if (fLayout)
		AttachedToLayout();
}


status_t
KLayoutItem::Perform(perform_code code, void* _data)
{
	return BArchivable::Perform(code, _data);
}


void
KLayoutItem::LayoutInvalidated(bool children)
{
	// hook method
}


void
KLayoutItem::AttachedToLayout()
{
	// hook method
}


void
KLayoutItem::DetachedFromLayout(KLayout* oldLayout)
{
	// hook method
}


void
KLayoutItem::AncestorVisibilityChanged(bool shown)
{
	// hook method
}


// Binary compatibility stuff


void KLayoutItem::_ReservedLayoutItem1() {}
void KLayoutItem::_ReservedLayoutItem2() {}
void KLayoutItem::_ReservedLayoutItem3() {}
void KLayoutItem::_ReservedLayoutItem4() {}
void KLayoutItem::_ReservedLayoutItem5() {}
void KLayoutItem::_ReservedLayoutItem6() {}
void KLayoutItem::_ReservedLayoutItem7() {}
void KLayoutItem::_ReservedLayoutItem8() {}
void KLayoutItem::_ReservedLayoutItem9() {}
void KLayoutItem::_ReservedLayoutItem10() {}

