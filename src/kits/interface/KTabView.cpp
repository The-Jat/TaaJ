/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Jérôme Duval (korli@users.berlios.de)
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Artur Wyszynski
 *		Rene Gollent (rene@gollent.com)
 */


#include <KTabView.h>
#include <KTabViewPrivate.h>

#include <new>

#include <math.h>
#include <string.h>

#include <KCardLayout.h>
#include <KControlLook.h>
#include <KGroupLayout.h>
#include <KLayoutUtils.h>
#include <List.h>
#include <Message.h>
#include <PropertyInfo.h>
#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <Khidki.h>

#include <binary_compatibility/Support.h>


static property_info sPropertyList[] = {
	{
		"Selection",
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
		{ B_INT32_TYPE }
	},

	{ 0 }
};


KTab::KTab(KView* contentsView)
	:
	fEnabled(true),
	fSelected(false),
	fFocus(false),
	fView(contentsView),
	fTabView(NULL)
{
}


KTab::KTab(BMessage* archive)
	:
	BArchivable(archive),
	fSelected(false),
	fFocus(false),
	fView(NULL),
	fTabView(NULL)
{
	bool disable;

	if (archive->FindBool("_disable", &disable) != B_OK)
		SetEnabled(true);
	else
		SetEnabled(!disable);
}


KTab::~KTab()
{
	if (fView == NULL)
		return;

	if (fSelected)
		fView->RemoveSelf();

	delete fView;
}


BArchivable*
KTab::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "KTab"))
		return new KTab(archive);

	return NULL;
}


status_t
KTab::Archive(BMessage* data, bool deep) const
{
	status_t result = BArchivable::Archive(data, deep);
	if (result != B_OK)
		return result;

	if (!fEnabled)
		result = data->AddBool("_disable", false);

	return result;
}


status_t
KTab::Perform(uint32 d, void* arg)
{
	return BArchivable::Perform(d, arg);
}


const char*
KTab::Label() const
{
	if (fView != NULL)
		return fView->Name();
	else
		return NULL;
}


void
KTab::SetLabel(const char* label)
{
	if (label == NULL || fView == NULL)
		return;

	fView->SetName(label);

	if (fTabView != NULL)
		fTabView->Invalidate();
}


bool
KTab::IsSelected() const
{
	return fSelected;
}


void
KTab::Select(KView* owner)
{
	fSelected = true;

	if (owner == NULL || fView == NULL)
		return;

	// NOTE: Views are not added/removed, if there is layout,
	// they are made visible/invisible in that case.
	if (owner->GetLayout() == NULL && fView->Parent() == NULL)
		owner->AddChild(fView);
}


void
KTab::Deselect()
{
	if (fView != NULL) {
		// NOTE: Views are not added/removed, if there is layout,
		// they are made visible/invisible in that case.
		bool removeView = false;
		KView* container = fView->Parent();
		if (container != NULL)
			removeView =
				dynamic_cast<KCardLayout*>(container->GetLayout()) == NULL;
		if (removeView)
			fView->RemoveSelf();
	}

	fSelected = false;
}


void
KTab::SetEnabled(bool enable)
{
	fEnabled = enable;
}


bool
KTab::IsEnabled() const
{
	return fEnabled;
}


void
KTab::MakeFocus(bool focus)
{
	fFocus = focus;
}


bool
KTab::IsFocus() const
{
	return fFocus;
}


void
KTab::SetView(KView* view)
{
	if (view == NULL || fView == view)
		return;

	if (fView != NULL) {
		fView->RemoveSelf();
		delete fView;
	}
	fView = view;

	if (fTabView != NULL && fSelected) {
		Select(fTabView->ContainerView());
		fTabView->Invalidate();
	}
}


KView*
KTab::View() const
{
	return fView;
}


void
KTab::DrawFocusMark(KView* owner, BRect frame)
{
	float width = owner->StringWidth(Label());

	owner->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));

	float offset = IsSelected() ? 3 : 2;
	switch (fTabView->TabSide()) {
		case KTabView::kTopSide:
			owner->StrokeLine(BPoint((frame.left + frame.right - width) / 2.0,
					frame.bottom - offset),
				BPoint((frame.left + frame.right + width) / 2.0,
					frame.bottom - offset));
			break;
		case KTabView::kBottomSide:
			owner->StrokeLine(BPoint((frame.left + frame.right - width) / 2.0,
					frame.top + offset),
				BPoint((frame.left + frame.right + width) / 2.0,
					frame.top + offset));
			break;
		case KTabView::kLeftSide:
			owner->StrokeLine(BPoint(frame.right - offset,
					(frame.top + frame.bottom - width) / 2.0),
				BPoint(frame.right - offset,
					(frame.top + frame.bottom + width) / 2.0));
			break;
		case KTabView::kRightSide:
			owner->StrokeLine(BPoint(frame.left + offset,
					(frame.top + frame.bottom - width) / 2.0),
				BPoint(frame.left + offset,
					(frame.top + frame.bottom + width) / 2.0));
			break;
	}
}


void
KTab::DrawLabel(KView* owner, BRect frame)
{
	float rotation = 0.0f;
	BPoint center(frame.left + frame.Width() / 2,
		frame.top + frame.Height() / 2);
	switch (fTabView->TabSide()) {
		case KTabView::kTopSide:
		case KTabView::kBottomSide:
			rotation = 0.0f;
			break;
		case KTabView::kLeftSide:
			rotation = 270.0f;
			break;
		case KTabView::kRightSide:
			rotation = 90.0f;
			break;
	}

	if (rotation != 0.0f) {
		// DrawLabel doesn't allow rendering rotated text
		// rotate frame first and BAffineTransform will handle the rotation
		// we can't give "unrotated" frame because it comes from
		// KTabView::TabFrame and it is also used to handle mouse clicks
		BRect originalFrame(frame);
		frame.top = center.y - originalFrame.Width() / 2;
		frame.bottom = center.y + originalFrame.Width() / 2;
		frame.left = center.x - originalFrame.Height() / 2;
		frame.right = center.x + originalFrame.Height() / 2;
	}

	BAffineTransform transform;
	transform.RotateBy(center, rotation * M_PI / 180.0f);
	owner->SetTransform(transform);
	k_be_control_look->DrawLabel(owner, Label(), frame, frame,
		ui_color(B_PANEL_BACKGROUND_COLOR),
		IsEnabled() ? 0 : KControlLook::B_DISABLED,
		BAlignment(B_ALIGN_HORIZONTAL_CENTER, B_ALIGN_VERTICAL_CENTER));
	owner->SetTransform(BAffineTransform());
}


void
KTab::DrawTab(KView* owner, BRect frame, tab_position, bool)
{
	if (fTabView == NULL)
		return;

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = 0;
	uint32 borders = _Borders(owner, frame);

	int32 index = fTabView->IndexOf(this);
	int32 selected = fTabView->Selection();
	int32 first = 0;
	int32 last = fTabView->CountTabs() - 1;

	if (index == selected) {
		k_be_control_look->DrawActiveTab(owner, frame, frame, base, flags,
			borders, fTabView->TabSide(), index, selected, first, last);
	} else {
		k_be_control_look->DrawInactiveTab(owner, frame, frame, base, flags,
			borders, fTabView->TabSide(), index, selected, first, last);
	}

	DrawLabel(owner, frame);
}


//	#pragma mark - KTab private methods


uint32
KTab::_Borders(KView* owner, BRect frame)
{
	uint32 borders = 0;
	if (owner == NULL || fTabView == NULL)
		return borders;

	if (fTabView->TabSide() == KTabView::kTopSide
		|| fTabView->TabSide() == KTabView::kBottomSide) {
		borders = KControlLook::B_TOP_BORDER | KControlLook::B_BOTTOM_BORDER;

		if (frame.left == owner->Bounds().left)
			borders |= KControlLook::B_LEFT_BORDER;

		if (frame.right == owner->Bounds().right)
			borders |= KControlLook::B_RIGHT_BORDER;
	} else if (fTabView->TabSide() == KTabView::kLeftSide
		|| fTabView->TabSide() == KTabView::kRightSide) {
		borders = KControlLook::B_LEFT_BORDER | KControlLook::B_RIGHT_BORDER;

		if (frame.top == owner->Bounds().top)
			borders |= KControlLook::B_TOP_BORDER;

		if (frame.bottom == owner->Bounds().bottom)
			borders |= KControlLook::B_BOTTOM_BORDER;
	}

	return borders;
}


//	#pragma mark - FBC padding and private methods


void KTab::_ReservedTab1() {}
void KTab::_ReservedTab2() {}
void KTab::_ReservedTab3() {}
void KTab::_ReservedTab4() {}
void KTab::_ReservedTab5() {}
void KTab::_ReservedTab6() {}
void KTab::_ReservedTab7() {}
void KTab::_ReservedTab8() {}
void KTab::_ReservedTab9() {}
void KTab::_ReservedTab10() {}
void KTab::_ReservedTab11() {}
void KTab::_ReservedTab12() {}

KTab &KTab::operator=(const KTab &)
{
	// this is private and not functional, but exported
	return *this;
}


//	#pragma mark - KTabView


KTabView::KTabView(const char* name, button_width width, uint32 flags)
	:
	KView(name, flags)
{
	_InitObject(true, width);
}


KTabView::KTabView(BRect frame, const char* name, button_width width,
	uint32 resizeMask, uint32 flags)
	:
	KView(frame, name, resizeMask, flags)
{
	_InitObject(false, width);
}


KTabView::~KTabView()
{
	for (int32 i = 0; i < CountTabs(); i++)
		delete TabAt(i);

	delete fTabList;
}


KTabView::KTabView(BMessage* archive)
	:
	KView(BUnarchiver::PrepareArchive(archive)),
	fTabList(new BList),
	fContainerView(NULL),
	fFocus(-1)
{
	BUnarchiver unarchiver(archive);

	int16 width;
	if (archive->FindInt16("_but_width", &width) == B_OK)
		fTabWidthSetting = (button_width)width;
	else
		fTabWidthSetting = B_WIDTH_AS_USUAL;

	if (archive->FindFloat("_high", &fTabHeight) != B_OK) {
		font_height fh;
		GetFontHeight(&fh);
		fTabHeight = ceilf(fh.ascent + fh.descent + fh.leading + 8.0f);
	}

	if (archive->FindInt32("_sel", &fSelection) != B_OK)
		fSelection = -1;

	if (archive->FindInt32("_border_style", (int32*)&fBorderStyle) != B_OK)
		fBorderStyle = B_FANCY_BORDER;

	if (archive->FindInt32("_TabSide", (int32*)&fTabSide) != B_OK)
		fTabSide = kTopSide;

	int32 i = 0;
	BMessage tabMsg;

	if (BUnarchiver::IsArchiveManaged(archive)) {
		int32 tabCount;
		archive->GetInfo("_l_items", NULL, &tabCount);
		for (int32 i = 0; i < tabCount; i++) {
			unarchiver.EnsureUnarchived("_l_items", i);
			unarchiver.EnsureUnarchived("_view_list", i);
		}
		return;
	}

	fContainerView = ChildAt(0);
	_InitContainerView(Flags() & B_SUPPORTS_LAYOUT);

	while (archive->FindMessage("_l_items", i, &tabMsg) == B_OK) {
		BArchivable* archivedTab = instantiate_object(&tabMsg);

		if (archivedTab) {
			KTab* tab = dynamic_cast<KTab*>(archivedTab);

			BMessage viewMsg;
			if (archive->FindMessage("_view_list", i, &viewMsg) == B_OK) {
				BArchivable* archivedView = instantiate_object(&viewMsg);
				if (archivedView)
					AddTab(dynamic_cast<KView*>(archivedView), tab);
			}
		}

		tabMsg.MakeEmpty();
		i++;
	}
}


BArchivable*
KTabView::Instantiate(BMessage* archive)
{
	if ( validate_instantiation(archive, "KTabView"))
		return new KTabView(archive);

	return NULL;
}


status_t
KTabView::Archive(BMessage* archive, bool deep) const
{
	BArchiver archiver(archive);

	status_t result = KView::Archive(archive, deep);

	if (result == B_OK)
		result = archive->AddInt16("_but_width", fTabWidthSetting);
	if (result == B_OK)
		result = archive->AddFloat("_high", fTabHeight);
	if (result == B_OK)
		result = archive->AddInt32("_sel", fSelection);
	if (result == B_OK && fBorderStyle != B_FANCY_BORDER)
		result = archive->AddInt32("_border_style", fBorderStyle);
	if (result == B_OK && fTabSide != kTopSide)
		result = archive->AddInt32("_TabSide", fTabSide);

	if (result == B_OK && deep) {
		for (int32 i = 0; i < CountTabs(); i++) {
			KTab* tab = TabAt(i);

			if ((result = archiver.AddArchivable("_l_items", tab, deep))
					!= B_OK) {
				break;
			}
			result = archiver.AddArchivable("_view_list", tab->View(), deep);
		}
	}

	return archiver.Finish(result);
}


status_t
KTabView::AllUnarchived(const BMessage* archive)
{
	status_t err = KView::AllUnarchived(archive);
	if (err != B_OK)
		return err;

	fContainerView = ChildAt(0);
	_InitContainerView(Flags() & B_SUPPORTS_LAYOUT);

	BUnarchiver unarchiver(archive);

	int32 tabCount;
	archive->GetInfo("_l_items", NULL, &tabCount);
	for (int32 i = 0; i < tabCount && err == B_OK; i++) {
		KTab* tab;
		err = unarchiver.FindObject("_l_items", i, tab);
		if (err == B_OK && tab) {
			KView* view;
			if ((err = unarchiver.FindObject("_view_list", i,
				BUnarchiver::B_DONT_ASSUME_OWNERSHIP, view)) != B_OK)
				break;

			tab->SetView(view);
			fTabList->AddItem(tab);
		}
	}

	if (err == B_OK)
		Select(fSelection);

	return err;
}


status_t
KTabView::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_ALL_UNARCHIVED:
		{
			perform_data_all_unarchived* data
				= (perform_data_all_unarchived*)_data;

			data->return_value = KTabView::AllUnarchived(data->archive);
			return B_OK;
		}
	}

	return KView::Perform(code, _data);
}


void
KTabView::AttachedToWindow()
{
	KView::AttachedToWindow();

	if (fSelection < 0 && CountTabs() > 0)
		Select(0);
}


void
KTabView::DetachedFromWindow()
{
	KView::DetachedFromWindow();
}


void
KTabView::AllAttached()
{
	KView::AllAttached();
}


void
KTabView::AllDetached()
{
	KView::AllDetached();
}


// #pragma mark -


void
KTabView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			BMessage reply(B_REPLY);
			bool handled = false;

			BMessage specifier;
			int32 index;
			int32 form;
			const char* property;
			if (message->GetCurrentSpecifier(&index, &specifier, &form,
					&property) == B_OK) {
				if (strcmp(property, "Selection") == 0) {
					if (message->what == B_GET_PROPERTY) {
						reply.AddInt32("result", fSelection);
						handled = true;
					} else {
						// B_GET_PROPERTY
						int32 selection;
						if (message->FindInt32("data", &selection) == B_OK) {
							Select(selection);
							reply.AddInt32("error", B_OK);
							handled = true;
						}
					}
				}
			}

			if (handled)
				message->SendReply(&reply);
			else
				KView::MessageReceived(message);
			break;
		}

#if 0
		// TODO this would be annoying as-is, but maybe it makes sense with
		// a modifier or using only deltaX (not the main mouse wheel)
		case B_MOUSE_WHEEL_CHANGED:
		{
			float deltaX = 0.0f;
			float deltaY = 0.0f;
			message->FindFloat("be:wheel_delta_x", &deltaX);
			message->FindFloat("be:wheel_delta_y", &deltaY);

			if (deltaX == 0.0f && deltaY == 0.0f)
				return;

			if (deltaY == 0.0f)
				deltaY = deltaX;

			int32 selection = Selection();
			int32 numTabs = CountTabs();
			if (deltaY > 0  && selection < numTabs - 1) {
				// move to the right tab.
				Select(Selection() + 1);
			} else if (deltaY < 0 && selection > 0 && numTabs > 1) {
				// move to the left tab.
				Select(selection - 1);
			}
			break;
		}
#endif

		default:
			KView::MessageReceived(message);
			break;
	}
}


void
KTabView::KeyDown(const char* bytes, int32 numBytes)
{
	if (IsHidden())
		return;

	switch (bytes[0]) {
		case B_DOWN_ARROW:
		case B_LEFT_ARROW: {
			int32 focus = fFocus - 1;
			if (focus < 0)
				focus = CountTabs() - 1;
			SetFocusTab(focus, true);
			break;
		}

		case B_UP_ARROW:
		case B_RIGHT_ARROW: {
			int32 focus = fFocus + 1;
			if (focus >= CountTabs())
				focus = 0;
			SetFocusTab(focus, true);
			break;
		}

		case B_RETURN:
		case B_SPACE:
			Select(FocusTab());
			break;

		default:
			KView::KeyDown(bytes, numBytes);
	}
}


void
KTabView::MouseDown(BPoint where)
{
	// Which button is pressed?
	uint32 buttons = 0;
	BMessage* currentMessage = Window()->CurrentMessage();
	if (currentMessage != NULL) {
		currentMessage->FindInt32("buttons", (int32*)&buttons);
	}

	int32 selection = Selection();
	int32 numTabs = CountTabs();
	if (buttons & B_MOUSE_BUTTON(4)) {
		// The "back" mouse button moves to previous tab
		if (selection > 0 && numTabs > 1)
			Select(Selection() - 1);
	} else if (buttons & B_MOUSE_BUTTON(5)) {
		// The "forward" mouse button moves to next tab
		if (selection < numTabs - 1)
			Select(Selection() + 1);
	} else {
		// Other buttons are used to select a tab by clicking directly on it
		for (int32 i = 0; i < CountTabs(); i++) {
			if (TabFrame(i).Contains(where)
					&& i != Selection()) {
				Select(i);
				return;
			}
		}
	}

	KView::MouseDown(where);
}


void
KTabView::MouseUp(BPoint where)
{
	KView::MouseUp(where);
}


void
KTabView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	KView::MouseMoved(where, transit, dragMessage);
}


void
KTabView::Pulse()
{
	KView::Pulse();
}


void
KTabView::Select(int32 index)
{
	if (index == Selection())
		return;

	if (index < 0 || index >= CountTabs())
		index = Selection();

	KTab* tab = TabAt(Selection());

	if (tab)
		tab->Deselect();

	tab = TabAt(index);
	if (tab != NULL && fContainerView != NULL) {
		if (index == 0)
			fTabOffset = 0.0f;

		tab->Select(fContainerView);
		fSelection = index;

		// make the view visible through the layout if there is one
		KCardLayout* layout
			= dynamic_cast<KCardLayout*>(fContainerView->GetLayout());
		if (layout != NULL)
			layout->SetVisibleItem(index);
	}

	Invalidate();

	if (index != 0 && !Bounds().Contains(TabFrame(index))){
		if (!Bounds().Contains(TabFrame(index).LeftTop()))
			fTabOffset += TabFrame(index).left - Bounds().left - 20.0f;
		else
			fTabOffset += TabFrame(index).right - Bounds().right + 20.0f;

		Invalidate();
	}

	SetFocusTab(index, true);
}


int32
KTabView::Selection() const
{
	return fSelection;
}


void
KTabView::WindowActivated(bool active)
{
	KView::WindowActivated(active);

	if (IsFocus())
		Invalidate();
}


void
KTabView::MakeFocus(bool focus)
{
	KView::MakeFocus(focus);

	SetFocusTab(Selection(), focus);
}


void
KTabView::SetFocusTab(int32 tab, bool focus)
{
	if (tab >= CountTabs())
		tab = 0;

	if (tab < 0)
		tab = CountTabs() - 1;

	if (focus) {
		if (tab == fFocus)
			return;

		if (fFocus != -1){
			if (TabAt (fFocus) != NULL)
				TabAt(fFocus)->MakeFocus(false);
			Invalidate(TabFrame(fFocus));
		}
		if (TabAt(tab) != NULL){
			TabAt(tab)->MakeFocus(true);
			Invalidate(TabFrame(tab));
			fFocus = tab;
		}
	} else if (fFocus != -1) {
		TabAt(fFocus)->MakeFocus(false);
		Invalidate(TabFrame(fFocus));
		fFocus = -1;
	}
}


int32
KTabView::FocusTab() const
{
	return fFocus;
}


void
KTabView::Draw(BRect updateRect)
{
	DrawTabs();
	DrawBox(TabFrame(fSelection));

	if (IsFocus() && fFocus != -1)
		TabAt(fFocus)->DrawFocusMark(this, TabFrame(fFocus));
}


BRect
KTabView::DrawTabs()
{
	BRect bounds(Bounds());
	BRect tabFrame(bounds);
	uint32 borders = 0;
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);

	// set tabFrame to area around tabs
	if (fTabSide == kTopSide || fTabSide == kBottomSide) {
		if (fTabSide == kTopSide)
			tabFrame.bottom = fTabHeight;
		else
			tabFrame.top = tabFrame.bottom - fTabHeight;
	} else if (fTabSide == kLeftSide || fTabSide == kRightSide) {
		if (fTabSide == kLeftSide)
			tabFrame.right = fTabHeight;
		else
			tabFrame.left = tabFrame.right - fTabHeight;
	}

	// draw frame behind tabs
	k_be_control_look->DrawTabFrame(this, tabFrame, bounds, base, 0,
		borders, fBorderStyle, fTabSide);

	// draw the tabs on top of the tab frame
	BRect activeTabFrame;
	int32 tabCount = CountTabs();
	for (int32 i = 0; i < tabCount; i++) {
		BRect tabFrame = TabFrame(i);
		if (i == fSelection)
			activeTabFrame = tabFrame;

		TabAt(i)->DrawTab(this, tabFrame,
			i == fSelection ? B_TAB_FRONT
				: (i == 0) ? B_TAB_FIRST : B_TAB_ANY,
			i != fSelection - 1);
	}

	BRect tabsBounds;
	float last = 0.0f;
	float lastTab = 0.0f;
	if (fTabSide == kTopSide || fTabSide == kBottomSide) {
		lastTab = TabFrame(tabCount - 1).right;
		last = tabFrame.right;
		tabsBounds.left = tabsBounds.right = lastTab;
		borders = KControlLook::B_TOP_BORDER | KControlLook::B_BOTTOM_BORDER;
	} else if (fTabSide == kLeftSide || fTabSide == kRightSide) {
		lastTab = TabFrame(tabCount - 1).bottom;
		last = tabFrame.bottom;
		tabsBounds.top = tabsBounds.bottom = lastTab;
		borders = KControlLook::B_LEFT_BORDER | KControlLook::B_RIGHT_BORDER;
	}

	if (lastTab < last) {
		// draw a 1px right border on the last tab
		k_be_control_look->DrawInactiveTab(this, tabsBounds, tabsBounds, base, 0,
			borders, fTabSide);
	}

	return fSelection < CountTabs() ? TabFrame(fSelection) : BRect();
}


void
KTabView::DrawBox(BRect selectedTabRect)
{
	BRect rect(Bounds());
	uint32 bordersToDraw = KControlLook::B_ALL_BORDERS;
	switch (fTabSide) {
		case kTopSide:
			bordersToDraw &= ~KControlLook::B_TOP_BORDER;
			rect.top = fTabHeight;
			break;
		case kBottomSide:
			bordersToDraw &= ~KControlLook::B_BOTTOM_BORDER;
			rect.bottom -= fTabHeight;
			break;
		case kLeftSide:
			bordersToDraw &= ~KControlLook::B_LEFT_BORDER;
			rect.left = fTabHeight;
			break;
		case kRightSide:
			bordersToDraw &= ~KControlLook::B_RIGHT_BORDER;
			rect.right -= fTabHeight;
			break;
	}

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	if (fBorderStyle == B_FANCY_BORDER)
		k_be_control_look->DrawGroupFrame(this, rect, rect, base, bordersToDraw);
	else if (fBorderStyle == B_PLAIN_BORDER) {
		k_be_control_look->DrawBorder(this, rect, rect, base, B_PLAIN_BORDER,
			0, bordersToDraw);
	} else
		; // B_NO_BORDER draws no box
}


BRect
KTabView::TabFrame(int32 index) const
{
	if (index >= CountTabs() || index < 0)
		return BRect();

	float width = 100.0f;
	float height = fTabHeight;
	float offset = KControlLook::ComposeSpacing(B_USE_WINDOW_SPACING);
	BRect bounds(Bounds());

	switch (fTabWidthSetting) {
		case B_WIDTH_FROM_LABEL:
		{
			float x = 0.0f;
			for (int32 i = 0; i < index; i++){
				x += StringWidth(TabAt(i)->Label()) + 20.0f;
			}

			switch (fTabSide) {
				case kTopSide:
					return BRect(offset + x, 0.0f,
						offset + x + StringWidth(TabAt(index)->Label()) + 20.0f,
						height);
				case kBottomSide:
					return BRect(offset + x, bounds.bottom - height,
						offset + x + StringWidth(TabAt(index)->Label()) + 20.0f,
						bounds.bottom);
				case kLeftSide:
					return BRect(0.0f, offset + x, height, offset + x
						+ StringWidth(TabAt(index)->Label()) + 20.0f);
				case kRightSide:
					return BRect(bounds.right - height, offset + x,
						bounds.right, offset + x
							+ StringWidth(TabAt(index)->Label()) + 20.0f);
				default:
					return BRect();
			}
		}

		case B_WIDTH_FROM_WIDEST:
			width = 0.0;
			for (int32 i = 0; i < CountTabs(); i++) {
				float tabWidth = StringWidth(TabAt(i)->Label()) + 20.0f;
				if (tabWidth > width)
					width = tabWidth;
			}
			// fall through

		case B_WIDTH_AS_USUAL:
		default:
			switch (fTabSide) {
				case kTopSide:
					return BRect(offset + index * width, 0.0f,
						offset + index * width + width, height);
				case kBottomSide:
					return BRect(offset + index * width, bounds.bottom - height,
						offset + index * width + width, bounds.bottom);
				case kLeftSide:
					return BRect(0.0f, offset + index * width, height,
						offset + index * width + width);
				case kRightSide:
					return BRect(bounds.right - height, offset + index * width,
						bounds.right, offset + index * width + width);
				default:
					return BRect();
			}
	}
}


void
KTabView::SetFlags(uint32 flags)
{
	KView::SetFlags(flags);
}


void
KTabView::SetResizingMode(uint32 mode)
{
	KView::SetResizingMode(mode);
}


// #pragma mark -


void
KTabView::ResizeToPreferred()
{
	KView::ResizeToPreferred();
}


void
KTabView::GetPreferredSize(float* _width, float* _height)
{
	KView::GetPreferredSize(_width, _height);
}


BSize
KTabView::MinSize()
{
	BSize size;
	if (GetLayout())
		size = GetLayout()->MinSize();
	else {
		size = _TabsMinSize();
		BSize containerSize = fContainerView->MinSize();
		containerSize.width += 2 * _BorderWidth();
		containerSize.height += 2 * _BorderWidth();
		if (containerSize.width > size.width)
			size.width = containerSize.width;
		size.height += containerSize.height;
	}
	return KLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}


BSize
KTabView::MaxSize()
{
	BSize size;
	if (GetLayout())
		size = GetLayout()->MaxSize();
	else {
		size = _TabsMinSize();
		BSize containerSize = fContainerView->MaxSize();
		containerSize.width += 2 * _BorderWidth();
		containerSize.height += 2 * _BorderWidth();
		if (containerSize.width > size.width)
			size.width = containerSize.width;
		size.height += containerSize.height;
	}
	return KLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}


BSize
KTabView::PreferredSize()
{
	BSize size;
	if (GetLayout() != NULL)
		size = GetLayout()->PreferredSize();
	else {
		size = _TabsMinSize();
		BSize containerSize = fContainerView->PreferredSize();
		containerSize.width += 2 * _BorderWidth();
		containerSize.height += 2 * _BorderWidth();
		if (containerSize.width > size.width)
			size.width = containerSize.width;
		size.height += containerSize.height;
	}
	return KLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}


void
KTabView::FrameMoved(BPoint newPosition)
{
	KView::FrameMoved(newPosition);
}


void
KTabView::FrameResized(float newWidth, float newHeight)
{
	KView::FrameResized(newWidth, newHeight);
}


// #pragma mark -


BHandler*
KTabView::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 what, const char* property)
{
	BPropertyInfo propInfo(sPropertyList);

	if (propInfo.FindMatch(message, 0, specifier, what, property) >= B_OK)
		return this;

	return KView::ResolveSpecifier(message, index, specifier, what, property);
}


status_t
KTabView::GetSupportedSuites(BMessage* message)
{
	message->AddString("suites", "suite/vnd.Be-tab-view");

	BPropertyInfo propInfo(sPropertyList);
	message->AddFlat("messages", &propInfo);

	return KView::GetSupportedSuites(message);
}


// #pragma mark -


void
KTabView::AddTab(KView* target, KTab* tab)
{
	if (tab == NULL)
		tab = new KTab(target);
	else
		tab->SetView(target);

	if (fContainerView->GetLayout())
		fContainerView->GetLayout()->AddView(CountTabs(), target);

	fTabList->AddItem(tab);
	KTab::Private(tab).SetTabView(this);

	// When we haven't had a any tabs before, but are already attached to the
	// window, select this one.
	if (CountTabs() == 1 && Window() != NULL)
		Select(0);
}


KTab*
KTabView::RemoveTab(int32 index)
{
	if (index < 0 || index >= CountTabs())
		return NULL;

	KTab* tab = (KTab*)fTabList->RemoveItem(index);
	if (tab == NULL)
		return NULL;

	tab->Deselect();
	KTab::Private(tab).SetTabView(NULL);

	if (fContainerView->GetLayout())
		fContainerView->GetLayout()->RemoveItem(index);

	if (CountTabs() == 0)
		fFocus = -1;
	else if (index <= fSelection)
		Select(fSelection - 1);

	if (fFocus >= 0) {
		if (fFocus == CountTabs() - 1 || CountTabs() == 0)
			SetFocusTab(fFocus, false);
		else
			SetFocusTab(fFocus, true);
	}

	return tab;
}


KTab*
KTabView::TabAt(int32 index) const
{
	return (KTab*)fTabList->ItemAt(index);
}


void
KTabView::SetTabWidth(button_width width)
{
	fTabWidthSetting = width;

	Invalidate();
}


button_width
KTabView::TabWidth() const
{
	return fTabWidthSetting;
}


void
KTabView::SetTabHeight(float height)
{
	if (fTabHeight == height)
		return;

	fTabHeight = height;
	_LayoutContainerView(GetLayout() != NULL);

	Invalidate();
}


float
KTabView::TabHeight() const
{
	return fTabHeight;
}


void
KTabView::SetBorder(border_style borderStyle)
{
	if (fBorderStyle == borderStyle)
		return;

	fBorderStyle = borderStyle;

	_LayoutContainerView((Flags() & B_SUPPORTS_LAYOUT) != 0);
}


border_style
KTabView::Border() const
{
	return fBorderStyle;
}


void
KTabView::SetTabSide(tab_side tabSide)
{
	if (fTabSide == tabSide)
		return;

	fTabSide = tabSide;
	_LayoutContainerView(Flags() & B_SUPPORTS_LAYOUT);
}


KTabView::tab_side
KTabView::TabSide() const
{
	return fTabSide;
}


KView*
KTabView::ContainerView() const
{
	return fContainerView;
}


int32
KTabView::CountTabs() const
{
	return fTabList->CountItems();
}


KView*
KTabView::ViewForTab(int32 tabIndex) const
{
	KTab* tab = TabAt(tabIndex);
	if (tab != NULL)
		return tab->View();

	return NULL;
}


int32
KTabView::IndexOf(KTab* tab) const
{
	if (tab != NULL) {
		int32 tabCount = CountTabs();
		for (int32 index = 0; index < tabCount; index++) {
			if (TabAt(index) == tab)
				return index;
		}
	}

	return -1;
}


void
KTabView::_InitObject(bool layouted, button_width width)
{
	fTabList = new BList;

	fTabWidthSetting = width;
	fSelection = -1;
	fFocus = -1;
	fTabOffset = 0.0f;
	fBorderStyle = B_FANCY_BORDER;
	fTabSide = kTopSide;

	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_PANEL_BACKGROUND_COLOR);

	font_height fh;
	GetFontHeight(&fh);
	fTabHeight = ceilf(fh.ascent + fh.descent + fh.leading + 8.0f);

	fContainerView = NULL;
	_InitContainerView(layouted);
}


void
KTabView::_InitContainerView(bool layouted)
{
	bool needsLayout = false;
	bool createdContainer = false;
	if (layouted) {
		if (GetLayout() == NULL) {
			SetLayout(new(std::nothrow) KGroupLayout(B_HORIZONTAL));
			needsLayout = true;
		}

		if (fContainerView == NULL) {
			fContainerView = new KView("view container", B_WILL_DRAW);
			fContainerView->SetLayout(new(std::nothrow) KCardLayout());
			createdContainer = true;
		}
	} else if (fContainerView == NULL) {
		fContainerView = new KView(Bounds(), "view container", B_FOLLOW_ALL,
			B_WILL_DRAW);
		createdContainer = true;
	}

	if (needsLayout || createdContainer)
		_LayoutContainerView(layouted);

	if (createdContainer) {
		fContainerView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
		fContainerView->SetLowUIColor(B_PANEL_BACKGROUND_COLOR);
		AddChild(fContainerView);
	}
}


BSize
KTabView::_TabsMinSize() const
{
	BSize size(0.0f, TabHeight());
	int32 count = min_c(2, CountTabs());
	for (int32 i = 0; i < count; i++) {
		BRect frame = TabFrame(i);
		size.width += frame.Width();
	}

	if (count < CountTabs()) {
		// TODO: Add size for yet to be implemented buttons that allow
		// "scrolling" the displayed tabs left/right.
	}

	return size;
}


float
KTabView::_BorderWidth() const
{
	switch (fBorderStyle) {
		default:
		case B_FANCY_BORDER:
			return 3.0f;

		case B_PLAIN_BORDER:
			return 1.0f;

		case B_NO_BORDER:
			return 0.0f;
	}
}


void
KTabView::_LayoutContainerView(bool layouted)
{
	float borderWidth = _BorderWidth();
	if (layouted) {
		float topBorderOffset;
		switch (fBorderStyle) {
			default:
			case B_FANCY_BORDER:
				topBorderOffset = 1.0f;
				break;

			case B_PLAIN_BORDER:
				topBorderOffset = 0.0f;
				break;

			case B_NO_BORDER:
				topBorderOffset = -1.0f;
				break;
		}
		KGroupLayout* layout = dynamic_cast<KGroupLayout*>(GetLayout());
		if (layout != NULL) {
			float inset = borderWidth + TabHeight() - topBorderOffset;
			switch (fTabSide) {
				case kTopSide:
					layout->SetInsets(borderWidth, inset, borderWidth,
						borderWidth);
					break;
				case kBottomSide:
					layout->SetInsets(borderWidth, borderWidth, borderWidth,
						inset);
					break;
				case kLeftSide:
					layout->SetInsets(inset, borderWidth, borderWidth,
						borderWidth);
					break;
				case kRightSide:
					layout->SetInsets(borderWidth, borderWidth, inset,
						borderWidth);
					break;
			}
		}
	} else {
		BRect bounds = Bounds();
		switch (fTabSide) {
			case kTopSide:
				bounds.top += TabHeight();
				break;
			case kBottomSide:
				bounds.bottom -= TabHeight();
				break;
			case kLeftSide:
				bounds.left += TabHeight();
				break;
			case kRightSide:
				bounds.right -= TabHeight();
				break;
		}
		bounds.InsetBy(borderWidth, borderWidth);

		fContainerView->MoveTo(bounds.left, bounds.top);
		fContainerView->ResizeTo(bounds.Width(), bounds.Height());
	}
}


// #pragma mark - FBC and forbidden


void KTabView::_ReservedTabView3() {}
void KTabView::_ReservedTabView4() {}
void KTabView::_ReservedTabView5() {}
void KTabView::_ReservedTabView6() {}
void KTabView::_ReservedTabView7() {}
void KTabView::_ReservedTabView8() {}
void KTabView::_ReservedTabView9() {}
void KTabView::_ReservedTabView10() {}
void KTabView::_ReservedTabView11() {}
void KTabView::_ReservedTabView12() {}


KTabView::KTabView(const KTabView& tabView)
	: KView(tabView)
{
	// this is private and not functional, but exported
}


KTabView&
KTabView::operator=(const KTabView&)
{
	// this is private and not functional, but exported
	return *this;
}

//	#pragma mark - binary compatibility


extern "C" void
B_IF_GCC_2(_ReservedTabView1__8KTabView, _ZN8KTabView17_ReservedTabView1Ev)(
	KTabView* tabView, border_style borderStyle)
{
	tabView->KTabView::SetBorder(borderStyle);
}

extern "C" void
B_IF_GCC_2(_ReservedTabView2__8KTabView, _ZN8KTabView17_ReservedTabView2Ev)(
	KTabView* tabView, KTabView::tab_side tabSide)
{
	tabView->KTabView::SetTabSide(tabSide);
}
