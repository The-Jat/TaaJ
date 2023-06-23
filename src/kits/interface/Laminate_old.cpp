
#include <Laminate.h>

#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <Cursor.h>
#include <File.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>
#include <InterfaceDefs.h>
#include <Layout.h>
#include <LayoutContext.h>
#include <LayoutUtils.h>
#include <MenuBar.h>
#include <Message.h>
#include <MessageQueue.h>
#include <ObjectList.h>
#include <Picture.h>
#include <Point.h>
#include <Polygon.h>
#include <PropertyInfo.h>
#include <Region.h>
#include <ScrollBar.h>
#include <Shape.h>
#include <Shelf.h>
#include <String.h>
//#include <Window.h>
#include <Khidki.h>

#include <AppMisc.h>
#include <AppServerLink.h>
#include <binary_compatibility/Interface.h>
#include <binary_compatibility/Support.h>
#include <MessagePrivate.h>
#include <MessageUtils.h>
#include <PortLink.h>
#include <ServerProtocol.h>
#include <ServerProtocolStructs.h>
#include <ShapePrivate.h>
#include <ToolTip.h>
#include <ToolTipManager.h>
#include <TokenSpace.h>
#include <ViewPrivate.h>


//khidki code
//start
//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TTRACE(x) debug_printf x
#else
#	define TTRACE(x) ;
#endif
//end

// archiving constants
namespace {
	const char* const k_kSizesField = "KView:sizes";
		// kSizesField = {min, max, pref}
	const char* const k_kAlignmentField = "KView:alignment";
	const char* const k_kLayoutField = "KView:layout";
}

struct KView::LayoutData {
	LayoutData()
		:
		fMinSize(),
		fMaxSize(),
		fPreferredSize(),
		fAlignment(),
		fLayoutInvalidationDisabled(0),
		fLayout(NULL),
		fLayoutContext(NULL),
		fLayoutItems(5, false),
		fLayoutValid(true),		// TODO: Rethink these initial values!
		fMinMaxValid(true),		//
		fLayoutInProgress(false),
		fNeedsRelayout(true)
	{
	}

	status_t
	AddDataToArchive(BMessage* archive)
	{
		status_t err = archive->AddSize(k_kSizesField, fMinSize);

		if (err == B_OK)
			err = archive->AddSize(k_kSizesField, fMaxSize);

		if (err == B_OK)
			err = archive->AddSize(k_kSizesField, fPreferredSize);

		if (err == B_OK)
			err = archive->AddAlignment(k_kAlignmentField, fAlignment);

		return err;
	}

	void
	PopulateFromArchive(BMessage* archive)
	{
		archive->FindSize(k_kSizesField, 0, &fMinSize);
		archive->FindSize(k_kSizesField, 1, &fMaxSize);
		archive->FindSize(k_kSizesField, 2, &fPreferredSize);
		archive->FindAlignment(k_kAlignmentField, &fAlignment);
	}

	BSize			fMinSize;
	BSize			fMaxSize;
	BSize			fPreferredSize;
	BAlignment		fAlignment;
	int				fLayoutInvalidationDisabled;
	BLayout*		fLayout;
	BLayoutContext*	fLayoutContext;
	BObjectList<BLayoutItem> fLayoutItems;
	bool			fLayoutValid;
	bool			fMinMaxValid;
	bool			fLayoutInProgress;
	bool			fNeedsRelayout;
};

KView::KView(BRect frame, const char* name, uint32 resizingMode, uint32 flags)
	:
	BHandler(name)
{
debug_printf("[KView]{KView constructor} Into KView Constructor...\n");
	_InitData(frame, name, resizingMode, flags);
}


KWindow*
KView::Window() const
{
	return fOwner;
}


void
KView::Draw(BRect updateRect)
{
	// Hook function
	//STRACE(("\tHOOK: BView(%s)::Draw()\n", Name()));
}


void
KView::DrawAfterChildren(BRect updateRect)
{
	// Hook function
	//STRACE(("\tHOOK: BView(%s)::DrawAfterChildren()\n", Name()));
}

void
KView::ConvertFromScreen(BPoint* point) const
{
	_ConvertFromScreen(point, true);
}


BPoint
KView::ConvertFromScreen(BPoint point) const
{
	ConvertFromScreen(&point);

	return point;
}


void
KView::ConvertFromScreen(BRect* rect) const
{
	BPoint offset(0.0, 0.0);
	ConvertFromScreen(&offset);
	rect->OffsetBy(offset);
}


BRect
KView::ConvertFromScreen(BRect rect) const
{
	ConvertFromScreen(&rect);

	return rect;
}


BRect
KView::Bounds() const
{
debug_printf("[KView]{Bounds} Into Bounds...\n");
	_CheckLock();

	if (fIsPrinting)
		return fState->print_rect;

	return fBounds;
}


BRect
KView::Frame() const
{
debug_printf("[KView]{Frame} Into Frame...\n");
	return Bounds().OffsetToCopy(fParentOffset.x, fParentOffset.y);
}


BPoint
KView::LeftTop() const
{
debug_printf("[KView]{LeftTop} Into LeftTop...\n");
	return Bounds().LeftTop();
}


void
KView::WindowActivated(bool active)
{
	// Hook function
	//STRACE(("\tHOOK: KView(%s)::WindowActivated()\n", Name()));
}


void
KView::SetViewUIColor(color_which which, float tint)
{
debug_printf("[KView]{SetViewUIColor} Into SetViewUIColor...\n");
	if (fState->IsValid(B_VIEW_WHICH_VIEW_COLOR_BIT)
		&& fState->which_view_color == which
		&& fState->which_view_color_tint == tint)
		return;

	if (fOwner != NULL) {
	debug_printf("[KView]{SetViewUIColor} fOwner != NULL...\n");
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_VIEW_UI_COLOR);
		fOwner->fLink->Attach<color_which>(which);
		fOwner->fLink->Attach<float>(tint);

		fState->valid_flags |= B_VIEW_WHICH_VIEW_COLOR_BIT;
	}

	fState->which_view_color = which;
	fState->which_view_color_tint = tint;

	if (which != B_NO_COLOR) {
		fState->archiving_flags |= B_VIEW_WHICH_VIEW_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_VIEW_COLOR_BIT;
		fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;

		fState->view_color = tint_color(ui_color(which), tint);
	} else {
		fState->valid_flags &= ~B_VIEW_VIEW_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_WHICH_VIEW_COLOR_BIT;
	}

	if (!fState->IsValid(B_VIEW_WHICH_LOW_COLOR_BIT))
		SetLowUIColor(which, tint);
}


color_which
KView::ViewUIColor(float* tint) const
{
debug_printf("[KView]{ViewUIColor} Into ViewUIColor...\n");
	if (!fState->IsValid(B_VIEW_WHICH_VIEW_COLOR_BIT)
		&& fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_GET_VIEW_UI_COLOR);

		int32 code;
		if (fOwner->fLink->FlushWithReply(code) == B_OK
			&& code == B_OK) {
			fOwner->fLink->Read<color_which>(&fState->which_view_color);
			fOwner->fLink->Read<float>(&fState->which_view_color_tint);
			fOwner->fLink->Read<rgb_color>(&fState->view_color);

			fState->valid_flags |= B_VIEW_WHICH_VIEW_COLOR_BIT;
			fState->valid_flags |= B_VIEW_VIEW_COLOR_BIT;
		}
	}

	if (tint != NULL)
		*tint = fState->which_view_color_tint;

	return fState->which_view_color;
}


void
KView::SetHighUIColor(color_which which, float tint)
{
debug_printf("[KView]{SetHighUIColor} Into SetHighUIColor...\n");
	if (fState->IsValid(B_VIEW_WHICH_HIGH_COLOR_BIT)
		&& fState->which_high_color == which
		&& fState->which_high_color_tint == tint)
		return;

	if (fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_HIGH_UI_COLOR);
		fOwner->fLink->Attach<color_which>(which);
		fOwner->fLink->Attach<float>(tint);

		fState->valid_flags |= B_VIEW_WHICH_HIGH_COLOR_BIT;
	}

	fState->which_high_color = which;
	fState->which_high_color_tint = tint;

	if (which != B_NO_COLOR) {
		fState->archiving_flags |= B_VIEW_WHICH_HIGH_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_HIGH_COLOR_BIT;
		fState->valid_flags |= B_VIEW_HIGH_COLOR_BIT;

		fState->high_color = tint_color(ui_color(which), tint);
	} else {
		fState->valid_flags &= ~B_VIEW_HIGH_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_WHICH_HIGH_COLOR_BIT;
	}
}



void
KView::SetLowUIColor(color_which which, float tint)
{
debug_printf("[KView]{SetLowUIColor} Into SetLowUIColor...\n");
	if (fState->IsValid(B_VIEW_WHICH_LOW_COLOR_BIT)
		&& fState->which_low_color == which
		&& fState->which_low_color_tint == tint)
		return;

	if (fOwner != NULL) {
		_CheckLockAndSwitchCurrent();

		fOwner->fLink->StartMessage(AS_VIEW_SET_LOW_UI_COLOR);
		fOwner->fLink->Attach<color_which>(which);
		fOwner->fLink->Attach<float>(tint);

		fState->valid_flags |= B_VIEW_WHICH_LOW_COLOR_BIT;
	}

	fState->which_low_color = which;
	fState->which_low_color_tint = tint;

	if (which != B_NO_COLOR) {
		fState->archiving_flags |= B_VIEW_WHICH_LOW_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_LOW_COLOR_BIT;
		fState->valid_flags |= B_VIEW_LOW_COLOR_BIT;

		fState->low_color = tint_color(ui_color(which), tint);
	} else {
		fState->valid_flags &= ~B_VIEW_LOW_COLOR_BIT;
		fState->archiving_flags &= ~B_VIEW_WHICH_LOW_COLOR_BIT;
	}
}


void
KView::PushState()
{
	_CheckOwnerLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_PUSH_STATE);

	// initialize origin, scale and transform, new states start "clean".
	fState->valid_flags |= B_VIEW_SCALE_BIT | B_VIEW_ORIGIN_BIT
		| B_VIEW_TRANSFORM_BIT;
	fState->scale = 1.0f;
	fState->origin.Set(0, 0);
	fState->transform.Reset();
}


void
KView::PopState()
{
	_CheckOwnerLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_POP_STATE);
	_FlushIfNotInTransaction();

	// invalidate all flags (except those that are not part of pop/push)
	fState->valid_flags = B_VIEW_VIEW_COLOR_BIT;
}



void
KView::Invalidate(BRect invalRect)
{
debug_printf("[KView]{Invalidate} \n");

	if (fOwner == NULL)
		return;

	// NOTE: This rounding of the invalid rect is to stay compatible with BeOS.
	// On the server side, the invalid rect will be converted to a BRegion,
	// which rounds in a different manner, so that it really includes the
	// fractional coordinates of a BRect (ie ceilf(rect.right) &
	// ceilf(rect.bottom)), which is also what BeOS does. So we have to do the
	// different rounding here to stay compatible in both ways.
	invalRect.left = (int)invalRect.left;
	invalRect.top = (int)invalRect.top;
	invalRect.right = (int)invalRect.right;
	invalRect.bottom = (int)invalRect.bottom;
	if (!invalRect.IsValid())
		return;

	_CheckLockAndSwitchCurrent();

	fOwner->fLink->StartMessage(AS_VIEW_INVALIDATE_RECT);
	fOwner->fLink->Attach<BRect>(invalRect);

// TODO: determine why this check isn't working correctly.
#if 0
	if (!fOwner->fUpdateRequested) {
		fOwner->fLink->Flush();
		fOwner->fUpdateRequested = true;
	}
#else
	fOwner->fLink->Flush();
#endif
}


void
KView::Invalidate()
{
	Invalidate(Bounds());
}


uint32
KView::Flags() const
{
debug_printf("[KView]{Flags} Into Flags...\n");
	_CheckLock();
	return fFlags & ~_RESIZE_MASK_;
}


uint32
KView::ResizingMode() const
{
debug_printf("[KView]{ResizingMode} Into ResizingMode...\n");
	return fFlags & _RESIZE_MASK_;
}



void
KView::MakeFocus(bool focus)
{
debug_printf("[KView]{MakeFocus} Into MakeFocus...\n");
	if (fOwner == NULL)
		return;

	// TODO: If this view has focus and focus == false,
	// will there really be no other view with focus? No
	// cycling to the next one?
	KView* focusView = fOwner->CurrentFocus();
	if (focus) {
		// Unfocus a previous focus view
		if (focusView != NULL && focusView != this)
			focusView->MakeFocus(false);

		// if we want to make this view the current focus view
		fOwner->_SetFocus(this, true);
	} else {
		// we want to unfocus this view, but only if it actually has focus
		if (focusView == this)
			fOwner->_SetFocus(NULL, true);
	}
}



bool
KView::IsHidden(const KView* lookingFrom) const
{
debug_printf("[KView]{IsHidden} Into IsHidden...\n");

	if (fShowLevel > 0)
		return true;

	// may we be egocentric?
	if (lookingFrom == this)
		return false;

	// we have the same visibility state as our
	// parent, if there is one
	if (fParent)
		return fParent->IsHidden(lookingFrom);

	// if we're the top view, and we're interested
	// in the "global" view, we're inheriting the
	// state of the window's visibility
	if (fOwner && lookingFrom == NULL)
		return fOwner->IsHidden();

	return false;
}


bool
KView::IsHidden() const
{
debug_printf("[KView]{IsHidden()} Into IsHidden()...\n");
	return IsHidden(NULL);
}


void
KView::Flush() const
{
	if (fOwner)
		fOwner->Flush();
}


BLayoutContext*
KView::LayoutContext() const
{
	return fLayoutData->fLayoutContext;
}


void
KView::Layout(bool force)
{
	BLayoutContext context;
	_Layout(force, &context);
}


void
KView::Relayout()
{
	if (fLayoutData->fLayoutValid && !fLayoutData->fLayoutInProgress) {
		fLayoutData->fNeedsRelayout = true;
		if (fLayoutData->fLayout)
			fLayoutData->fLayout->RequireLayout();

		// Layout() is recursive, that is if the parent view is currently laid
		// out, we don't call layout() on this view, but wait for the parent's
		// Layout() to do that for us.
		if (!fParent || !fParent->fLayoutData->fLayoutInProgress)
			Layout(false);
	}
}


void
KView::DoLayout()
{
	if (fLayoutData->fLayout)
		fLayoutData->fLayout->_LayoutWithinContext(false, LayoutContext());
}


void
KView::LayoutChanged()
{
	// hook method
}


void
KView::_Layout(bool force, BLayoutContext* context)
{
debug_printf("[KView]{_Layout}\n");

//printf("%p->BView::_Layout(%d, %p)\n", this, force, context);
//printf("  fNeedsRelayout: %d, fLayoutValid: %d, fLayoutInProgress: %d\n",
//fLayoutData->fNeedsRelayout, fLayoutData->fLayoutValid,
//fLayoutData->fLayoutInProgress);
	if (fLayoutData->fNeedsRelayout || !fLayoutData->fLayoutValid || force) {
		fLayoutData->fLayoutValid = false;

		if (fLayoutData->fLayoutInProgress)
			return;

		BLayoutContext* oldContext = fLayoutData->fLayoutContext;
		fLayoutData->fLayoutContext = context;

		fLayoutData->fLayoutInProgress = true;
		DoLayout();
		fLayoutData->fLayoutInProgress = false;

		fLayoutData->fLayoutValid = true;
		fLayoutData->fMinMaxValid = true;
		fLayoutData->fNeedsRelayout = false;

		// layout children
		for(KView* child = fFirstChild; child; child = child->fNextSibling) {
			if (!child->IsHidden(child))
				child->_Layout(force, context);
		}

		LayoutChanged();

		fLayoutData->fLayoutContext = oldContext;

		// invalidate the drawn content, if requested
		if (fFlags & B_INVALIDATE_AFTER_LAYOUT)
			Invalidate();
	}
}



void
KView::_InitData(BRect frame, const char* name, uint32 resizingMode,
	uint32 flags)
{
debug_printf("[KView]{_InitData} Into _InitData...\n");
	// Info: The name of the view is set by BHandler constructor

	//STRACE(("BView::_InitData: enter\n"));

	// initialize members
	if ((resizingMode & ~_RESIZE_MASK_) || (flags & _RESIZE_MASK_))
		debug_printf("%s BView::_InitData(): resizing mode or flags swapped\n", name);

	// There are applications that swap the resize mask and the flags in the
	// BView constructor. This does not cause problems under BeOS as it just
	// ors the two fields to one 32bit flag.
	// For now we do the same but print the above warning message.
	// TODO: this should be removed at some point and the original
	// version restored:
	fFlags = (resizingMode & _RESIZE_MASK_) | (flags & ~_RESIZE_MASK_);
	fFlags = resizingMode | flags;
debug_printf("[KView]{_InitData} fFlags =%d...\n",fFlags);

debug_printf("[KView]{_InitData} before roundf frame.left =%f...\n",frame.left);
debug_printf("[KView]{_InitData} before roundf frame.top =%f...\n",frame.top);
debug_printf("[KView]{_InitData} before roundf frame.right =%f...\n",frame.right);
debug_printf("[KView]{_InitData} before roundf frame.bottom =%f...\n",frame.bottom);

	// handle rounding
	frame.left = roundf(frame.left);
	frame.top = roundf(frame.top);
	frame.right = roundf(frame.right);
	frame.bottom = roundf(frame.bottom);

debug_printf("[KView]{_InitData} after roundf frame.left =%f...\n",frame.left);
debug_printf("[KView]{_InitData} after roundf frame.top =%f...\n",frame.top);
debug_printf("[KView]{_InitData} after roundf frame.right =%f...\n",frame.right);
debug_printf("[KView]{_InitData} after roundf frame.bottom =%f...\n",frame.bottom);


	fParentOffset.Set(frame.left, frame.top);

	fOwner = NULL;
	fParent = NULL;
	fNextSibling = NULL;
	fPreviousSibling = NULL;
	fFirstChild = NULL;

	fShowLevel = 0;
	fTopLevelView = false;

	fCurrentPicture = NULL;
	fCommArray = NULL;

	fVerScroller = NULL;
	fHorScroller = NULL;

	fIsPrinting = false;
	fAttached = false;

	// TODO: Since we cannot communicate failure, we don't use std::nothrow here
	// TODO: Maybe we could auto-delete those views on AddChild() instead?
	fState = new BPrivate::ViewState;

	fBounds = frame.OffsetToCopy(B_ORIGIN);
	fShelf = NULL;

	fEventMask = 0;
	fEventOptions = 0;
	fMouseEventOptions = 0;

	fLayoutData = new LayoutData;

	fToolTip = NULL;

	if ((flags & B_SUPPORTS_LAYOUT) != 0) {
	
	debug_printf("[KView]{_InitData} (flags & B_SUPPORT_LAYOUT) != 0 ...\n");
		SetViewUIColor(B_WINDOW_TAB_COLOR);//B_PANEL_BACKGROUND_COLOR);
		SetLowUIColor(ViewUIColor());
		SetHighUIColor(B_PANEL_TEXT_COLOR);
	}
}


void
KView::_RemoveCommArray()
{
debug_printf("[KView]{_RemoveCommArray} Into _RemoveCommArray...\n");
	if (fCommArray) {
		delete [] fCommArray->array;
		delete fCommArray;
		fCommArray = NULL;
	}
}


bool
KView::_CheckOwnerLock() const
{
	if (fOwner) {
		fOwner->check_lock();
		return true;
	} else {
		debugger("View method requires owner and doesn't have one.");
		return false;
	}
}


void
KView::_SetOwner(KWindow* newOwner)
{
debug_printf("[KView]{_SetOwner} Into SetOwner...\n");
	if (!newOwner)
		_RemoveCommArray();

	if (fOwner != newOwner && fOwner) {
		if (fOwner->fFocus == this)
			MakeFocus(false);

		if (fOwner->fLastMouseMovedView == this)
			fOwner->fLastMouseMovedView = NULL;

		fOwner->RemoveHandler(this);
		if (fShelf)
			fOwner->RemoveHandler(fShelf);
	}

	if (newOwner && newOwner != fOwner) {
		newOwner->AddHandler(this);
		if (fShelf)
			newOwner->AddHandler(fShelf);

		if (fTopLevelView)
			SetNextHandler(newOwner);
		else
			SetNextHandler(fParent);
	}

	fOwner = newOwner;

	for (KView* child = fFirstChild; child != NULL; child = child->fNextSibling)
		child->_SetOwner(newOwner);
}


bool
KView::_CheckOwnerLockAndSwitchCurrent() const
{
debug_printf("[KView]{_CheckOwnerLockAndSwitchCurrent} Into _CheckOwnerLockAndSwitchCurrent...\n");
	//STRACE(("BView(%s)::_CheckOwnerLockAndSwitchCurrent()\n", Name()));

	if (fOwner == NULL) {
	debug_printf("[KView]{_CheckOwnerLockAndSwitchCurrent} fOwner == NULL...\n");
		debugger("View method requires owner and doesn't have one.");
		return false;
	}

	_CheckLockAndSwitchCurrent();

	return true;
}



void
KView::_CheckLock() const
{debug_printf("[KView]{_CheckLock} Into _CheckLock...\n");
	if (fOwner)
		fOwner->check_lock();
}



void
KView::_CheckLockAndSwitchCurrent() const
{
debug_printf("[KView]{_CheckLockAndSwitchCurrent} Into _CheckLockAndSwitchCurrent...\n");
	//STRACE(("BView(%s)::_CheckLockAndSwitchCurrent()\n", Name()));

	if (!fOwner)
		return;

	fOwner->check_lock();

	_SwitchServerCurrentView();
}


void
KView::_SwitchServerCurrentView() const
{
debug_printf("[KView]{_SwitchServerCurrentView} Into _SwitchServerCurrentView...\n");
	int32 serverToken = _get_object_token_(this);

	if (fOwner->fLastViewToken != serverToken) {
	debug_printf("[KView]{_SwitchServerCurrentView} fOwner->fLastViewToken != serverToken...\n");
		//STRACE(("contacting app_server... sending token: %" B_PRId32 "\n",
		//	serverToken));
		fOwner->fLink->StartMessage(AS_SET_CURRENT_VIEW_2);
		fOwner->fLink->Attach<int32>(serverToken);

		fOwner->fLastViewToken = serverToken;
	}
}



void
KView::_FlushIfNotInTransaction()
{
	if (!fOwner->fInTransaction) {
		fOwner->Flush();
	}
}


bool
KView::_CreateSelf()
{
debug_printf("[KView]{_CreateSelf} Into _CreateSelf...\n");
	// AS_VIEW_CREATE & AS_VIEW_CREATE_ROOT do not use the
	// current view mechanism via _CheckLockAndSwitchCurrent() - the token
	// of the view and its parent are both send to the server.

	if (fTopLevelView)
	{
		fOwner->fLink->StartMessage(AS_CREATE_LAMINATE_JADD);
		debug_printf("[KView]{_CreateSelf} AS_CREATE_LAMINATE_JADD...\n");
	}
	else
 	{
 		fOwner->fLink->StartMessage(AS_VIEW_CREATE);
		debug_printf("[KView]{_CreateSelf} AS_VIEW_CREATE...\n");
	}

	fOwner->fLink->Attach<int32>(_get_object_token_(this));
	fOwner->fLink->AttachString(Name());
	fOwner->fLink->Attach<BRect>(Frame());
	fOwner->fLink->Attach<BPoint>(LeftTop());
	fOwner->fLink->Attach<uint32>(ResizingMode());
	fOwner->fLink->Attach<uint32>(fEventMask);
	fOwner->fLink->Attach<uint32>(fEventOptions);
	fOwner->fLink->Attach<uint32>(Flags());
	fOwner->fLink->Attach<bool>(IsHidden(this));
	fOwner->fLink->Attach<rgb_color>(fState->view_color);
	if (fTopLevelView)
		fOwner->fLink->Attach<int32>(B_NULL_TOKEN);
	else
		fOwner->fLink->Attach<int32>(_get_object_token_(fParent));
	fOwner->fLink->Flush();

	_CheckOwnerLockAndSwitchCurrent();
	fState->UpdateServerState(*fOwner->fLink);

	// we create all its children, too

	for (KView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		child->_CreateSelf();
	}

	fOwner->fLink->Flush();
	return true;
}


/*!	Sets the new view position.
	It doesn't contact the server, though - the only case where this
	is called outside of MoveTo() is as reaction of moving a view
	in the server (a.k.a. B_WINDOW_RESIZED).
	It also calls the BView's FrameMoved() hook.
*/
void
KView::_MoveTo(int32 x, int32 y)
{
	fParentOffset.Set(x, y);

	if (Window() != NULL && fFlags & B_FRAME_EVENTS) {
		BMessage moved(B_VIEW_MOVED);
		moved.AddInt64("when", system_time());
		moved.AddPoint("where", BPoint(x, y));

		BMessenger target(this);
		target.SendMessage(&moved);
	}
}



/*!	Computes the actual new frame size and recalculates the size of
	the children as well.
	It doesn't contact the server, though - the only case where this
	is called outside of ResizeBy() is as reaction of resizing a view
	in the server (a.k.a. B_WINDOW_RESIZED).
	It also calls the BView's FrameResized() hook.
*/
void
KView::_ResizeBy(int32 deltaWidth, int32 deltaHeight)
{
	fBounds.right += deltaWidth;
	fBounds.bottom += deltaHeight;

	if (Window() == NULL) {
		// we're not supposed to exercise the resizing code in case
		// we haven't been attached to a window yet
		return;
	}

	// layout the children
	if ((fFlags & B_SUPPORTS_LAYOUT) != 0) {
		Relayout();
	} else {
		for (KView* child = fFirstChild; child; child = child->fNextSibling)
			child->_ParentResizedBy(deltaWidth, deltaHeight);
	}

	if (fFlags & B_FRAME_EVENTS) {
		BMessage resized(B_VIEW_RESIZED);
		resized.AddInt64("when", system_time());
		resized.AddInt32("width", fBounds.IntegerWidth());
		resized.AddInt32("height", fBounds.IntegerHeight());

		BMessenger target(this);
		target.SendMessage(&resized);
	}
}


/*!	Relayouts the view according to its resizing mode. */
void
KView::_ParentResizedBy(int32 x, int32 y)
{
	uint32 resizingMode = fFlags & _RESIZE_MASK_;
	BRect newFrame = Frame();

	// follow with left side
	if ((resizingMode & 0x0F00U) == _VIEW_RIGHT_ << 8)
		newFrame.left += x;
	else if ((resizingMode & 0x0F00U) == _VIEW_CENTER_ << 8)
		newFrame.left += x / 2;

	// follow with right side
	if ((resizingMode & 0x000FU) == _VIEW_RIGHT_)
		newFrame.right += x;
	else if ((resizingMode & 0x000FU) == _VIEW_CENTER_)
		newFrame.right += x / 2;

	// follow with top side
	if ((resizingMode & 0xF000U) == _VIEW_BOTTOM_ << 12)
		newFrame.top += y;
	else if ((resizingMode & 0xF000U) == _VIEW_CENTER_ << 12)
		newFrame.top += y / 2;

	// follow with bottom side
	if ((resizingMode & 0x00F0U) == _VIEW_BOTTOM_ << 4)
		newFrame.bottom += y;
	else if ((resizingMode & 0x00F0U) == _VIEW_CENTER_ << 4)
		newFrame.bottom += y / 2;

	if (newFrame.LeftTop() != fParentOffset) {
		// move view
		_MoveTo((int32)roundf(newFrame.left), (int32)roundf(newFrame.top));
	}

	if (newFrame != Frame()) {
		// resize view
		int32 widthDiff = (int32)(newFrame.Width() - fBounds.Width());
		int32 heightDiff = (int32)(newFrame.Height() - fBounds.Height());
		_ResizeBy(widthDiff, heightDiff);
	}
}


void
KView::_ConvertFromScreen(BPoint* point, bool checkLock) const
{
	if (!fParent) {
		if (fOwner)
			fOwner->ConvertFromScreen(point);

		return;
	}

	if (checkLock)
		_CheckOwnerLock();

	_ConvertFromParent(point, false);
	fParent->_ConvertFromScreen(point, false);
}


void
KView::_ConvertFromParent(BPoint* point, bool checkLock) const
{
	if (!fParent)
		return;

	if (checkLock)
		_CheckLock();

	// - our bounds location within the parent
	// + our scrolling offset
	point->x += -fParentOffset.x + fBounds.left;
	point->y += -fParentOffset.y + fBounds.top;
}



void
KView::_Activate(bool active)
{
debug_printf("[KView]{_Activate}\n");

	WindowActivated(active);

	for (KView* child = fFirstChild; child != NULL;
			child = child->fNextSibling) {
		child->_Activate(active);
	}
}


void
KView::_Draw(BRect updateRect)
{
	if (IsHidden(this) || !(Flags() & B_WILL_DRAW))
		return;

	// NOTE: if ViewColor() == B_TRANSPARENT_COLOR and no B_WILL_DRAW
	// -> View is simply not drawn at all

	_SwitchServerCurrentView();

	ConvertFromScreen(&updateRect);

	// TODO: make states robust (the hook implementation could
	// mess things up if it uses non-matching Push- and PopState(),
	// we would not be guaranteed to still have the same state on
	// the stack after having called Draw())
	PushState();
	Draw(updateRect);
	PopState();
	Flush();
}


void
KView::_DrawAfterChildren(BRect updateRect)
{
	if (IsHidden(this) || !(Flags() & B_WILL_DRAW)
		|| !(Flags() & B_DRAW_ON_CHILDREN))
		return;

	_SwitchServerCurrentView();

	ConvertFromScreen(&updateRect);

	// TODO: make states robust (see above)
	PushState();
	DrawAfterChildren(updateRect);
	PopState();
	Flush();
}
