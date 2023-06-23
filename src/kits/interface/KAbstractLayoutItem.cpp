/*
 * Copyright 2010, Haiku, Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <KAbstractLayoutItem.h>

#include <LayoutUtils.h>
#include <Message.h>


namespace {
	const char* const kSizesField = "KAbstractLayoutItem:sizes";
		// kSizesField == {min, max, preferred}
	const char* const kAlignmentField = "KAbstractLayoutItem:alignment";
}


KAbstractLayoutItem::KAbstractLayoutItem()
	:
	fMinSize(),
	fMaxSize(),
	fPreferredSize(),
	fAlignment()
{
}


KAbstractLayoutItem::KAbstractLayoutItem(BMessage* from)
	:
	KLayoutItem(from),
	fMinSize(),
	fMaxSize(),
	fPreferredSize(),
	fAlignment()
{
	from->FindSize(kSizesField, 0, &fMinSize);
	from->FindSize(kSizesField, 1, &fMaxSize);
	from->FindSize(kSizesField, 2, &fPreferredSize);
	from->FindAlignment(kAlignmentField, &fAlignment);
}


KAbstractLayoutItem::~KAbstractLayoutItem()
{
}


BSize
KAbstractLayoutItem::MinSize()
{
	return BLayoutUtils::ComposeSize(fMinSize, BaseMinSize());
}


BSize
KAbstractLayoutItem::MaxSize()
{
	return BLayoutUtils::ComposeSize(fMaxSize, BaseMaxSize());
}


BSize
KAbstractLayoutItem::PreferredSize()
{
	return BLayoutUtils::ComposeSize(fMaxSize, BasePreferredSize());
}


BAlignment
KAbstractLayoutItem::Alignment()
{
	return BLayoutUtils::ComposeAlignment(fAlignment, BaseAlignment());
}


void
KAbstractLayoutItem::SetExplicitMinSize(BSize size)
{
	fMinSize = size;
}


void
KAbstractLayoutItem::SetExplicitMaxSize(BSize size)
{
	fMaxSize = size;
}


void
KAbstractLayoutItem::SetExplicitPreferredSize(BSize size)
{
	fPreferredSize = size;
}


void
KAbstractLayoutItem::SetExplicitAlignment(BAlignment alignment)
{
	fAlignment = alignment;
}


BSize
KAbstractLayoutItem::BaseMinSize()
{
	return BSize(0, 0);
}


BSize
KAbstractLayoutItem::BaseMaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


BSize
KAbstractLayoutItem::BasePreferredSize()
{
	return BSize(0, 0);
}


BAlignment
KAbstractLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER);
}


status_t
KAbstractLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = KLayoutItem::Archive(into, deep);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fMinSize);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fMaxSize);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fPreferredSize);

	if (err == B_OK)
		err = into->AddAlignment(kAlignmentField, fAlignment);

	return archiver.Finish(err);
}


status_t
KAbstractLayoutItem::AllUnarchived(const BMessage* archive)
{
	return KLayoutItem::AllUnarchived(archive);
}


status_t
KAbstractLayoutItem::AllArchived(BMessage* archive) const
{
	return KLayoutItem::AllArchived(archive);
}


void
KAbstractLayoutItem::LayoutInvalidated(bool children)
{
	KLayoutItem::LayoutInvalidated(children);
}


void
KAbstractLayoutItem::AttachedToLayout()
{
	KLayoutItem::AttachedToLayout();
}


void
KAbstractLayoutItem::DetachedFromLayout(KLayout* layout)
{
	KLayoutItem::DetachedFromLayout(layout);
}


void
KAbstractLayoutItem::AncestorVisibilityChanged(bool shown)
{
	KLayoutItem::AncestorVisibilityChanged(shown);
}


status_t
KAbstractLayoutItem::Perform(perform_code d, void* arg)
{
	return KLayoutItem::Perform(d, arg);
}


void KAbstractLayoutItem::_ReservedAbstractLayoutItem1() {}
void KAbstractLayoutItem::_ReservedAbstractLayoutItem2() {}
void KAbstractLayoutItem::_ReservedAbstractLayoutItem3() {}
void KAbstractLayoutItem::_ReservedAbstractLayoutItem4() {}
void KAbstractLayoutItem::_ReservedAbstractLayoutItem5() {}
void KAbstractLayoutItem::_ReservedAbstractLayoutItem6() {}
void KAbstractLayoutItem::_ReservedAbstractLayoutItem7() {}
void KAbstractLayoutItem::_ReservedAbstractLayoutItem8() {}
void KAbstractLayoutItem::_ReservedAbstractLayoutItem9() {}
void KAbstractLayoutItem::_ReservedAbstractLayoutItem10() {}

