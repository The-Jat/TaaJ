/*
 * Copyright 2001-2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 *		Marc Flerackers (mflerackers@androme.be)
 *		John Scipione (jscipione@gmail.com)
 */


#include "KTextInput.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <InterfaceDefs.h>
#include <KLayoutUtils.h>
#include <Message.h>
#include <String.h>
#include <KTextControl.h>
#include <TextView.h>
#include <Khidki.h>


namespace BPrivate {


_KTextInput_::_KTextInput_(BRect frame, BRect textRect, uint32 resizeMask,
	uint32 flags)
	:
	KTextView(frame, "_input_", textRect, resizeMask, flags),
	fPreviousText(NULL),
	fInMouseDown(false)
{
	MakeResizable(true);
}


_KTextInput_::_KTextInput_(BMessage* archive)
	:
	KTextView(archive),
	fPreviousText(NULL),
	fInMouseDown(false)
{
	MakeResizable(true);
}


_KTextInput_::~_KTextInput_()
{
	free(fPreviousText);
}


BArchivable*
_KTextInput_::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "_KTextInput_"))
		return new _KTextInput_(archive);

	return NULL;
}


status_t
_KTextInput_::Archive(BMessage* data, bool deep) const
{
	return KTextView::Archive(data, true);
}


void
_KTextInput_::MouseDown(BPoint where)
{
	fInMouseDown = true;
	KTextView::MouseDown(where);
	fInMouseDown = false;
}


void
_KTextInput_::FrameResized(float width, float height)
{
	KTextView::FrameResized(width, height);
}


void
_KTextInput_::KeyDown(const char* bytes, int32 numBytes)
{
	switch (*bytes) {
		case B_ENTER:
		{
			if (!TextControl()->IsEnabled())
				break;

			if (fPreviousText == NULL || strcmp(Text(), fPreviousText) != 0) {
				TextControl()->Invoke();
				free(fPreviousText);
				fPreviousText = strdup(Text());
			}

			SelectAll();
			break;
		}

		case B_TAB:
			KView::KeyDown(bytes, numBytes);
			break;

		default:
			KTextView::KeyDown(bytes, numBytes);
			break;
	}
}


void
_KTextInput_::MakeFocus(bool state)
{
	if (state == IsFocus())
		return;

	KTextView::MakeFocus(state);

	if (state) {
		SetInitialText();
		if (!fInMouseDown)
			SelectAll();
	} else {
		if (strcmp(Text(), fPreviousText) != 0)
			TextControl()->Invoke();

		free(fPreviousText);
		fPreviousText = NULL;
	}

	if (Window() != NULL) {
		// Invalidate parent to draw or remove the focus mark
		if (KTextControl* parent = dynamic_cast<KTextControl*>(Parent())) {
			BRect frame = Frame();
			frame.InsetBy(-1.0, -1.0);
			parent->Invalidate(frame);
		}
	}
}


BSize
_KTextInput_::MinSize()
{
	BSize min;
	min.height = ceilf(LineHeight(0) + 2.0);
	// we always add at least one pixel vertical inset top/bottom for
	// the text rect.
	min.width = min.height * 3;
	return KLayoutUtils::ComposeSize(ExplicitMinSize(), min);
}


void
_KTextInput_::SetInitialText()
{
	free(fPreviousText);
	fPreviousText = NULL;

	if (Text() != NULL)
		fPreviousText = strdup(Text());
}


void
_KTextInput_::Paste(BClipboard* clipboard)
{
	KTextView::Paste(clipboard);
	Invalidate();
}


void
_KTextInput_::InsertText(const char* inText, int32 inLength,
	int32 inOffset, const k_text_run_array* inRuns)
{
	// Filter all line breaks, note that inText is not terminated.
	if (inLength == 1) {
		if (*inText == '\n' || *inText == '\r')
			KTextView::InsertText(" ", 1, inOffset, inRuns);
		else
			KTextView::InsertText(inText, 1, inOffset, inRuns);
	} else {
		BString filteredText(inText, inLength);
		filteredText.ReplaceAll('\n', ' ');
		filteredText.ReplaceAll('\r', ' ');
		KTextView::InsertText(filteredText.String(), inLength, inOffset,
			inRuns);
	}

	TextControl()->InvokeNotify(TextControl()->ModificationMessage(),
		B_CONTROL_MODIFIED);
}


void
_KTextInput_::DeleteText(int32 fromOffset, int32 toOffset)
{
	KTextView::DeleteText(fromOffset, toOffset);

	TextControl()->InvokeNotify(TextControl()->ModificationMessage(),
		B_CONTROL_MODIFIED);
}


KTextControl*
_KTextInput_::TextControl()
{
	KTextControl* textControl = NULL;
	if (Parent() != NULL)
		textControl = dynamic_cast<KTextControl*>(Parent());

	if (textControl == NULL)
		debugger("_KTextInput_ should have a KTextControl as parent");

	return textControl;
}


}	// namespace BPrivate

