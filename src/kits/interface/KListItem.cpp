/*
 * Copyright 2001-2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ulrich Wimboeck
 *		Marc Flerackers (mflerackers@androme.be)
 *		Rene Gollent
 */


#include <KListItem.h>

#include <Message.h>
#include <Laminate.h>


KListItem::KListItem(uint32 level, bool expanded)
	:
	fTop(0.0),
	fTemporaryList(0),
	fWidth(0),
	fHeight(0),
	fLevel(level),
	fSelected(false),
	fEnabled(true),
	fExpanded(expanded),
	fHasSubitems(false),
	fVisible(true)
{
}


KListItem::KListItem(BMessage* data)
	:
	BArchivable(data),
	fTop(0.0),
	fWidth(0),
	fHeight(0),
	fLevel(0),
	fSelected(false),
	fEnabled(true),
	fExpanded(false),
	fHasSubitems(false),
	fVisible(true)
{
	data->FindBool("_sel", &fSelected);

	if (data->FindBool("_disable", &fEnabled) != B_OK)
		fEnabled = true;
	else
		fEnabled = false;

	data->FindBool("_li_expanded", &fExpanded);
	data->FindInt32("_li_outline_level", (int32*)&fLevel);
}


KListItem::~KListItem()
{
}


status_t
KListItem::Archive(BMessage* archive, bool deep) const
{
	status_t status = BArchivable::Archive(archive, deep);
	if (status == B_OK && fSelected)
		status = archive->AddBool("_sel", true);

	if (status == B_OK && !fEnabled)
		status = archive->AddBool("_disable", true);

	if (status == B_OK && fExpanded)
		status = archive->AddBool("_li_expanded", true);

	if (status == B_OK && fLevel != 0)
		status = archive->AddInt32("_li_outline_level", fLevel);

	return status;
}


float
KListItem::Height() const
{
	return fHeight;
}


float
KListItem::Width() const
{
	return fWidth;
}


bool
KListItem::IsSelected() const
{
	return fSelected;
}


void
KListItem::Select()
{
	fSelected = true;
}


void
KListItem::Deselect()
{
	fSelected = false;
}


void
KListItem::SetEnabled(bool on)
{
	fEnabled = on;
}


bool
KListItem::IsEnabled() const
{
	return fEnabled;
}


void
KListItem::SetHeight(float height)
{
	fHeight = height;
}


void
KListItem::SetWidth(float width)
{
	fWidth = width;
}


void
KListItem::Update(KView* owner, const BFont* font)
{
	font_height fh;
	font->GetHeight(&fh);

	SetWidth(owner->Bounds().Width());
	SetHeight(ceilf(fh.ascent + fh.descent + fh.leading));
}


status_t
KListItem::Perform(perform_code d, void* arg)
{
	return BArchivable::Perform(d, arg);
}


void
KListItem::SetExpanded(bool expanded)
{
	fExpanded = expanded;
}


bool
KListItem::IsExpanded() const
{
	return fExpanded;
}


uint32
KListItem::OutlineLevel() const
{
	return fLevel;
}


void
KListItem::SetOutlineLevel(uint32 level)
{
	fLevel = level;
}


bool
KListItem::HasSubitems() const
{
	return fHasSubitems;
}


void KListItem::_ReservedListItem1() {}
void KListItem::_ReservedListItem2() {}


bool
KListItem::IsItemVisible() const
{
	return fVisible;
}


void
KListItem::SetTop(float top)
{
	fTop = top;
}


void
KListItem::SetItemVisible(bool visible)
{
	fVisible = visible;
}
