/*
 *	Copyright 2001-2015 Haiku Inc. All rights reserved.
 *  Distributed under the terms of the MIT License.
 *
 *	Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Mike Wilber
 *		Stefano Ceccherini (burton666@libero.it)
 *		Ivan Tonizza
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


#include <KButton.h>

#include <algorithm>
#include <new>

#include <KBitmap.h>
#include <KControlLook.h>
#include <Font.h>
#include <LayoutUtils.h>
#include <String.h>
#include <Khidki.h>

#include<KLayout.h>//khidki
#include <binary_compatibility/Interface.h>


enum {
	FLAG_DEFAULT 		= 0x01,
	FLAG_FLAT			= 0x02,
	FLAG_INSIDE			= 0x04,
	FLAG_WAS_PRESSED	= 0x08,
};


KButton::KButton(BRect frame, const char* name, const char* label,
	BMessage* message, uint32 resizingMode, uint32 flags)
	:
	KControl(frame, name, label, message, resizingMode,
		flags | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fPreferredSize(-1, -1),
	fFlags(0),
	fBehavior(B_BUTTON_BEHAVIOR),
	fPopUpMessage(NULL)
{
	// Resize to minimum height if needed
	font_height fh;
	GetFontHeight(&fh);
	float minHeight = 12.0f + (float)ceil(fh.ascent + fh.descent);
	if (Bounds().Height() < minHeight)
		ResizeTo(Bounds().Width(), minHeight);
}


KButton::KButton(const char* name, const char* label, BMessage* message,
	uint32 flags)
	:
	KControl(name, label, message,
		flags | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fPreferredSize(-1, -1),
	fFlags(0),
	fBehavior(B_BUTTON_BEHAVIOR),
	fPopUpMessage(NULL)
{
}


KButton::KButton(const char* label, BMessage* message)
	:
	KControl(NULL, label, message,
		B_WILL_DRAW | B_NAVIGABLE | B_FULL_UPDATE_ON_RESIZE),
	fPreferredSize(-1, -1),
	fFlags(0),
	fBehavior(B_BUTTON_BEHAVIOR),
	fPopUpMessage(NULL)
{
}


KButton::~KButton()
{
	SetPopUpMessage(NULL);
}


KButton::KButton(BMessage* data)
	:
	KControl(data),
	fPreferredSize(-1, -1),
	fFlags(0),
	fBehavior(B_BUTTON_BEHAVIOR),
	fPopUpMessage(NULL)
{
	bool isDefault = false;
	if (data->FindBool("_default", &isDefault) == B_OK && isDefault)
		_SetFlag(FLAG_DEFAULT, true);
	// NOTE: Default button state will be synchronized with the window
	// in AttachedToWindow().
}


BArchivable*
KButton::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "KButton"))
		return new(std::nothrow) KButton(data);

	return NULL;
}


status_t
KButton::Archive(BMessage* data, bool deep) const
{
	status_t err = KControl::Archive(data, deep);

	if (err != B_OK)
		return err;

	if (IsDefault())
		err = data->AddBool("_default", true);

	return err;
}


void
KButton::Draw(BRect updateRect)
{
	BRect rect(Bounds());
	rgb_color background = ViewColor();
	rgb_color base = LowColor();
	rgb_color textColor = ui_color(B_CONTROL_TEXT_COLOR);

	uint32 flags = k_be_control_look->Flags(this);
	if (_Flag(FLAG_DEFAULT))
		flags |= KControlLook::B_DEFAULT_BUTTON;
	if (_Flag(FLAG_FLAT) && !IsTracking())
		flags |= KControlLook::B_FLAT;
	if (_Flag(FLAG_INSIDE))
		flags |= KControlLook::B_HOVER;

	k_be_control_look->DrawButtonFrame(this, rect, updateRect,
		base, background, flags);

	if (fBehavior == B_POP_UP_BEHAVIOR) {
		k_be_control_look->DrawButtonWithPopUpBackground(this, rect, updateRect,
			base, flags);
	} else {
		k_be_control_look->DrawButtonBackground(this, rect, updateRect,
			base, flags);
	}

	// always leave some room around the label
	float labelMargin = k_be_control_look->DefaultLabelSpacing() / 2;
	rect.InsetBy(labelMargin, labelMargin);

	const KBitmap* icon = IconBitmap(
		(Value() == K_CONTROL_OFF
				? B_INACTIVE_ICON_BITMAP : B_ACTIVE_ICON_BITMAP)
			| (IsEnabled() ? 0 : B_DISABLED_ICON_BITMAP));

	k_be_control_look->DrawLabel(this, Label(), icon, rect, updateRect, base,
		flags, BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE), &textColor);
}


void
KButton::MouseDown(BPoint where)
{
	if (!IsEnabled())
		return;

	if (fBehavior == B_POP_UP_BEHAVIOR && _PopUpRect().Contains(where)) {
		InvokeNotify(fPopUpMessage, B_CONTROL_MODIFIED);
		return;
	}

	bool toggleBehavior = fBehavior == B_TOGGLE_BEHAVIOR;

	if (toggleBehavior) {
		bool wasPressed = Value() == K_CONTROL_ON;
		_SetFlag(FLAG_WAS_PRESSED, wasPressed);
		SetValue(wasPressed ? K_CONTROL_OFF : K_CONTROL_ON);
		Invalidate();
	} else
		SetValue(K_CONTROL_ON);

	if (Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) {
		SetTracking(true);
		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	} else {
		BRect bounds = Bounds();
		uint32 buttons;
		bool inside = false;

		do {
			Window()->UpdateIfNeeded();
			snooze(40000);

			GetMouse(&where, &buttons, true);
			inside = bounds.Contains(where);

			if (toggleBehavior) {
				bool pressed = inside ^ _Flag(FLAG_WAS_PRESSED);
				SetValue(pressed ? K_CONTROL_ON : K_CONTROL_OFF);
			} else {
				if ((Value() == K_CONTROL_ON) != inside)
					SetValue(inside ? K_CONTROL_ON : K_CONTROL_OFF);
			}
		} while (buttons != 0);

		if (inside) {
			if (toggleBehavior) {
				SetValue(
					_Flag(FLAG_WAS_PRESSED) ? K_CONTROL_OFF : K_CONTROL_ON);
			}

			Invoke();
		} else if (_Flag(FLAG_FLAT))
			Invalidate();
	}
}

void
KButton::AttachedToWindow()
{
	KControl::AttachedToWindow();

	// Tint default control background color to match default panel background.
	SetLowUIColor(B_CONTROL_BACKGROUND_COLOR, 1.115);
	SetHighUIColor(B_CONTROL_TEXT_COLOR);

	if (IsDefault())
		Window()->SetDefaultButton(this);
}


void
KButton::KeyDown(const char* bytes, int32 numBytes)
{
	if (*bytes == B_ENTER || *bytes == B_SPACE) {
		if (!IsEnabled())
			return;

		SetValue(K_CONTROL_ON);

		// make sure the user saw that
		Window()->UpdateIfNeeded();
		snooze(25000);

		Invoke();
	} else
		KControl::KeyDown(bytes, numBytes);
}


void
KButton::MakeDefault(bool flag)
{
	KButton* oldDefault = NULL;
	KWindow* window = Window();

	if (window != NULL)
		oldDefault = window->DefaultButton();

	if (flag) {
		if (_Flag(FLAG_DEFAULT) && oldDefault == this)
			return;

		if (_SetFlag(FLAG_DEFAULT, true)) {
			if ((Flags() & B_SUPPORTS_LAYOUT) != 0)
				InvalidateLayout();
			else {
				ResizeBy(6.0f, 6.0f);
				MoveBy(-3.0f, -3.0f);
			}
		}

		if (window && oldDefault != this)
			window->SetDefaultButton(this);
	} else {
		if (!_SetFlag(FLAG_DEFAULT, false))
			return;

		if ((Flags() & B_SUPPORTS_LAYOUT) != 0)
			InvalidateLayout();
		else {
			ResizeBy(-6.0f, -6.0f);
			MoveBy(3.0f, 3.0f);
		}

		if (window && oldDefault == this)
			window->SetDefaultButton(NULL);
	}
}


void
KButton::SetLabel(const char* label)
{
	KControl::SetLabel(label);
}


bool
KButton::IsDefault() const
{
	return _Flag(FLAG_DEFAULT);
}


bool
KButton::IsFlat() const
{
	return _Flag(FLAG_FLAT);
}


void
KButton::SetFlat(bool flat)
{
	if (_SetFlag(FLAG_FLAT, flat))
		Invalidate();
}


KButton::BBehavior
KButton::Behavior() const
{
	return fBehavior;
}


void
KButton::SetBehavior(BBehavior behavior)
{
	if (behavior != fBehavior) {
		fBehavior = behavior;
		InvalidateLayout();
		Invalidate();
	}
}


BMessage*
KButton::PopUpMessage() const
{
	return fPopUpMessage;
}


void
KButton::SetPopUpMessage(BMessage* message)
{
	delete fPopUpMessage;
	fPopUpMessage = message;
}


void
KButton::MessageReceived(BMessage* message)
{
	KControl::MessageReceived(message);
}


void
KButton::WindowActivated(bool active)
{
	KControl::WindowActivated(active);
}


void
KButton::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	bool inside = (code != B_EXITED_VIEW) && Bounds().Contains(where);
	if (_SetFlag(FLAG_INSIDE, inside))
		Invalidate();

	if (!IsTracking())
		return;

	if (fBehavior == B_TOGGLE_BEHAVIOR) {
		bool pressed = inside ^ _Flag(FLAG_WAS_PRESSED);
		SetValue(pressed ? K_CONTROL_ON : K_CONTROL_OFF);
	} else {
		if ((Value() == K_CONTROL_ON) != inside)
			SetValue(inside ? K_CONTROL_ON : K_CONTROL_OFF);
	}
}


void
KButton::MouseUp(BPoint where)
{
	if (!IsTracking())
		return;

	if (Bounds().Contains(where)) {
		if (fBehavior == B_TOGGLE_BEHAVIOR)
			SetValue(_Flag(FLAG_WAS_PRESSED) ? K_CONTROL_OFF : K_CONTROL_ON);

		Invoke();
	} else if (_Flag(FLAG_FLAT))
		Invalidate();

	SetTracking(false);
}


void
KButton::DetachedFromWindow()
{
	KControl::DetachedFromWindow();
}


void
KButton::SetValue(int32 value)
{
	if (value != Value())
		KControl::SetValue(value);
}


void
KButton::GetPreferredSize(float* _width, float* _height)
{
	_ValidatePreferredSize();

	if (_width)
		*_width = fPreferredSize.width;

	if (_height)
		*_height = fPreferredSize.height;
}


void
KButton::ResizeToPreferred()
{
	KControl::ResizeToPreferred();
}


status_t
KButton::Invoke(BMessage* message)
{
	Sync();
	snooze(50000);

	status_t err = KControl::Invoke(message);

	if (fBehavior != B_TOGGLE_BEHAVIOR)
		SetValue(K_CONTROL_OFF);

	return err;
}


void
KButton::FrameMoved(BPoint newPosition)
{
	KControl::FrameMoved(newPosition);
}


void
KButton::FrameResized(float newWidth, float newHeight)
{
	KControl::FrameResized(newWidth, newHeight);
}


void
KButton::MakeFocus(bool focus)
{
	KControl::MakeFocus(focus);
}


void
KButton::AllAttached()
{
	KControl::AllAttached();
}


void
KButton::AllDetached()
{
	KControl::AllDetached();
}


BHandler*
KButton::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	return KControl::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
KButton::GetSupportedSuites(BMessage* message)
{
	return KControl::GetSupportedSuites(message);
}


status_t
KButton::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= KButton::MinSize();
			return B_OK;

		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= KButton::MaxSize();
			return B_OK;

		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= KButton::PreferredSize();
			return B_OK;

		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= KButton::LayoutAlignment();
			return B_OK;

		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= KButton::HasHeightForWidth();
			return B_OK;

		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			KButton::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}

		case PERFORM_CODE_SET_LAYOUT:
		{
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KButton::SetLayout(data->layout);
			return B_OK;
		}

		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			KButton::LayoutInvalidated(data->descendants);
			return B_OK;
		}

		case PERFORM_CODE_DO_LAYOUT:
		{
			KButton::DoLayout();
			return B_OK;
		}

		case PERFORM_CODE_SET_ICON:
		{
			k_perform_data_set_icon* data = (k_perform_data_set_icon*)_data;
			return KButton::SetIcon(data->icon, data->flags);
		}
	}

	return KControl::Perform(code, _data);
}


BSize
KButton::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(),
		_ValidatePreferredSize());
}


BSize
KButton::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(),
		_ValidatePreferredSize());
}


BSize
KButton::PreferredSize()
{
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		_ValidatePreferredSize());
}


status_t
KButton::SetIcon(const KBitmap* icon, uint32 flags)
{
	return KControl::SetIcon(icon,
		flags | B_CREATE_ACTIVE_ICON_BITMAP | B_CREATE_DISABLED_ICON_BITMAPS);
}


void
KButton::LayoutInvalidated(bool descendants)
{
	// invalidate cached preferred size
	fPreferredSize.Set(-1, -1);
}


void KButton::_ReservedButton1() {}
void KButton::_ReservedButton2() {}
void KButton::_ReservedButton3() {}


KButton &
KButton::operator=(const KButton &)
{
	return *this;
}


BSize
KButton::_ValidatePreferredSize()
{
	if (fPreferredSize.width < 0) {
		KControlLook::background_type backgroundType
			= fBehavior == B_POP_UP_BEHAVIOR
				? KControlLook::B_BUTTON_WITH_POP_UP_BACKGROUND
				: KControlLook::B_BUTTON_BACKGROUND;
		float left, top, right, bottom;
		k_be_control_look->GetInsets(KControlLook::B_BUTTON_FRAME, backgroundType,
			IsDefault() ? KControlLook::B_DEFAULT_BUTTON : 0,
			left, top, right, bottom);

		// width
		float width = left + right + k_be_control_look->DefaultLabelSpacing() - 1;

		const char* label = Label();
		if (label != NULL) {
			width = std::max(width, 20.0f);
			width += (float)ceil(StringWidth(label));
		}

		const KBitmap* icon = IconBitmap(B_INACTIVE_ICON_BITMAP);
		if (icon != NULL)
			width += icon->Bounds().Width() + 1;

		if (label != NULL && icon != NULL)
			width += k_be_control_look->DefaultLabelSpacing();

		// height
		float minHorizontalMargins = top + bottom + k_be_control_look->DefaultLabelSpacing();
		float height = -1;

		if (label != NULL) {
			font_height fontHeight;
			GetFontHeight(&fontHeight);
			float textHeight = fontHeight.ascent + fontHeight.descent;
			height = ceilf(textHeight * 1.8);
			float margins = height - ceilf(textHeight);
			if (margins < minHorizontalMargins)
				height += minHorizontalMargins - margins;
		}

		if (icon != NULL) {
			height = std::max(height,
				icon->Bounds().Height() + minHorizontalMargins);
		}

		// force some minimum width/height values
		width = std::max(width, label != NULL ? 75.0f : 5.0f);
		height = std::max(height, 5.0f);

		fPreferredSize.Set(width, height);

		ResetLayoutInvalidation();
	}

	return fPreferredSize;
}


BRect
KButton::_PopUpRect() const
{
	if (fBehavior != B_POP_UP_BEHAVIOR)
		return BRect();

	float left, top, right, bottom;
	k_be_control_look->GetInsets(KControlLook::B_BUTTON_FRAME,
		KControlLook::B_BUTTON_WITH_POP_UP_BACKGROUND,
		IsDefault() ? KControlLook::B_DEFAULT_BUTTON : 0,
		left, top, right, bottom);

	BRect rect(Bounds());
	rect.left = rect.right - right + 1;
	return rect;
}


inline bool
KButton::_Flag(uint32 flag) const
{
	return (fFlags & flag) != 0;
}


inline bool
KButton::_SetFlag(uint32 flag, bool set)
{
	if (((fFlags & flag) != 0) == set)
		return false;

	if (set)
		fFlags |= flag;
	else
		fFlags &= ~flag;

	return true;
}


extern "C" void
B_IF_GCC_2(InvalidateLayout__7KButtonb, _ZN7KButton16InvalidateLayoutEb)(
	KView* view, bool descendants)
{
	perform_data_layout_invalidated data;
	data.descendants = descendants;

	view->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}
