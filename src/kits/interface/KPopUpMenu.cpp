/*
 * Copyright 2001-2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */


#include <Application.h>
#include <Looper.h>
#include <KMenuItem.h>
#include <KPopUpMenu.h>
#include <Khidki.h>

#include <new>

#include<KLayout.h>//khidki
#include <binary_compatibility/Interface.h>


struct k_popup_menu_data {
	KPopUpMenu* object;
	KWindow* window;
	KMenuItem* selected;

	BPoint where;
	BRect rect;

	bool async;
	bool autoInvoke;
	bool startOpened;
	bool useRect;

	sem_id lock;
};


KPopUpMenu::KPopUpMenu(const char* name, bool radioMode, bool labelFromMarked,
	k_menu_layout layout)
	:
	KMenu(name, layout),
	fUseWhere(false),
	fAutoDestruct(false),
	fTrackThread(-1)
{
	if (radioMode)
		SetRadioMode(true);

	if (labelFromMarked)
		SetLabelFromMarked(true);
}


KPopUpMenu::KPopUpMenu(BMessage* archive)
	:
	KMenu(archive),
	fUseWhere(false),
	fAutoDestruct(false),
	fTrackThread(-1)
{
}


KPopUpMenu::~KPopUpMenu()
{
	if (fTrackThread >= 0) {
		status_t status;
		while (wait_for_thread(fTrackThread, &status) == B_INTERRUPTED)
			;
	}
}


status_t
KPopUpMenu::Archive(BMessage* data, bool deep) const
{
	return KMenu::Archive(data, deep);
}


BArchivable*
KPopUpMenu::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "KPopUpMenu"))
		return new KPopUpMenu(data);

	return NULL;
}


KMenuItem*
KPopUpMenu::Go(BPoint where, bool deliversMessage, bool openAnyway, bool async)
{
	return _Go(where, deliversMessage, openAnyway, NULL, async);
}


KMenuItem*
KPopUpMenu::Go(BPoint where, bool deliversMessage, bool openAnyway,
	BRect clickToOpen, bool async)
{
	return _Go(where, deliversMessage, openAnyway, &clickToOpen, async);
}


void
KPopUpMenu::MessageReceived(BMessage* message)
{
	KMenu::MessageReceived(message);
}


void
KPopUpMenu::MouseDown(BPoint point)
{
	KView::MouseDown(point);
}


void
KPopUpMenu::MouseUp(BPoint point)
{
	KView::MouseUp(point);
}


void
KPopUpMenu::MouseMoved(BPoint point, uint32 code, const BMessage* message)
{
	KView::MouseMoved(point, code, message);
}


void
KPopUpMenu::AttachedToWindow()
{
	KMenu::AttachedToWindow();
}


void
KPopUpMenu::DetachedFromWindow()
{
	KMenu::DetachedFromWindow();
}


void
KPopUpMenu::FrameMoved(BPoint newPosition)
{
	KMenu::FrameMoved(newPosition);
}


void
KPopUpMenu::FrameResized(float newWidth, float newHeight)
{
	KMenu::FrameResized(newWidth, newHeight);
}


BHandler*
KPopUpMenu::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	return KMenu::ResolveSpecifier(message, index, specifier, form, property);
}


status_t
KPopUpMenu::GetSupportedSuites(BMessage* data)
{
	return KMenu::GetSupportedSuites(data);
}


status_t
KPopUpMenu::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= KPopUpMenu::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= KPopUpMenu::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= KPopUpMenu::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= KPopUpMenu::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= KPopUpMenu::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			KPopUpMenu::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
		}
		case PERFORM_CODE_SET_LAYOUT:
		{
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KPopUpMenu::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			KPopUpMenu::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			KPopUpMenu::DoLayout();
			return B_OK;
		}
	}

	return KMenu::Perform(code, _data);
}


void
KPopUpMenu::ResizeToPreferred()
{
	KMenu::ResizeToPreferred();
}


void
KPopUpMenu::GetPreferredSize(float* _width, float* _height)
{
	KMenu::GetPreferredSize(_width, _height);
}


void
KPopUpMenu::MakeFocus(bool state)
{
	KMenu::MakeFocus(state);
}


void
KPopUpMenu::AllAttached()
{
	KMenu::AllAttached();
}


void
KPopUpMenu::AllDetached()
{
	KMenu::AllDetached();
}


void
KPopUpMenu::SetAsyncAutoDestruct(bool on)
{
	fAutoDestruct = on;
}


bool
KPopUpMenu::AsyncAutoDestruct() const
{
	return fAutoDestruct;
}


BPoint
KPopUpMenu::ScreenLocation()
{
	// This case is when the KPopUpMenu is standalone
	if (fUseWhere)
		return fWhere;

	// This case is when the KPopUpMenu is inside a BMenuField
	KMenuItem* superItem = Superitem();
	KMenu* superMenu = Supermenu();
	KMenuItem* selectedItem = FindItem(superItem->Label());
	BPoint point = superItem->Frame().LeftTop();

	superMenu->ConvertToScreen(&point);

	if (selectedItem != NULL) {
		while (selectedItem->Menu() != this
			&& selectedItem->Menu()->Superitem() != NULL) {
			selectedItem = selectedItem->Menu()->Superitem();
		}
		point.y -= selectedItem->Frame().top;
	}

	return point;
}


//	#pragma mark - private methods


void KPopUpMenu::_ReservedPopUpMenu1() {}
void KPopUpMenu::_ReservedPopUpMenu2() {}
void KPopUpMenu::_ReservedPopUpMenu3() {}


KPopUpMenu&
KPopUpMenu::operator=(const KPopUpMenu& other)
{
	return *this;
}


KMenuItem*
KPopUpMenu::_Go(BPoint where, bool autoInvoke, bool startOpened,
	BRect* _specialRect, bool async)
{
	if (fTrackThread >= B_OK) {
		// we already have an active menu, wait for it to go away before
		// spawning another
		status_t unused;
		while (wait_for_thread(fTrackThread, &unused) == B_INTERRUPTED)
			;
	}

	k_popup_menu_data* data = new (std::nothrow) k_popup_menu_data;
	if (data == NULL)
		return NULL;

	sem_id sem = create_sem(0, "window close lock");
	if (sem < B_OK) {
		delete data;
		return NULL;
	}

	// Get a pointer to the window from which Go() was called
	KWindow* window
		= dynamic_cast<KWindow*>(BLooper::LooperForThread(find_thread(NULL)));
	data->window = window;

	// Asynchronous menu: we set the KWindow menu's semaphore
	// and let KWindow block when needed
	if (async && window != NULL)
		k_set_menu_sem_(window, sem);

	data->object = this;
	data->autoInvoke = autoInvoke;
	data->useRect = _specialRect != NULL;
	if (_specialRect != NULL)
		data->rect = *_specialRect;
	data->async = async;
	data->where = where;
	data->startOpened = startOpened;
	data->selected = NULL;
	data->lock = sem;

	// Spawn the tracking thread
	fTrackThread = spawn_thread(_thread_entry, "popup", B_DISPLAY_PRIORITY,
		data);
	if (fTrackThread < B_OK) {
		// Something went wrong. Cleanup and return NULL
		delete_sem(sem);
		if (async && window != NULL)
			k_set_menu_sem_(window, B_BAD_SEM_ID);
		delete data;
		return NULL;
	}

	resume_thread(fTrackThread);

	if (!async)
		return _WaitMenu(data);

	return 0;
}


/* static */
int32
KPopUpMenu::_thread_entry(void* menuData)
{
	k_popup_menu_data* data = static_cast<k_popup_menu_data*>(menuData);
	KPopUpMenu* menu = data->object;
	BRect* rect = NULL;

	if (data->useRect)
		rect = &data->rect;

	data->selected = menu->_StartTrack(data->where, data->autoInvoke,
		data->startOpened, rect);

	// Reset the window menu semaphore
	if (data->async && data->window)
		k_set_menu_sem_(data->window, B_BAD_SEM_ID);

	delete_sem(data->lock);

	// Commit suicide if needed
	if (data->async && menu->fAutoDestruct) {
		menu->fTrackThread = -1;
		delete menu;
	}

	if (data->async)
		delete data;

	return 0;
}


KMenuItem*
KPopUpMenu::_StartTrack(BPoint where, bool autoInvoke, bool startOpened,
	BRect* _specialRect)
{
	// I know, this doesn't look senseful, but don't be fooled,
	// fUseWhere is used in ScreenLocation(), which is a virtual
	// called by KMenu::Track()
	fWhere = where;
	fUseWhere = true;

	// Determine when mouse-down-up will be taken as a 'press',
	// rather than a 'click'
	bigtime_t clickMaxTime = 0;
	get_click_speed(&clickMaxTime);
	clickMaxTime += system_time();

	// Show the menu's window
	Show();
	snooze(50000);
	KMenuItem* result = Track(startOpened, _specialRect);

	// If it was a click, keep the menu open and tracking
	if (system_time() <= clickMaxTime)
		result = Track(true, _specialRect);
	if (result != NULL && autoInvoke)
		result->Invoke();

	fUseWhere = false;

	Hide();
	be_app->ShowCursor();

	return result;
}


KMenuItem*
KPopUpMenu::_WaitMenu(void* _data)
{
	k_popup_menu_data* data = static_cast<k_popup_menu_data*>(_data);
	KWindow* window = data->window;
	sem_id sem = data->lock;
	if (window != NULL) {
		while (acquire_sem_etc(sem, 1, B_TIMEOUT, 50000) != B_BAD_SEM_ID)
			window->UpdateIfNeeded();
	}

 	status_t unused;
	while (wait_for_thread(fTrackThread, &unused) == B_INTERRUPTED)
		;

	fTrackThread = -1;

	KMenuItem* selected = data->selected;
		// data->selected is filled by the tracking thread

	delete data;

	return selected;
}
