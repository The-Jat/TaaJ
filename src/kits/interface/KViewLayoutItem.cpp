/*
 * Copyright 2010, Haiku, Inc.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "KViewLayoutItem.h"

#include <new>

#include <KLayout.h>
#include <Laminate.h>
#include <ViewPrivate.h>


namespace {
	const char* const kViewField = "KViewLayoutItem:view";
}


KViewLayoutItem::KViewLayoutItem(KView* view)
	:
	fView(view),
	fAncestorsVisible(true)
{
}


KViewLayoutItem::KViewLayoutItem(BMessage* from)
	:
	KLayoutItem(BUnarchiver::PrepareArchive(from)),
	fView(NULL),
	fAncestorsVisible(true)
{
	BUnarchiver unarchiver(from);
	unarchiver.Finish(unarchiver.FindObject<KView>(kViewField, 0,
		BUnarchiver::B_DONT_ASSUME_OWNERSHIP, fView));
}


KViewLayoutItem::~KViewLayoutItem()
{
}


BSize
KViewLayoutItem::MinSize()
{
	return fView->MinSize();
}


BSize
KViewLayoutItem::MaxSize()
{
	return fView->MaxSize();
}


BSize
KViewLayoutItem::PreferredSize()
{
	return fView->PreferredSize();
}


BAlignment
KViewLayoutItem::Alignment()
{
	return fView->LayoutAlignment();
}


void
KViewLayoutItem::SetExplicitMinSize(BSize size)
{
	fView->SetExplicitMinSize(size);
}


void
KViewLayoutItem::SetExplicitMaxSize(BSize size)
{
	fView->SetExplicitMaxSize(size);
}


void
KViewLayoutItem::SetExplicitPreferredSize(BSize size)
{
	fView->SetExplicitPreferredSize(size);
}


void
KViewLayoutItem::SetExplicitAlignment(BAlignment alignment)
{
	fView->SetExplicitAlignment(alignment);
}


bool
KViewLayoutItem::IsVisible()
{
	int16 showLevel = KView::KPrivate(fView).ShowLevel();
	return showLevel - (fAncestorsVisible ? 0 : 1) <= 0;
}


void
KViewLayoutItem::SetVisible(bool visible)
{
	if (visible != IsVisible()) {
		if (visible)
			fView->Show();
		else
			fView->Hide();
	}
}


BRect
KViewLayoutItem::Frame()
{
	return fView->Frame();
}


void
KViewLayoutItem::SetFrame(BRect frame)
{
	fView->MoveTo(frame.LeftTop());
	fView->ResizeTo(frame.Width(), frame.Height());
}


bool
KViewLayoutItem::HasHeightForWidth()
{
	return fView->HasHeightForWidth();
}


void
KViewLayoutItem::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	fView->GetHeightForWidth(width, min, max, preferred);
}


KView*
KViewLayoutItem::View()
{
	return fView;
}


void
KViewLayoutItem::Relayout(bool immediate)
{
	if (immediate)
		fView->Layout(false);
	else
		fView->Relayout();
}


status_t
KViewLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = KLayoutItem::Archive(into, deep);

	return archiver.Finish(err);
}


status_t
KViewLayoutItem::AllArchived(BMessage* into) const
{
	BArchiver archiver(into);
	status_t err = KLayoutItem::AllArchived(into);

	if (err == B_OK) {
		if (archiver.IsArchived(fView))
			err = archiver.AddArchivable(kViewField, fView);
		else
			err = B_NAME_NOT_FOUND;
	}

	return err;
}


status_t
KViewLayoutItem::AllUnarchived(const BMessage* from)
{
	if (!fView)
		return B_ERROR;

	return KLayoutItem::AllUnarchived(from);
}


BArchivable*
KViewLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KViewLayoutItem"))
		return new(std::nothrow) KViewLayoutItem(from);
	return NULL;
}


void
KViewLayoutItem::LayoutInvalidated(bool children)
{
	fView->InvalidateLayout(children);
}


void
KViewLayoutItem::AncestorVisibilityChanged(bool shown)
{
	if (fAncestorsVisible == shown)
		return;

	fAncestorsVisible = shown;
	if (shown)
		fView->Show();
	if (!shown)
		fView->Hide();
}

