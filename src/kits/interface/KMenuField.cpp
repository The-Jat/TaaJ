/*
 * Copyright 2006-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Marc Flerackers, mflerackers@androme.be
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, bonefish@cs.tu-berlin.de
 */


#include <KMenuField.h>

#include <algorithm>

#include <stdio.h>
	// for printf in TRACE
#include <stdlib.h>
#include <string.h>

#include <KAbstractLayoutItem.h>
#include <Archivable.h>
#include <KBMCPrivate.h>
#include <KControlLook.h>
#include <KLayoutUtils.h>
#include <KMenuBar.h>
#include <KMenuItem.h>
#include <KMenuItemPrivate.h>
#include <KMenuPrivate.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Khidki.h>

#include <binary_compatibility/Interface.h>
#include <binary_compatibility/Support.h>


#ifdef CALLED
#	undef CALLED
#endif
#ifdef TRACE
#	undef TRACE
#endif

//#define TRACE_MENU_FIELD
#ifdef TRACE_MENU_FIELD
#	include <FunctionTracer.h>
	static int32 sFunctionDepth = -1;
#	define CALLED(x...)	FunctionTracer _ft("KMenuField", __FUNCTION__, \
							sFunctionDepth)
#	define TRACE(x...)	{ BString _to; \
							_to.Append(' ', (sFunctionDepth + 1) * 2); \
							printf("%s", _to.String()); printf(x); }
#else
#	define CALLED(x...)
#	define TRACE(x...)
#endif


static const float kMinMenuBarWidth = 20.0f;
	// found by experimenting on BeOS R5


namespace {
	const char* const kFrameField = "KMenuField:layoutItem:frame";
	const char* const kMenuBarItemField = "KMenuField:barItem";
	const char* const kLabelItemField = "KMenuField:labelItem";
}


//	#pragma mark - LabelLayoutItem


class KMenuField::LabelLayoutItem : public KAbstractLayoutItem {
public:
								LabelLayoutItem(KMenuField* parent);
								LabelLayoutItem(BMessage* archive);

			BRect				FrameInParent() const;

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(KMenuField* parent);
	virtual	KView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

private:
			KMenuField*			fParent;
			BRect				fFrame;
};


//	#pragma mark - MenuBarLayoutItem


class KMenuField::MenuBarLayoutItem : public KAbstractLayoutItem {
public:
								MenuBarLayoutItem(KMenuField* parent);
								MenuBarLayoutItem(BMessage* from);

			BRect				FrameInParent() const;

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

			void				SetParent(KMenuField* parent);
	virtual	KView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

private:
			KMenuField*			fParent;
			BRect				fFrame;
};


//	#pragma mark - LayoutData


struct KMenuField::LayoutData {
	LayoutData()
		:
		label_layout_item(NULL),
		menu_bar_layout_item(NULL),
		previous_height(-1),
		valid(false)
	{
	}

	LabelLayoutItem*	label_layout_item;
	MenuBarLayoutItem*	menu_bar_layout_item;
	float				previous_height;	// used in FrameResized() for
											// invalidation
	font_height			font_info;
	float				label_width;
	float				label_height;
	BSize				min;
	BSize				menu_bar_min;
	bool				valid;
};


// #pragma mark - MouseDownFilter

namespace {

class MouseDownFilter : public BMessageFilter
{
public:
								MouseDownFilter();
	virtual						~MouseDownFilter();

	virtual	filter_result		Filter(BMessage* message, BHandler** target);
};


MouseDownFilter::MouseDownFilter()
	:
	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
{
}


MouseDownFilter::~MouseDownFilter()
{
}


filter_result
MouseDownFilter::Filter(BMessage* message, BHandler** target)
{
	return message->what == B_MOUSE_DOWN ? B_SKIP_MESSAGE : B_DISPATCH_MESSAGE;
}

};



// #pragma mark - KMenuField


KMenuField::KMenuField(BRect frame, const char* name, const char* label,
	KMenu* menu, uint32 resizingMode, uint32 flags)
	:
	KView(frame, name, resizingMode, flags)
{
	CALLED();

	TRACE("frame.width: %.2f, height: %.2f\n", frame.Width(), frame.Height());

	InitObject(label);

	frame.OffsetTo(B_ORIGIN);
	_InitMenuBar(menu, frame, false);

	InitObject2();
}


KMenuField::KMenuField(BRect frame, const char* name, const char* label,
	KMenu* menu, bool fixedSize, uint32 resizingMode, uint32 flags)
	:
	KView(frame, name, resizingMode, flags)
{
	InitObject(label);

	fFixedSizeMB = fixedSize;

	frame.OffsetTo(B_ORIGIN);
	_InitMenuBar(menu, frame, fixedSize);

	InitObject2();
}


KMenuField::KMenuField(const char* name, const char* label, KMenu* menu,
	uint32 flags)
	:
	KView(name, flags | B_FRAME_EVENTS)
{
	InitObject(label);

	_InitMenuBar(menu, BRect(0, 0, 100, 15), true);

	InitObject2();
}


KMenuField::KMenuField(const char* label, KMenu* menu, uint32 flags)
	:
	KView(NULL, flags | B_FRAME_EVENTS)
{
	InitObject(label);

	_InitMenuBar(menu, BRect(0, 0, 100, 15), true);

	InitObject2();
}


//! Copy&Paste error, should be removed at some point (already private)
KMenuField::KMenuField(const char* name, const char* label, KMenu* menu,
		BMessage* message, uint32 flags)
	:
	KView(name, flags | B_FRAME_EVENTS)
{
	InitObject(label);

	_InitMenuBar(menu, BRect(0, 0, 100, 15), true);

	InitObject2();
}


//! Copy&Paste error, should be removed at some point (already private)
KMenuField::KMenuField(const char* label, KMenu* menu, BMessage* message)
	:
	KView(NULL, B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS)
{
	InitObject(label);

	_InitMenuBar(menu, BRect(0, 0, 100, 15), true);

	InitObject2();
}


KMenuField::KMenuField(BMessage* data)
	:
	KView(BUnarchiver::PrepareArchive(data))
{
	BUnarchiver unarchiver(data);
	const char* label = NULL;
	data->FindString("_label", &label);

	InitObject(label);

	data->FindFloat("_divide", &fDivider);

	int32 align;
	if (data->FindInt32("_align", &align) == B_OK)
		SetAlignment((alignment)align);

	if (!BUnarchiver::IsArchiveManaged(data))
		_InitMenuBar(data);

	unarchiver.Finish();
}


KMenuField::~KMenuField()
{
	free(fLabel);

	status_t dummy;
	if (fMenuTaskID >= 0)
		wait_for_thread(fMenuTaskID, &dummy);

	delete fLayoutData;
	delete fMouseDownFilter;
}


BArchivable*
KMenuField::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "KMenuField"))
		return new KMenuField(data);

	return NULL;
}


status_t
KMenuField::Archive(BMessage* data, bool deep) const
{
	BArchiver archiver(data);
	status_t ret = KView::Archive(data, deep);

	if (ret == B_OK && Label())
		ret = data->AddString("_label", Label());

	if (ret == B_OK && !IsEnabled())
		ret = data->AddBool("_disable", true);

	if (ret == B_OK)
		ret = data->AddInt32("_align", Alignment());
	if (ret == B_OK)
		ret = data->AddFloat("_divide", Divider());

	if (ret == B_OK && fFixedSizeMB)
		ret = data->AddBool("be:fixeds", true);

	bool dmark = false;
	if (K_BMCMenuBar_* menuBar = dynamic_cast<K_BMCMenuBar_*>(fMenuBar))
		dmark = menuBar->IsPopUpMarkerShown();

	data->AddBool("be:dmark", dmark);

	return archiver.Finish(ret);
}


status_t
KMenuField::AllArchived(BMessage* into) const
{
	status_t err;
	if ((err = KView::AllArchived(into)) != B_OK)
		return err;

	BArchiver archiver(into);

	BArchivable* menuBarItem = fLayoutData->menu_bar_layout_item;
	if (archiver.IsArchived(menuBarItem))
		err = archiver.AddArchivable(kMenuBarItemField, menuBarItem);

	if (err != B_OK)
		return err;

	BArchivable* labelBarItem = fLayoutData->label_layout_item;
	if (archiver.IsArchived(labelBarItem))
		err = archiver.AddArchivable(kLabelItemField, labelBarItem);

	return err;
}


status_t
KMenuField::AllUnarchived(const BMessage* from)
{
	BUnarchiver unarchiver(from);

	status_t err = B_OK;
	if ((err = KView::AllUnarchived(from)) != B_OK)
		return err;

	_InitMenuBar(from);

	if (unarchiver.IsInstantiated(kMenuBarItemField)) {
		MenuBarLayoutItem*& menuItem = fLayoutData->menu_bar_layout_item;
		err = unarchiver.FindObject(kMenuBarItemField,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP, menuItem);

		if (err == B_OK)
			menuItem->SetParent(this);
		else
			return err;
	}

	if (unarchiver.IsInstantiated(kLabelItemField)) {
		LabelLayoutItem*& labelItem = fLayoutData->label_layout_item;
		err = unarchiver.FindObject(kLabelItemField,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP, labelItem);

		if (err == B_OK)
			labelItem->SetParent(this);
	}

	return err;
}


void
KMenuField::Draw(BRect updateRect)
{
	_DrawLabel(updateRect);
	_DrawMenuBar(updateRect);
}


void
KMenuField::AttachedToWindow()
{
	CALLED();

	// Our low color must match the parent's view color.
	if (Parent() != NULL) {
		AdoptParentColors();

		float tint = B_NO_TINT;
		color_which which = ViewUIColor(&tint);

		if (which == B_NO_COLOR)
			SetLowColor(ViewColor());
		else
			SetLowUIColor(which, tint);
	} else
		AdoptSystemColors();
}


void
KMenuField::AllAttached()
{
	CALLED();

	TRACE("width: %.2f, height: %.2f\n", Frame().Width(), Frame().Height());

	float width = Bounds().Width();
	if (!fFixedSizeMB && _MenuBarWidth() < kMinMenuBarWidth) {
		// The menu bar is too narrow, resize it to fit the menu items
		KMenuItem* item = fMenuBar->ItemAt(0);
		if (item != NULL) {
			float right;
			fMenuBar->GetItemMargins(NULL, NULL, &right, NULL);
			width = item->Frame().Width() + k_kVMargin + _MenuBarOffset() + right;
		}
	}

	ResizeTo(width, fMenuBar->Bounds().Height() + k_kVMargin * 2);

	TRACE("width: %.2f, height: %.2f\n", Frame().Width(), Frame().Height());
}


void
KMenuField::MouseDown(BPoint where)
{
	BRect bounds = fMenuBar->ConvertFromParent(Bounds());

	fMenuBar->StartMenuBar(-1, false, true, &bounds);

	fMenuTaskID = spawn_thread((thread_func)_thread_entry,
		"_m_task_", B_NORMAL_PRIORITY, this);
	if (fMenuTaskID >= 0 && resume_thread(fMenuTaskID) == B_OK) {
		if (fMouseDownFilter->Looper() == NULL)
			Window()->AddCommonFilter(fMouseDownFilter);

		SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
	}
}


void
KMenuField::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_SPACE:
		case B_RIGHT_ARROW:
		case B_DOWN_ARROW:
		{
			if (!IsEnabled())
				break;

			BRect bounds = fMenuBar->ConvertFromParent(Bounds());

			fMenuBar->StartMenuBar(0, true, true, &bounds);

			bounds = Bounds();
			bounds.right = fDivider;

			Invalidate(bounds);
		}

		default:
			KView::KeyDown(bytes, numBytes);
	}
}


void
KMenuField::MakeFocus(bool focused)
{
	if (IsFocus() == focused)
		return;

	KView::MakeFocus(focused);

	if (Window() != NULL)
		Invalidate(); // TODO: use fLayoutData->label_width
}


void
KMenuField::MessageReceived(BMessage* message)
{
	KView::MessageReceived(message);
}


void
KMenuField::WindowActivated(bool active)
{
	KView::WindowActivated(active);

	if (IsFocus())
		Invalidate();
}


void
KMenuField::MouseMoved(BPoint point, uint32 code, const BMessage* message)
{
	KView::MouseMoved(point, code, message);
}


void
KMenuField::MouseUp(BPoint where)
{
	Window()->RemoveCommonFilter(fMouseDownFilter);
	KView::MouseUp(where);
}


void
KMenuField::DetachedFromWindow()
{
	KView::DetachedFromWindow();
}


void
KMenuField::AllDetached()
{
	KView::AllDetached();
}


void
KMenuField::FrameMoved(BPoint newPosition)
{
	KView::FrameMoved(newPosition);
}


void
KMenuField::FrameResized(float newWidth, float newHeight)
{
	KView::FrameResized(newWidth, newHeight);

	if (fFixedSizeMB) {
		// we have let the menubar resize itself, but
		// in fixed size mode, the menubar is supposed to
		// be at the right end of the view always. Since
		// the menu bar is in follow left/right mode then,
		// resizing ourselfs might have caused the menubar
		// to be outside now
		fMenuBar->ResizeTo(_MenuBarWidth(), fMenuBar->Frame().Height());
	}

	if (newHeight != fLayoutData->previous_height && Label()) {
		// The height changed, which means the label has to move and we
		// probably also invalidate a part of the borders around the menu bar.
		// So don't be shy and invalidate the whole thing.
		Invalidate();
	}

	fLayoutData->previous_height = newHeight;
}


KMenu*
KMenuField::Menu() const
{
	return fMenu;
}


KMenuBar*
KMenuField::MenuBar() const
{
	return fMenuBar;
}


KMenuItem*
KMenuField::MenuItem() const
{
	return fMenuBar->ItemAt(0);
}


void
KMenuField::SetLabel(const char* label)
{
	if (fLabel) {
		if (label && strcmp(fLabel, label) == 0)
			return;

		free(fLabel);
	}

	fLabel = strdup(label);

	if (Window())
		Invalidate();

	InvalidateLayout();
}


const char*
KMenuField::Label() const
{
	return fLabel;
}


void
KMenuField::SetEnabled(bool on)
{
	if (fEnabled == on)
		return;

	fEnabled = on;
	fMenuBar->SetEnabled(on);

	if (Window()) {
		fMenuBar->Invalidate(fMenuBar->Bounds());
		Invalidate(Bounds());
	}
}


bool
KMenuField::IsEnabled() const
{
	return fEnabled;
}


void
KMenuField::SetAlignment(alignment label)
{
	fAlign = label;
}


alignment
KMenuField::Alignment() const
{
	return fAlign;
}


void
KMenuField::SetDivider(float position)
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
		BRect dirty(fMenuBar->Frame());

		fMenuBar->MoveTo(_MenuBarOffset(), k_kVMargin);

		if (fFixedSizeMB)
			fMenuBar->ResizeTo(_MenuBarWidth(), dirty.Height());

		dirty = dirty | fMenuBar->Frame();
		dirty.InsetBy(-k_kVMargin, -k_kVMargin);

		Invalidate(dirty);
	}
}


float
KMenuField::Divider() const
{
	return fDivider;
}


void
KMenuField::ShowPopUpMarker()
{
	if (K_BMCMenuBar_* menuBar = dynamic_cast<K_BMCMenuBar_*>(fMenuBar)) {
		menuBar->TogglePopUpMarker(true);
		menuBar->Invalidate();
	}
}


void
KMenuField::HidePopUpMarker()
{
	if (K_BMCMenuBar_* menuBar = dynamic_cast<K_BMCMenuBar_*>(fMenuBar)) {
		menuBar->TogglePopUpMarker(false);
		menuBar->Invalidate();
	}
}


BHandler*
KMenuField::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	return KView::ResolveSpecifier(message, index, specifier, form, property);
}


status_t
KMenuField::GetSupportedSuites(BMessage* data)
{
	return KView::GetSupportedSuites(data);
}


void
KMenuField::ResizeToPreferred()
{
	CALLED();

	TRACE("fMenuBar->Frame().width: %.2f, height: %.2f\n",
		fMenuBar->Frame().Width(), fMenuBar->Frame().Height());

	fMenuBar->ResizeToPreferred();

	TRACE("fMenuBar->Frame().width: %.2f, height: %.2f\n",
		fMenuBar->Frame().Width(), fMenuBar->Frame().Height());

	KView::ResizeToPreferred();

	Invalidate();
}


void
KMenuField::GetPreferredSize(float* _width, float* _height)
{
	CALLED();

	_ValidateLayoutData();

	if (_width)
		*_width = fLayoutData->min.width;

	if (_height)
		*_height = fLayoutData->min.height;
}


BSize
KMenuField::MinSize()
{
	CALLED();

	_ValidateLayoutData();
	return KLayoutUtils::ComposeSize(ExplicitMinSize(), fLayoutData->min);
}


BSize
KMenuField::MaxSize()
{
	CALLED();

	_ValidateLayoutData();

	BSize max = fLayoutData->min;
	max.width = B_SIZE_UNLIMITED;

	return KLayoutUtils::ComposeSize(ExplicitMaxSize(), max);
}


BSize
KMenuField::PreferredSize()
{
	CALLED();

	_ValidateLayoutData();
	return KLayoutUtils::ComposeSize(ExplicitPreferredSize(), fLayoutData->min);
}


KLayoutItem*
KMenuField::CreateLabelLayoutItem()
{
	if (fLayoutData->label_layout_item == NULL)
		fLayoutData->label_layout_item = new LabelLayoutItem(this);

	return fLayoutData->label_layout_item;
}


KLayoutItem*
KMenuField::CreateMenuBarLayoutItem()
{
	if (fLayoutData->menu_bar_layout_item == NULL) {
		// align the menu bar in the full available space
		fMenuBar->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_VERTICAL_UNSET));
		fLayoutData->menu_bar_layout_item = new MenuBarLayoutItem(this);
	}

	return fLayoutData->menu_bar_layout_item;
}


status_t
KMenuField::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= KMenuField::MinSize();
			return B_OK;

		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= KMenuField::MaxSize();
			return B_OK;

		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= KMenuField::PreferredSize();
			return B_OK;

		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= KMenuField::LayoutAlignment();
			return B_OK;

		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= KMenuField::HasHeightForWidth();
			return B_OK;

		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			KMenuField::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}

		case PERFORM_CODE_SET_LAYOUT:
		{
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KMenuField::SetLayout(data->layout);
			return B_OK;
		}

		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			KMenuField::LayoutInvalidated(data->descendants);
			return B_OK;
		}

		case PERFORM_CODE_DO_LAYOUT:
		{
			KMenuField::DoLayout();
			return B_OK;
		}

		case PERFORM_CODE_ALL_UNARCHIVED:
		{
			perform_data_all_unarchived* data
				= (perform_data_all_unarchived*)_data;
			data->return_value = KMenuField::AllUnarchived(data->archive);
			return B_OK;
		}

		case PERFORM_CODE_ALL_ARCHIVED:
		{
			perform_data_all_archived* data
				= (perform_data_all_archived*)_data;
			data->return_value = KMenuField::AllArchived(data->archive);
			return B_OK;
		}
	}

	return KView::Perform(code, _data);
}


void
KMenuField::LayoutInvalidated(bool descendants)
{
	CALLED();

	fLayoutData->valid = false;
}


void
KMenuField::DoLayout()
{
	// Bail out, if we shan't do layout.
	if ((Flags() & B_SUPPORTS_LAYOUT) == 0)
		return;

	CALLED();

	// If the user set a layout, we let the base class version call its
	// hook.
	if (GetLayout() != NULL) {
		KView::DoLayout();
		return;
	}

	_ValidateLayoutData();

	// validate current size
	BSize size(Bounds().Size());
	if (size.width < fLayoutData->min.width)
		size.width = fLayoutData->min.width;

	if (size.height < fLayoutData->min.height)
		size.height = fLayoutData->min.height;

	// divider
	float divider = 0;
	if (fLayoutData->label_layout_item != NULL
		&& fLayoutData->menu_bar_layout_item != NULL
		&& fLayoutData->label_layout_item->Frame().IsValid()
		&& fLayoutData->menu_bar_layout_item->Frame().IsValid()) {
		// We have valid layout items, they define the divider location.
		divider = fabs(fLayoutData->menu_bar_layout_item->Frame().left
			- fLayoutData->label_layout_item->Frame().left);
	} else if (fLayoutData->label_width > 0) {
		divider = fLayoutData->label_width
			+ k_be_control_look->DefaultLabelSpacing();
	}

	// menu bar
	BRect dirty(fMenuBar->Frame());
	BRect menuBarFrame(divider + k_kVMargin, k_kVMargin, size.width - k_kVMargin,
		size.height - k_kVMargin);

	// place the menu bar and set the divider
	KLayoutUtils::AlignInFrame(fMenuBar, menuBarFrame);

	fDivider = divider;

	// invalidate dirty region
	dirty = dirty | fMenuBar->Frame();
	dirty.InsetBy(-k_kVMargin, -k_kVMargin);

	Invalidate(dirty);
}


void KMenuField::_ReservedMenuField1() {}
void KMenuField::_ReservedMenuField2() {}
void KMenuField::_ReservedMenuField3() {}


void
KMenuField::InitObject(const char* label)
{
	CALLED();

	fLabel = NULL;
	fMenu = NULL;
	fMenuBar = NULL;
	fAlign = B_ALIGN_LEFT;
	fEnabled = true;
	fFixedSizeMB = false;
	fMenuTaskID = -1;
	fLayoutData = new LayoutData;
	fMouseDownFilter = new MouseDownFilter();

	SetLabel(label);

	if (label)
		fDivider = floorf(Frame().Width() / 2.0f);
	else
		fDivider = 0;
}


void
KMenuField::InitObject2()
{
	CALLED();

	if (!fFixedSizeMB) {
		float height;
		fMenuBar->GetPreferredSize(NULL, &height);
		fMenuBar->ResizeTo(_MenuBarWidth(), height);
	}

	TRACE("frame(%.1f, %.1f, %.1f, %.1f) (%.2f, %.2f)\n",
		fMenuBar->Frame().left, fMenuBar->Frame().top,
		fMenuBar->Frame().right, fMenuBar->Frame().bottom,
		fMenuBar->Frame().Width(), fMenuBar->Frame().Height());

	fMenuBar->AddFilter(new K_BMCFilter_(this, B_MOUSE_DOWN));
}


void
KMenuField::_DrawLabel(BRect updateRect)
{
	CALLED();

	_ValidateLayoutData();

	const char* label = Label();
	if (label == NULL)
		return;

	BRect rect;
	if (fLayoutData->label_layout_item != NULL)
		rect = fLayoutData->label_layout_item->FrameInParent();
	else {
		rect = Bounds();
		rect.right = fDivider;
	}

	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	uint32 flags = 0;
	if (!IsEnabled())
		flags |= KControlLook::B_DISABLED;

	// save the current low color
	PushState();
	rgb_color textColor;

	BPrivate::KMenuPrivate menuPrivate(fMenuBar);
	if (menuPrivate.State() != K_MENU_STATE_CLOSED) {
		// highlight the background of the label grey (like BeOS R5)
		SetLowColor(ui_color(B_MENU_SELECTED_BACKGROUND_COLOR));
		BRect fillRect(rect.InsetByCopy(0, k_kVMargin));
		FillRect(fillRect, B_SOLID_LOW);
		textColor = ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR);
	} else
		textColor = ui_color(B_PANEL_TEXT_COLOR);

	k_be_control_look->DrawLabel(this, label, rect, updateRect, LowColor(), flags,
		BAlignment(fAlign, B_ALIGN_MIDDLE), &textColor);

	// restore the previous low color
	PopState();
}


void
KMenuField::_DrawMenuBar(BRect updateRect)
{
	CALLED();

	BRect rect(fMenuBar->Frame().InsetByCopy(-k_kVMargin, -k_kVMargin));
	if (!rect.IsValid() || !rect.Intersects(updateRect))
		return;

	uint32 flags = 0;
	if (!IsEnabled())
		flags |= KControlLook::B_DISABLED;

	if (IsFocus() && Window()->IsActive())
		flags |= KControlLook::B_FOCUSED;

	k_be_control_look->DrawMenuFieldFrame(this, rect, updateRect,
		fMenuBar->LowColor(), LowColor(), flags);
}


void
KMenuField::InitMenu(KMenu* menu)
{
	menu->SetFont(be_plain_font);

	int32 index = 0;
	KMenu* subMenu;

	while ((subMenu = menu->SubmenuAt(index++)) != NULL)
		InitMenu(subMenu);
}


/*static*/ int32
KMenuField::_thread_entry(void* arg)
{
	return static_cast<KMenuField*>(arg)->_MenuTask();
}


int32
KMenuField::_MenuTask()
{
	if (!LockLooper())
		return 0;

	Invalidate();
	UnlockLooper();

	bool tracking;
	do {
		snooze(20000);
		if (!LockLooper())
			return 0;

		tracking = fMenuBar->fTracking;

		UnlockLooper();
	} while (tracking);

	if (LockLooper()) {
		Invalidate();
		UnlockLooper();
	}

	return 0;
}


void
KMenuField::_UpdateFrame()
{
	CALLED();

	if (fLayoutData->label_layout_item == NULL
		|| fLayoutData->menu_bar_layout_item == NULL) {
		return;
	}

	BRect labelFrame = fLayoutData->label_layout_item->Frame();
	BRect menuFrame = fLayoutData->menu_bar_layout_item->Frame();

	if (!labelFrame.IsValid() || !menuFrame.IsValid())
		return;

	// update divider
	fDivider = menuFrame.left - labelFrame.left;

	// update our frame
	MoveTo(labelFrame.left, labelFrame.top);
	BSize oldSize = Bounds().Size();
	ResizeTo(menuFrame.left + menuFrame.Width() - labelFrame.left,
		menuFrame.top + menuFrame.Height() - labelFrame.top);
	BSize newSize = Bounds().Size();

	// If the size changes, ResizeTo() will trigger a relayout, otherwise
	// we need to do that explicitly.
	if (newSize != oldSize)
		Relayout();
}


void
KMenuField::_InitMenuBar(KMenu* menu, BRect frame, bool fixedSize)
{
	CALLED();

	if ((Flags() & B_SUPPORTS_LAYOUT) != 0) {
		fMenuBar = new K_BMCMenuBar_(this);
	} else {
		frame.left = _MenuBarOffset();
		frame.top = k_kVMargin;
		frame.right -= k_kVMargin;
		frame.bottom -= k_kVMargin;

		TRACE("frame(%.1f, %.1f, %.1f, %.1f) (%.2f, %.2f)\n",
			frame.left, frame.top, frame.right, frame.bottom,
			frame.Width(), frame.Height());

		fMenuBar = new K_BMCMenuBar_(frame, fixedSize, this);
	}

	if (fixedSize) {
		// align the menu bar in the full available space
		fMenuBar->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
			B_ALIGN_VERTICAL_UNSET));
	} else {
		// align the menu bar left in the available space
		fMenuBar->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
			B_ALIGN_VERTICAL_UNSET));
	}

	AddChild(fMenuBar);

	_AddMenu(menu);

	fMenuBar->SetFont(be_plain_font);
}


void
KMenuField::_InitMenuBar(const BMessage* archive)
{
	bool fixed;
	if (archive->FindBool("be:fixeds", &fixed) == B_OK)
		fFixedSizeMB = fixed;

	fMenuBar = (KMenuBar*)FindView("_mc_mb_");
	if (fMenuBar == NULL) {
		_InitMenuBar(new KMenu(""), BRect(0, 0, 100, 15), fFixedSizeMB);
		InitObject2();
	} else {
		fMenuBar->AddFilter(new K_BMCFilter_(this, B_MOUSE_DOWN));
			// this is normally done in InitObject2()
	}

	_AddMenu(fMenuBar->SubmenuAt(0));

	bool disable;
	if (archive->FindBool("_disable", &disable) == B_OK)
		SetEnabled(!disable);

	bool dmark = false;
	archive->FindBool("be:dmark", &dmark);
	K_BMCMenuBar_* menuBar = dynamic_cast<K_BMCMenuBar_*>(fMenuBar);
	if (menuBar != NULL)
		menuBar->TogglePopUpMarker(dmark);
}


void
KMenuField::_AddMenu(KMenu* menu)
{
	if (menu == NULL || fMenuBar == NULL)
		return;

	fMenu = menu;
	InitMenu(menu);

	KMenuItem* item = NULL;
	if (!menu->IsRadioMode() || (item = menu->FindMarked()) == NULL) {
		// find the first enabled non-seperator item
		int32 itemCount = menu->CountItems();
		for (int32 i = 0; i < itemCount; i++) {
			item = menu->ItemAt((int32)i);
			if (item == NULL || !item->IsEnabled()
				|| dynamic_cast<KSeparatorItem*>(item) != NULL) {
				item = NULL;
				continue;
			}
			break;
		}
	}

	if (item == NULL) {
		fMenuBar->AddItem(menu);
		return;
	}

	// build an empty copy of item

	BMessage data;
	status_t result = item->Archive(&data, false);
	if (result != B_OK) {
		fMenuBar->AddItem(menu);
		return;
	}

	BArchivable* object = instantiate_object(&data);
	if (object == NULL) {
		fMenuBar->AddItem(menu);
		return;
	}

	KMenuItem* newItem = static_cast<KMenuItem*>(object);

	// unset parameters
	BPrivate::KMenuItemPrivate newMenuItemPrivate(newItem);
	newMenuItemPrivate.Uninstall();

	// set the menu
	newMenuItemPrivate.SetSubmenu(menu);
	fMenuBar->AddItem(newItem);
}


void
KMenuField::_ValidateLayoutData()
{
	CALLED();

	if (fLayoutData->valid)
		return;

	// cache font height
	font_height& fh = fLayoutData->font_info;
	GetFontHeight(&fh);

	const char* label = Label();
	if (label != NULL) {
		fLayoutData->label_width = ceilf(StringWidth(label));
		fLayoutData->label_height = ceilf(fh.ascent) + ceilf(fh.descent);
	} else {
		fLayoutData->label_width = 0;
		fLayoutData->label_height = 0;
	}

	// compute the minimal divider
	float divider = 0;
	if (fLayoutData->label_width > 0) {
		divider = fLayoutData->label_width
			+ k_be_control_look->DefaultLabelSpacing();
	}

	// If we shan't do real layout, we let the current divider take influence.
	if ((Flags() & B_SUPPORTS_LAYOUT) == 0)
		divider = std::max(divider, fDivider);

	// get the minimal (== preferred) menu bar size
	// TODO: KMenu::MinSize() is using the ResizeMode() to decide the
	// minimum width. If the mode is B_FOLLOW_LEFT_RIGHT, it will use the
	// parent's frame width or window's frame width. So at least the returned
	// size is wrong, but apparantly it doesn't have much bad effect.
	fLayoutData->menu_bar_min = fMenuBar->MinSize();

	TRACE("menu bar min width: %.2f\n", fLayoutData->menu_bar_min.width);

	// compute our minimal (== preferred) size
	BSize min(fLayoutData->menu_bar_min);
	min.width += 2 * k_kVMargin;
	min.height += 2 * k_kVMargin;

	if (divider > 0)
		min.width += divider;

	if (fLayoutData->label_height > min.height)
		min.height = fLayoutData->label_height;

	fLayoutData->min = min;

	fLayoutData->valid = true;
	ResetLayoutInvalidation();

	TRACE("width: %.2f, height: %.2f\n", min.width, min.height);
}


float
KMenuField::_MenuBarOffset() const
{
	return std::max(fDivider + k_kVMargin, k_kVMargin);
}


float
KMenuField::_MenuBarWidth() const
{
	return Bounds().Width() - (_MenuBarOffset() + k_kVMargin);
}


// #pragma mark - KMenuField::LabelLayoutItem


KMenuField::LabelLayoutItem::LabelLayoutItem(KMenuField* parent)
	:
	fParent(parent),
	fFrame()
{
}


KMenuField::LabelLayoutItem::LabelLayoutItem(BMessage* from)
	:
	KAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
}


BRect
KMenuField::LabelLayoutItem::FrameInParent() const
{
	return fFrame.OffsetByCopy(-fParent->Frame().left, -fParent->Frame().top);
}


bool
KMenuField::LabelLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
KMenuField::LabelLayoutItem::SetVisible(bool visible)
{
	// not allowed
}


BRect
KMenuField::LabelLayoutItem::Frame()
{
	return fFrame;
}


void
KMenuField::LabelLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


void
KMenuField::LabelLayoutItem::SetParent(KMenuField* parent)
{
	fParent = parent;
}


KView*
KMenuField::LabelLayoutItem::View()
{
	return fParent;
}


BSize
KMenuField::LabelLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	if (fParent->Label() == NULL)
		return BSize(-1, -1);

	return BSize(fParent->fLayoutData->label_width
			+ k_be_control_look->DefaultLabelSpacing(),
		fParent->fLayoutData->label_height);
}


BSize
KMenuField::LabelLayoutItem::BaseMaxSize()
{
	return BaseMinSize();
}


BSize
KMenuField::LabelLayoutItem::BasePreferredSize()
{
	return BaseMinSize();
}


BAlignment
KMenuField::LabelLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


status_t
KMenuField::LabelLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = KAbstractLayoutItem::Archive(into, deep);

	if (err == B_OK)
		err = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(err);
}


BArchivable*
KMenuField::LabelLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KMenuField::LabelLayoutItem"))
		return new LabelLayoutItem(from);

	return NULL;
}


// #pragma mark - KMenuField::MenuBarLayoutItem


KMenuField::MenuBarLayoutItem::MenuBarLayoutItem(KMenuField* parent)
	:
	fParent(parent),
	fFrame()
{
	// by default the part right of the divider shall have an unlimited maximum
	// width
	SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
}


KMenuField::MenuBarLayoutItem::MenuBarLayoutItem(BMessage* from)
	:
	KAbstractLayoutItem(from),
	fParent(NULL),
	fFrame()
{
	from->FindRect(kFrameField, &fFrame);
}


BRect
KMenuField::MenuBarLayoutItem::FrameInParent() const
{
	return fFrame.OffsetByCopy(-fParent->Frame().left, -fParent->Frame().top);
}


bool
KMenuField::MenuBarLayoutItem::IsVisible()
{
	return !fParent->IsHidden(fParent);
}


void
KMenuField::MenuBarLayoutItem::SetVisible(bool visible)
{
	// not allowed
}


BRect
KMenuField::MenuBarLayoutItem::Frame()
{
	return fFrame;
}


void
KMenuField::MenuBarLayoutItem::SetFrame(BRect frame)
{
	fFrame = frame;
	fParent->_UpdateFrame();
}


void
KMenuField::MenuBarLayoutItem::SetParent(KMenuField* parent)
{
	fParent = parent;
}


KView*
KMenuField::MenuBarLayoutItem::View()
{
	return fParent;
}


BSize
KMenuField::MenuBarLayoutItem::BaseMinSize()
{
	fParent->_ValidateLayoutData();

	BSize size = fParent->fLayoutData->menu_bar_min;
	size.width += 2 * k_kVMargin;
	size.height += 2 * k_kVMargin;

	return size;
}


BSize
KMenuField::MenuBarLayoutItem::BaseMaxSize()
{
	BSize size(BaseMinSize());
	size.width = B_SIZE_UNLIMITED;

	return size;
}


BSize
KMenuField::MenuBarLayoutItem::BasePreferredSize()
{
	return BaseMinSize();
}


BAlignment
KMenuField::MenuBarLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


status_t
KMenuField::MenuBarLayoutItem::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = KAbstractLayoutItem::Archive(into, deep);

	if (err == B_OK)
		err = into->AddRect(kFrameField, fFrame);

	return archiver.Finish(err);
}


BArchivable*
KMenuField::MenuBarLayoutItem::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KMenuField::MenuBarLayoutItem"))
		return new MenuBarLayoutItem(from);
	return NULL;
}


extern "C" void
B_IF_GCC_2(InvalidateLayout__10KMenuFieldb, _ZN10KMenuField16InvalidateLayoutEb)(
	KMenuField* field, bool descendants)
{
	perform_data_layout_invalidated data;
	data.descendants = descendants;

	field->Perform(PERFORM_CODE_LAYOUT_INVALIDATED, &data);
}
