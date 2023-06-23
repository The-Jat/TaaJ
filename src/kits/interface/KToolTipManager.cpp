/*
 * Copyright 2009-2012, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <KToolTipManager.h>
#include <KToolTipWindow.h>

#include <pthread.h>

#include <Autolock.h>
#include <KLayoutBuilder.h>
#include <MessageRunner.h>
#include <Screen.h>

#include <WindowPrivate.h>
#include <KToolTip.h>


static pthread_once_t sManagerInitOnce = PTHREAD_ONCE_INIT;
KToolTipManager* KToolTipManager::sDefaultInstance;

static const uint32 kMsgHideToolTip = 'hide';
static const uint32 kMsgShowToolTip = 'show';
static const uint32 kMsgCurrentToolTip = 'curr';
static const uint32 kMsgCloseToolTip = 'clos';


namespace BPrivate {


class KToolTipView : public KView {
public:
								KToolTipView(KToolTip* tip);
	virtual						~KToolTipView();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();

	virtual	void				FrameResized(float width, float height);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

			void				HideTip();
			void				ShowTip();

			void				ResetWindowFrame();
			void				ResetWindowFrame(BPoint where);

			KToolTip*			Tip() const { return fToolTip; }
			bool				IsTipHidden() const { return fHidden; }

private:
			KToolTip*			fToolTip;
			bool				fHidden;
};


KToolTipView::KToolTipView(KToolTip* tip)
	:
	KView("tool tip", B_WILL_DRAW | B_FRAME_EVENTS),
	fToolTip(tip),
	fHidden(false)
{
	fToolTip->AcquireReference();
	SetViewUIColor(B_TOOL_TIP_BACKGROUND_COLOR);
	SetHighUIColor(B_TOOL_TIP_TEXT_COLOR);

	KGroupLayout* layout = new KGroupLayout(B_VERTICAL);
	layout->SetInsets(5, 5, 5, 5);
	SetLayout(layout);

	AddChild(fToolTip->View());
}


KToolTipView::~KToolTipView()
{
	fToolTip->ReleaseReference();
}


void
KToolTipView::AttachedToWindow()
{
	SetEventMask(B_POINTER_EVENTS | B_KEYBOARD_EVENTS, 0);
	fToolTip->AttachedToWindow();
}


void
KToolTipView::DetachedFromWindow()
{
	KToolTipManager* manager = KToolTipManager::Manager();
	manager->Lock();

	RemoveChild(fToolTip->View());
		// don't delete this one!
	fToolTip->DetachedFromWindow();

	manager->Unlock();
}


void
KToolTipView::FrameResized(float width, float height)
{
	ResetWindowFrame();
}


void
KToolTipView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (fToolTip->IsSticky()) {
		ResetWindowFrame(ConvertToScreen(where));
	} else if (transit == B_ENTERED_VIEW) {
		// close instantly if the user managed to enter
		Window()->Quit();
	} else {
		// close with the preferred delay in case the mouse just moved
		HideTip();
	}
}


void
KToolTipView::KeyDown(const char* bytes, int32 numBytes)
{
	if (!fToolTip->IsSticky())
		HideTip();
}


void
KToolTipView::HideTip()
{
	if (fHidden)
		return;

	BMessage quit(kMsgCloseToolTip);
	BMessageRunner::StartSending(Window(), &quit,
		KToolTipManager::Manager()->HideDelay(), 1);
	fHidden = true;
}


void
KToolTipView::ShowTip()
{
	fHidden = false;
}


void
KToolTipView::ResetWindowFrame()
{
	BPoint where;
	GetMouse(&where, NULL, false);

	ResetWindowFrame(ConvertToScreen(where));
}


/*!	Tries to find the right frame to show the tool tip in, trying to use the
	alignment that the tool tip specifies.
	Makes sure the tool tip can be shown on screen in its entirety, ie. it will
	resize the window if necessary.
*/
void
KToolTipView::ResetWindowFrame(BPoint where)
{
	if (Window() == NULL)
		return;

	BSize size = PreferredSize();

	BScreen screen(Window());
	BRect screenFrame = screen.Frame().InsetBySelf(2, 2);
	BPoint offset = fToolTip->MouseRelativeLocation();

	// Ensure that the tip can be placed on screen completely

	if (size.width > screenFrame.Width())
		size.width = screenFrame.Width();

	if (size.width > where.x - screenFrame.left
		&& size.width > screenFrame.right - where.x) {
		// There is no space to put the tip to the left or the right of the
		// cursor, it can either be below or above it
		if (size.height > where.y - screenFrame.top
			&& where.y - screenFrame.top > screenFrame.Height() / 2) {
			size.height = where.y - offset.y - screenFrame.top;
		} else if (size.height > screenFrame.bottom - where.y
			&& screenFrame.bottom - where.y > screenFrame.Height() / 2) {
			size.height = screenFrame.bottom - where.y - offset.y;
		}
	}

	// Find best alignment, starting with the requested one

	BAlignment alignment = fToolTip->Alignment();
	BPoint location = where;
	bool doesNotFit = false;

	switch (alignment.horizontal) {
		case B_ALIGN_LEFT:
			location.x -= size.width + offset.x;
			if (location.x < screenFrame.left) {
				location.x = screenFrame.left;
				doesNotFit = true;
			}
			break;
		case B_ALIGN_CENTER:
			location.x -= size.width / 2 - offset.x;
			if (location.x < screenFrame.left) {
				location.x = screenFrame.left;
				doesNotFit = true;
			} else if (location.x + size.width > screenFrame.right) {
				location.x = screenFrame.right - size.width;
				doesNotFit = true;
			}
			break;

		default:
			location.x += offset.x;
			if (location.x + size.width > screenFrame.right) {
				location.x = screenFrame.right - size.width;
				doesNotFit = true;
			}
			break;
	}

	if ((doesNotFit && alignment.vertical == B_ALIGN_MIDDLE)
		|| (alignment.vertical == B_ALIGN_MIDDLE
			&& alignment.horizontal == B_ALIGN_CENTER))
		alignment.vertical = B_ALIGN_BOTTOM;

	// Adjust the tooltip position in cases where it would be partly out of the
	// screen frame. Try to fit the tooltip on the requested side of the
	// cursor, if that fails, try the opposite side, and if that fails again,
	// give up and leave the tooltip under the mouse cursor.
	bool firstTry = true;
	while (true) {
		switch (alignment.vertical) {
			case B_ALIGN_TOP:
				location.y = where.y - size.height - offset.y;
				if (location.y < screenFrame.top) {
					alignment.vertical = firstTry ? B_ALIGN_BOTTOM
						: B_ALIGN_MIDDLE;
					firstTry = false;
					continue;
				}
				break;

			case B_ALIGN_MIDDLE:
				location.y -= size.height / 2 - offset.y;
				if (location.y < screenFrame.top)
					location.y = screenFrame.top;
				else if (location.y + size.height > screenFrame.bottom)
					location.y = screenFrame.bottom - size.height;
				break;

			default:
				location.y = where.y + offset.y;
				if (location.y + size.height > screenFrame.bottom) {
					alignment.vertical = firstTry ? B_ALIGN_TOP
						: B_ALIGN_MIDDLE;
					firstTry = false;
					continue;
				}
				break;
		}
		break;
	}

	where = location;

	// Cut off any out-of-screen areas

	if (screenFrame.left > where.x) {
		size.width -= where.x - screenFrame.left;
		where.x = screenFrame.left;
	} else if (screenFrame.right < where.x + size.width)
		size.width = screenFrame.right - where.x;

	if (screenFrame.top > where.y) {
		size.height -= where.y - screenFrame.top;
		where.y = screenFrame.top;
	} else if (screenFrame.bottom < where.y + size.height)
		size.height -= screenFrame.bottom - where.y;

	// Change window frame

	Window()->ResizeTo(size.width, size.height);
	Window()->MoveTo(where);
}


// #pragma mark -


KToolTipWindow::KToolTipWindow(KToolTip* tip, BPoint where, void* owner)
	:
	KWindow(BRect(0, 0, 250, 10).OffsetBySelf(where), "tool tip",
		B_BORDERED_WINDOW_LOOK, kMenuWindowFeel,
		B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_AUTO_UPDATE_SIZE_LIMITS
			| B_AVOID_FRONT | B_AVOID_FOCUS),
	fOwner(owner)
{
	SetLayout(new KGroupLayout(B_VERTICAL));

	KToolTipManager* manager = KToolTipManager::Manager();
	KToolTipView* view = new KToolTipView(tip);

	manager->Lock();
	AddChild(view);
	manager->Unlock();

	// figure out size and location

	view->ResetWindowFrame(where);
}


void
KToolTipWindow::MessageReceived(BMessage* message)
{
	KToolTipView* view = static_cast<KToolTipView*>(ChildAt(0));

	switch (message->what) {
		case kMsgHideToolTip:
			view->HideTip();
			break;

		case kMsgCurrentToolTip:
		{
			KToolTip* tip = view->Tip();

			BMessage reply(B_REPLY);
			reply.AddPointer("current", tip);
			reply.AddPointer("owner", fOwner);

			if (message->SendReply(&reply) == B_OK)
				tip->AcquireReference();
			break;
		}

		case kMsgShowToolTip:
			view->ShowTip();
			break;

		case kMsgCloseToolTip:
			if (view->IsTipHidden())
				Quit();
			break;

		default:
			KWindow::MessageReceived(message);
	}
}


}	// namespace BPrivate


// #pragma mark -


/*static*/ KToolTipManager*
KToolTipManager::Manager()
{
	// Note: The check is not necessary; it's just faster than always calling
	// pthread_once(). It requires reading/writing of pointers to be atomic
	// on the architecture.
	if (sDefaultInstance == NULL)
		pthread_once(&sManagerInitOnce, &_InitSingleton);

	return sDefaultInstance;
}


void
KToolTipManager::ShowTip(KToolTip* tip, BPoint where, void* owner)
{
	KToolTip* current = NULL;
	void* currentOwner = NULL;
	BMessage reply;
	if (fWindow.SendMessage(kMsgCurrentToolTip, &reply) == B_OK) {
		reply.FindPointer("current", (void**)&current);
		reply.FindPointer("owner", &currentOwner);
	}

	// Release reference from the message
	if (current != NULL)
		current->ReleaseReference();

	if (current == tip || currentOwner == owner) {
		fWindow.SendMessage(kMsgShowToolTip);
		return;
	}

	fWindow.SendMessage(kMsgHideToolTip);

	if (tip != NULL) {
		KWindow* window = new BPrivate::KToolTipWindow(tip, where, owner);
		window->Show();

		fWindow = BMessenger(window);
	}
}


void
KToolTipManager::HideTip()
{
	fWindow.SendMessage(kMsgHideToolTip);
}


void
KToolTipManager::SetShowDelay(bigtime_t time)
{
	// between 10ms and 3s
	if (time < 10000)
		time = 10000;
	else if (time > 3000000)
		time = 3000000;

	fShowDelay = time;
}


bigtime_t
KToolTipManager::ShowDelay() const
{
	return fShowDelay;
}


void
KToolTipManager::SetHideDelay(bigtime_t time)
{
	// between 0 and 0.5s
	if (time < 0)
		time = 0;
	else if (time > 500000)
		time = 500000;

	fHideDelay = time;
}


bigtime_t
KToolTipManager::HideDelay() const
{
	return fHideDelay;
}


KToolTipManager::KToolTipManager()
	:
	fLock("tool tip manager"),
	fShowDelay(750000),
	fHideDelay(50000)
{
}


KToolTipManager::~KToolTipManager()
{
}


/*static*/ void
KToolTipManager::_InitSingleton()
{
	sDefaultInstance = new KToolTipManager();
}
