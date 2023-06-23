/*
 * Copyright 2001-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Graham MacDonald (macdonag@btopenworld.com)
 */


#include <KPictureButton.h>

#include <new>

#include<KLayout.h>//khidki
#include <binary_compatibility/Interface.h>


KPictureButton::KPictureButton(BRect frame, const char* name,
	KPicture* off, KPicture* on, BMessage* message,
	uint32 behavior, uint32 resizingMode, uint32 flags)
	:
	KControl(frame, name, "", message, resizingMode, flags),
	fEnabledOff(new(std::nothrow) KPicture(*off)),
	fEnabledOn(new(std::nothrow) KPicture(*on)),
	fDisabledOff(NULL),
	fDisabledOn(NULL),
	fBehavior(behavior)
{
}


KPictureButton::KPictureButton(BMessage* data)
	:
	KControl(data),
	fEnabledOff(NULL),
	fEnabledOn(NULL),
	fDisabledOff(NULL),
	fDisabledOn(NULL)
{
	BMessage pictureArchive;

	// Default to 1 state button if not here - is this valid?
	if (data->FindInt32("_behave", (int32*)&fBehavior) != B_OK)
		fBehavior = K_ONE_STATE_BUTTON;

	// Now expand the pictures:
	if (data->FindMessage("_e_on", &pictureArchive) == B_OK)
		fEnabledOn = new(std::nothrow) KPicture(&pictureArchive);

	if (data->FindMessage("_e_off", &pictureArchive) == B_OK)
		fEnabledOff = new(std::nothrow) KPicture(&pictureArchive);

	if (data->FindMessage("_d_on", &pictureArchive) == B_OK)
		fDisabledOn = new(std::nothrow) KPicture(&pictureArchive);

	if (data->FindMessage("_d_off", &pictureArchive) == B_OK)
		fDisabledOff = new(std::nothrow) KPicture(&pictureArchive);
}


KPictureButton::~KPictureButton()
{
	delete fEnabledOn;
	delete fEnabledOff;
	delete fDisabledOn;
	delete fDisabledOff;
}


BArchivable*
KPictureButton::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "KPictureButton"))
		return new (std::nothrow) KPictureButton(data);

	return NULL;
}


status_t
KPictureButton::Archive(BMessage* data, bool deep) const
{
	status_t err = KControl::Archive(data, deep);
	if (err != B_OK)
		return err;

	// Fill out message, depending on whether a deep copy is required or not.
	if (deep) {
		BMessage pictureArchive;
		if (fEnabledOn->Archive(&pictureArchive, deep) == B_OK) {
			err = data->AddMessage("_e_on", &pictureArchive);
			if (err != B_OK)
				return err;
		}

		pictureArchive.MakeEmpty();
		if (fEnabledOff->Archive(&pictureArchive, deep) == B_OK) {
			err = data->AddMessage("_e_off", &pictureArchive);
			if (err != B_OK)
				return err;
		}

		pictureArchive.MakeEmpty();
		if (fDisabledOn && fDisabledOn->Archive(&pictureArchive, deep) == B_OK) {
			err = data->AddMessage("_d_on", &pictureArchive);
			if (err != B_OK)
				return err;
		}

		pictureArchive.MakeEmpty();
		if (fDisabledOff && fDisabledOff->Archive(&pictureArchive, deep) == B_OK) {
			err = data->AddMessage("_d_off", &pictureArchive);
			if (err != B_OK)
				return err;
		}
	}

	return data->AddInt32("_behave", fBehavior);
}


void
KPictureButton::AttachedToWindow()
{
	KControl::AttachedToWindow();
}


void
KPictureButton::DetachedFromWindow()
{
	KControl::DetachedFromWindow();
}


void
KPictureButton::AllAttached()
{
	KControl::AllAttached();
}


void
KPictureButton::AllDetached()
{
	KControl::AllDetached();
}


void
KPictureButton::ResizeToPreferred()
{
	KControl::ResizeToPreferred();
}


void
KPictureButton::GetPreferredSize(float* _width, float* _height)
{
	KControl::GetPreferredSize(_width, _height);
}


void
KPictureButton::FrameMoved(BPoint newPosition)
{
	KControl::FrameMoved(newPosition);
}


void
KPictureButton::FrameResized(float newWidth, float newHeight)
{
	KControl::FrameResized(newWidth, newHeight);
}


void
KPictureButton::WindowActivated(bool active)
{
	KControl::WindowActivated(active);
}


void
KPictureButton::MakeFocus(bool focus)
{
	KControl::MakeFocus(focus);
}


void
KPictureButton::Draw(BRect updateRect)
{
	if (IsEnabled()) {
		if (Value() == K_CONTROL_ON)
			DrawPicture(fEnabledOn);
		else
			DrawPicture(fEnabledOff);
	} else {

		if (fDisabledOff == NULL
			|| (fDisabledOn == NULL && fBehavior == K_TWO_STATE_BUTTON))
			debugger("Need to set the 'disabled' pictures for this KPictureButton ");

		if (Value() == K_CONTROL_ON)
			DrawPicture(fDisabledOn);
		else
			DrawPicture(fDisabledOff);
	}

	if (IsFocus()) {
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(Bounds(), B_SOLID_HIGH);
	}
}


void
KPictureButton::MessageReceived(BMessage* message)
{
	KControl::MessageReceived(message);
}


void
KPictureButton::KeyDown(const char* bytes, int32 numBytes)
{
	if (numBytes == 1) {
		switch (bytes[0]) {
			case B_ENTER:
			case B_SPACE:
				if (fBehavior == K_ONE_STATE_BUTTON) {
					SetValue(K_CONTROL_ON);
					snooze(50000);
					SetValue(K_CONTROL_OFF);
				} else {
					if (Value() == K_CONTROL_ON)
						SetValue(K_CONTROL_OFF);
					else
						SetValue(K_CONTROL_ON);
				}
				Invoke();
				return;
		}
	}

	KControl::KeyDown(bytes, numBytes);
}


void
KPictureButton::MouseDown(BPoint where)
{
	if (!IsEnabled()) {
		KControl::MouseDown(where);
		return;
	}

	SetMouseEventMask(B_POINTER_EVENTS,
		B_NO_POINTER_HISTORY | B_SUSPEND_VIEW_FOCUS);

	if (fBehavior == K_ONE_STATE_BUTTON) {
		SetValue(K_CONTROL_ON);
	} else {
		if (Value() == K_CONTROL_ON)
			SetValue(K_CONTROL_OFF);
		else
			SetValue(K_CONTROL_ON);
	}
	SetTracking(true);
}


void
KPictureButton::MouseUp(BPoint where)
{
	if (IsEnabled() && IsTracking()) {
		if (Bounds().Contains(where)) {
			if (fBehavior == K_ONE_STATE_BUTTON) {
				if (Value() == K_CONTROL_ON) {
					snooze(75000);
					SetValue(K_CONTROL_OFF);
				}
			}
			Invoke();
		}

		SetTracking(false);
	}
}


void
KPictureButton::MouseMoved(BPoint where, uint32 code,
	const BMessage* dragMessage)
{
	if (IsEnabled() && IsTracking()) {
		if (code == B_EXITED_VIEW)
			SetValue(K_CONTROL_OFF);
		else if (code == B_ENTERED_VIEW)
			SetValue(K_CONTROL_ON);
	} else
		KControl::MouseMoved(where, code, dragMessage);
}


// #pragma mark -


void
KPictureButton::SetEnabledOn(KPicture* picture)
{
	delete fEnabledOn;
	fEnabledOn = new (std::nothrow) KPicture(*picture);
}


void
KPictureButton::SetEnabledOff(KPicture* picture)
{
	delete fEnabledOff;
	fEnabledOff = new (std::nothrow) KPicture(*picture);
}


void
KPictureButton::SetDisabledOn(KPicture* picture)
{
	delete fDisabledOn;
	fDisabledOn = new (std::nothrow) KPicture(*picture);
}


void
KPictureButton::SetDisabledOff(KPicture* picture)
{
	delete fDisabledOff;
	fDisabledOff = new (std::nothrow) KPicture(*picture);
}


KPicture*
KPictureButton::EnabledOn() const
{
	return fEnabledOn;
}


KPicture*
KPictureButton::EnabledOff() const
{
	return fEnabledOff;
}


KPicture*
KPictureButton::DisabledOn() const
{
	return fDisabledOn;
}


KPicture*
KPictureButton::DisabledOff() const
{
	return fDisabledOff;
}


void
KPictureButton::SetBehavior(uint32 behavior)
{
	fBehavior = behavior;
}


uint32
KPictureButton::Behavior() const
{
	return fBehavior;
}


void
KPictureButton::SetValue(int32 value)
{
	KControl::SetValue(value);
}


status_t
KPictureButton::Invoke(BMessage* message)
{
	return KControl::Invoke(message);
}


BHandler*
KPictureButton::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	return KControl::ResolveSpecifier(message, index, specifier,
		what, property);
}


status_t
KPictureButton::GetSupportedSuites(BMessage* data)
{
	return KControl::GetSupportedSuites(data);
}


status_t
KPictureButton::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= KPictureButton::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= KPictureButton::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= KPictureButton::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= KPictureButton::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= KPictureButton::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			KPictureButton::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KPictureButton::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			KPictureButton::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			KPictureButton::DoLayout();
			return B_OK;
		}
		case PERFORM_CODE_SET_ICON:
		{
			k_perform_data_set_icon* data = (k_perform_data_set_icon*)_data;
			return KPictureButton::SetIcon(data->icon, data->flags);
		}
	}

	return KControl::Perform(code, _data);
}


status_t
KPictureButton::SetIcon(const KBitmap* icon, uint32 flags)
{
	return KControl::SetIcon(icon, flags);
}


// #pragma mark - KPictureButton private methods


void KPictureButton::_ReservedPictureButton1() {}
void KPictureButton::_ReservedPictureButton2() {}
void KPictureButton::_ReservedPictureButton3() {}


KPictureButton&
KPictureButton::operator=(const KPictureButton &button)
{
	return *this;
}

