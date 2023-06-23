/*
 * Copyright 2001-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ulrich Wimboeck
 *		Marc Flerackers (mflerackers@androme.be)
 *		Rene Gollent
 */


#include <KStringItem.h>

#include <stdlib.h>
#include <string.h>

#include <KControlLook.h>
#include <Message.h>
#include <Laminate.h>


KStringItem::KStringItem(const char* text, uint32 level, bool expanded)
	:
	KListItem(level, expanded),
	fText(NULL),
	fBaselineOffset(0)
{
	SetText(text);
}


KStringItem::KStringItem(BMessage* archive)
	:
	KListItem(archive),
	fText(NULL),
	fBaselineOffset(0)
{
	const char* string;
	if (archive->FindString("_label", &string) == B_OK)
		SetText(string);
}


KStringItem::~KStringItem()
{
	free(fText);
}


BArchivable*
KStringItem::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "KStringItem"))
		return new KStringItem(archive);

	return NULL;
}


status_t
KStringItem::Archive(BMessage* archive, bool deep) const
{
	status_t status = KListItem::Archive(archive);

	if (status == B_OK && fText != NULL)
		status = archive->AddString("_label", fText);

	return status;
}


void
KStringItem::DrawItem(KView* owner, BRect frame, bool complete)
{
	if (fText == NULL)
		return;

	rgb_color lowColor = owner->LowColor();

	if (IsSelected() || complete) {
		rgb_color color;
		if (IsSelected())
			color = ui_color(B_LIST_SELECTED_BACKGROUND_COLOR);
		else
			color = owner->ViewColor();

		owner->SetLowColor(color);
		owner->FillRect(frame, B_SOLID_LOW);
	} else
		owner->SetLowColor(owner->ViewColor());

	owner->MovePenTo(frame.left + k_be_control_look->DefaultLabelSpacing(),
		frame.top + fBaselineOffset);

	owner->DrawString(fText);

	owner->SetLowColor(lowColor);
}


void
KStringItem::SetText(const char* text)
{
	free(fText);
	fText = NULL;

	if (text)
		fText = strdup(text);
}


const char*
KStringItem::Text() const
{
	return fText;
}


void
KStringItem::Update(KView* owner, const BFont* font)
{
	if (fText != NULL) {
		SetWidth(font->StringWidth(fText)
			+ k_be_control_look->DefaultLabelSpacing());
	}

	font_height fheight;
	font->GetHeight(&fheight);

	fBaselineOffset = 2 + ceilf(fheight.ascent + fheight.leading / 2);

	SetHeight(ceilf(fheight.ascent) + ceilf(fheight.descent)
		+ ceilf(fheight.leading) + 4);
}


status_t
KStringItem::Perform(perform_code d, void* arg)
{
	return KListItem::Perform(d, arg);
}


float
KStringItem::BaselineOffset() const
{
	return fBaselineOffset;
}


void KStringItem::_ReservedStringItem1() {}
void KStringItem::_ReservedStringItem2() {}


KStringItem::KStringItem(const KStringItem &)
{
}


KStringItem	&
KStringItem::operator=(const KStringItem &)
{
	return *this;
}
