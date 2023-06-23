/*
 * Copyright 2010, Haiku, Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <KSpaceLayoutItem.h>

#include <new>

#include <KControlLook.h>
#include <Message.h>


namespace {
	const char* const kSizesField = "KSpaceLayoutItem:sizes";
		// kSizesField = {min, max, preferred}
	const char* const kAlignmentField = "KSpaceLayoutItem:alignment";
	const char* const kFrameField = "KSpaceLayoutItem:frame";
	const char* const kVisibleField = "KSpaceLayoutItem:visible";

	BSize& ComposeSpacingInPlace(BSize& size)
	{
		size.width = KControlLook::ComposeSpacing(size.width);
		size.height = KControlLook::ComposeSpacing(size.height);
		return size;
	}
}


KSpaceLayoutItem::KSpaceLayoutItem(BSize minSize, BSize maxSize,
	BSize preferredSize, BAlignment alignment)
	:
	fFrame(),
	fMinSize(ComposeSpacingInPlace(minSize)),
	fMaxSize(ComposeSpacingInPlace(maxSize)),
	fPreferredSize(ComposeSpacingInPlace(preferredSize)),
	fAlignment(alignment),
	fVisible(true)
{
}


KSpaceLayoutItem::KSpaceLayoutItem(BMessage* archive)
	:
	KLayoutItem(archive)
{
	archive->FindSize(kSizesField, 0, &fMinSize);
	archive->FindSize(kSizesField, 1, &fMaxSize);
	archive->FindSize(kSizesField, 2, &fPreferredSize);

	archive->FindAlignment(kAlignmentField, &fAlignment);

	archive->FindRect(kFrameField, &fFrame);
	archive->FindBool(kVisibleField, &fVisible);
}


KSpaceLayoutItem::~KSpaceLayoutItem()
{
}


KSpaceLayoutItem*
KSpaceLayoutItem::CreateGlue()
{
	return new KSpaceLayoutItem(
		BSize(-1, -1),
		BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED),
		BSize(-1, -1),
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
}


KSpaceLayoutItem*
KSpaceLayoutItem::CreateHorizontalStrut(float width)
{
	return new KSpaceLayoutItem(
		BSize(width, -1),
		BSize(width, B_SIZE_UNLIMITED),
		BSize(width, -1),
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
}


KSpaceLayoutItem*
KSpaceLayoutItem::CreateVerticalStrut(float height)
{
	return new KSpaceLayoutItem(
		BSize(-1, height),
		BSize(B_SIZE_UNLIMITED, height),
		BSize(-1, height),
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
}


BSize
KSpaceLayoutItem::MinSize()
{
	return fMinSize;
}


BSize
KSpaceLayoutItem::MaxSize()
{
	return fMaxSize;
}


BSize
KSpaceLayoutItem::PreferredSize()
{
	return fPreferredSize;
}


BAlignment
KSpaceLayoutItem::Alignment()
{
	return fAlignment;
}


void
KSpaceLayoutItem::SetExplicitMinSize(BSize size)
{
	if (size.IsWidthSet())
		fMinSize.width = size.width;
	if (size.IsHeightSet())
		fMinSize.height = size.height;

	InvalidateLayout();
}


void
KSpaceLayoutItem::SetExplicitMaxSize(BSize size)
{
	if (size.IsWidthSet())
		fMaxSize.width = size.width;
	if (size.IsHeightSet())
		fMaxSize.height = size.height;

	InvalidateLayout();
}


void
KSpaceLayoutItem::SetExplicitPreferredSize(BSize size)
{
	if (size.IsWidthSet())
		fPreferredSize.width = size.width;
	if (size.IsHeightSet())
		fPreferredSize.height = size.height;

	InvalidateLayout();
}


void
KSpaceLayoutItem::SetExplicitAlignment(BAlignment alignment)
{
	if (alignment.IsHorizontalSet())
		fAlignment.horizontal = alignment.horizontal;
	if (alignment.IsVerticalSet())
		fAlignment.vertical = alignment.vertical;

	InvalidateLayout();
}


bool
KSpaceLayoutItem::IsVisible()
{
	return fVisible;
}


void
KSpaceLayoutItem::SetVisible(bool visible)
{
	fVisible = visible;
}


BRect
KSpaceLayoutItem::Frame()
{
	return fFrame;
}


void
KSpaceLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
}


status_t
KSpaceLayoutItem::Archive(BMessage* into, bool deep) const
{
	status_t err = KLayoutItem::Archive(into, deep);

	if (err == B_OK)
		err = into->AddRect(kFrameField, fFrame);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fMinSize);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fMaxSize);

	if (err == B_OK)
		err = into->AddSize(kSizesField, fPreferredSize);

	if (err == B_OK)
		err = into->AddAlignment(kAlignmentField, fAlignment);

	if (err == B_OK)
		err = into->AddBool(kVisibleField, fVisible);

	return err;
}


BArchivable*
KSpaceLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KSpaceLayoutItem"))
		return new(std::nothrow) KSpaceLayoutItem(from);
	return NULL;
}


void KSpaceLayoutItem::_ReservedSpaceLayoutItem1() {}
void KSpaceLayoutItem::_ReservedSpaceLayoutItem2() {}
void KSpaceLayoutItem::_ReservedSpaceLayoutItem3() {}
void KSpaceLayoutItem::_ReservedSpaceLayoutItem4() {}
void KSpaceLayoutItem::_ReservedSpaceLayoutItem5() {}
void KSpaceLayoutItem::_ReservedSpaceLayoutItem6() {}
void KSpaceLayoutItem::_ReservedSpaceLayoutItem7() {}
void KSpaceLayoutItem::_ReservedSpaceLayoutItem8() {}
void KSpaceLayoutItem::_ReservedSpaceLayoutItem9() {}
void KSpaceLayoutItem::_ReservedSpaceLayoutItem10() {}

