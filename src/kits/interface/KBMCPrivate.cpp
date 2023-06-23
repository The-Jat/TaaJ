/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Marc Flerackers, mflerackers@androme.be
 *		John Scipione, jcipione@gmail.com
 */


#include <KBMCPrivate.h>

#include <algorithm>

#include <KControlLook.h>
#include <LayoutUtils.h>
#include <KMenuField.h>
#include <KMenuItem.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Khidki.h>


static const float kPopUpIndicatorWidth = 13.0f;


#if __GNUC__ == 2


// This is kept only for binary compatibility with BeOS R5. This class was
// used in their KMenuField implementation and we may come across some archived
// KMenuField that needs it.
class K_BMCItem_: public KMenuItem {
public:
	K_BMCItem_(BMessage* data);
	static BArchivable* Instantiate(BMessage *data);
};


K_BMCItem_::K_BMCItem_(BMessage* data)
	:
	KMenuItem(data)
{
}


/*static*/ BArchivable*
K_BMCItem_::Instantiate(BMessage *data) {
	if (validate_instantiation(data, "K_BMCItem_"))
		return new K_BMCItem_(data);

	return NULL;
}


#endif


//	#pragma mark - K_BMCFilter_


K_BMCFilter_::K_BMCFilter_(KMenuField* menuField, uint32 what)
	:
	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE, what),
	fMenuField(menuField)
{
}


K_BMCFilter_::~K_BMCFilter_()
{
}


filter_result
K_BMCFilter_::Filter(BMessage* message, BHandler** handler)
{
	if (message->what == B_MOUSE_DOWN) {
		if (BView* view = dynamic_cast<BView*>(*handler)) {
			BPoint point;
			message->FindPoint("be:view_where", &point);
			view->ConvertToParent(&point);
			message->ReplacePoint("be:view_where", point);
			*handler = fMenuField;
		}
	}

	return B_DISPATCH_MESSAGE;
}


//	#pragma mark - K_BMCMenuBar_


K_BMCMenuBar_::K_BMCMenuBar_(BRect frame, bool fixedSize, KMenuField* menuField)
	:
	KMenuBar(frame, "_mc_mb_", B_FOLLOW_LEFT | B_FOLLOW_TOP, K_ITEMS_IN_ROW,
		!fixedSize),
	fMenuField(menuField),
	fFixedSize(fixedSize),
	fShowPopUpMarker(true)
{
	_Init();
}


K_BMCMenuBar_::K_BMCMenuBar_(KMenuField* menuField)
	:
	KMenuBar("_mc_mb_", K_ITEMS_IN_ROW),
	fMenuField(menuField),
	fFixedSize(true),
	fShowPopUpMarker(true)
{
	_Init();
}


K_BMCMenuBar_::K_BMCMenuBar_(BMessage* data)
	:
	KMenuBar(data),
	fMenuField(NULL),
	fFixedSize(true),
	fShowPopUpMarker(true)
{
	SetFlags(Flags() | B_FRAME_EVENTS);

	bool resizeToFit;
	if (data->FindBool("_rsize_to_fit", &resizeToFit) == B_OK)
		fFixedSize = !resizeToFit;
}


K_BMCMenuBar_::~K_BMCMenuBar_()
{
}


//	#pragma mark - K_BMCMenuBar_ public methods


BArchivable*
K_BMCMenuBar_::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "K_BMCMenuBar_"))
		return new K_BMCMenuBar_(data);

	return NULL;
}


void
K_BMCMenuBar_::AttachedToWindow()
{
	fMenuField = static_cast<KMenuField*>(Parent());

	// Don't cause the KeyMenuBar to change by being attached
	KMenuBar* menuBar = Window()->KeyMenuBar();
	KMenuBar::AttachedToWindow();
	Window()->SetKeyMenuBar(menuBar);

	if (fFixedSize && (Flags() & B_SUPPORTS_LAYOUT) == 0)
		SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	if (Parent() != NULL) {
		color_which which = Parent()->LowUIColor();
		if (which == B_NO_COLOR)
			SetLowColor(Parent()->LowColor());
		else
			SetLowUIColor(which);

	} else
		SetLowUIColor(B_MENU_BACKGROUND_COLOR);

	fPreviousWidth = Bounds().Width();
}


void
K_BMCMenuBar_::Draw(BRect updateRect)
{
	if (fFixedSize) {
		// Set the width of the menu bar because the menu bar bounds may have
		// been expanded by the selected menu item.
		ResizeTo(fMenuField->_MenuBarWidth(), Bounds().Height());
	} else {
		// For compatability with BeOS R5:
		//  - Set to the minimum of the menu bar width set by the menu frame
		//    and the selected menu item width.
		//  - Set the height to the preferred height ignoring the height of the
		//    menu field.
		float height;
		KMenuBar::GetPreferredSize(NULL, &height);
		ResizeTo(std::min(Bounds().Width(), fMenuField->_MenuBarWidth()),
			height);
	}

	BRect rect(Bounds());
	rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);
	uint32 flags = 0;
	if (!IsEnabled())
		flags |= KControlLook::B_DISABLED;
	if (IsFocus())
		flags |= KControlLook::B_FOCUSED;

	k_be_control_look->DrawMenuFieldBackground(this, rect,
		updateRect, base, fShowPopUpMarker, flags);

	DrawItems(updateRect);
}


void
K_BMCMenuBar_::FrameResized(float width, float height)
{
	// we need to take care of cleaning up the parent menu field
	float diff = width - fPreviousWidth;
	fPreviousWidth = width;

	if (Window() != NULL && diff != 0) {
		BRect dirty(fMenuField->Bounds());
		if (diff > 0) {
			// clean up the dirty right border of
			// the menu field when enlarging
			dirty.right = Frame().right + k_kVMargin;
			dirty.left = dirty.right - diff - k_kVMargin * 2;
			fMenuField->Invalidate(dirty);
		} else if (diff < 0) {
			// clean up the dirty right line of
			// the menu field when shrinking
			dirty.left = Frame().right - k_kVMargin;
			fMenuField->Invalidate(dirty);
		}
	}

	KMenuBar::FrameResized(width, height);
}


void
K_BMCMenuBar_::MakeFocus(bool focused)
{
	if (IsFocus() == focused)
		return;

	KMenuBar::MakeFocus(focused);
}


void
K_BMCMenuBar_::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case 'TICK':
		{
			KMenuItem* item = ItemAt(0);

			if (item != NULL && item->Submenu() != NULL
				&& item->Submenu()->Window() != NULL) {
				BMessage message(B_KEY_DOWN);

				message.AddInt8("byte", B_ESCAPE);
				message.AddInt8("key", B_ESCAPE);
				message.AddInt32("modifiers", 0);
				message.AddInt8("raw_char", B_ESCAPE);

				Window()->PostMessage(&message, this, NULL);
			}
		}
		// fall through
		default:
			KMenuBar::MessageReceived(message);
			break;
	}
}


void
K_BMCMenuBar_::SetMaxContentWidth(float width)
{
	float left;
	float right;
	GetItemMargins(&left, NULL, &right, NULL);

	KMenuBar::SetMaxContentWidth(width - (left + right));
}


void
K_BMCMenuBar_::SetEnabled(bool enabled)
{
	fMenuField->SetEnabled(enabled);

	KMenuBar::SetEnabled(enabled);
}


BSize
K_BMCMenuBar_::MinSize()
{
	BSize size;
	KMenuBar::GetPreferredSize(&size.width, &size.height);
	if (fShowPopUpMarker) {
		// account for popup indicator + a few pixels margin
		size.width += kPopUpIndicatorWidth;
	}

	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
K_BMCMenuBar_::MaxSize()
{
	// The maximum width of a normal KMenuBar is unlimited, but we want it
	// limited.
	BSize size;
	KMenuBar::GetPreferredSize(&size.width, &size.height);

	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


//	#pragma mark - K_BMCMenuBar_ private methods


void
K_BMCMenuBar_::_Init()
{
	SetFlags(Flags() | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE);
	SetBorder(K_BORDER_CONTENTS);

	float left, top, right, bottom;
	GetItemMargins(&left, &top, &right, &bottom);

#if 0
	// TODO: Better fix would be to make KMenuItem draw text properly
	// centered
	font_height fontHeight;
	GetFontHeight(&fontHeight);
	top = ceilf((Bounds().Height() - ceilf(fontHeight.ascent)
		- ceilf(fontHeight.descent)) / 2) + 1;
	bottom = top - 1;
#else
	// TODO: Fix content location properly. This is just a quick fix to
	// make the KMenuField label and the super-item of the KMenuBar
	// align vertically.
	top++;
	bottom--;
#endif

	left = right = k_be_control_look->DefaultLabelSpacing();

	SetItemMargins(left, top,
		right + (fShowPopUpMarker ? kPopUpIndicatorWidth : 0), bottom);
}
