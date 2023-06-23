/*
 * Copyright 2004 DarkWyrm <darkwyrm@earthlink.net>
 * Copyright 2013 FeemanLou
 * Copyright 2014-2015 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT license.
 *
 * Originally written by DarkWyrm <darkwyrm@earthlink.net>
 * Updated by FreemanLou as part of Google GCI 2013
 *
 * Authors:
 *		DarkWyrm, darkwyrm@earthlink.net
 *		FeemanLou
 *		John Scipione, jscipione@gmail.com
 */


#include <KAbstractSpinner.h>

#include <algorithm>

#include <KAbstractLayoutItem.h>
#include <Alignment.h>
#include <KControlLook.h>
#include <Font.h>
#include <GradientLinear.h>
#include <KLayoutItem.h>
#include <KLayoutUtils.h>
#include <Message.h>
#include <MessageFilter.h>
#include <MessageRunner.h>
#include <Point.h>
#include <PropertyInfo.h>
#include <KTextView.h>
#include <Laminate.h>
#include <Khidki.h>


static const float kFrameMargin			= 2.0f;

const char* const kFrameField			= "KAbstractSpinner:layoutItem:frame";
const char* const kLabelItemField		= "KAbstractSpinner:labelItem";
const char* const kTextViewItemField	= "KAbstractSpinner:textViewItem";


static property_info sProperties[] = {
	{
		"Align",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the alignment of the spinner label.",
		0,
		{ B_INT32_TYPE }
	},
	{
		"Align",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the alignment of the spinner label.",
		0,
		{ B_INT32_TYPE }
	},

	{
		"ButtonStyle",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the style of the spinner buttons.",
		0,
		{ B_INT32_TYPE }
	},
	{
		"ButtonStyle",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the style of the spinner buttons.",
		0,
		{ B_INT32_TYPE }
	},

	{
		"Divider",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the divider position of the spinner.",
		0,
		{ B_FLOAT_TYPE }
	},
	{
		"Divider",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the divider position of the spinner.",
		0,
		{ B_FLOAT_TYPE }
	},

	{
		"Enabled",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns whether or not the spinner is enabled.",
		0,
		{ B_BOOL_TYPE }
	},
	{
		"Enabled",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets whether or not the spinner is enabled.",
		0,
		{ B_BOOL_TYPE }
	},

	{
		"Label",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the spinner label.",
		0,
		{ B_STRING_TYPE }
	},
	{
		"Label",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the spinner label.",
		0,
		{ B_STRING_TYPE }
	},

	{
		"Message",
		{ B_GET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0 },
		"Returns the spinner invocation message.",
		0,
		{ B_MESSAGE_TYPE }
	},
	{
		"Message",
		{ B_SET_PROPERTY, 0 },
		{ B_DIRECT_SPECIFIER, 0},
		"Sets the spinner invocation message.",
		0,
		{ B_MESSAGE_TYPE }
	},

	{ 0 }
};


typedef enum {
	SPINNER_INCREMENT,
	SPINNER_DECREMENT
} spinner_direction;


class KSpinnerButton : public KView {
public:
								KSpinnerButton(BRect frame, const char* name,
									spinner_direction direction);
	virtual						~KSpinnerButton();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* message);
	virtual void				MessageReceived(BMessage* message);

			bool				IsEnabled() const { return fIsEnabled; }
	virtual	void				SetEnabled(bool enable) { fIsEnabled = enable; };

private:
			spinner_direction	fSpinnerDirection;
			KAbstractSpinner*	fParent;
			bool				fIsEnabled;
			bool				fIsMouseDown;
			bool				fIsMouseOver;
			BMessageRunner*		fRepeater;
};


class KSpinnerTextView : public KTextView {
public:
								KSpinnerTextView(BRect rect, BRect textRect);
	virtual						~KSpinnerTextView();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MakeFocus(bool focus);

private:
			KAbstractSpinner*	fParent;
};


class KAbstractSpinner::LabelLayoutItem : public KAbstractLayoutItem {
public:
								LabelLayoutItem(KAbstractSpinner* parent);
								LabelLayoutItem(BMessage* archive);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(KAbstractSpinner* parent);
	virtual	KView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

			BRect				FrameInParent() const;

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

private:
			KAbstractSpinner*	fParent;
			BRect				fFrame;
};


class KAbstractSpinner::TextViewLayoutItem : public KAbstractLayoutItem {
public:
								TextViewLayoutItem(KAbstractSpinner* parent);
								TextViewLayoutItem(BMessage* archive);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(KAbstractSpinner* parent);
	virtual	KView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

			BRect				FrameInParent() const;

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

private:
			KAbstractSpinner*	fParent;
			BRect				fFrame;
};


struct KAbstractSpinner::LayoutData {
	LayoutData(float width, float height)
	:
	label_layout_item(NULL),
	text_view_layout_item(NULL),
	label_width(0),
	label_height(0),
	text_view_width(0),
	text_view_height(0),
	previous_width(width),
	previous_height(height),
	valid(false)
	{
	}

	LabelLayoutItem* label_layout_item;
	TextViewLayoutItem* text_view_layout_item;

	font_height font_info;

	float label_width;
	float label_height;
	float text_view_width;
	float text_view_height;

	float previous_width;
	float previous_height;

	BSize min;
	BAlignment alignment;

	bool valid;
};


//	#pragma mark - KSpinnerButton


KSpinnerButton::KSpinnerButton(BRect frame, const char* name,
	spinner_direction direction)
	:
	KView(frame, name, B_FOLLOW_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW),
	fSpinnerDirection(direction),
	fParent(NULL),
	fIsEnabled(true),
	fIsMouseDown(false),
	fIsMouseOver(false),
	fRepeater(NULL)
{
}


KSpinnerButton::~KSpinnerButton()
{
	delete fRepeater;
}


void
KSpinnerButton::AttachedToWindow()
{
	fParent = static_cast<KAbstractSpinner*>(Parent());

	AdoptParentColors();
	KView::AttachedToWindow();
}


void
KSpinnerButton::DetachedFromWindow()
{
	fParent = NULL;

	KView::DetachedFromWindow();
}


void
KSpinnerButton::Draw(BRect updateRect)
{
	BRect rect(Bounds());
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	KView::Draw(updateRect);

	float frameTint = fIsEnabled ? B_DARKEN_1_TINT : B_NO_TINT;

	float fgTint;
	if (!fIsEnabled)
		fgTint = B_DARKEN_1_TINT;
	else if (fIsMouseDown)
		fgTint = B_DARKEN_MAX_TINT;
	else
		fgTint = 1.777f;	// 216 --> 48.2 (48)

	float bgTint;
	if (fIsEnabled && fIsMouseOver)
		bgTint = B_DARKEN_1_TINT;
	else
		bgTint = B_NO_TINT;

	rgb_color bgColor = ui_color(B_PANEL_BACKGROUND_COLOR);
	if (bgColor.red + bgColor.green + bgColor.blue <= 128 * 3) {
		// if dark background make the tint lighter
		frameTint = 2.0f - frameTint;
		fgTint = 2.0f - fgTint;
		bgTint = 2.0f - bgTint;
	}

	uint32 borders = k_be_control_look->B_TOP_BORDER
		| k_be_control_look->B_BOTTOM_BORDER;

	if (fSpinnerDirection == SPINNER_INCREMENT)
		borders |= k_be_control_look->B_RIGHT_BORDER;
	else
		borders |= k_be_control_look->B_LEFT_BORDER;

	uint32 flags = fIsMouseDown ? KControlLook::B_ACTIVATED : 0;
	flags |= !fIsEnabled ? KControlLook::B_DISABLED : 0;

	// draw the button
	k_be_control_look->DrawButtonFrame(this, rect, updateRect,
		tint_color(bgColor, frameTint), bgColor, flags, borders);
	k_be_control_look->DrawButtonBackground(this, rect, updateRect,
		tint_color(bgColor, bgTint), flags, borders);

	switch (fParent->ButtonStyle()) {
		case SPINNER_BUTTON_HORIZONTAL_ARROWS:
		{
			int32 arrowDirection = fSpinnerDirection == SPINNER_INCREMENT
				? k_be_control_look->B_RIGHT_ARROW
				: k_be_control_look->B_LEFT_ARROW;

			rect.InsetBy(0.0f, 1.0f);
			k_be_control_look->DrawArrowShape(this, rect, updateRect, bgColor,
				arrowDirection, 0, fgTint);
			break;
		}

		case SPINNER_BUTTON_VERTICAL_ARROWS:
		{
			int32 arrowDirection = fSpinnerDirection == SPINNER_INCREMENT
				? k_be_control_look->B_UP_ARROW
				: k_be_control_look->B_DOWN_ARROW;

			rect.InsetBy(0.0f, 1.0f);
			k_be_control_look->DrawArrowShape(this, rect, updateRect, bgColor,
				arrowDirection, 0, fgTint);
			break;
		}

		default:
		case SPINNER_BUTTON_PLUS_MINUS:
		{
			BFont font;
			fParent->GetFont(&font);
			float inset = floorf(font.Size() / 4);
			rect.InsetBy(inset, inset);

			if (rect.IntegerWidth() % 2 != 0)
				rect.right -= 1;

			if (rect.IntegerHeight() % 2 != 0)
				rect.bottom -= 1;

			SetHighColor(tint_color(bgColor, fgTint));

			// draw the +/-
			float halfHeight = floorf(rect.Height() / 2);
			StrokeLine(BPoint(rect.left, rect.top + halfHeight),
				BPoint(rect.right, rect.top + halfHeight));
			if (fSpinnerDirection == SPINNER_INCREMENT) {
				float halfWidth = floorf(rect.Width() / 2);
				StrokeLine(BPoint(rect.left + halfWidth, rect.top + 1),
					BPoint(rect.left + halfWidth, rect.bottom - 1));
			}
		}
	}
}


void
KSpinnerButton::MouseDown(BPoint where)
{
	if (fIsEnabled) {
		fIsMouseDown = true;
		fSpinnerDirection == SPINNER_INCREMENT
			? fParent->Increment()
			: fParent->Decrement();
		Invalidate();
		BMessage repeatMessage('rept');
		SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
		fRepeater = new BMessageRunner(BMessenger(this), repeatMessage,
			200000);
	}

	KView::MouseDown(where);
}


void
KSpinnerButton::MouseMoved(BPoint where, uint32 transit,
	const BMessage* message)
{
	switch (transit) {
		case B_ENTERED_VIEW:
		case B_INSIDE_VIEW:
		{
			BPoint where;
			uint32 buttons;
			GetMouse(&where, &buttons);
			fIsMouseOver = Bounds().Contains(where) && buttons == 0;

			break;
		}

		case B_EXITED_VIEW:
		case B_OUTSIDE_VIEW:
			fIsMouseOver = false;
			MouseUp(Bounds().LeftTop());
			break;
	}

	KView::MouseMoved(where, transit, message);
}


void
KSpinnerButton::MouseUp(BPoint where)
{
	fIsMouseDown = false;
	delete fRepeater;
	fRepeater = NULL;
	Invalidate();

	KView::MouseUp(where);
}


void
KSpinnerButton::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case 'rept':
		{
			if (fIsMouseDown && fRepeater != NULL) {
				fSpinnerDirection == SPINNER_INCREMENT
					? fParent->Increment()
					: fParent->Decrement();
			}

			break;
		}

		default:
			KView::MessageReceived(message);
	}
}


//	#pragma mark - KSpinnerTextView


KSpinnerTextView::KSpinnerTextView(BRect rect, BRect textRect)
	:
	KTextView(rect, "textview", textRect, B_FOLLOW_ALL,
		B_WILL_DRAW | B_NAVIGABLE),
	fParent(NULL)
{
	MakeResizable(true);
}


KSpinnerTextView::~KSpinnerTextView()
{
}


void
KSpinnerTextView::AttachedToWindow()
{
	fParent = static_cast<KAbstractSpinner*>(Parent());

	KTextView::AttachedToWindow();
}


void
KSpinnerTextView::DetachedFromWindow()
{
	fParent = NULL;

	KTextView::DetachedFromWindow();
}


void
KSpinnerTextView::KeyDown(const char* bytes, int32 numBytes)
{
	if (fParent == NULL) {
		KTextView::KeyDown(bytes, numBytes);
		return;
	}

	switch (bytes[0]) {
		case B_ENTER:
		case B_SPACE:
			fParent->SetValueFromText();
			break;

		case B_TAB:
			fParent->KeyDown(bytes, numBytes);
			break;

		case B_LEFT_ARROW:
			if (fParent->ButtonStyle() == SPINNER_BUTTON_HORIZONTAL_ARROWS
				&& (modifiers() & B_CONTROL_KEY) != 0) {
				// need to hold down control, otherwise can't move cursor
				fParent->Decrement();
			} else
				KTextView::KeyDown(bytes, numBytes);
			break;

		case B_UP_ARROW:
			if (fParent->ButtonStyle() != SPINNER_BUTTON_HORIZONTAL_ARROWS)
				fParent->Increment();
			else
				KTextView::KeyDown(bytes, numBytes);
			break;

		case B_RIGHT_ARROW:
			if (fParent->ButtonStyle() == SPINNER_BUTTON_HORIZONTAL_ARROWS
				&& (modifiers() & B_CONTROL_KEY) != 0) {
				// need to hold down control, otherwise can't move cursor
				fParent->Increment();
			} else
				KTextView::KeyDown(bytes, numBytes);
			break;

		case B_DOWN_ARROW:
			if (fParent->ButtonStyle() != SPINNER_BUTTON_HORIZONTAL_ARROWS)
				fParent->Decrement();
			else
				KTextView::KeyDown(bytes, numBytes);
			break;

		default:
			KTextView::KeyDown(bytes, numBytes);
			break;
	}
}


void
KSpinnerTextView::MakeFocus(bool focus)
{
	KTextView::MakeFocus(focus);

	if (fParent == NULL)
		return;

	if (focus)
		SelectAll();
	else
		fParent->SetValueFromText();

	fParent->_DrawTextView(fParent->Bounds());
}


//	#pragma mark - KAbstractSpinner::LabelLayoutItem


KAbstractSpinner::LabelLayoutItem::LabelLayoutItem(KAbstractSpinner* parent)
	:
	fParent(parent),
	fFrame()
{
}


KAbstractSpinner::LabelLayoutItem::LabelLayoutItem(BMessage* from)
	:
	KAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
}


bool
KAbstractSpinner::LabelLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
KAbstractSpinner::LabelLayoutItem::SetVisible(bool visible)
{
}


BRect
KAbstractSpinner::LabelLayoutItem::Frame()
{
	return fFrame;
}


void
KAbstractSpinner::LabelLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


void
KAbstractSpinner::LabelLayoutItem::SetParent(KAbstractSpinner* parent)
{
	fParent = parent;
}


KView*
KAbstractSpinner::LabelLayoutItem::View()
{
	return fParent;
}


BSize
KAbstractSpinner::LabelLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	if (fParent->Label() == NULL)
		return BSize(-1.0f, -1.0f);

	return BSize(fParent->fLayoutData->label_width
			+ k_be_control_look->DefaultLabelSpacing(),
		fParent->fLayoutData->label_height);
}


BSize
KAbstractSpinner::LabelLayoutItem::BaseMaxSize()
{
	return BaseMinSize();
}


BSize
KAbstractSpinner::LabelLayoutItem::BasePreferredSize()
{
	return BaseMinSize();
}


BAlignment
KAbstractSpinner::LabelLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


BRect
KAbstractSpinner::LabelLayoutItem::FrameInParent() const
{
	return fFrame.OffsetByCopy(-fParent->Frame().left, -fParent->Frame().top);
}


status_t
KAbstractSpinner::LabelLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t result = KAbstractLayoutItem::Archive(into, deep);

	if (result == B_OK)
		result = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(result);
}


BArchivable*
KAbstractSpinner::LabelLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KAbstractSpinner::LabelLayoutItem"))
		return new LabelLayoutItem(from);

	return NULL;
}


//	#pragma mark - KAbstractSpinner::TextViewLayoutItem


KAbstractSpinner::TextViewLayoutItem::TextViewLayoutItem(KAbstractSpinner* parent)
	:
	fParent(parent),
	fFrame()
{
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
}


KAbstractSpinner::TextViewLayoutItem::TextViewLayoutItem(BMessage* from)
	:
	KAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
}


bool
KAbstractSpinner::TextViewLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
KAbstractSpinner::TextViewLayoutItem::SetVisible(bool visible)
{
	// not allowed
}


BRect
KAbstractSpinner::TextViewLayoutItem::Frame()
{
	return fFrame;
}


void
KAbstractSpinner::TextViewLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


void
KAbstractSpinner::TextViewLayoutItem::SetParent(KAbstractSpinner* parent)
{
	fParent = parent;
}


KView*
KAbstractSpinner::TextViewLayoutItem::View()
{
	return fParent;
}


BSize
KAbstractSpinner::TextViewLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	BSize size(fParent->fLayoutData->text_view_width,
		fParent->fLayoutData->text_view_height);

	return size;
}


BSize
KAbstractSpinner::TextViewLayoutItem::BaseMaxSize()
{
	return BaseMinSize();
}


BSize
KAbstractSpinner::TextViewLayoutItem::BasePreferredSize()
{
	return BaseMinSize();
}


BAlignment
KAbstractSpinner::TextViewLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


BRect
KAbstractSpinner::TextViewLayoutItem::FrameInParent() const
{
	return fFrame.OffsetByCopy(-fParent->Frame().left, -fParent->Frame().top);
}


status_t
KAbstractSpinner::TextViewLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t result = KAbstractLayoutItem::Archive(into, deep);

	if (result == B_OK)
		result = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(result);
}


BArchivable*
KAbstractSpinner::TextViewLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KAbstractSpinner::TextViewLayoutItem"))
		return new LabelLayoutItem(from);

	return NULL;
}


//	#pragma mark - KAbstractSpinner


KAbstractSpinner::KAbstractSpinner(BRect frame, const char* name, const char* label,
	BMessage* message, uint32 resizingMode, uint32 flags)
	:
	KControl(frame, name, label, message, resizingMode,
		flags | B_WILL_DRAW | B_FRAME_EVENTS)
{
	_InitObject();
}


KAbstractSpinner::KAbstractSpinner(const char* name, const char* label, BMessage* message,
	uint32 flags)
	:
	KControl(name, label, message, flags | B_WILL_DRAW | B_FRAME_EVENTS)
{
	_InitObject();
}


KAbstractSpinner::KAbstractSpinner(BMessage* data)
	:
	KControl(data),
	fButtonStyle(SPINNER_BUTTON_PLUS_MINUS)
{
	_InitObject();

	if (data->FindInt32("_align") != B_OK)
		fAlignment = B_ALIGN_LEFT;

	if (data->FindInt32("_button_style") != B_OK)
		fButtonStyle = SPINNER_BUTTON_PLUS_MINUS;

	if (data->FindInt32("_divider") != B_OK)
		fDivider = 0.0f;
}


KAbstractSpinner::~KAbstractSpinner()
{
	delete fLayoutData;
	fLayoutData = NULL;
}


BArchivable*
KAbstractSpinner::Instantiate(BMessage* data)
{
	// cannot instantiate an abstract spinner
	return NULL;
}


status_t
KAbstractSpinner::Archive(BMessage* data, bool deep) const
{
	status_t status = KControl::Archive(data, deep);
	data->AddString("class", "Spinner");

	if (status == B_OK)
		status = data->AddInt32("_align", fAlignment);

	if (status == B_OK)
		data->AddInt32("_button_style", fButtonStyle);

	if (status == B_OK)
		status = data->AddFloat("_divider", fDivider);

	return status;
}


status_t
KAbstractSpinner::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/vnd.Haiku-spinner");

	BPropertyInfo prop_info(sProperties);
	message->AddFlat("messages", &prop_info);

	return KView::GetSupportedSuites(message);
}


BHandler*
KAbstractSpinner::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
	return KView::ResolveSpecifier(message, index, specifier, form,
		property);
}


void
KAbstractSpinner::AttachedToWindow()
{
	if (!Messenger().IsValid())
		SetTarget(Window());

	KControl::SetValue(Value());
		// sets the text and enables or disables the arrows

	_UpdateTextViewColors(IsEnabled());
	fTextView->MakeEditable(IsEnabled());

	KView::AttachedToWindow();
}


void
KAbstractSpinner::Draw(BRect updateRect)
{
	_DrawLabel(updateRect);
	_DrawTextView(updateRect);
	fIncrement->Invalidate();
	fDecrement->Invalidate();
}


void
KAbstractSpinner::FrameResized(float width, float height)
{
	KView::FrameResized(width, height);

	// TODO: this causes flickering still...

	// changes in width

	BRect bounds = Bounds();

	if (bounds.Width() > fLayoutData->previous_width) {
		// invalidate the region between the old and the new right border
		BRect rect = bounds;
		rect.left += fLayoutData->previous_width - kFrameMargin;
		rect.right--;
		Invalidate(rect);
	} else if (bounds.Width() < fLayoutData->previous_width) {
		// invalidate the region of the new right border
		BRect rect = bounds;
		rect.left = rect.right - kFrameMargin;
		Invalidate(rect);
	}

	// changes in height

	if (bounds.Height() > fLayoutData->previous_height) {
		// invalidate the region between the old and the new bottom border
		BRect rect = bounds;
		rect.top += fLayoutData->previous_height - kFrameMargin;
		rect.bottom--;
		Invalidate(rect);
		// invalidate label area
		rect = bounds;
		rect.right = fDivider;
		Invalidate(rect);
	} else if (bounds.Height() < fLayoutData->previous_height) {
		// invalidate the region of the new bottom border
		BRect rect = bounds;
		rect.top = rect.bottom - kFrameMargin;
		Invalidate(rect);
		// invalidate label area
		rect = bounds;
		rect.right = fDivider;
		Invalidate(rect);
	}

	fLayoutData->previous_width = bounds.Width();
	fLayoutData->previous_height = bounds.Height();
}


void
KAbstractSpinner::ValueChanged()
{
	// hook method - does nothing
}


void
KAbstractSpinner::MessageReceived(BMessage* message)
{
	if (!IsEnabled() && message->what == B_COLORS_UPDATED)
		_UpdateTextViewColors(false);

	KControl::MessageReceived(message);
}


void
KAbstractSpinner::MakeFocus(bool focus)
{
	fTextView->MakeFocus(focus);
}


void
KAbstractSpinner::ResizeToPreferred()
{
	KView::ResizeToPreferred();

	const char* label = Label();
	if (label != NULL) {
		fDivider = ceilf(StringWidth(label))
			+ k_be_control_look->DefaultLabelSpacing();
	} else
		fDivider = 0.0f;

	_LayoutTextView();
}


void
KAbstractSpinner::SetFlags(uint32 flags)
{
	// If the textview is navigable, set it to not navigable if needed,
	// else if it is not navigable, set it to navigable if needed
	if (fTextView->Flags() & B_NAVIGABLE) {
		if (!(flags & B_NAVIGABLE))
			fTextView->SetFlags(fTextView->Flags() & ~B_NAVIGABLE);
	} else {
		if (flags & B_NAVIGABLE)
			fTextView->SetFlags(fTextView->Flags() | B_NAVIGABLE);
	}

	// Don't make this one navigable
	flags &= ~B_NAVIGABLE;

	KView::SetFlags(flags);
}


void
KAbstractSpinner::WindowActivated(bool active)
{
	_DrawTextView(fTextView->Frame());
}


void
KAbstractSpinner::SetAlignment(alignment align)
{
	fAlignment = align;
}


void
KAbstractSpinner::SetButtonStyle(spinner_button_style buttonStyle)
{
	fButtonStyle = buttonStyle;
}


void
KAbstractSpinner::SetDivider(float position)
{
	position = roundf(position);

	float delta = fDivider - position;
	if (delta == 0.0f)
		return;

	fDivider = position;

	if ((Flags() & B_SUPPORTS_LAYOUT) != 0) {
		// We should never get here, since layout support means, we also
		// layout the divider, and don't use this method at all.
		Relayout();
	} else {
		_LayoutTextView();
		Invalidate();
	}
}


void
KAbstractSpinner::SetEnabled(bool enable)
{
	if (IsEnabled() == enable)
		return;

	KControl::SetEnabled(enable);

	fTextView->MakeEditable(enable);
	if (enable)
		fTextView->SetFlags(fTextView->Flags() | B_NAVIGABLE);
	else
		fTextView->SetFlags(fTextView->Flags() & ~B_NAVIGABLE);

	_UpdateTextViewColors(enable);
	fTextView->Invalidate();

	_LayoutTextView();
	Invalidate();
	if (Window() != NULL)
		Window()->UpdateIfNeeded();
}


void
KAbstractSpinner::SetLabel(const char* label)
{
	KControl::SetLabel(label);

	if (Window() != NULL)
		Window()->UpdateIfNeeded();
}


bool
KAbstractSpinner::IsDecrementEnabled() const
{
	return fDecrement->IsEnabled();
}


void
KAbstractSpinner::SetDecrementEnabled(bool enable)
{
	if (IsDecrementEnabled() == enable)
		return;

	fDecrement->SetEnabled(enable);
	fDecrement->Invalidate();
}


bool
KAbstractSpinner::IsIncrementEnabled() const
{
	return fIncrement->IsEnabled();
}


void
KAbstractSpinner::SetIncrementEnabled(bool enable)
{
	if (IsIncrementEnabled() == enable)
		return;

	fIncrement->SetEnabled(enable);
	fIncrement->Invalidate();
}


BSize
KAbstractSpinner::MinSize()
{
	_ValidateLayoutData();
	return KLayoutUtils::ComposeSize(ExplicitMinSize(), fLayoutData->min);
}


BSize
KAbstractSpinner::MaxSize()
{
	_ValidateLayoutData();

	BSize max = fLayoutData->min;
	max.width = B_SIZE_UNLIMITED;

	return KLayoutUtils::ComposeSize(ExplicitMaxSize(), max);
}


BSize
KAbstractSpinner::PreferredSize()
{
	_ValidateLayoutData();
	return KLayoutUtils::ComposeSize(ExplicitPreferredSize(),
		fLayoutData->min);
}


BAlignment
KAbstractSpinner::LayoutAlignment()
{
	_ValidateLayoutData();
	return KLayoutUtils::ComposeAlignment(ExplicitAlignment(),
		BAlignment(B_ALIGN_LEFT, B_ALIGN_VERTICAL_CENTER));
}


KLayoutItem*
KAbstractSpinner::CreateLabelLayoutItem()
{
	if (fLayoutData->label_layout_item == NULL)
		fLayoutData->label_layout_item = new LabelLayoutItem(this);

	return fLayoutData->label_layout_item;
}


KLayoutItem*
KAbstractSpinner::CreateTextViewLayoutItem()
{
	if (fLayoutData->text_view_layout_item == NULL)
		fLayoutData->text_view_layout_item = new TextViewLayoutItem(this);

	return fLayoutData->text_view_layout_item;
}


KTextView*
KAbstractSpinner::TextView() const
{
	return dynamic_cast<KTextView*>(fTextView);
}


//	#pragma mark - KAbstractSpinner protected methods


status_t
KAbstractSpinner::AllArchived(BMessage* into) const
{
	status_t result;
	if ((result = KControl::AllArchived(into)) != B_OK)
		return result;

	BArchiver archiver(into);

	BArchivable* textViewItem = fLayoutData->text_view_layout_item;
	if (archiver.IsArchived(textViewItem))
		result = archiver.AddArchivable(kTextViewItemField, textViewItem);

	if (result != B_OK)
		return result;

	BArchivable* labelBarItem = fLayoutData->label_layout_item;
	if (archiver.IsArchived(labelBarItem))
		result = archiver.AddArchivable(kLabelItemField, labelBarItem);

	return result;
}


status_t
KAbstractSpinner::AllUnarchived(const BMessage* from)
{
	BUnarchiver unarchiver(from);

	status_t result = B_OK;
	if ((result = KControl::AllUnarchived(from)) != B_OK)
		return result;

	if (unarchiver.IsInstantiated(kTextViewItemField)) {
		TextViewLayoutItem*& textViewItem
			= fLayoutData->text_view_layout_item;
		result = unarchiver.FindObject(kTextViewItemField,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP, textViewItem);

		if (result == B_OK)
			textViewItem->SetParent(this);
		else
			return result;
	}

	if (unarchiver.IsInstantiated(kLabelItemField)) {
		LabelLayoutItem*& labelItem = fLayoutData->label_layout_item;
		result = unarchiver.FindObject(kLabelItemField,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP, labelItem);

		if (result == B_OK)
			labelItem->SetParent(this);
	}

	return result;
}


void
KAbstractSpinner::DoLayout()
{
	if ((Flags() & B_SUPPORTS_LAYOUT) == 0)
		return;

	if (GetLayout()) {
		KControl::DoLayout();
		return;
	}

	_ValidateLayoutData();

	BSize size(Bounds().Size());
	if (size.width < fLayoutData->min.width)
		size.width = fLayoutData->min.width;

	if (size.height < fLayoutData->min.height)
		size.height = fLayoutData->min.height;

	float divider = 0;
	if (fLayoutData->label_layout_item != NULL
		&& fLayoutData->text_view_layout_item != NULL
		&& fLayoutData->label_layout_item->Frame().IsValid()
		&& fLayoutData->text_view_layout_item->Frame().IsValid()) {
		divider = fLayoutData->text_view_layout_item->Frame().left
			- fLayoutData->label_layout_item->Frame().left;
	} else if (fLayoutData->label_width > 0) {
		divider = fLayoutData->label_width
			+ k_be_control_look->DefaultLabelSpacing();
	}
	fDivider = divider;

	BRect dirty(fTextView->Frame());
	_LayoutTextView();

	// invalidate dirty region
	dirty = dirty | fTextView->Frame();
	dirty = dirty | fIncrement->Frame();
	dirty = dirty | fDecrement->Frame();

	Invalidate(dirty);
}


void
KAbstractSpinner::LayoutInvalidated(bool descendants)
{
	if (fLayoutData != NULL)
		fLayoutData->valid = false;
}


//	#pragma mark - KAbstractSpinner private methods


void
KAbstractSpinner::_DrawLabel(BRect updateRect)
{
	BRect rect(Bounds());
	rect.right = fDivider;
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	_ValidateLayoutData();

	const char* label = Label();
	if (label == NULL)
		return;

	// horizontal position
	float x;
	switch (fAlignment) {
		case B_ALIGN_RIGHT:
			x = fDivider - fLayoutData->label_width - 3.0f;
			break;

		case B_ALIGN_CENTER:
			x = fDivider - roundf(fLayoutData->label_width / 2.0f);
			break;

		default:
			x = 0.0f;
			break;
	}

	// vertical position
	font_height& fontHeight = fLayoutData->font_info;
	float y = rect.top
		+ roundf((rect.Height() + 1.0f - fontHeight.ascent
			- fontHeight.descent) / 2.0f)
		+ fontHeight.ascent;

	uint32 flags = k_be_control_look->Flags(this);

	// erase the is control flag before drawing the label so that the label
	// will get drawn using B_PANEL_TEXT_COLOR.
	flags &= ~KControlLook::B_IS_CONTROL;

	k_be_control_look->DrawLabel(this, label, LowColor(), flags, BPoint(x, y));
}


void
KAbstractSpinner::_DrawTextView(BRect updateRect)
{
	BRect rect = fTextView->Frame();
	rect.InsetBy(-kFrameMargin, -kFrameMargin);
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = 0;
	if (!IsEnabled())
		flags |= KControlLook::B_DISABLED;

	if (fTextView->IsFocus() && Window()->IsActive())
		flags |= KControlLook::B_FOCUSED;

	k_be_control_look->DrawTextControlBorder(this, rect, updateRect, base,
		flags);
}


void
KAbstractSpinner::_InitObject()
{
	fAlignment = B_ALIGN_LEFT;
	fButtonStyle = SPINNER_BUTTON_PLUS_MINUS;

	if (Label() != NULL) {
		fDivider = StringWidth(Label())
			+ k_be_control_look->DefaultLabelSpacing();
	} else
		fDivider = 0.0f;

	KControl::SetEnabled(true);
	KControl::SetValue(0);

	BRect rect(Bounds());
	fLayoutData = new LayoutData(rect.Width(), rect.Height());

	rect.left = fDivider;
	rect.InsetBy(kFrameMargin, kFrameMargin);
	rect.right -= rect.Height() * 2 + kFrameMargin * 2 + 1.0f;
	BRect textRect(rect.OffsetToCopy(B_ORIGIN));

	fTextView = new KSpinnerTextView(rect, textRect);
	AddChild(fTextView);

	rect.InsetBy(0.0f, -kFrameMargin);

	rect.left = rect.right + kFrameMargin * 2;
	rect.right = rect.left + rect.Height() - kFrameMargin * 2;

	fDecrement = new KSpinnerButton(rect, "decrement", SPINNER_DECREMENT);
	AddChild(fDecrement);

	rect.left = rect.right + 1.0f;
	rect.right = rect.left + rect.Height() - kFrameMargin * 2;

	fIncrement = new KSpinnerButton(rect, "increment", SPINNER_INCREMENT);
	AddChild(fIncrement);

	uint32 navigableFlags = Flags() & B_NAVIGABLE;
	if (navigableFlags != 0)
		KControl::SetFlags(Flags() & ~B_NAVIGABLE);
}


void
KAbstractSpinner::_LayoutTextView()
{
	BRect rect;
	if (fLayoutData->text_view_layout_item != NULL) {
		rect = fLayoutData->text_view_layout_item->FrameInParent();
	} else {
		rect = Bounds();
		rect.left = fDivider;
	}
	rect.InsetBy(kFrameMargin, kFrameMargin);
	rect.right -= rect.Height() * 2 + kFrameMargin * 2 + 1.0f;

	fTextView->MoveTo(rect.left, rect.top);
	fTextView->ResizeTo(rect.Width(), rect.Height());
	fTextView->SetTextRect(rect.OffsetToCopy(B_ORIGIN));

	rect.InsetBy(0.0f, -kFrameMargin);

	rect.left = rect.right + kFrameMargin * 2;
	rect.right = rect.left + rect.Height() - kFrameMargin * 2;

	fDecrement->ResizeTo(rect.Width(), rect.Height());
	fDecrement->MoveTo(rect.LeftTop());

	rect.left = rect.right + 1.0f;
	rect.right = rect.left + rect.Height() - kFrameMargin * 2;

	fIncrement->ResizeTo(rect.Width(), rect.Height());
	fIncrement->MoveTo(rect.LeftTop());
}


void
KAbstractSpinner::_UpdateFrame()
{
	if (fLayoutData->label_layout_item == NULL
		|| fLayoutData->text_view_layout_item == NULL) {
		return;
	}

	BRect labelFrame = fLayoutData->label_layout_item->Frame();
	BRect textViewFrame = fLayoutData->text_view_layout_item->Frame();

	if (!labelFrame.IsValid() || !textViewFrame.IsValid())
		return;

	// update divider
	fDivider = textViewFrame.left - labelFrame.left;

	BRect frame = textViewFrame | labelFrame;
	MoveTo(frame.left, frame.top);
	BSize oldSize = Bounds().Size();
	ResizeTo(frame.Width(), frame.Height());
	BSize newSize = Bounds().Size();

	// If the size changes, ResizeTo() will trigger a relayout, otherwise
	// we need to do that explicitly.
	if (newSize != oldSize)
		Relayout();
}


void
KAbstractSpinner::_UpdateTextViewColors(bool enable)
{
	// Mimick BTextControl's appearance.
	rgb_color textColor = ui_color(B_DOCUMENT_TEXT_COLOR);

	if (enable) {
		fTextView->SetViewUIColor(B_DOCUMENT_BACKGROUND_COLOR);
		fTextView->SetLowUIColor(ViewUIColor());
	} else {
		rgb_color color = ui_color(B_DOCUMENT_BACKGROUND_COLOR);
		color = disable_color(ViewColor(), color);
		textColor = disable_color(textColor, ViewColor());

		fTextView->SetViewColor(color);
		fTextView->SetLowColor(color);
	}

	BFont font;
	fTextView->GetFontAndColor(0, &font);
	fTextView->SetFontAndColor(&font, B_FONT_ALL, &textColor);
}


void
KAbstractSpinner::_ValidateLayoutData()
{
	if (fLayoutData->valid)
		return;

	font_height& fontHeight = fLayoutData->font_info;
	GetFontHeight(&fontHeight);

	if (Label() != NULL) {
		fLayoutData->label_width = StringWidth(Label());
		fLayoutData->label_height = ceilf(fontHeight.ascent
			+ fontHeight.descent + fontHeight.leading);
	} else {
		fLayoutData->label_width = 0;
		fLayoutData->label_height = 0;
	}

	float divider = 0;
	if (fLayoutData->label_width > 0) {
		divider = ceilf(fLayoutData->label_width
			+ k_be_control_look->DefaultLabelSpacing());
	}

	if ((Flags() & B_SUPPORTS_LAYOUT) == 0)
		divider = std::max(divider, fDivider);

	float minTextWidth = fTextView->StringWidth("99999");

	float textViewHeight = fTextView->LineHeight(0) + kFrameMargin * 2;
	float textViewWidth = minTextWidth + textViewHeight * 2;

	fLayoutData->text_view_width = textViewWidth;
	fLayoutData->text_view_height = textViewHeight;

	BSize min(textViewWidth, textViewHeight);
	if (divider > 0.0f)
		min.width += divider;

	if (fLayoutData->label_height > min.height)
		min.height = fLayoutData->label_height;

	fLayoutData->min = min;
	fLayoutData->valid = true;

	ResetLayoutInvalidation();
}


// FBC padding

void KAbstractSpinner::_ReservedAbstractSpinner20() {}
void KAbstractSpinner::_ReservedAbstractSpinner19() {}
void KAbstractSpinner::_ReservedAbstractSpinner18() {}
void KAbstractSpinner::_ReservedAbstractSpinner17() {}
void KAbstractSpinner::_ReservedAbstractSpinner16() {}
void KAbstractSpinner::_ReservedAbstractSpinner15() {}
void KAbstractSpinner::_ReservedAbstractSpinner14() {}
void KAbstractSpinner::_ReservedAbstractSpinner13() {}
void KAbstractSpinner::_ReservedAbstractSpinner12() {}
void KAbstractSpinner::_ReservedAbstractSpinner11() {}
void KAbstractSpinner::_ReservedAbstractSpinner10() {}
void KAbstractSpinner::_ReservedAbstractSpinner9() {}
void KAbstractSpinner::_ReservedAbstractSpinner8() {}
void KAbstractSpinner::_ReservedAbstractSpinner7() {}
void KAbstractSpinner::_ReservedAbstractSpinner6() {}
void KAbstractSpinner::_ReservedAbstractSpinner5() {}
void KAbstractSpinner::_ReservedAbstractSpinner4() {}
void KAbstractSpinner::_ReservedAbstractSpinner3() {}
void KAbstractSpinner::_ReservedAbstractSpinner2() {}
void KAbstractSpinner::_ReservedAbstractSpinner1() {}
