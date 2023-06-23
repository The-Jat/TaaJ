/*
 * Copyright 2001-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers, mflerackers@androme.be
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 */


// KControl is the base class for user-event handling objects.


#include <stdlib.h>
#include <string.h>

#include <KControl.h>
#include <PropertyInfo.h>
#include <Khidki.h>

#include<KLayout.h>//khidki
#include <binary_compatibility/Interface.h>
#include <KIcon.h>


static property_info sPropertyList[] = {
	{
		"Enabled",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_BOOL_TYPE }
	},
	{
		"Label",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_STRING_TYPE }
	},
	{
		"Value",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_INT32_TYPE }
	},

	{ 0 }
};


KControl::KControl(BRect frame, const char* name, const char* label,
	BMessage* message, uint32 resizingMode, uint32 flags)
	:
	KView(frame, name, resizingMode, flags)
{
	InitData(NULL);

	SetLabel(label);
	SetMessage(message);
}


KControl::KControl(const char* name, const char* label, BMessage* message,
	uint32 flags)
	:
	KView(name, flags)
{
	InitData(NULL);

	SetLabel(label);
	SetMessage(message);
}


KControl::~KControl()
{
	free(fLabel);
	delete fIcon;
	SetMessage(NULL);
}


KControl::KControl(BMessage* data)
	:
	KView(data)
{
	InitData(data);

	BMessage message;
	if (data->FindMessage("_msg", &message) == B_OK)
		SetMessage(new BMessage(message));

	const char* label;
	if (data->FindString("_label", &label) == B_OK)
		SetLabel(label);

	int32 value;
	if (data->FindInt32("_val", &value) == B_OK)
		SetValue(value);

	bool toggle;
	if (data->FindBool("_disable", &toggle) == B_OK)
		SetEnabled(!toggle);

	if (data->FindBool("be:wants_nav", &toggle) == B_OK)
		fWantsNav = toggle;
}


BArchivable*
KControl::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "KControl"))
		return new KControl(data);

	return NULL;
}


status_t
KControl::Archive(BMessage* data, bool deep) const
{
	status_t status = KView::Archive(data, deep);

	if (status == B_OK && Message())
		status = data->AddMessage("_msg", Message());

	if (status == B_OK && fLabel)
		status = data->AddString("_label", fLabel);

	if (status == B_OK && fValue != K_CONTROL_OFF)
		status = data->AddInt32("_val", fValue);

	if (status == B_OK && !fEnabled)
		status = data->AddBool("_disable", true);

	return status;
}


void
KControl::WindowActivated(bool active)
{
	KView::WindowActivated(active);

	if (IsFocus())
		Invalidate();
}


void
KControl::AttachedToWindow()
{
	AdoptParentColors();

	if (ViewColor() == B_TRANSPARENT_COLOR
		|| Parent() == NULL) {
		AdoptSystemColors();
	}

	// Force view color as low color
	if (Parent() != NULL) {
		float tint = B_NO_TINT;
		color_which which = ViewUIColor(&tint);
		if (which != B_NO_COLOR)
			SetLowUIColor(which, tint);
		else
			SetLowColor(ViewColor());
	}

	if (!Messenger().IsValid())
		SetTarget(Window());

	KView::AttachedToWindow();
}


void
KControl::DetachedFromWindow()
{
	KView::DetachedFromWindow();
}


void
KControl::AllAttached()
{
	KView::AllAttached();
}


void
KControl::AllDetached()
{
	KView::AllDetached();
}


void
KControl::MessageReceived(BMessage* message)
{
	if (message->what == B_GET_PROPERTY || message->what == B_SET_PROPERTY) {
		BMessage reply(B_REPLY);
		bool handled = false;

		BMessage specifier;
		int32 index;
		int32 form;
		const char* property;
		if (message->GetCurrentSpecifier(&index, &specifier, &form, &property) == B_OK) {
			if (strcmp(property, "Label") == 0) {
				if (message->what == B_GET_PROPERTY) {
					reply.AddString("result", fLabel);
					handled = true;
				} else {
					// B_SET_PROPERTY
					const char* label;
					if (message->FindString("data", &label) == B_OK) {
						SetLabel(label);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if (strcmp(property, "Value") == 0) {
				if (message->what == B_GET_PROPERTY) {
					reply.AddInt32("result", fValue);
					handled = true;
				} else {
					// B_SET_PROPERTY
					int32 value;
					if (message->FindInt32("data", &value) == B_OK) {
						SetValue(value);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
				}
			} else if (strcmp(property, "Enabled") == 0) {
				if (message->what == B_GET_PROPERTY) {
					reply.AddBool("result", fEnabled);
					handled = true;
				} else {
					// B_SET_PROPERTY
					bool enabled;
					if (message->FindBool("data", &enabled) == B_OK) {
						SetEnabled(enabled);
						reply.AddInt32("error", B_OK);
						handled = true;
					}
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
KControl::MakeFocus(bool focus)
{
	if (focus == IsFocus())
		return;

	KView::MakeFocus(focus);

	if (Window() != NULL) {
		fFocusChanging = true;
		Invalidate(Bounds());
		Flush();
		fFocusChanging = false;
	}
}


void
KControl::KeyDown(const char* bytes, int32 numBytes)
{
	if (*bytes == B_ENTER || *bytes == B_SPACE) {
		if (!fEnabled)
			return;

		SetValue(Value() ? K_CONTROL_OFF : K_CONTROL_ON);
		Invoke();
	} else
		KView::KeyDown(bytes, numBytes);
}


void
KControl::MouseDown(BPoint where)
{
	KView::MouseDown(where);
}


void
KControl::MouseUp(BPoint where)
{
	KView::MouseUp(where);
}


void
KControl::MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage)
{
	KView::MouseMoved(where, code, dragMessage);
}


void
KControl::SetLabel(const char* label)
{
	if (label != NULL && !label[0])
		label = NULL;

	// Has the label been changed?
	if ((fLabel && label && !strcmp(fLabel, label))
		|| ((fLabel == NULL || !fLabel[0]) && label == NULL))
		return;

	free(fLabel);
	fLabel = label ? strdup(label) : NULL;

	InvalidateLayout();
	Invalidate();
}


const char*
KControl::Label() const
{
	return fLabel;
}


void
KControl::SetValue(int32 value)
{
	if (value == fValue)
		return;

	fValue = value;
	Invalidate();
}


void
KControl::SetValueNoUpdate(int32 value)
{
	fValue = value;
}


int32
KControl::Value() const
{
	return fValue;
}


void
KControl::SetEnabled(bool enabled)
{
	if (fEnabled == enabled)
		return;

	fEnabled = enabled;

	if (fEnabled && fWantsNav)
		SetFlags(Flags() | B_NAVIGABLE);
	else if (!fEnabled && (Flags() & B_NAVIGABLE)) {
		fWantsNav = true;
		SetFlags(Flags() & ~B_NAVIGABLE);
	} else
		fWantsNav = false;

	if (Window()) {
		Invalidate(Bounds());
		Flush();
	}
}


bool
KControl::IsEnabled() const
{
	return fEnabled;
}


void
KControl::GetPreferredSize(float* _width, float* _height)
{
	KView::GetPreferredSize(_width, _height);
}


void
KControl::ResizeToPreferred()
{
	KView::ResizeToPreferred();
}


status_t
KControl::Invoke(BMessage* message)
{
	bool notify = false;
	uint32 kind = InvokeKind(&notify);

	if (!message && !notify)
		message = Message();

	BMessage clone(kind);

	if (!message) {
		if (!IsWatched())
			return B_BAD_VALUE;
	} else
		clone = *message;

	clone.AddInt64("when", (int64)system_time());
	clone.AddPointer("source", this);
	clone.AddInt32("be:value", fValue);
	clone.AddMessenger("be:sender", BMessenger(this));

	// ToDo: is this correct? If message == NULL (even if IsWatched()), we always return B_BAD_VALUE
	status_t err;
	if (message)
		err = BInvoker::Invoke(&clone);
	else
		err = B_BAD_VALUE;

	// TODO: asynchronous messaging
	SendNotices(kind, &clone);

	return err;
}


BHandler*
KControl::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	BPropertyInfo propInfo(sPropertyList);

	if (propInfo.FindMatch(message, 0, specifier, what, property) >= B_OK)
		return this;

	return KView::ResolveSpecifier(message, index, specifier, what,
		property);
}


status_t
KControl::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/vnd.Be-control");

	BPropertyInfo propInfo(sPropertyList);
	message->AddFlat("messages", &propInfo);

	return KView::GetSupportedSuites(message);
}


status_t
KControl::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= KControl::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= KControl::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= KControl::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= KControl::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= KControl::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			KControl::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
}
		case PERFORM_CODE_SET_LAYOUT:
		{
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KControl::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			KControl::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			KControl::DoLayout();
			return B_OK;
		}
		case PERFORM_CODE_SET_ICON:
		{
			k_perform_data_set_icon* data = (k_perform_data_set_icon*)_data;
			return KControl::SetIcon(data->icon, data->flags);
		}
	}

	return KView::Perform(code, _data);
}


status_t
KControl::SetIcon(const KBitmap* bitmap, uint32 flags)
{
	status_t error = KIcon::UpdateIcon(bitmap, flags, fIcon);

	if (error == B_OK) {
		InvalidateLayout();
		Invalidate();
	}

	return error;
}


status_t
KControl::SetIconBitmap(const KBitmap* bitmap, uint32 which, uint32 flags)
{
	status_t error = KIcon::SetIconBitmap(bitmap, which, flags, fIcon);

	if (error != B_OK) {
		InvalidateLayout();
		Invalidate();
	}

	return error;
}
	

const KBitmap*
KControl::IconBitmap(uint32 which) const
{
	return fIcon != NULL ? fIcon->Bitmap(which) : NULL;
}


bool
KControl::IsFocusChanging() const
{
	return fFocusChanging;
}


bool
KControl::IsTracking() const
{
	return fTracking;
}


void
KControl::SetTracking(bool state)
{
	fTracking = state;
}


extern "C" status_t
B_IF_GCC_2(_ReservedControl1__8KControl, _ZN8KControl17_ReservedControl1Ev)(
	KControl* control, const KBitmap* icon, uint32 flags)
{
	// SetIcon()
	k_perform_data_set_icon data;
	data.icon = icon;
	data.flags = flags;
	return control->Perform(PERFORM_CODE_SET_ICON, &data);
}


void KControl::_ReservedControl2() {}
void KControl::_ReservedControl3() {}
void KControl::_ReservedControl4() {}


KControl &
KControl::operator=(const KControl &)
{
	return *this;
}


void
KControl::InitData(BMessage* data)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(ViewUIColor());

	fLabel = NULL;
	SetLabel(B_EMPTY_STRING);
	fValue = K_CONTROL_OFF;
	fEnabled = true;
	fFocusChanging = false;
	fTracking = false;
	fWantsNav = Flags() & B_NAVIGABLE;
	fIcon = NULL;

	if (data && data->HasString("_fname"))
		SetFont(be_plain_font, B_FONT_FAMILY_AND_STYLE);
}

