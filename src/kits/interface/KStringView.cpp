/*
 * Copyright 2001-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Frans van Nispen (xlr8@tref.nl)
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


//!	KStringView draws a non-editable text string.


#include <KStringView.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <LayoutUtils.h>
#include <Message.h>
#include <PropertyInfo.h>
#include <StringList.h>
#include <Laminate.h>
#include <Khidki.h>

#include<KLayout.h>//khidki
#include <binary_compatibility/Interface.h>


static property_info sPropertyList[] = {
	{
		"Text",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_STRING_TYPE }
	},
	{
		"Alignment",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_INT32_TYPE }
	},

	{ 0 }
};


KStringView::KStringView(BRect frame, const char* name, const char* text,
	uint32 resizingMode, uint32 flags)
	:
	KView(frame, name, resizingMode, flags | B_FULL_UPDATE_ON_RESIZE),
	fText(text ? strdup(text) : NULL),
	fTruncation(B_NO_TRUNCATION),
	fAlign(B_ALIGN_LEFT),
	fPreferredSize(text ? _StringWidth(text) : 0.0, -1)
{
}


KStringView::KStringView(const char* name, const char* text, uint32 flags)
	:
	KView(name, flags | B_FULL_UPDATE_ON_RESIZE),
	fText(text ? strdup(text) : NULL),
	fTruncation(B_NO_TRUNCATION),
	fAlign(B_ALIGN_LEFT),
	fPreferredSize(text ? _StringWidth(text) : 0.0, -1)
{
}


KStringView::KStringView(BMessage* archive)
	:
	KView(archive),
	fText(NULL),
	fTruncation(B_NO_TRUNCATION),
	fPreferredSize(0, -1)
{
	fAlign = (alignment)archive->GetInt32("_align", B_ALIGN_LEFT);
	fTruncation = (uint32)archive->GetInt32("_truncation", B_NO_TRUNCATION);

	const char* text = archive->GetString("_text", NULL);

	SetText(text);
	SetFlags(Flags() | B_FULL_UPDATE_ON_RESIZE);
}


KStringView::~KStringView()
{
	free(fText);
}


// #pragma mark - Archiving methods


BArchivable*
KStringView::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, "KStringView"))
		return NULL;

	return new KStringView(data);
}


status_t
KStringView::Archive(BMessage* data, bool deep) const
{
	status_t status = KView::Archive(data, deep);

	if (status == B_OK && fText)
		status = data->AddString("_text", fText);
	if (status == B_OK && fTruncation != B_NO_TRUNCATION)
		status = data->AddInt32("_truncation", fTruncation);
	if (status == B_OK)
		status = data->AddInt32("_align", fAlign);

	return status;
}


// #pragma mark - Hook methods


void
KStringView::AttachedToWindow()
{
	if (HasDefaultColors())
		SetHighUIColor(B_PANEL_TEXT_COLOR);

	KView* parent = Parent();

	if (parent != NULL) {
		float tint = B_NO_TINT;
		color_which which = parent->ViewUIColor(&tint);

		if (which != B_NO_COLOR) {
			SetViewUIColor(which, tint);
			SetLowUIColor(which, tint);
		} else {
			SetViewColor(parent->ViewColor());
			SetLowColor(ViewColor());
		}
	}

	if (ViewColor() == B_TRANSPARENT_COLOR)
		AdoptSystemColors();
}


void
KStringView::DetachedFromWindow()
{
	KView::DetachedFromWindow();
}


void
KStringView::AllAttached()
{
	KView::AllAttached();
}


void
KStringView::AllDetached()
{
	KView::AllDetached();
}


// #pragma mark - Layout methods


void
KStringView::MakeFocus(bool focus)
{
	KView::MakeFocus(focus);
}


void
KStringView::GetPreferredSize(float* _width, float* _height)
{
	_ValidatePreferredSize();

	if (_width)
		*_width = fPreferredSize.width;

	if (_height)
		*_height = fPreferredSize.height;
}


BSize
KStringView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(),
		_ValidatePreferredSize());
}


BSize
KStringView::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		_ValidatePreferredSize());
}


BSize
KStringView::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		_ValidatePreferredSize());
}


void
KStringView::ResizeToPreferred()
{
	float width, height;
	GetPreferredSize(&width, &height);

	// Resize the width only for B_ALIGN_LEFT (if its large enough already, that is)
	if (Bounds().Width() > width && Alignment() != B_ALIGN_LEFT)
		width = Bounds().Width();

	KView::ResizeTo(width, height);
}


BAlignment
KStringView::LayoutAlignment()
{
	return BLayoutUtils::ComposeAlignment(ExplicitAlignment(),
		BAlignment(fAlign, B_ALIGN_MIDDLE));
}


// #pragma mark - More hook methods


void
KStringView::FrameMoved(BPoint newPosition)
{
	KView::FrameMoved(newPosition);
}


void
KStringView::FrameResized(float newWidth, float newHeight)
{
	KView::FrameResized(newWidth, newHeight);
}


void
KStringView::Draw(BRect updateRect)
{
	if (!fText)
		return;

	if (LowUIColor() == B_NO_COLOR)
		SetLowColor(ViewColor());

	font_height fontHeight;
	GetFontHeight(&fontHeight);

	BRect bounds = Bounds();

	BStringList lines;
	BString(fText).Split("\n", false, lines);
	for (int i = 0; i < lines.CountStrings(); i++) {
		const char* text = lines.StringAt(i).String();
		float width = StringWidth(text);
		BString truncated;
		if (fTruncation != B_NO_TRUNCATION && width > bounds.Width()) {
			// The string needs to be truncated
			// TODO: we should cache this
			truncated = lines.StringAt(i);
			TruncateString(&truncated, fTruncation, bounds.Width());
			text = truncated.String();
			width = StringWidth(text);
		}

		float y = (bounds.top + bounds.bottom - ceilf(fontHeight.descent))
			- ceilf(fontHeight.ascent + fontHeight.descent + fontHeight.leading)
				* (lines.CountStrings() - i - 1);
		float x;
		switch (fAlign) {
			case B_ALIGN_RIGHT:
				x = bounds.Width() - width;
				break;

			case B_ALIGN_CENTER:
				x = (bounds.Width() - width) / 2.0;
				break;

			default:
				x = 0.0;
				break;
		}

		DrawString(text, BPoint(x, y));
	}
}


void
KStringView::MessageReceived(BMessage* message)
{
	if (message->what == B_GET_PROPERTY || message->what == B_SET_PROPERTY) {
		int32 index;
		BMessage specifier;
		int32 form;
		const char* property;
		if (message->GetCurrentSpecifier(&index, &specifier, &form, &property)
				!= B_OK) {
			KView::MessageReceived(message);
			return;
		}

		BMessage reply(B_REPLY);
		bool handled = false;
		if (strcmp(property, "Text") == 0) {
			if (message->what == B_GET_PROPERTY) {
				reply.AddString("result", fText);
				handled = true;
			} else {
				const char* text;
				if (message->FindString("data", &text) == B_OK) {
					SetText(text);
					reply.AddInt32("error", B_OK);
					handled = true;
				}
			}
		} else if (strcmp(property, "Alignment") == 0) {
			if (message->what == B_GET_PROPERTY) {
				reply.AddInt32("result", (int32)fAlign);
				handled = true;
			} else {
				int32 align;
				if (message->FindInt32("data", &align) == B_OK) {
					SetAlignment((alignment)align);
					reply.AddInt32("error", B_OK);
					handled = true;
				}
			}
		}

		if (handled) {
			message->SendReply(&reply);
			return;
		}
	}

	KView::MessageReceived(message);
}


void
KStringView::MouseDown(BPoint point)
{
	KView::MouseDown(point);
}


void
KStringView::MouseUp(BPoint point)
{
	KView::MouseUp(point);
}


void
KStringView::MouseMoved(BPoint point, uint32 transit, const BMessage* msg)
{
	KView::MouseMoved(point, transit, msg);
}


// #pragma mark -


void
KStringView::SetText(const char* text)
{
	if ((text && fText && !strcmp(text, fText)) || (!text && !fText))
		return;

	free(fText);
	fText = text ? strdup(text) : NULL;

	float newStringWidth = _StringWidth(fText);
	if (fPreferredSize.width != newStringWidth) {
		fPreferredSize.width = newStringWidth;
		InvalidateLayout();
	}

	Invalidate();
}


const char*
KStringView::Text() const
{
	return fText;
}


void
KStringView::SetAlignment(alignment flag)
{
	fAlign = flag;
	Invalidate();
}


alignment
KStringView::Alignment() const
{
	return fAlign;
}


void
KStringView::SetTruncation(uint32 truncationMode)
{
	if (fTruncation != truncationMode) {
		fTruncation = truncationMode;
		Invalidate();
	}
}


uint32
KStringView::Truncation() const
{
	return fTruncation;
}


BHandler*
KStringView::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	BPropertyInfo propInfo(sPropertyList);
	if (propInfo.FindMatch(message, 0, specifier, form, property) >= B_OK)
		return this;

	return KView::ResolveSpecifier(message, index, specifier, form, property);
}


status_t
KStringView::GetSupportedSuites(BMessage* data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t status = data->AddString("suites", "suite/vnd.Be-string-view");
	if (status != B_OK)
		return status;

	BPropertyInfo propertyInfo(sPropertyList);
	status = data->AddFlat("messages", &propertyInfo);
	if (status != B_OK)
		return status;

	return KView::GetSupportedSuites(data);
}


void
KStringView::SetFont(const BFont* font, uint32 mask)
{
	KView::SetFont(font, mask);

	fPreferredSize.width = _StringWidth(fText);

	Invalidate();
	InvalidateLayout();
}


void
KStringView::LayoutInvalidated(bool descendants)
{
	// invalidate cached preferred size
	fPreferredSize.height = -1;
}


// #pragma mark - Perform


status_t
KStringView::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= KStringView::MinSize();
			return B_OK;

		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= KStringView::MaxSize();
			return B_OK;

		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= KStringView::PreferredSize();
			return B_OK;

		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= KStringView::LayoutAlignment();
			return B_OK;

		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= KStringView::HasHeightForWidth();
			return B_OK;

		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			KStringView::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}

		case PERFORM_CODE_SET_LAYOUT:
		{
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KStringView::SetLayout(data->layout);
			return B_OK;
		}

		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			KStringView::LayoutInvalidated(data->descendants);
			return B_OK;
		}

		case PERFORM_CODE_DO_LAYOUT:
		{
			KStringView::DoLayout();
			return B_OK;
		}
	}

	return KView::Perform(code, _data);
}


// #pragma mark - FBC padding methods


void KStringView::_ReservedStringView1() {}
void KStringView::_ReservedStringView2() {}
void KStringView::_ReservedStringView3() {}


// #pragma mark - Private methods


KStringView&
KStringView::operator=(const KStringView&)
{
	// Assignment not allowed (private)
	return *this;
}


BSize
KStringView::_ValidatePreferredSize()
{
	if (fPreferredSize.height < 0) {
		// height
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		int32 lines = 1;
		char* temp = fText ? strchr(fText, '\n') : NULL;
		while (temp != NULL) {
			temp = strchr(temp + 1, '\n');
			lines++;
		};

		fPreferredSize.height = ceilf(fontHeight.ascent + fontHeight.descent
			+ fontHeight.leading) * lines;

		ResetLayoutInvalidation();
	}

	return fPreferredSize;
}


float
KStringView::_StringWidth(const char* text)
{
	if(text == NULL)
		return 0.0f;

	float maxWidth = 0.0f;
	BStringList lines;
	BString(fText).Split("\n", false, lines);
	for (int i = 0; i < lines.CountStrings(); i++) {
		float width = StringWidth(lines.StringAt(i));
		if (maxWidth < width)
			maxWidth = width;
	}
	return maxWidth;
}


extern "C" void
B_IF_GCC_2(InvalidateLayout__11KStringViewb,
	_ZN11KStringView16InvalidateLayoutEb)(KView* view, bool descendants)
{
	perform_data_layout_invalidated data;
	data.descendants = descendants;

	view->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}
