/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 */


// KCheckBox displays an on/off control.


#include <KCheckBox.h>

#include <algorithm>
#include <new>

#include <KBitmap.h>
#include <KControlLook.h>
#include <KLayoutUtils.h>
#include <Khidki.h>

#include<KLayout.h>//khidki
#include <binary_compatibility/Interface.h>


KCheckBox::KCheckBox(BRect frame, const char* name, const char* label,
	BMessage* message, uint32 resizingMode, uint32 flags)
	:
	KControl(frame, name, label, message, resizingMode, flags),
	fPreferredSize(),
	fOutlined(false),
	fPartialToOff(false)
{
	// Resize to minimum height if needed
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	float minHeight = (float)ceil(6.0f + fontHeight.ascent
		+ fontHeight.descent);
	if (Bounds().Height() < minHeight)
		ResizeTo(Bounds().Width(), minHeight);
}


KCheckBox::KCheckBox(const char* name, const char* label, BMessage* message,
	uint32 flags)
	:
	KControl(name, label, message, flags | B_WILL_DRAW | B_NAVIGABLE),
	fPreferredSize(),
	fOutlined(false),
	fPartialToOff(false)
{
}


KCheckBox::KCheckBox(const char* label, BMessage* message)
	:
	KControl(NULL, label, message, B_WILL_DRAW | B_NAVIGABLE),
	fPreferredSize(),
	fOutlined(false),
	fPartialToOff(false)
{
}


KCheckBox::KCheckBox(BMessage* data)
	:
	KControl(data),
	fOutlined(false),
	fPartialToOff(false)
{
}


KCheckBox::~KCheckBox()
{
}


// #pragma mark - Archiving methods


BArchivable*
KCheckBox::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "KCheckBox"))
		return new(std::nothrow) KCheckBox(data);

	return NULL;
}


status_t
KCheckBox::Archive(BMessage* data, bool deep) const
{
	return KControl::Archive(data, deep);
}


// #pragma mark - Hook methods


void
KCheckBox::Draw(BRect updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	uint32 flags = k_be_control_look->Flags(this);
	if (fOutlined)
		flags |= KControlLook::B_CLICKED;

	BRect checkBoxRect(_CheckBoxFrame());
	BRect rect(checkBoxRect);
	k_be_control_look->DrawCheckBox(this, rect, updateRect, base, flags);

	// erase the is control flag before drawing the label so that the label
	// will get drawn using B_PANEL_TEXT_COLOR
	flags &= ~KControlLook::B_IS_CONTROL;

	BRect labelRect(Bounds());
	labelRect.left = checkBoxRect.right + 1
		+ k_be_control_look->DefaultLabelSpacing();

	const KBitmap* icon = IconBitmap(
		B_INACTIVE_ICON_BITMAP | (IsEnabled() ? 0 : B_DISABLED_ICON_BITMAP));

	k_be_control_look->DrawLabel(this, Label(), icon, labelRect, updateRect,
		base, flags);
}


void
KCheckBox::AttachedToWindow()
{
	KControl::AttachedToWindow();
}


void
KCheckBox::DetachedFromWindow()
{
	KControl::DetachedFromWindow();
}


void
KCheckBox::AllAttached()
{
	KControl::AllAttached();
}


void
KCheckBox::AllDetached()
{
	KControl::AllDetached();
}


void
KCheckBox::FrameMoved(BPoint newPosition)
{
	KControl::FrameMoved(newPosition);
}


void
KCheckBox::FrameResized(float newWidth, float newHeight)
{
	KControl::FrameResized(newWidth, newHeight);
}


void
KCheckBox::WindowActivated(bool active)
{
	KControl::WindowActivated(active);
}


void
KCheckBox::MessageReceived(BMessage* message)
{
	KControl::MessageReceived(message);
}


void
KCheckBox::KeyDown(const char* bytes, int32 numBytes)
{
	if (*bytes == B_ENTER || *bytes == B_SPACE) {
		if (!IsEnabled())
			return;

		SetValue(_NextState());
		Invoke();
	} else {
		// skip the KControl implementation
		KView::KeyDown(bytes, numBytes);
	}
}


void
KCheckBox::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	fOutlined = true;

	if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) {
		Invalidate();
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	} else {
		BRect bounds = Bounds();
		uint32 buttons;

		Invalidate();
		Window()->UpdateIfNeeded();

		do {
			snooze(40000);

			GetMouse(&where, &buttons, true);

			bool inside = bounds.Contains(where);
			if (fOutlined != inside) {
				fOutlined = inside;
				Invalidate();
				Window()->UpdateIfNeeded();
			}
		} while (buttons != 0);

		if (fOutlined) {
			fOutlined = false;
			SetValue(_NextState());
			Invoke();
		} else {
			Invalidate();
			Window()->UpdateIfNeeded();
		}
	}
}


void
KCheckBox::MouseUp(BPoint where)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(where);

	if (fOutlined != inside) {
		fOutlined = inside;
		Invalidate();
	}

	if (fOutlined) {
		fOutlined = false;
		SetValue(_NextState());
		Invoke();
	} else {
		Invalidate();
	}

	SetTracking(false);
}


void
KCheckBox::MouseMoved(BPoint where, uint32 code,
	const BMessage* dragMessage)
{
	if (!IsTracking())
		return;

	bool inside = Bounds().Contains(where);

	if (fOutlined != inside) {
		fOutlined = inside;
		Invalidate();
	}
}


// #pragma mark -


void
KCheckBox::GetPreferredSize(float* _width, float* _height)
{
	_ValidatePreferredSize();

	if (_width)
		*_width = fPreferredSize.width;

	if (_height)
		*_height = fPreferredSize.height;
}


void
KCheckBox::ResizeToPreferred()
{
	KControl::ResizeToPreferred();
}


BSize
KCheckBox::MinSize()
{
	return KLayoutUtils::ComposeSize(ExplicitMinSize(),
		_ValidatePreferredSize());
}


BSize
KCheckBox::MaxSize()
{
	return KLayoutUtils::ComposeSize(ExplicitMaxSize(),
		_ValidatePreferredSize());
}


BSize
KCheckBox::PreferredSize()
{
	return KLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		_ValidatePreferredSize());
}


BAlignment
KCheckBox::LayoutAlignment()
{
	return KLayoutUtils::ComposeAlignment(ExplicitAlignment(),
		BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
}


// #pragma mark -


void
KCheckBox::MakeFocus(bool focused)
{
	KControl::MakeFocus(focused);
}


void
KCheckBox::SetValue(int32 value)
{
	// We only accept three possible values.
	switch (value) {
		case K_CONTROL_OFF:
		case K_CONTROL_ON:
		case K_CONTROL_PARTIALLY_ON:
			break;
		default:
			value = K_CONTROL_ON;
			break;
	}

	if (value != Value()) {
		KControl::SetValueNoUpdate(value);
		Invalidate(_CheckBoxFrame());
	}
}


status_t
KCheckBox::Invoke(BMessage* message)
{
	return KControl::Invoke(message);
}


BHandler*
KCheckBox::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	return KControl::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
KCheckBox::GetSupportedSuites(BMessage* message)
{
	return KControl::GetSupportedSuites(message);
}


status_t
KCheckBox::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= KCheckBox::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= KCheckBox::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= KCheckBox::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= KCheckBox::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= KCheckBox::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			KCheckBox::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KCheckBox::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			KCheckBox::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			KCheckBox::DoLayout();
			return B_OK;
		}
		case PERFORM_CODE_SET_ICON:
		{
			k_perform_data_set_icon* data = (k_perform_data_set_icon*)_data;
			return KCheckBox::SetIcon(data->icon, data->flags);
		}
	}

	return KControl::Perform(code, _data);
}


status_t
KCheckBox::SetIcon(const KBitmap* icon, uint32 flags)
{
	return KControl::SetIcon(icon, flags | B_CREATE_DISABLED_ICON_BITMAPS);
}


void
KCheckBox::LayoutInvalidated(bool descendants)
{
	// invalidate cached preferred size
	fPreferredSize.Set(B_SIZE_UNSET, B_SIZE_UNSET);
}


bool
KCheckBox::IsPartialStateToOff() const
{
	return fPartialToOff;
}


void
KCheckBox::SetPartialStateToOff(bool partialToOff)
{
	fPartialToOff = partialToOff;
}


// #pragma mark - FBC padding


void KCheckBox::_ReservedCheckBox1() {}
void KCheckBox::_ReservedCheckBox2() {}
void KCheckBox::_ReservedCheckBox3() {}


BRect
KCheckBox::_CheckBoxFrame(const font_height& fontHeight) const
{
	return BRect(0.0f, 2.0f, ceilf(3.0f + fontHeight.ascent),
		ceilf(5.0f + fontHeight.ascent));
}


BRect
KCheckBox::_CheckBoxFrame() const
{
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	return _CheckBoxFrame(fontHeight);
}


BSize
KCheckBox::_ValidatePreferredSize()
{
	if (!fPreferredSize.IsWidthSet()) {
		font_height fontHeight;
		GetFontHeight(&fontHeight);

		BRect rect(_CheckBoxFrame(fontHeight));
		float width = rect.right + rect.left;
		float height = rect.bottom + rect.top;

		const KBitmap* icon = IconBitmap(B_INACTIVE_ICON_BITMAP);
		if (icon != NULL) {
			width += k_be_control_look->DefaultLabelSpacing()
				+ icon->Bounds().Width() + 1;
			height = std::max(height, icon->Bounds().Height());
		}

		if (const char* label = Label()) {
			width += k_be_control_look->DefaultLabelSpacing()
				+ ceilf(StringWidth(label));
			height = std::max(height,
				ceilf(6.0f + fontHeight.ascent + fontHeight.descent));
		}

		fPreferredSize.Set(width, height);

		ResetLayoutInvalidation();
	}

	return fPreferredSize;
}


int32
KCheckBox::_NextState() const
{
	switch (Value()) {
		case K_CONTROL_OFF:
			return K_CONTROL_ON;
		case K_CONTROL_PARTIALLY_ON:
			return fPartialToOff ? K_CONTROL_OFF : K_CONTROL_ON;
		case K_CONTROL_ON:
		default:
			return K_CONTROL_OFF;
	}
}


KCheckBox &
KCheckBox::operator=(const KCheckBox &)
{
	return *this;
}


extern "C" void
B_IF_GCC_2(InvalidateLayout__9KCheckBoxb, _ZN9KCheckBox16InvalidateLayoutEb)(
	KCheckBox* box, bool descendants)
{
	perform_data_layout_invalidated data;
	data.descendants = descendants;

	box->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}

