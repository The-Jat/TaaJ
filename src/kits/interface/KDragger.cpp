/*
 * Copyright 2001-2012, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Rene Gollent (rene@gollent.com)
 *		Alexandre Deckner (alex@zappotek.com)
 */


//!	KDragger represents a replicant "handle".


#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <KAlert.h>
#include <Beep.h>
#include <KBitmap.h>
#include <KDragger.h>
#include <KMenuItem.h>
#include <Message.h>
#include <KPopUpMenu.h>
#include <KShelf.h>
#include <SystemCatalog.h>
#include <Khidki.h>

#include <AutoLocker.h>

#include <AppServerLink.h>
#include <KDragTrackingFilter.h>

#include<KLayout.h>//khidki
#include <binary_compatibility/Interface.h>
#include <ServerProtocol.h>
#include <ViewPrivate.h>

#include "KZombieReplicantView.h"

using BPrivate::gSystemCatalog;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Dragger"

#undef B_TRANSLATE
#define B_TRANSLATE(str) \
	gSystemCatalog.GetString(B_TRANSLATE_MARK(str), "Dragger")


static const uint32 kMsgDragStarted = 'Drgs';

static const unsigned char kHandBitmap[] = {
	255, 255,   0,   0,   0, 255, 255, 255,
	255, 255,   0, 131, 131,   0, 255, 255,
	  0,   0,   0,   0, 131, 131,   0,   0,
	  0, 131,   0,   0, 131, 131,   0,   0,
	  0, 131, 131, 131, 131, 131,   0,   0,
	255,   0, 131, 131, 131, 131,   0,   0,
	255, 255,   0,   0,   0,   0,   0,   0,
	255, 255, 255, 255, 255, 255,   0,   0
};


namespace {

struct DraggerManager {
	bool	visible;
	bool	visibleInitialized;
	BList	list;

	DraggerManager()
		:
		visible(false),
		visibleInitialized(false),
		fLock("KDragger static")
	{
	}

	bool Lock()
	{
		return fLock.Lock();
	}

	void Unlock()
	{
		fLock.Unlock();
	}

	static DraggerManager* Default()
	{
		if (sDefaultInstance == NULL)
			pthread_once(&sDefaultInitOnce, &_InitSingleton);

		return sDefaultInstance;
	}

private:
	static void _InitSingleton()
	{
		sDefaultInstance = new DraggerManager;
	}

private:
	BLocker					fLock;

	static pthread_once_t	sDefaultInitOnce;
	static DraggerManager*	sDefaultInstance;
};

pthread_once_t DraggerManager::sDefaultInitOnce = PTHREAD_ONCE_INIT;
DraggerManager* DraggerManager::sDefaultInstance = NULL;

}	// unnamed namespace


KDragger::KDragger(BRect frame, KView* target, uint32 resizingMode,
	uint32 flags)
	:
	KView(frame, "_dragger_", resizingMode, flags),
	fTarget(target),
	fRelation(TARGET_UNKNOWN),
	fShelf(NULL),
	fTransition(false),
	fIsZombie(false),
	fErrCount(0),
	fPopUpIsCustom(false),
	fPopUp(NULL)
{
	_InitData();
}


KDragger::KDragger(KView* target, uint32 flags)
	:
	KView("_dragger_", flags),
	fTarget(target),
	fRelation(TARGET_UNKNOWN),
	fShelf(NULL),
	fTransition(false),
	fIsZombie(false),
	fErrCount(0),
	fPopUpIsCustom(false),
	fPopUp(NULL)
{
	_InitData();
}


KDragger::KDragger(BMessage* data)
	:
	KView(data),
	fTarget(NULL),
	fRelation(TARGET_UNKNOWN),
	fShelf(NULL),
	fTransition(false),
	fIsZombie(false),
	fErrCount(0),
	fPopUpIsCustom(false),
	fPopUp(NULL)
{
	data->FindInt32("_rel", (int32*)&fRelation);

	_InitData();

	BMessage popupMsg;
	if (data->FindMessage("_popup", &popupMsg) == B_OK) {
		BArchivable* archivable = instantiate_object(&popupMsg);

		if (archivable) {
			fPopUp = dynamic_cast<KPopUpMenu*>(archivable);
			fPopUpIsCustom = true;
		}
	}
}


KDragger::~KDragger()
{
	delete fPopUp;
	delete fBitmap;
}


BArchivable	*
KDragger::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "KDragger"))
		return new KDragger(data);
	return NULL;
}


status_t
KDragger::Archive(BMessage* data, bool deep) const
{
	status_t ret = KView::Archive(data, deep);
	if (ret != B_OK)
		return ret;

	BMessage popupMsg;

	if (fPopUp != NULL && fPopUpIsCustom) {
		bool windowLocked = fPopUp->Window()->Lock();

		ret = fPopUp->Archive(&popupMsg, deep);

		if (windowLocked) {
			fPopUp->Window()->Unlock();
				// TODO: Investigate, in some (rare) occasions the menu window
				//		 has already been unlocked
		}

		if (ret == B_OK)
			ret = data->AddMessage("_popup", &popupMsg);
	}

	if (ret == B_OK)
		ret = data->AddInt32("_rel", fRelation);
	return ret;
}


void
KDragger::AttachedToWindow()
{
	if (fIsZombie) {
		SetLowColor(kZombieColor);
		SetViewColor(kZombieColor);
	} else {
		SetFlags(Flags() | B_TRANSPARENT_BACKGROUND);
		SetLowColor(B_TRANSPARENT_COLOR);
		SetViewColor(B_TRANSPARENT_COLOR);
	}

	_DetermineRelationship();
	_AddToList();

	AddFilter(new KDragTrackingFilter(this, kMsgDragStarted));
}


void
KDragger::DetachedFromWindow()
{
	_RemoveFromList();
}


void
KDragger::Draw(BRect update)
{
	BRect bounds(Bounds());

	if (AreDraggersDrawn() && (fShelf == NULL || fShelf->AllowsDragging())) {
		BPoint where = bounds.RightBottom() - BPoint(fBitmap->Bounds().Width(),
			fBitmap->Bounds().Height());
		SetDrawingMode(B_OP_OVER);
		DrawBitmap(fBitmap, where);
		SetDrawingMode(B_OP_COPY);

		if (fIsZombie) {
			// TODO: should draw it differently ?
		}
	}
}


void
KDragger::MouseDown(BPoint where)
{
	if (fTarget == NULL || !AreDraggersDrawn())
		return;

	uint32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);

	if (fShelf != NULL && (buttons & B_SECONDARY_MOUSE_BUTTON) != 0)
		_ShowPopUp(fTarget, where);
}


void
KDragger::MouseUp(BPoint point)
{
	KView::MouseUp(point);
}


void
KDragger::MouseMoved(BPoint point, uint32 code, const BMessage* msg)
{
	KView::MouseMoved(point, code, msg);
}


void
KDragger::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case B_TRASH_TARGET:
			if (fShelf != NULL)
				Window()->PostMessage(kDeleteReplicant, fTarget, NULL);
			else {
				KAlert* alert = new KAlert(B_TRANSLATE("Warning"),
					B_TRANSLATE("Can't delete this replicant from its original "
					"application. Life goes on."),
					B_TRANSLATE("OK"), NULL, NULL, B_WIDTH_FROM_WIDEST,
					B_WARNING_ALERT);
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go(NULL);
			}
			break;

		case _SHOW_DRAG_HANDLES_:
			// This code is used whenever the "are draggers drawn" option is
			// changed.
			if (fRelation == TARGET_IS_CHILD) {
				Invalidate(Bounds());
			} else {
				if ((fShelf != NULL && fShelf->AllowsDragging()
						&& AreDraggersDrawn())
					|| AreDraggersDrawn()) {
					Show();
				} else
					Hide();
			}
			break;

		case kMsgDragStarted:
			if (fTarget != NULL) {
				BMessage archive(B_ARCHIVED_OBJECT);

				if (fRelation == TARGET_IS_PARENT)
					fTarget->Archive(&archive);
				else if (fRelation == TARGET_IS_CHILD)
					Archive(&archive);
				else if (fTarget->Archive(&archive)) {
					BMessage archivedSelf(B_ARCHIVED_OBJECT);

					if (Archive(&archivedSelf))
						archive.AddMessage("__widget", &archivedSelf);
				}

				archive.AddInt32("be:actions", B_TRASH_TARGET);
				BPoint offset;
				drawing_mode mode;
				KBitmap* bitmap = DragBitmap(&offset, &mode);
				if (bitmap != NULL)
					DragMessage(&archive, bitmap, mode, offset, this);
				else {
					DragMessage(&archive, ConvertFromScreen(
						fTarget->ConvertToScreen(fTarget->Bounds())), this);
				}
			}
			break;

		default:
			KView::MessageReceived(msg);
			break;
	}
}


void
KDragger::FrameMoved(BPoint newPosition)
{
	KView::FrameMoved(newPosition);
}


void
KDragger::FrameResized(float newWidth, float newHeight)
{
	KView::FrameResized(newWidth, newHeight);
}


status_t
KDragger::ShowAllDraggers()
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_SHOW_ALL_DRAGGERS);
	link.Attach<bool>(true);

	status_t status = link.Flush();
	if (status == B_OK) {
		DraggerManager* manager = DraggerManager::Default();
		AutoLocker<DraggerManager> locker(manager);
		manager->visible = true;
		manager->visibleInitialized = true;
	}

	return status;
}


status_t
KDragger::HideAllDraggers()
{
	BPrivate::AppServerLink link;
	link.StartMessage(AS_SET_SHOW_ALL_DRAGGERS);
	link.Attach<bool>(false);

	status_t status = link.Flush();
	if (status == B_OK) {
		DraggerManager* manager = DraggerManager::Default();
		AutoLocker<DraggerManager> locker(manager);
		manager->visible = false;
		manager->visibleInitialized = true;
	}

	return status;
}


bool
KDragger::AreDraggersDrawn()
{
	DraggerManager* manager = DraggerManager::Default();
	AutoLocker<DraggerManager> locker(manager);

	if (!manager->visibleInitialized) {
		BPrivate::AppServerLink link;
		link.StartMessage(AS_GET_SHOW_ALL_DRAGGERS);

		status_t status;
		if (link.FlushWithReply(status) == B_OK && status == B_OK) {
			link.Read<bool>(&manager->visible);
			manager->visibleInitialized = true;
		} else
			return false;
	}

	return manager->visible;
}


BHandler*
KDragger::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier,
	int32 form, const char* property)
{
	return KView::ResolveSpecifier(message, index, specifier, form, property);
}


status_t
KDragger::GetSupportedSuites(BMessage* data)
{
	return KView::GetSupportedSuites(data);
}


status_t
KDragger::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_MIN_SIZE:
			((perform_data_min_size*)_data)->return_value
				= KDragger::MinSize();
			return B_OK;
		case PERFORM_CODE_MAX_SIZE:
			((perform_data_max_size*)_data)->return_value
				= KDragger::MaxSize();
			return B_OK;
		case PERFORM_CODE_PREFERRED_SIZE:
			((perform_data_preferred_size*)_data)->return_value
				= KDragger::PreferredSize();
			return B_OK;
		case PERFORM_CODE_LAYOUT_ALIGNMENT:
			((perform_data_layout_alignment*)_data)->return_value
				= KDragger::LayoutAlignment();
			return B_OK;
		case PERFORM_CODE_HAS_HEIGHT_FOR_WIDTH:
			((perform_data_has_height_for_width*)_data)->return_value
				= KDragger::HasHeightForWidth();
			return B_OK;
		case PERFORM_CODE_GET_HEIGHT_FOR_WIDTH:
		{
			perform_data_get_height_for_width* data
				= (perform_data_get_height_for_width*)_data;
			KDragger::GetHeightForWidth(data->width, &data->min, &data->max,
				&data->preferred);
			return B_OK;
}
		case PERFORM_CODE_SET_LAYOUT:
		{
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KDragger::SetLayout(data->layout);
			return B_OK;
		}
		case PERFORM_CODE_LAYOUT_INVALIDATED:
		{
			perform_data_layout_invalidated* data
				= (perform_data_layout_invalidated*)_data;
			KDragger::LayoutInvalidated(data->descendants);
			return B_OK;
		}
		case PERFORM_CODE_DO_LAYOUT:
		{
			KDragger::DoLayout();
			return B_OK;
		}
	}

	return KView::Perform(code, _data);
}


void
KDragger::ResizeToPreferred()
{
	KView::ResizeToPreferred();
}


void
KDragger::GetPreferredSize(float* _width, float* _height)
{
	KView::GetPreferredSize(_width, _height);
}


void
KDragger::MakeFocus(bool state)
{
	KView::MakeFocus(state);
}


void
KDragger::AllAttached()
{
	KView::AllAttached();
}


void
KDragger::AllDetached()
{
	KView::AllDetached();
}


status_t
KDragger::SetPopUp(KPopUpMenu* menu)
{
	if (menu != NULL && menu != fPopUp) {
		delete fPopUp;
		fPopUp = menu;
		fPopUpIsCustom = true;
		return B_OK;
	}
	return B_ERROR;
}


KPopUpMenu*
KDragger::PopUp() const
{
	if (fPopUp == NULL && fTarget)
		const_cast<KDragger*>(this)->_BuildDefaultPopUp();

	return fPopUp;
}


bool
KDragger::InShelf() const
{
	return fShelf != NULL;
}


KView*
KDragger::Target() const
{
	return fTarget;
}


KBitmap*
KDragger::DragBitmap(BPoint* offset, drawing_mode* mode)
{
	return NULL;
}


bool
KDragger::IsVisibilityChanging() const
{
	return fTransition;
}


void KDragger::_ReservedDragger2() {}
void KDragger::_ReservedDragger3() {}
void KDragger::_ReservedDragger4() {}


KDragger&
KDragger::operator=(const KDragger&)
{
	return *this;
}


/*static*/ void
KDragger::_UpdateShowAllDraggers(bool visible)
{
	DraggerManager* manager = DraggerManager::Default();
	AutoLocker<DraggerManager> locker(manager);

	manager->visibleInitialized = true;
	manager->visible = visible;

	for (int32 i = manager->list.CountItems(); i-- > 0;) {
		KDragger* dragger = (KDragger*)manager->list.ItemAt(i);
		BMessenger target(dragger);
		target.SendMessage(_SHOW_DRAG_HANDLES_);
	}
}


void
KDragger::_InitData()
{
	fBitmap = new KBitmap(BRect(0.0f, 0.0f, 7.0f, 7.0f), B_CMAP8, false, false);
	fBitmap->SetBits(kHandBitmap, fBitmap->BitsLength(), 0, B_CMAP8);
}


void
KDragger::_AddToList()
{
	DraggerManager* manager = DraggerManager::Default();
	AutoLocker<DraggerManager> locker(manager);
	manager->list.AddItem(this);

	bool allowsDragging = true;
	if (fShelf)
		allowsDragging = fShelf->AllowsDragging();

	if (!AreDraggersDrawn() || !allowsDragging) {
		// The dragger is not shown - but we can't hide us in case we're the
		// parent of the actual target view (because then you couldn't see
		// it anymore).
		if (fRelation != TARGET_IS_CHILD && !IsHidden(this))
			Hide();
	}
}


void
KDragger::_RemoveFromList()
{
	DraggerManager* manager = DraggerManager::Default();
	AutoLocker<DraggerManager> locker(manager);
	manager->list.RemoveItem(this);
}


status_t
KDragger::_DetermineRelationship()
{
	if (fTarget != NULL) {
		if (fTarget == Parent())
			fRelation = TARGET_IS_PARENT;
		else if (fTarget == ChildAt(0))
			fRelation = TARGET_IS_CHILD;
		else
			fRelation = TARGET_IS_SIBLING;
	} else {
		if (fRelation == TARGET_IS_PARENT)
			fTarget = Parent();
		else if (fRelation == TARGET_IS_CHILD)
			fTarget = ChildAt(0);
		else
			return B_ERROR;
	}

	if (fRelation == TARGET_IS_PARENT) {
		BRect bounds(Frame());
		BRect parentBounds(Parent()->Bounds());
		if (!parentBounds.Contains(bounds)) {
			MoveTo(parentBounds.right - bounds.Width(),
				parentBounds.bottom - bounds.Height());
		}
	}

	return B_OK;
}


status_t
KDragger::_SetViewToDrag(KView* target)
{
	if (target->Window() != Window())
		return B_ERROR;

	fTarget = target;

	if (Window() != NULL)
		_DetermineRelationship();

	return B_OK;
}


void
KDragger::_SetShelf(KShelf* shelf)
{
	fShelf = shelf;
}


void
KDragger::_SetZombied(bool state)
{
	fIsZombie = state;

	if (state) {
		SetLowColor(kZombieColor);
		SetViewColor(kZombieColor);
	}
}


void
KDragger::_BuildDefaultPopUp()
{
	fPopUp = new KPopUpMenu("Shelf", false, false, K_ITEMS_IN_COLUMN);

	// About
	BMessage* msg = new BMessage(B_ABOUT_REQUESTED);

	const char* name = fTarget->Name();
	if (name != NULL)
		msg->AddString("target", name);

	BString about(B_TRANSLATE("About %app" B_UTF8_ELLIPSIS));
	about.ReplaceFirst("%app", name);

	fPopUp->AddItem(new KMenuItem(about.String(), msg));
	fPopUp->AddSeparatorItem();
	fPopUp->AddItem(new KMenuItem(B_TRANSLATE("Remove replicant"),
		new BMessage(kDeleteReplicant)));
}


void
KDragger::_ShowPopUp(KView* target, BPoint where)
{
	BPoint point = ConvertToScreen(where);

	if (fPopUp == NULL && fTarget != NULL)
		_BuildDefaultPopUp();

	fPopUp->SetTargetForItems(fTarget);

	float menuWidth, menuHeight;
	fPopUp->GetPreferredSize(&menuWidth, &menuHeight);
	BRect rect(0, 0, menuWidth, menuHeight);
	rect.InsetBy(-0.5, -0.5);
	rect.OffsetTo(point);

	fPopUp->Go(point, true, false, rect, true);
}


#if __GNUC__ < 3

extern "C" KBitmap*
_ReservedDragger1__8BDragger(KDragger* dragger, BPoint* offset,
	drawing_mode* mode)
{
	return dragger->KDragger::DragBitmap(offset, mode);
}

#endif
