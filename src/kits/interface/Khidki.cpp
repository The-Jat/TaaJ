
#include <Khidki.h>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <AppMisc.h>
#include <AppServerLink.h>
#include <ApplicationPrivate.h>
#include <Autolock.h>

#include <KBitmap.h>
#include <KButton.h>

#include <Jaatbar.h>
#include <DirectMessageTarget.h>

#include <FindDirectory.h>

#include <InputServerTypes.h>

#include <KLayout.h>
#include <KLayoutUtils.h>

#include <KMenuBar.h>
#include <KMenuItem.h>
#include <KMenuPrivate.h>

#include <MessagePrivate.h>
#include <MessageQueue.h>
#include <MessageRunner.h>

#include <Path.h>
#include <PortLink.h>
#include <PropertyInfo.h>

#include <Roster.h>
#include <RosterPrivate.h>

#include <Screen.h>
#include <ServerProtocol.h>
#include <String.h>

#include <KTextView.h>
#include <TokenSpace.h>
#include <KToolTipManager.h>
#include <KToolTipWindow.h>

#include <UnicodeChar.h>

#include <WindowPrivate.h>

#include <binary_compatibility/Interface.h>
#include <input_globals.h>
#include <tracker_private.h>

#include <TheConfig.h> // configuration file.
//khidki code
//start
#if defined(OVERRIDE_DEBUGGING) || defined(DEBUG_KHIDKI)
#	define DEBUG(x) debug_printf x
#else
#	define DEBUG(x) ;
#endif
//end

#define B_HIDE_APPLICATION '_AHD'

#define _MINIMIZE_			'_WMZ'
#define _ZOOM_				'_WZO'
#define _SEND_BEHIND_		'_WSB'
#define _SEND_TO_FRONT_		'_WSF'

void k_do_minimize_team(BRect zoomRect, team_id team, bool zoom);



struct KWindow::unpack_cookie {
	unpack_cookie();

	BMessage*	message;
	int32		index;
	BHandler*	focus;
	int32		focus_token;
	int32		last_view_token;
	bool		found_focus;
	bool		tokens_scanned;
};



class KWindow::Shortcut {
public:
							Shortcut(uint32 key, uint32 modifiers,
								KMenuItem* item);
							Shortcut(uint32 key, uint32 modifiers,
								BMessage* message, BHandler* target);
							~Shortcut();

			bool			Matches(uint32 key, uint32 modifiers) const;

			KMenuItem*		MenuItem() const { return fMenuItem; }
			BMessage*		Message() const { return fMessage; }
			BHandler*		Target() const { return fTarget; }

	static	uint32			AllowedModifiers();
	static	uint32			PrepareKey(uint32 key);
	static	uint32			PrepareModifiers(uint32 modifiers);

private:
			uint32			fKey;
			uint32			fModifiers;
			KMenuItem*		fMenuItem;
			BMessage*		fMessage;
			BHandler*		fTarget;
};


using BPrivate::gDefaultTokens;
using BPrivate::KMenuPrivate;


static property_info sWindowPropInfo[] = {
	{
		"Active", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_BOOL_TYPE }
	},

	{
		"Feel", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_INT32_TYPE }
	},

	{
		"Flags", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_INT32_TYPE }
	},

	{
		"Frame", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_RECT_TYPE }
	},

	{
		"Hidden", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_BOOL_TYPE }
	},

	{
		"Look", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_INT32_TYPE }
	},

	{
		"Title", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_STRING_TYPE }
	},

	{
		"Workspaces", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_INT32_TYPE }
	},

	{
		"MenuBar", {},
		{ B_DIRECT_SPECIFIER }, NULL, 0, {}
	},

	{
		"View", { B_COUNT_PROPERTIES },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_INT32_TYPE }
	},

	{
		"View", {}, {}, NULL, 0, {}
	},

	{
		"Minimize", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_BOOL_TYPE }
	},

	{
		"TabFrame", { B_GET_PROPERTY },
		{ B_DIRECT_SPECIFIER }, NULL, 0, { B_RECT_TYPE }
	},

	{ 0 }
};

static value_info sWindowValueInfo[] = {
	{
		"MoveTo", 'WDMT', B_COMMAND_KIND,
		"Moves to the position in the BPoint data"
	},

	{
		"MoveBy", 'WDMB', B_COMMAND_KIND,
		"Moves by the offsets in the BPoint data"
	},

	{
		"ResizeTo", 'WDRT', B_COMMAND_KIND,
		"Resize to the size in the BPoint data"
	},

	{
		"ResizeBy", 'WDRB', B_COMMAND_KIND,
		"Resize by the offsets in the BPoint data"
	},

	{ 0 }
};


void
k_set_menu_sem_(KWindow* window, sem_id sem)
{
	if (window != NULL)
		window->fMenuSem = sem;
}


//	#pragma mark -


KWindow::unpack_cookie::unpack_cookie()
	:
	message((BMessage*)~0UL),
		// message == NULL is our exit condition
	index(0),
	focus_token(B_NULL_TOKEN),
	last_view_token(B_NULL_TOKEN),
	found_focus(false),
	tokens_scanned(false)
{
}


//	#pragma mark - KWindow::Shortcut


KWindow::Shortcut::Shortcut(uint32 key, uint32 modifiers, KMenuItem* item)
	:
	fKey(PrepareKey(key)),
	fModifiers(PrepareModifiers(modifiers)),
	fMenuItem(item),
	fMessage(NULL),
	fTarget(NULL)
{
}


KWindow::Shortcut::Shortcut(uint32 key, uint32 modifiers, BMessage* message,
	BHandler* target)
	:
	fKey(PrepareKey(key)),
	fModifiers(PrepareModifiers(modifiers)),
	fMenuItem(NULL),
	fMessage(message),
	fTarget(target)
{
}


KWindow::Shortcut::~Shortcut()
{
	// we own the message, if any
	delete fMessage;
}


bool
KWindow::Shortcut::Matches(uint32 key, uint32 modifiers) const
{
	return fKey == key && fModifiers == modifiers;
}


/*static*/
uint32
KWindow::Shortcut::AllowedModifiers()
{
	return B_COMMAND_KEY | B_OPTION_KEY | B_SHIFT_KEY | B_CONTROL_KEY
		| B_MENU_KEY;
}


/*static*/
uint32
KWindow::Shortcut::PrepareModifiers(uint32 modifiers)
{
	return (modifiers & AllowedModifiers()) | B_COMMAND_KEY;
}


/*static*/
uint32
KWindow::Shortcut::PrepareKey(uint32 key)
{
	return BUnicodeChar::ToLower(key);
}




KWindow::KWindow(/*Int_Rect*/BRect frame, const char* title, window_type type,
		uint32 flags, uint32 workspace)
	:
	BLooper(title, B_DISPLAY_PRIORITY)
{
DEBUG(("[KWindow] Into the KWindow constructor.\n"));
DEBUG(("[KWindow] workspace = %d\n",workspace));
	window_look look;
	window_feel feel;

	_DecomposeType(type, &look, &feel);
DEBUG(("[KWindow] window_type =%d\n", (int)type));
DEBUG(("[KWindow] window_feel =%d\n", (int)feel));

	_InitData(frame, title, look, feel, flags, workspace);
DEBUG(("[KWindow]  KWindow constructor ends.\n"));
}


KWindow::KWindow(BRect frame, const char* title, window_look look,
		window_feel feel, uint32 flags, uint32 workspace)
	:
	BLooper(title, B_DISPLAY_PRIORITY)
{
	_InitData(frame, title, look, feel, flags, workspace);
}



KWindow::KWindow(BMessage* data)
	:
	BLooper(data)
{
	data->FindRect("_frame", &fFrame);

	const char* title;
	data->FindString("_title", &title);

	window_look look;
	data->FindInt32("_wlook", (int32*)&look);

	window_feel feel;
	data->FindInt32("_wfeel", (int32*)&feel);

	if (data->FindInt32("_flags", (int32*)&fFlags) != B_OK)
		fFlags = 0;

	uint32 workspaces;
	data->FindInt32("_wspace", (int32*)&workspaces);

	uint32 type;
	if (data->FindInt32("_type", (int32*)&type) == B_OK)
		_DecomposeType((window_type)type, &fLook, &fFeel);

		// connect to app_server and initialize data
	_InitData(fFrame, title, look, feel, fFlags, workspaces);

	if (data->FindFloat("_zoom", 0, &fMaxZoomWidth) == B_OK
		&& data->FindFloat("_zoom", 1, &fMaxZoomHeight) == B_OK)
		SetZoomLimits(fMaxZoomWidth, fMaxZoomHeight);

	if (data->FindFloat("_sizel", 0, &fMinWidth) == B_OK
		&& data->FindFloat("_sizel", 1, &fMinHeight) == B_OK
		&& data->FindFloat("_sizel", 2, &fMaxWidth) == B_OK
		&& data->FindFloat("_sizel", 3, &fMaxHeight) == B_OK)
		SetSizeLimits(fMinWidth, fMaxWidth,
			fMinHeight, fMaxHeight);

	if (data->FindInt64("_pulse", &fPulseRate) == B_OK)
		SetPulseRate(fPulseRate);

	BMessage msg;
	int32 i = 0;
	while (data->FindMessage("_views", i++, &msg) == B_OK) {
		BArchivable* obj = instantiate_object(&msg);
		if (KView* child = dynamic_cast<KView*>(obj))
			AddChild(child);
	}
}


KWindow::KWindow(BRect frame, int32 bitmapToken)
	:
	BLooper("offscreen bitmap")
{
debug_printf("[KWindow] consructor with BRect and bitmapToken\n");
	_DecomposeType(B_UNTYPED_WINDOW, &fLook, &fFeel);
	_InitData(frame, "offscreen", fLook, fFeel, 0, 0, bitmapToken);

debug_printf("[KWindow] consructor with BRect and bitmapToken end\n");
}



void
KWindow::_InitData(/*Int_Rect*/ BRect frame, const char* title, window_look look,
	window_feel feel, uint32 flags, uint32 workspace, int32 bitmapToken)
{
debug_printf("[KWindow] {_InitData} Into the _InitData of KWindow....\n");
debug_printf("[KWindow] {_InitData} workspace = %d\n",workspace);

	//STRACE(("KWindow::InitData()\n"));

	if (be_app == NULL) {
		debug_printf("[KWindow] be_app is not running...\n");
		debugger("You need a valid BApplication object before interacting with "
			"the app_server");
		return;
	}

debug_printf("[KWindow]{_InitData} before roundf frame.left =%f\n",frame.left);
debug_printf("[KWindow]{_InitData} before roundf frame.top =%f\n",frame.top);
debug_printf("[KWindow]{_InitData} before roundf frame.right =%f\n",frame.right);
debug_printf("[KWindow]{_InitData} before roundf frame.bottom =%f\n",frame.bottom);

	frame.left = roundf(frame.left);
	frame.top = roundf(frame.top);
	frame.right = roundf(frame.right);
	frame.bottom = roundf(frame.bottom);

debug_printf("[KWindow]{_InitData} frame.left =%f\n",frame.left);
debug_printf("[KWindow]{_InitData} frame.top =%f\n",frame.top);
debug_printf("[KWindow]{_InitData} frame.right =%f\n",frame.right);
debug_printf("[KWindow]{_InitData} frame.bottom =%f\n",frame.bottom);

	fFrame = frame;

	if (title == NULL)
		{
		title = "";
		debug_printf("[KWindow] {_InitData} title == NULL...\n");
		}

	fTitle = strdup(title);

	_SetName(title);

	fFeel = feel;
	fLook = look;
	fFlags = flags | B_ASYNCHRONOUS_CONTROLS;

	fInTransaction = bitmapToken >= 0;

debug_printf("[KWindow] {_InitData} bitmapToken =%d...\n",bitmapToken);

	fUpdateRequested = false;
	fActive = false;
	fShowLevel = 1;

	fTopView = NULL;
	fFocus = NULL;
	fLastMouseMovedView	= NULL;
	fKeyMenuBar = NULL;
	fDefaultButton = NULL;

	// Shortcut 'Q' is handled in _HandleKeyDown() directly, as its message
	// get sent to the application, and not one of our handlers.
	// It is only installed for non-modal windows, though.
	fNoQuitShortcut = IsModal();

	if ((fFlags & B_NOT_CLOSABLE) == 0 && !IsModal()) {
		// Modal windows default to non-closable, but you can add the
		// shortcut manually, if a different behaviour is wanted
		AddShortcut('W', B_COMMAND_KEY, new BMessage(B_QUIT_REQUESTED));
	}

	// Edit modifier keys

	AddShortcut('X', B_COMMAND_KEY, new BMessage(B_CUT), NULL);
	AddShortcut('C', B_COMMAND_KEY, new BMessage(B_COPY), NULL);
	AddShortcut('V', B_COMMAND_KEY, new BMessage(B_PASTE), NULL);
	AddShortcut('A', B_COMMAND_KEY, new BMessage(B_SELECT_ALL), NULL);

	// Window modifier keys

	AddShortcut('M', B_COMMAND_KEY | B_CONTROL_KEY,
		new BMessage(_MINIMIZE_), NULL);
	AddShortcut('Z', B_COMMAND_KEY | B_CONTROL_KEY,
		new BMessage(_ZOOM_), NULL);
	AddShortcut('Z', B_SHIFT_KEY | B_COMMAND_KEY | B_CONTROL_KEY,
		new BMessage(_ZOOM_), NULL);
	AddShortcut('H', B_COMMAND_KEY | B_CONTROL_KEY,
		new BMessage(B_HIDE_APPLICATION), NULL);
	AddShortcut('F', B_COMMAND_KEY | B_CONTROL_KEY,
		new BMessage(_SEND_TO_FRONT_), NULL);
	AddShortcut('B', B_COMMAND_KEY | B_CONTROL_KEY,
		new BMessage(_SEND_BEHIND_), NULL);

	// We set the default pulse rate, but we don't start the pulse
	fPulseRate = 500000;
	fPulseRunner = NULL;

	fIsFilePanel = false;

	fMenuSem = -1;

	fMinimized = false;

	fMaxZoomHeight = 32768.0;
	fMaxZoomWidth = 32768.0;
	fMinHeight = 0.0;
	fMinWidth = 0.0;
	fMaxHeight = 32768.0;
	fMaxWidth = 32768.0;

	fLastViewToken = B_NULL_TOKEN;

	// TODO: other initializations!
	fOffscreen = false;

	// Create the server-side window

	port_id receivePort = create_port(B_LOOPER_PORT_DEFAULT_CAPACITY,
		"w<app_server");
	if (receivePort < B_OK) {
		// TODO: huh?
		debug_printf("[KWindow]{_InitData} KWindow`s receive port failed on creation...\n");
		debugger("Could not create KWindow's receive port, used for "
				 "interacting with the app_server!");
		delete this;
		return;
	}

	//STRACE(("KWindow::InitData(): contacting app_server...\n"));

debug_printf("[KWindow]{_InitData} contacting app_server...\n");

	// let app_server know that a window has been created.
	fLink = new(std::nothrow) BPrivate::PortLink(
		BApplication::Private::ServerLink()->SenderPort(), receivePort);
	if (fLink == NULL) {

	debug_printf("[KWindow]{_InitData} failed to create window...\n");

		// Zombie!
		return;
	}

	{
		BPrivate::AppServerLink lockLink;
			// we're talking to the server application using our own
			// communication channel (fLink) - we better make sure no one
			// interferes by locking that channel (which AppServerLink does
			// implicetly)

		if (bitmapToken < 0) {

			debug_printf("[KWindow] {_InitData} bitmapToken < 0 so send the [AS_KHIDKI_KHOL] msg\n");

			fLink->StartMessage(AS_KHIDKI_KHOL);
		} else {
			fLink->StartMessage(AS_CREATE_OFFSCREEN_WINDOW_2);
			fLink->Attach<int32>(bitmapToken);
			fOffscreen = true;
		}

		fLink->Attach</*Int_Rect*/BRect>(fFrame);
		fLink->Attach<uint32>((uint32)fLook);
		fLink->Attach<uint32>((uint32)fFeel);
		fLink->Attach<uint32>(fFlags);
		fLink->Attach<uint32>(workspace);
		fLink->Attach<int32>(_get_object_token_(this));
		fLink->Attach<port_id>(receivePort);
		fLink->Attach<port_id>(fMsgPort);
		fLink->AttachString(title);
		debug_printf("[KWindow] {_InitData} before Flushing.\n");

		port_id sendPort;
		int32 code;
		if (fLink->FlushWithReply(code) == B_OK
			&& code == B_OK
			&& fLink->Read<port_id>(&sendPort) == B_OK) {
			// read the frame size and its limits that were really
			// enforced on the server side
			
			debug_printf("[KWindow] {_InitData} got the reply.\n");

			fLink->Read</*Int_Rect*/BRect>(&fFrame);
			fLink->Read<float>(&fMinWidth);
			fLink->Read<float>(&fMaxWidth);
			fLink->Read<float>(&fMinHeight);
			fLink->Read<float>(&fMaxHeight);

			fMaxZoomWidth = fMaxWidth;
			fMaxZoomHeight = fMaxHeight;
		} else
			sendPort = -1;

debug_printf("[KWindow] {_InitData} code =%d...\n",code);

		// Redirect our link to the new window connection
		fLink->SetSenderPort(sendPort);
		debug_printf("[KWindow] {_InitData} server says that our send port is = %d\n",sendPort);
		//STRACE(("Server says that our send port is %ld\n", sendPort));
	}

	debug_printf("[KWindoW] {_InitData} Window locked?: %s\n", IsLocked() ? "True" : "False");

	_CreateTopView();

debug_printf("[KWindow] {_InitData} ends\n");
}


void
KWindow::AddChild(KView* child, KView* before)
{
	BAutolock locker(this);
	if (locker.IsLocked())
		fTopView->AddChild(child, before);
}


void
KWindow::AddChild(KLayoutItem* child)
{
	BAutolock locker(this);
	if (locker.IsLocked())
		fTopView->AddChild(child);
}


bool
KWindow::RemoveChild(KView* child)
{
	BAutolock locker(this);
	if (!locker.IsLocked())
		return false;

	return fTopView->RemoveChild(child);
}


int32
KWindow::CountChildren() const
{
	BAutolock locker(const_cast<KWindow*>(this));
	if (!locker.IsLocked())
		return 0;

	return fTopView->CountChildren();
}


KView*
KWindow::ChildAt(int32 index) const
{
	BAutolock locker(const_cast<KWindow*>(this));
	if (!locker.IsLocked())
		return NULL;

	return fTopView->ChildAt(index);
}

void
KWindow::Minimize(bool minimize)
{
debug_printf("[KWindow]{Minimize}\n");

	if (IsModal() || IsFloating() || IsHidden() || fMinimized == minimize
		|| !Lock())
		return;

	fMinimized = minimize;

	fLink->StartMessage(AS_MINIMIZE_WINDOW);
	fLink->Attach<bool>(minimize);
	fLink->Flush();

	Unlock();

debug_printf("[KWindow]{Minimize} end\n");
}


status_t
KWindow::SendBehind(const KWindow* window)
{
debug_printf("[KWindow client]{SendBehind}\n");

	if (!Lock())
		return B_ERROR;

	fLink->StartMessage(AS_SEND_BEHIND); // tododo
	fLink->Attach<int32>(window != NULL ? _get_object_token_(window) : -1);
	fLink->Attach<team_id>(Team());

	status_t status = B_ERROR;
	fLink->FlushWithReply(status);

	Unlock();

	return status;
}


bool
KWindow::IsFront() const
{
	BAutolock locker(const_cast<KWindow*>(this));
	if (!locker.IsLocked())
		return false;

	fLink->StartMessage(AS_IS_FRONT_WINDOW);// tododo

	status_t status;
	if (fLink->FlushWithReply(status) == B_OK)
		return status >= B_OK;

	return false;
}


void
KWindow::MessageReceived(BMessage* message)
{
	if (!message->HasSpecifiers()) {
		if (message->what == B_KEY_DOWN)
			_KeyboardNavigation();

		if (message->what == (int32)kMsgAppServerRestarted) {
			fLink->SetSenderPort(
				BApplication::Private::ServerLink()->SenderPort());

			BPrivate::AppServerLink lockLink;
				// we're talking to the server application using our own
				// communication channel (fLink) - we better make sure no one
				// interferes by locking that channel (which AppServerLink does
				// implicitly)

			fLink->StartMessage(AS_KHIDKI_KHOL);// tododo

			fLink->Attach<BRect>(fFrame);
			fLink->Attach<uint32>((uint32)fLook);
			fLink->Attach<uint32>((uint32)fFeel);
			fLink->Attach<uint32>(fFlags);
			fLink->Attach<uint32>(0);
			fLink->Attach<int32>(_get_object_token_(this));
			fLink->Attach<port_id>(fLink->ReceiverPort());
			fLink->Attach<port_id>(fMsgPort);
			fLink->AttachString(fTitle);

			port_id sendPort;
			int32 code;
			if (fLink->FlushWithReply(code) == B_OK
				&& code == B_OK
				&& fLink->Read<port_id>(&sendPort) == B_OK) {
				// read the frame size and its limits that were really
				// enforced on the server side

				fLink->Read<BRect>(&fFrame);
				fLink->Read<float>(&fMinWidth);
				fLink->Read<float>(&fMaxWidth);
				fLink->Read<float>(&fMinHeight);
				fLink->Read<float>(&fMaxHeight);

				fMaxZoomWidth = fMaxWidth;
				fMaxZoomHeight = fMaxHeight;
			} else
				sendPort = -1;

			// Redirect our link to the new window connection
			fLink->SetSenderPort(sendPort);

			// connect all views to the server again
			fTopView->_CreateSelf();

			_SendShowOrHideMessage();
		}

		return BLooper::MessageReceived(message);
	}

	BMessage replyMsg(B_REPLY);
	bool handled = false;

	BMessage specifier;
	int32 what;
	const char* prop;
	int32 index;

	if (message->GetCurrentSpecifier(&index, &specifier, &what, &prop) != B_OK)
		return BLooper::MessageReceived(message);

	BPropertyInfo propertyInfo(sWindowPropInfo);
	switch (propertyInfo.FindMatch(message, index, &specifier, what, prop)) {
		case 0:
			if (message->what == B_GET_PROPERTY) {
				replyMsg.AddBool("result", IsActive());
				handled = true;
			} else if (message->what == B_SET_PROPERTY) {
				bool newActive;
				if (message->FindBool("data", &newActive) == B_OK) {
					Activate(newActive);
					handled = true;
				}
			}
			break;
		case 1:
			if (message->what == B_GET_PROPERTY) {
				replyMsg.AddInt32("result", (uint32)Feel());
				handled = true;
			} else {
				uint32 newFeel;
				if (message->FindInt32("data", (int32*)&newFeel) == B_OK) {
					SetFeel((window_feel)newFeel);
					handled = true;
				}
			}
			break;
		case 2:
			if (message->what == B_GET_PROPERTY) {
				replyMsg.AddInt32("result", Flags());
				handled = true;
			} else {
				uint32 newFlags;
				if (message->FindInt32("data", (int32*)&newFlags) == B_OK) {
					SetFlags(newFlags);
					handled = true;
				}
			}
			break;
		case 3:
			if (message->what == B_GET_PROPERTY) {
				replyMsg.AddRect("result", Frame());
				handled = true;
			} else {
				BRect newFrame;
				if (message->FindRect("data", &newFrame) == B_OK) {
					MoveTo(newFrame.LeftTop());
					ResizeTo(newFrame.Width(), newFrame.Height());
					handled = true;
				}
			}
			break;
		case 4:
			if (message->what == B_GET_PROPERTY) {
				replyMsg.AddBool("result", IsHidden());
				handled = true;
			} else {
				bool hide;
				if (message->FindBool("data", &hide) == B_OK) {
					if (hide) {
						if (!IsHidden())
							Hide();
					} else if (IsHidden())
						Show();
					handled = true;
				}
			}
			break;
		case 5:
			if (message->what == B_GET_PROPERTY) {
				replyMsg.AddInt32("result", (uint32)Look());
				handled = true;
			} else {
				uint32 newLook;
				if (message->FindInt32("data", (int32*)&newLook) == B_OK) {
					SetLook((window_look)newLook);
					handled = true;
				}
			}
			break;
		case 6:
			if (message->what == B_GET_PROPERTY) {
				replyMsg.AddString("result", Title());
				handled = true;
			} else {
				const char* newTitle = NULL;
				if (message->FindString("data", &newTitle) == B_OK) {
					SetTitle(newTitle);
					handled = true;
				}
			}
			break;
		case 7:
			if (message->what == B_GET_PROPERTY) {
				replyMsg.AddInt32( "result", Workspaces());
				handled = true;
			} else {
				uint32 newWorkspaces;
				if (message->FindInt32("data", (int32*)&newWorkspaces) == B_OK) {
					SetWorkspaces(newWorkspaces);
					handled = true;
				}
			}
			break;
		case 11:
			if (message->what == B_GET_PROPERTY) {
				replyMsg.AddBool("result", IsMinimized());
				handled = true;
			} else {
				bool minimize;
				if (message->FindBool("data", &minimize) == B_OK) {
					Minimize(minimize);
					handled = true;
				}
			}
			break;
		case 12:
			if (message->what == B_GET_PROPERTY) {
				BMessage settings;
				if (GetDecoratorSettings(&settings) == B_OK) {
					BRect frame;
					if (settings.FindRect("tab frame", &frame) == B_OK) {
						replyMsg.AddRect("result", frame);
						handled = true;
					}
				}
			}
			break;
		default:
			return BLooper::MessageReceived(message);
	}

	if (handled) {
		if (message->what == B_SET_PROPERTY)
			replyMsg.AddInt32("error", B_OK);
	} else {
		replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;
		replyMsg.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
		replyMsg.AddString("message", "Didn't understand the specifier(s)");
	}
	message->SendReply(&replyMsg);
}



void
KWindow::DispatchMessage(BMessage* message, BHandler* target)
{
debug_printf("[KWindow client]{DispatchMessage}\n");

	if (message == NULL)
		return;

	switch (message->what) {
		case B_ZOOM:
		debug_printf("[KWindow client]{DispatchMessage} B_ZOOM\n");
			Zoom();
			break;

		case _MINIMIZE_:
		debug_printf("[KWindow client]{DispatchMessage} _MINIMIZE_\n");
			// Used by the minimize shortcut
			if ((Flags() & B_NOT_MINIMIZABLE) == 0)
				Minimize(true);
			break;

		case _ZOOM_:
		debug_printf("[KWindow client]{DispatchMessage} _ZOOM_\n");
			// Used by the zoom shortcut
			if ((Flags() & B_NOT_ZOOMABLE) == 0)
				Zoom();
			break;

		case _SEND_BEHIND_:
		debug_printf("[KWindow client]{DispatchMessage} _SEND_BEHIND_\n");
			SendBehind(NULL);
			break;

		case _SEND_TO_FRONT_:
		debug_printf("[KWindow client]{DispatchMessage} _SEND_TO_FRONT_\n");
			Activate();
			break;

		case B_MINIMIZE:
		{
		debug_printf("[KWindow client]{DispatchMessage} B_MINIMIZED\n");
			bool minimize;
			if (message->FindBool("minimize", &minimize) == B_OK)
				Minimize(minimize);
			break;
		}

		case B_HIDE_APPLICATION:
		{
		debug_printf("[KWindow client]{DispatchMessage} B_HIDE_APPLICATION\n");
			// Hide all applications with the same signature
			// (ie. those that are part of the same group to be consistent
			// to what the Deskbar shows you).
			app_info info;
			be_app->GetAppInfo(&info);

			BList list;
			be_roster->GetAppList(info.signature, &list);

			for (int32 i = 0; i < list.CountItems(); i++) {
				k_do_minimize_team(BRect(), (team_id)(addr_t)list.ItemAt(i),
					false);
			}
			break;
		}

		case B_WINDOW_RESIZED:
		{
		debug_printf("[KWindow client]{DispatchMessage} B_WINDOW_RESIZED\n");
			int32 width, height;
			if (message->FindInt32("width", &width) == B_OK
				&& message->FindInt32("height", &height) == B_OK) {
				// combine with pending resize notifications
				BMessage* pendingMessage;
				while ((pendingMessage
						= MessageQueue()->FindMessage(B_WINDOW_RESIZED, 0))) {
					int32 nextWidth;
					if (pendingMessage->FindInt32("width", &nextWidth) == B_OK)
						width = nextWidth;

					int32 nextHeight;
					if (pendingMessage->FindInt32("height", &nextHeight)
							== B_OK) {
						height = nextHeight;
					}

					MessageQueue()->RemoveMessage(pendingMessage);
					delete pendingMessage;
						// this deletes the first *additional* message
						// fCurrentMessage is safe
				}
				if (width != fFrame.Width() || height != fFrame.Height()) {
					// NOTE: we might have already handled the resize
					// in an _UPDATE_ message
					fFrame.right = fFrame.left + width;
					fFrame.bottom = fFrame.top + height;

					_AdoptResize();
//					FrameResized(width, height);
				}
// call hook function anyways
// TODO: When a window is resized programmatically,
// it receives this message, and maybe it is wise to
// keep the asynchronous nature of this process to
// not risk breaking any apps.
FrameResized(width, height);
			}
			break;
		}

		case B_WINDOW_MOVED:
		{
		debug_printf("[KWindow client]{DispatchMessage} B_WINDOW_MOVED\n");

			BPoint origin;
			if (message->FindPoint("where", &origin) == B_OK) {
				if (fFrame.LeftTop() != origin) {
					// NOTE: we might have already handled the move
					// in an _UPDATE_ message
					fFrame.OffsetTo(origin);

//					FrameMoved(origin);
				}
// call hook function anyways
// TODO: When a window is moved programmatically,
// it receives this message, and maybe it is wise to
// keep the asynchronous nature of this process to
// not risk breaking any apps.
FrameMoved(origin);
			}
			break;
		}

		case K_WINDOW_ACTIVATED:
			debug_printf("[KWindow client] {DispatchMessage} K_WINDOW_ACTIVATED\n");
			if (target != this) {
				target->MessageReceived(message);
				break;
			}

			bool active;
			if (message->FindBool("active", &active) != B_OK)
				break;

			// find latest activation message

			while (true) {
				BMessage* pendingMessage = MessageQueue()->FindMessage(
					K_WINDOW_ACTIVATED, 0);
				if (pendingMessage == NULL)
					break;

				bool nextActive;
				if (pendingMessage->FindBool("active", &nextActive) == B_OK)
					active = nextActive;

				MessageQueue()->RemoveMessage(pendingMessage);
				delete pendingMessage;
			}

			if (active != fActive) {
				fActive = active;

				WindowActivated(active);

				// call hook function 'WindowActivated(bool)' for all
				// views attached to this window.
				fTopView->_Activate(active);

				// we notify the input server if we are gaining or losing focus
				// from a view which has the B_INPUT_METHOD_AWARE on a window
				// activation
				if (!active)
					break;
				bool inputMethodAware = false;
				if (fFocus)
					inputMethodAware = fFocus->Flags() & B_INPUT_METHOD_AWARE;
				BMessage message(inputMethodAware ? IS_FOCUS_IM_AWARE_VIEW : IS_UNFOCUS_IM_AWARE_VIEW);
				BMessenger messenger(fFocus);
				BMessage reply;
				if (fFocus)
					message.AddMessenger("view", messenger);
				_control_input_server_(&message, &reply);
			}
			break;

		case B_SCREEN_CHANGED:
		debug_printf("[KWindow client]{DispatchMessage} B_SCREEN_CHANGED\n");
			if (target == this) {
				BRect frame;
				uint32 mode;
				if (message->FindRect("frame", &frame) == B_OK
					&& message->FindInt32("mode", (int32*)&mode) == B_OK) {
					// propegate message to child views
					int32 childCount = CountChildren();
					for (int32 i = 0; i < childCount; i++) {
						KView* view = ChildAt(i);
						if (view != NULL)
							view->MessageReceived(message);
					}
					// call hook method
					ScreenChanged(frame, (color_space)mode);
				}
			} else
				target->MessageReceived(message);
			break;

		case B_WORKSPACE_ACTIVATED:
		debug_printf("[KWindow client]{DispatchMessage} B_WORKSPACE_ACTIVATED\n");
			if (target == this) {
				uint32 workspace;
				bool active;
				if (message->FindInt32("workspace", (int32*)&workspace) == B_OK
					&& message->FindBool("active", &active) == B_OK)
					WorkspaceActivated(workspace, active);
			} else
				target->MessageReceived(message);
			break;

		case B_WORKSPACES_CHANGED:
		debug_printf("[KWindow client]{DispatchMessage} B_WORKSPACE_CHANGED\n");
			if (target == this) {
				uint32 oldWorkspace, newWorkspace;
				if (message->FindInt32("old", (int32*)&oldWorkspace) == B_OK
					&& message->FindInt32("new", (int32*)&newWorkspace) == B_OK)
					WorkspacesChanged(oldWorkspace, newWorkspace);
			} else
				target->MessageReceived(message);
			break;

		case B_KEY_DOWN:
			if (!_HandleKeyDown(message))
				target->MessageReceived(message);
			break;

		case B_UNMAPPED_KEY_DOWN:
			if (!_HandleUnmappedKeyDown(message))
				target->MessageReceived(message);
			break;

		case B_PULSE:
			if (target == this && fPulseRunner) {
				fTopView->_Pulse();
				fLink->Flush();
			} else
				target->MessageReceived(message);
			break;

		case _UPDATE_:
		{
		debug_printf("[KWindow] {DispatchMessage} _UPDATE_ ...\n");
//bigtime_t now = system_time();
//bigtime_t drawTime = 0;
			//STRACE(("info:KWindow handling _UPDATE_.\n"));

			fLink->StartMessage(AS_BEGIN_UPDATE);
			fInTransaction = true;

			int32 code;
			if (fLink->FlushWithReply(code) == B_OK
				&& code == B_OK) {
				// read current window position and size first,
				// the update rect is in screen coordinates...
				// so we need to be up to date
				BPoint origin;
				fLink->Read<BPoint>(&origin);
				float width;
				float height;
				fLink->Read<float>(&width);
				fLink->Read<float>(&height);

				// read tokens for views that need to be drawn
				// NOTE: we need to read the tokens completely
				// first, we cannot draw views in between reading
				// the tokens, since other communication would likely
				// mess up the data in the link.
				struct ViewUpdateInfo {
					int32 token;
					BRect updateRect;
				};
				BList infos(20);
				while (true) {
					// read next token and create/add ViewUpdateInfo
					int32 token;
					status_t error = fLink->Read<int32>(&token);
					if (error < B_OK || token == B_NULL_TOKEN)
						break;
					ViewUpdateInfo* info = new(std::nothrow) ViewUpdateInfo;
					if (info == NULL || !infos.AddItem(info)) {
						delete info;
						break;
					}
					info->token = token;
					// read culmulated update rect (is in screen coords)
					error = fLink->Read<BRect>(&(info->updateRect));
					if (error < B_OK)
						break;
				}
				// Hooks should be called after finishing reading reply because
				// they can access fLink.
				if (origin != fFrame.LeftTop()) {
					// TODO: remove code duplicatation with
					// B_WINDOW_MOVED case...
					//printf("window position was not up to date\n");
					fFrame.OffsetTo(origin);
					FrameMoved(origin);
				}
				if (width != fFrame.Width() || height != fFrame.Height()) {
					// TODO: remove code duplicatation with
					// B_WINDOW_RESIZED case...
					//printf("window size was not up to date\n");
					fFrame.right = fFrame.left + width;
					fFrame.bottom = fFrame.top + height;

					_AdoptResize();
					FrameResized(width, height);
				}

				// draw
				int32 count = infos.CountItems();
				for (int32 i = 0; i < count; i++) {
//bigtime_t drawStart = system_time();
					ViewUpdateInfo* info
						= (ViewUpdateInfo*)infos.ItemAtFast(i);
					if (KView* view = _FindView(info->token))
						view->_Draw(info->updateRect);
					else {
						printf("_UPDATE_ - didn't find view by token: %"
							B_PRId32 "\n", info->token);
					}
//drawTime += system_time() - drawStart;
				}
				// NOTE: The tokens are actually hirachically sorted,
				// so traversing the list in revers and calling
				// child->_DrawAfterChildren() actually works like intended.
				for (int32 i = count - 1; i >= 0; i--) {
					ViewUpdateInfo* info
						= (ViewUpdateInfo*)infos.ItemAtFast(i);
					if (KView* view = _FindView(info->token))
						view->_DrawAfterChildren(info->updateRect);
					delete info;
				}

//printf("  %ld views drawn, total Draw() time: %lld\n", count, drawTime);
			}

			fLink->StartMessage(AS_END_UPDATE);
			fLink->Flush();
			fInTransaction = false;
			fUpdateRequested = false;

//printf("KWindow(%s) - UPDATE took %lld usecs\n", Title(), system_time() - now);
			break;
		}

		case _MENUS_DONE_:
			MenusEnded();
			break;

		// These two are obviously some kind of old scripting messages
		// this is NOT an app_server message and we have to be cautious
		case B_WINDOW_MOVE_BY:
		{
			BPoint offset;
			if (message->FindPoint("data", &offset) == B_OK)
				MoveBy(offset.x, offset.y);
			else
				message->SendReply(B_MESSAGE_NOT_UNDERSTOOD);
			break;
		}

		// this is NOT an app_server message and we have to be cautious
		case B_WINDOW_MOVE_TO:
		{
			BPoint origin;
			if (message->FindPoint("data", &origin) == B_OK)
				MoveTo(origin);
			else
				message->SendReply(B_MESSAGE_NOT_UNDERSTOOD);
			break;
		}

		case B_LAYOUT_WINDOW:
		{
		debug_printf("[KWindow client]{DispatchMessage} B_LAYOUT_WINDOW\n");
			Layout(false);
			break;
		}

		case B_COLORS_UPDATED:
		{
			fTopView->_ColorsUpdated(message);
			target->MessageReceived(message);
			break;
		}

		case B_FONTS_UPDATED:
		{
			fTopView->_FontsUpdated(message);
			target->MessageReceived(message);
			break;
		}

		default:
		debug_printf("[KWindow client]{DispatchMessage} default case\n");
			BLooper::DispatchMessage(message, target);
			break;
	}
}


void
KWindow::FrameMoved(BPoint newPosition)
{
	// does nothing
	// Hook function
}


void
KWindow::FrameResized(float newWidth, float newHeight)
{
	// does nothing
	// Hook function
}



void
KWindow::WorkspacesChanged(uint32 oldWorkspaces, uint32 newWorkspaces)
{
	// does nothing
	// Hook function
}


void
KWindow::WorkspaceActivated(int32 workspace, bool state)
{
	// does nothing
	// Hook function
}


void
KWindow::WindowActivated(bool focus)
{
	// hook function
	// does nothing
}


void
KWindow::MenusBeginning()
{
	// does nothing
	// Hook function
}


void
KWindow::MenusEnded()
{
	// does nothing
	// Hook function
}



void
KWindow::SetSizeLimits(float minWidth, float maxWidth,
	float minHeight, float maxHeight)
{
debug_printf("[KWindow]{SetSizeLimits}start\n");
	if (minWidth > maxWidth || minHeight > maxHeight)
		return;

	if (!Lock())
		return;


debug_printf("[KWindow]{UpdateSizeLimits} sending AS_SET_SIZE_LIMITS\n");
	fLink->StartMessage(AS_SET_SIZE_LIMITS);
	fLink->Attach<float>(minWidth);
	fLink->Attach<float>(maxWidth);
	fLink->Attach<float>(minHeight);
	fLink->Attach<float>(maxHeight);

	int32 code;
	if (fLink->FlushWithReply(code) == B_OK
		&& code == B_OK) {
		// read the values that were really enforced on
		// the server side (the window frame could have
		// been changed, too)
		fLink->Read<BRect>(&fFrame);
		fLink->Read<float>(&fMinWidth);
		fLink->Read<float>(&fMaxWidth);
		fLink->Read<float>(&fMinHeight);
		fLink->Read<float>(&fMaxHeight);

		_AdoptResize();
			// TODO: the same has to be done for SetLook() (that can alter
			//		the size limits, and hence, the size of the window
	}
	Unlock();

debug_printf("[KWindow]{SetSizeLimits}end\n");
}


void
KWindow::GetSizeLimits(float* _minWidth, float* _maxWidth, float* _minHeight,
	float* _maxHeight)
{
	// TODO: What about locking?!?
	if (_minHeight != NULL)
		*_minHeight = fMinHeight;
	if (_minWidth != NULL)
		*_minWidth = fMinWidth;
	if (_maxHeight != NULL)
		*_maxHeight = fMaxHeight;
	if (_maxWidth != NULL)
		*_maxWidth = fMaxWidth;
}


void
KWindow::UpdateSizeLimits()
{
debug_printf("[KWindow]{UpdateSizeLimits}start\n");
	BAutolock locker(this);

	if ((fFlags & B_AUTO_UPDATE_SIZE_LIMITS) != 0) {
	debug_printf("[KWindow]{UpdateSizeLimits}fFlags\n");
		// Get min/max constraints of the top view and enforce window
		// size limits respectively.
		BSize minSize = fTopView->MinSize();
		BSize maxSize = fTopView->MaxSize();
		SetSizeLimits(minSize.width, maxSize.width,
			minSize.height, maxSize.height);
	}
debug_printf("[KWindow]{UpdateSizeLimits}end\n");
}


status_t
KWindow::SetDecoratorSettings(const BMessage& settings)
{
	// flatten the given settings into a buffer and send
	// it to the app_server to apply the settings to the
	// decorator

	int32 size = settings.FlattenedSize();
	char buffer[size];
	status_t status = settings.Flatten(buffer, size);
	if (status != B_OK)
		return status;

	if (!Lock())
		return B_ERROR;

	status = fLink->StartMessage(AS_SET_DECORATOR_SETTINGS);// tododo

	if (status == B_OK)
		status = fLink->Attach<int32>(size);

	if (status == B_OK)
		status = fLink->Attach(buffer, size);

	if (status == B_OK)
		status = fLink->Flush();

	Unlock();

	return status;
}


status_t
KWindow::GetDecoratorSettings(BMessage* settings) const
{
	// read a flattened settings message from the app_server
	// and put it into settings

	if (!const_cast<KWindow*>(this)->Lock())
		return B_ERROR;

	status_t status = fLink->StartMessage(AS_GET_DECORATOR_SETTINGS);// tododo

	if (status == B_OK) {
		int32 code;
		status = fLink->FlushWithReply(code);
		if (status == B_OK && code != B_OK)
			status = code;
	}

	if (status == B_OK) {
		int32 size;
		status = fLink->Read<int32>(&size);
		if (status == B_OK) {
			char buffer[size];
			status = fLink->Read(buffer, size);
			if (status == B_OK) {
				status = settings->Unflatten(buffer);
			}
		}
	}

	const_cast<KWindow*>(this)->Unlock();

	return status;
}


void
KWindow::SetZoomLimits(float maxWidth, float maxHeight)
{
	// TODO: What about locking?!?
	if (maxWidth > fMaxWidth)
		maxWidth = fMaxWidth;
	fMaxZoomWidth = maxWidth;

	if (maxHeight > fMaxHeight)
		maxHeight = fMaxHeight;
	fMaxZoomHeight = maxHeight;
}


void
KWindow::Zoom(BPoint origin, float width, float height)
{
	// the default implementation of this hook function
	// just does the obvious:
	MoveTo(origin);
	ResizeTo(width, height);
}


void
KWindow::Zoom()
{
	// TODO: What about locking?!?

	// From BeBook:
	// The dimensions that non-virtual Zoom() passes to hook Zoom() are deduced
	// from the smallest of three rectangles:

	// 1) the rectangle defined by SetZoomLimits() and,
	// 2) the rectangle defined by SetSizeLimits()
	float maxZoomWidth = std::min(fMaxZoomWidth, fMaxWidth);
	float maxZoomHeight = std::min(fMaxZoomHeight, fMaxHeight);

	// 3) the screen rectangle
	BRect screenFrame = (BScreen(this)).Frame();
	maxZoomWidth = std::min(maxZoomWidth, screenFrame.Width());
	maxZoomHeight = std::min(maxZoomHeight, screenFrame.Height());

	BRect zoomArea = screenFrame; // starts at screen size

	KDeskbar deskbar;
	BRect deskbarFrame = deskbar.Frame();
	bool isShiftDown = (modifiers() & B_SHIFT_KEY) != 0;
	if (!isShiftDown && !deskbar.IsAutoHide()) {
		// remove area taken up by Deskbar unless hidden or shift is held down
		switch (deskbar.Location()) {
			case B_DESKBAR_TOP:
				zoomArea.top = deskbarFrame.bottom + 2;
				break;

			case B_DESKBAR_BOTTOM:
			case B_DESKBAR_LEFT_BOTTOM:
			case B_DESKBAR_RIGHT_BOTTOM:
				zoomArea.bottom = deskbarFrame.top - 2;
				break;

			// in vertical expando mode only if not always-on-top or auto-raise
			case B_DESKBAR_LEFT_TOP:
				if (!deskbar.IsExpanded())
					zoomArea.top = deskbarFrame.bottom + 2;
				else if (!deskbar.IsAlwaysOnTop() && !deskbar.IsAutoRaise())
					zoomArea.left = deskbarFrame.right + 2;
				break;

			default:
			case B_DESKBAR_RIGHT_TOP:
				if (!deskbar.IsExpanded())
					break;
				else if (!deskbar.IsAlwaysOnTop() && !deskbar.IsAutoRaise())
					zoomArea.right = deskbarFrame.left - 2;
				break;
		}
	}

	// TODO: Broken for tab on left side windows...
	float borderWidth;
	float tabHeight;
	_GetDecoratorSize(&borderWidth, &tabHeight);

	// remove the area taken up by the tab and border
	zoomArea.left += borderWidth;
	zoomArea.top += borderWidth + tabHeight;
	zoomArea.right -= borderWidth;
	zoomArea.bottom -= borderWidth;

	// inset towards center vertically first to see if there will be room
	// above or below Deskbar
	if (zoomArea.Height() > maxZoomHeight)
		zoomArea.InsetBy(0, roundf((zoomArea.Height() - maxZoomHeight) / 2));

	if (zoomArea.top > deskbarFrame.bottom
		|| zoomArea.bottom < deskbarFrame.top) {
		// there is room above or below Deskbar, start from screen width
		// minus borders instead of desktop width minus borders
		zoomArea.left = screenFrame.left + borderWidth;
		zoomArea.right = screenFrame.right - borderWidth;
	}

	// inset towards center
	if (zoomArea.Width() > maxZoomWidth)
		zoomArea.InsetBy(roundf((zoomArea.Width() - maxZoomWidth) / 2), 0);

	// Un-Zoom

	if (fPreviousFrame.IsValid()
		// NOTE: don't check for fFrame.LeftTop() == zoomArea.LeftTop()
		// -> makes it easier on the user to get a window back into place
		&& fFrame.Width() == zoomArea.Width()
		&& fFrame.Height() == zoomArea.Height()) {
		// already zoomed!
		Zoom(fPreviousFrame.LeftTop(), fPreviousFrame.Width(),
			fPreviousFrame.Height());
		return;
	}

	// Zoom

	// remember fFrame for later "unzooming"
	fPreviousFrame = fFrame;

	Zoom(zoomArea.LeftTop(), zoomArea.Width(), zoomArea.Height());
}


void
KWindow::ScreenChanged(BRect screenSize, color_space depth)
{
	// Hook function
}




void
KWindow::SetPulseRate(bigtime_t rate)
{
	// TODO: What about locking?!?
	if (rate < 0
		|| (rate == fPulseRate && !((rate == 0) ^ (fPulseRunner == NULL))))
		return;

	fPulseRate = rate;

	if (rate > 0) {
		if (fPulseRunner == NULL) {
			BMessage message(B_PULSE);
			fPulseRunner = new(std::nothrow) BMessageRunner(BMessenger(this),
				&message, rate);
		} else {
			fPulseRunner->SetInterval(rate);
		}
	} else {
		// rate == 0
		delete fPulseRunner;
		fPulseRunner = NULL;
	}
}


bigtime_t
KWindow::PulseRate() const
{
	return fPulseRate;
}


void
KWindow::AddShortcut(uint32 key, uint32 modifiers, KMenuItem* item)
{
	Shortcut* shortcut = new(std::nothrow) Shortcut(key, modifiers, item);
	if (shortcut == NULL)
		return;

	// removes the shortcut if it already exists!
	RemoveShortcut(key, modifiers);

	fShortcuts.AddItem(shortcut);
}


void
KWindow::AddShortcut(uint32 key, uint32 modifiers, BMessage* message)
{
	AddShortcut(key, modifiers, message, this);
}


void
KWindow::AddShortcut(uint32 key, uint32 modifiers, BMessage* message,
	BHandler* target)
{
	if (message == NULL)
		return;

	Shortcut* shortcut = new(std::nothrow) Shortcut(key, modifiers, message,
		target);
	if (shortcut == NULL)
		return;

	// removes the shortcut if it already exists!
	RemoveShortcut(key, modifiers);

	fShortcuts.AddItem(shortcut);
}


bool
KWindow::HasShortcut(uint32 key, uint32 modifiers)
{
	return _FindShortcut(key, modifiers) != NULL;
}


void
KWindow::RemoveShortcut(uint32 key, uint32 modifiers)
{
	Shortcut* shortcut = _FindShortcut(key, modifiers);
	if (shortcut != NULL) {
		fShortcuts.RemoveItem(shortcut);
		delete shortcut;
	} else if ((key == 'q' || key == 'Q') && modifiers == B_COMMAND_KEY) {
		// the quit shortcut is a fake shortcut
		fNoQuitShortcut = true;
	}
}


KButton*
KWindow::DefaultButton() const
{
	// TODO: What about locking?!?
	return fDefaultButton;
}


void
KWindow::SetDefaultButton(KButton* button)
{
	// TODO: What about locking?!?
	if (fDefaultButton == button)
		return;

	if (fDefaultButton != NULL) {
		// tell old button it's no longer the default one
		KButton* oldDefault = fDefaultButton;
		oldDefault->MakeDefault(false);
		oldDefault->Invalidate();
	}

	fDefaultButton = button;

	if (button != NULL) {
		// notify new default button
		fDefaultButton->MakeDefault(true);
		fDefaultButton->Invalidate();
	}
}



bool
KWindow::NeedsUpdate() const
{
	if (!const_cast<KWindow*>(this)->Lock())
		return false;

	fLink->StartMessage(AS_NEEDS_UPDATE);//tododo

	int32 code = B_ERROR;
	fLink->FlushWithReply(code);

	const_cast<KWindow*>(this)->Unlock();

	return code == B_OK;
}


void
KWindow::UpdateIfNeeded()
{
	// works only from the window thread
	if (find_thread(NULL) != Thread())
		return;

	// if the queue is already locked we are called recursivly
	// from our own dispatched update message
	if (((const BMessageQueue*)MessageQueue())->IsLocked())
		return;

	if (!Lock())
		return;

	// make sure all requests that would cause an update have
	// arrived at the server
	Sync();

	// Since we're blocking the event loop, we need to retrieve
	// all messages that are pending on the port.
	_DequeueAll();

	BMessageQueue* queue = MessageQueue();

	// First process and remove any _UPDATE_ message in the queue
	// With the current design, there can only be one at a time

	while (true) {
		queue->Lock();

		BMessage* message = queue->FindMessage(_UPDATE_, 0);
		queue->RemoveMessage(message);

		queue->Unlock();

		if (message == NULL)
			break;

		KWindow::DispatchMessage(message, this);
		delete message;
	}

	Unlock();
}



KView*
KWindow::FindView(const char* viewName) const
{
	BAutolock locker(const_cast<KWindow*>(this));
	if (!locker.IsLocked())
		return NULL;

	return fTopView->FindView(viewName);
}


KView*
KWindow::FindView(BPoint point) const
{
	BAutolock locker(const_cast<KWindow*>(this));
	if (!locker.IsLocked())
		return NULL;

	// point is assumed to be in window coordinates,
	// fTopView has same bounds as window
	return _FindView(fTopView, point);
}


void
KWindow::Activate(bool active)
{
debug_printf("[KWindow]{Activate}\n");

	if (!Lock())
		return;

	if (!IsHidden()) {
		fMinimized = false;
			// activating a window will also unminimize it

		fLink->StartMessage(AS_ACTIVATE_WINDOW);// tododo
		fLink->Attach<bool>(active);
		fLink->Flush();
	}

	Unlock();

debug_printf("[KWindow]{Activate}end\n");
}


void
KWindow::ConvertToScreen(BPoint* point) const
{
	point->x += fFrame.left;
	point->y += fFrame.top;
}


BPoint
KWindow::ConvertToScreen(BPoint point) const
{
	return point + fFrame.LeftTop();
}


void
KWindow::ConvertFromScreen(BPoint* point) const
{
	point->x -= fFrame.left;
	point->y -= fFrame.top;
}


BPoint
KWindow::ConvertFromScreen(BPoint point) const
{
	return point - fFrame.LeftTop();
}


void
KWindow::ConvertToScreen(BRect* rect) const
{
	rect->OffsetBy(fFrame.LeftTop());
}


BRect
KWindow::ConvertToScreen(BRect rect) const
{
	return rect.OffsetByCopy(fFrame.LeftTop());
}


void
KWindow::ConvertFromScreen(BRect* rect) const
{
	rect->OffsetBy(-fFrame.left, -fFrame.top);
}


BRect
KWindow::ConvertFromScreen(BRect rect) const
{
	return rect.OffsetByCopy(-fFrame.left, -fFrame.top);
}


bool
KWindow::IsMinimized() const
{
	BAutolock locker(const_cast<KWindow*>(this));
	if (!locker.IsLocked())
		return false;

	return fMinimized;
}


BRect
KWindow::Bounds() const
{
	return BRect(0, 0, fFrame.Width(), fFrame.Height());
}


BRect
KWindow::Frame() const
{
	return fFrame;
}


BRect
KWindow::DecoratorFrame() const
{
	BRect decoratorFrame(Frame());
	BRect tabRect(0, 0, 0, 0);

	float borderWidth = 5.0;

	BMessage settings;
	if (GetDecoratorSettings(&settings) == B_OK) {
		settings.FindRect("tab frame", &tabRect);
		settings.FindFloat("border width", &borderWidth);
	} else {
		// probably no-border window look
		if (fLook == B_NO_BORDER_WINDOW_LOOK)
			borderWidth = 0.f;
		else if (fLook == B_BORDERED_WINDOW_LOOK)
			borderWidth = 1.f;
		// else use fall-back values from above
	}

	if (fLook == kLeftTitledWindowLook) {
		decoratorFrame.top -= borderWidth;
		decoratorFrame.left -= borderWidth + tabRect.Width();
		decoratorFrame.right += borderWidth;
		decoratorFrame.bottom += borderWidth;
	} else {
		decoratorFrame.top -= borderWidth + tabRect.Height();
		decoratorFrame.left -= borderWidth;
		decoratorFrame.right += borderWidth;
		decoratorFrame.bottom += borderWidth;
	}

	return decoratorFrame;
}


BSize
KWindow::Size() const
{
	return BSize(fFrame.Width(), fFrame.Height());
}


const char*
KWindow::Title() const
{
	return fTitle;
}


void
KWindow::SetTitle(const char* title)
{
	if (title == NULL)
		title = "";

	free(fTitle);
	fTitle = strdup(title);

	_SetName(title);

	// we notify the app_server so we can actually see the change
	if (Lock()) {
		fLink->StartMessage(AS_SET_WINDOW_TITLE); //khidki
		fLink->AttachString(fTitle);
		fLink->Flush();
		Unlock();
	}
}


bool
KWindow::IsActive() const
{
	return fActive;
}



void
KWindow::SetKeyMenuBar(KMenuBar* bar)
{
	fKeyMenuBar = bar;
}


KMenuBar*
KWindow::KeyMenuBar() const
{
	return fKeyMenuBar;
}


status_t
KWindow::SetFlags(uint32 flags)
{
	BAutolock locker(this);
	if (!locker.IsLocked())
		return B_BAD_VALUE;

	fLink->StartMessage(AS_SET_FLAGS);
	fLink->Attach<uint32>(flags);

	int32 status = B_ERROR;
	if (fLink->FlushWithReply(status) == B_OK && status == B_OK)
		fFlags = flags;

	return status;
}


uint32
KWindow::Flags() const
{
	return fFlags;
}



status_t
KWindow::SetWindowAlignment(window_alignment mode,
	int32 h, int32 hOffset, int32 width, int32 widthOffset,
	int32 v, int32 vOffset, int32 height, int32 heightOffset)
{
	if ((mode & (B_BYTE_ALIGNMENT | B_PIXEL_ALIGNMENT)) == 0
		|| (hOffset >= 0 && hOffset <= h)
		|| (vOffset >= 0 && vOffset <= v)
		|| (widthOffset >= 0 && widthOffset <= width)
		|| (heightOffset >= 0 && heightOffset <= height))
		return B_BAD_VALUE;

	// TODO: test if hOffset = 0 and set it to 1 if true.

	if (!Lock())
		return B_ERROR;

	fLink->StartMessage(AS_SET_ALIGNMENT);// tododo
	fLink->Attach<int32>((int32)mode);
	fLink->Attach<int32>(h);
	fLink->Attach<int32>(hOffset);
	fLink->Attach<int32>(width);
	fLink->Attach<int32>(widthOffset);
	fLink->Attach<int32>(v);
	fLink->Attach<int32>(vOffset);
	fLink->Attach<int32>(height);
	fLink->Attach<int32>(heightOffset);

	status_t status = B_ERROR;
	fLink->FlushWithReply(status);

	Unlock();

	return status;
}


status_t
KWindow::GetWindowAlignment(window_alignment* mode,
	int32* h, int32* hOffset, int32* width, int32* widthOffset,
	int32* v, int32* vOffset, int32* height, int32* heightOffset) const
{
	if (!const_cast<KWindow*>(this)->Lock())
		return B_ERROR;

	fLink->StartMessage(AS_GET_ALIGNMENT); // tododo

	status_t status;
	if (fLink->FlushWithReply(status) == B_OK && status == B_OK) {
		fLink->Read<int32>((int32*)mode);
		fLink->Read<int32>(h);
		fLink->Read<int32>(hOffset);
		fLink->Read<int32>(width);
		fLink->Read<int32>(widthOffset);
		fLink->Read<int32>(v);
		fLink->Read<int32>(hOffset);
		fLink->Read<int32>(height);
		fLink->Read<int32>(heightOffset);
	}

	const_cast<KWindow*>(this)->Unlock();
	return status;
}



uint32
KWindow::Workspaces() const
{
debug_printf("[KWindow] Workspaces()\n");
	if (!const_cast<KWindow*>(this)->Lock())
		return 0;

	uint32 workspaces = 0;

	fLink->StartMessage(AS_GET_WORKSPACES); // tododo

	status_t status;
	if (fLink->FlushWithReply(status) == B_OK && status == B_OK)
		fLink->Read<uint32>(&workspaces);

	const_cast<KWindow*>(this)->Unlock();
	return workspaces;
}


void
KWindow::SetWorkspaces(uint32 workspaces)
{
debug_printf("[KWindow] SetWorkspaces()\n");
	// TODO: don't forget about Tracker's background window.
	if (fFeel != B_NORMAL_WINDOW_FEEL)
		return;

	if (Lock()) {
		fLink->StartMessage(AS_SET_WORKSPACES);// tododo
		fLink->Attach<uint32>(workspaces);
		fLink->Flush();
		Unlock();
	}
}


KView*
KWindow::LastMouseMovedView() const
{
	return fLastMouseMovedView;
}


bool
KWindow::IsModal() const
{
	return fFeel == B_MODAL_SUBSET_WINDOW_FEEL
		|| fFeel == B_MODAL_APP_WINDOW_FEEL
		|| fFeel == B_MODAL_ALL_WINDOW_FEEL
		|| fFeel == kMenuWindowFeel;
}


bool
KWindow::IsFloating() const
{
	return fFeel == B_FLOATING_SUBSET_WINDOW_FEEL
		|| fFeel == B_FLOATING_APP_WINDOW_FEEL
		|| fFeel == B_FLOATING_ALL_WINDOW_FEEL;
}


status_t
KWindow::AddToSubset(KWindow* window)
{
	if (window == NULL || window->Feel() != B_NORMAL_WINDOW_FEEL
		|| (fFeel != B_MODAL_SUBSET_WINDOW_FEEL
			&& fFeel != B_FLOATING_SUBSET_WINDOW_FEEL))
		return B_BAD_VALUE;

	if (!Lock())
		return B_ERROR;

	status_t status = B_ERROR;
	fLink->StartMessage(AS_ADD_TO_SUBSET);// tododo
	fLink->Attach<int32>(_get_object_token_(window));
	fLink->FlushWithReply(status);

	Unlock();

	return status;
}


status_t
KWindow::RemoveFromSubset(KWindow* window)
{
	if (window == NULL || window->Feel() != B_NORMAL_WINDOW_FEEL
		|| (fFeel != B_MODAL_SUBSET_WINDOW_FEEL
			&& fFeel != B_FLOATING_SUBSET_WINDOW_FEEL))
		return B_BAD_VALUE;

	if (!Lock())
		return B_ERROR;

	status_t status = B_ERROR;
	fLink->StartMessage(AS_REMOVE_FROM_SUBSET);// tododo
	fLink->Attach<int32>(_get_object_token_(window));
	fLink->FlushWithReply(status);

	Unlock();

	return status;
}


status_t
KWindow::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_SET_LAYOUT:
		{
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KWindow::SetLayout(data->layout);
			return B_OK;
}
	}

	return BLooper::Perform(code, _data);
}


status_t
KWindow::SetType(window_type type)
{
	window_look look;
	window_feel feel;
	_DecomposeType(type, &look, &feel);

	status_t status = SetLook(look);
	if (status == B_OK)
		status = SetFeel(feel);

	return status;
}


window_type
KWindow::Type() const
{
	return _ComposeType(fLook, fFeel);
}


status_t
KWindow::SetLook(window_look look)
{
	BAutolock locker(this);
	if (!locker.IsLocked())
		return B_BAD_VALUE;

	fLink->StartMessage(AS_SET_LOOK);// tododo
	fLink->Attach<int32>((int32)look);

	status_t status = B_ERROR;
	if (fLink->FlushWithReply(status) == B_OK && status == B_OK)
		fLook = look;

	// TODO: this could have changed the window size, and thus, we
	//	need to get it from the server (and call _AdoptResize()).

	return status;
}


window_look
KWindow::Look() const
{
	return fLook;
}


status_t
KWindow::SetFeel(window_feel feel)
{
	BAutolock locker(this);
	if (!locker.IsLocked())
		return B_BAD_VALUE;

	fLink->StartMessage(AS_SET_FEEL);// tododo
	fLink->Attach<int32>((int32)feel);

	status_t status = B_ERROR;
	if (fLink->FlushWithReply(status) == B_OK && status == B_OK)
		fFeel = feel;

	return status;
}


window_feel
KWindow::Feel() const
{
	return fFeel;
}


//! Rename the handler and its thread
void
KWindow::_SetName(const char* title)
{
	if (title == NULL)
		title = "";

	// we will change KWindow's thread name to "w>window title"

	char threadName[B_OS_NAME_LENGTH];
	strcpy(threadName, "w>");
#ifdef __HAIKU__
	strlcat(threadName, title, B_OS_NAME_LENGTH);
#else
	int32 length = strlen(title);
	length = min_c(length, B_OS_NAME_LENGTH - 3);
	memcpy(threadName + 2, title, length);
	threadName[length + 2] = '\0';
#endif

	// change the handler's name
	SetName(threadName);

	// if the message loop has been started...
	if (Thread() >= B_OK)
		rename_thread(Thread(), threadName);
}



//!	Reads all pending messages from the window port and put them into the queue.
void
KWindow::_DequeueAll()
{
	//	Get message count from port
	int32 count = port_count(fMsgPort);

	for (int32 i = 0; i < count; i++) {
		BMessage* message = MessageFromPort(0);
		if (message != NULL)
			fDirectTarget->Queue()->AddMessage(message);
	}
}


/*!	This here is an almost complete code duplication to BLooper::task_looper()
	but with some important differences:
	 a)	it uses the _DetermineTarget() method to tell what the later target of
		a message will be, if no explicit target is supplied.
	 b)	it calls _UnpackMessage() and _SanitizeMessage() to duplicate the message
		to all of its intended targets, and to add all fields the target would
		expect in such a message.

	This is important because the app_server sends all input events to the
	preferred handler, and expects them to be correctly distributed to their
	intended targets.
*/
void
KWindow::task_looper()
{
debug_printf("[KWindow]{task_looper}\n");

	//STRACE(("info: KWindow::task_looper() started.\n"));

	// Check that looper is locked (should be)
	AssertLocked();
	Unlock();

	if (IsLocked())
		debugger("window must not be locked!");

	while (!fTerminating) {
		// Did we get a message?
		BMessage* msg = MessageFromPort();
		if (msg)
			_AddMessagePriv(msg);

		//	Get message count from port
		int32 msgCount = port_count(fMsgPort);
		for (int32 i = 0; i < msgCount; ++i) {
			// Read 'count' messages from port (so we will not block)
			// We use zero as our timeout since we know there is stuff there
			msg = MessageFromPort(0);
			// Add messages to queue
			if (msg)
				_AddMessagePriv(msg);
		}

		bool dispatchNextMessage = true;
		while (!fTerminating && dispatchNextMessage) {
			// Get next message from queue (assign to fLastMessage after
			// locking)
			BMessage* message = fDirectTarget->Queue()->NextMessage();

			// Lock the looper
			if (!Lock()) {
				delete message;
				break;
			}

			fLastMessage = message;

			if (fLastMessage == NULL) {
				// No more messages: Unlock the looper and terminate the
				// dispatch loop.
				dispatchNextMessage = false;
			} else {
				// Get the target handler
				BMessage::Private messagePrivate(fLastMessage);
				bool usePreferred = messagePrivate.UsePreferredTarget();
				BHandler* handler = NULL;
				bool dropMessage = false;

				if (usePreferred) {
					handler = PreferredHandler();
					if (handler == NULL)
						handler = this;
				} else {
					gDefaultTokens.GetToken(messagePrivate.GetTarget(),
						B_HANDLER_TOKEN, (void**)&handler);

					// if this handler doesn't belong to us, we drop the message
					if (handler != NULL && handler->Looper() != this) {
						dropMessage = true;
						handler = NULL;
					}
				}

				if ((handler == NULL && !dropMessage) || usePreferred)
					handler = _DetermineTarget(fLastMessage, handler);

				unpack_cookie cookie;
				while (_UnpackMessage(cookie, &fLastMessage, &handler, &usePreferred)) {
					// if there is no target handler, the message is dropped
					if (handler != NULL) {
						_SanitizeMessage(fLastMessage, handler, usePreferred);

						// Is this a scripting message?
						if (fLastMessage->HasSpecifiers()) {
							int32 index = 0;
							// Make sure the current specifier is kosher
							if (fLastMessage->GetCurrentSpecifier(&index) == B_OK)
								handler = resolve_specifier(handler, fLastMessage);
						}

						if (handler != NULL)
							handler = _TopLevelFilter(fLastMessage, handler);

						if (handler != NULL)
							DispatchMessage(fLastMessage, handler);
					}

					// Delete the current message
					delete fLastMessage;
					fLastMessage = NULL;
				}
			}

			if (fTerminating) {
				// we leave the looper locked when we quit
				return;
			}

			Unlock();

			// Are any messages on the port?
			if (port_count(fMsgPort) > 0) {
				// Do outer loop
				dispatchNextMessage = false;
			}
		}
	}
}


window_type
KWindow::_ComposeType(window_look look, window_feel feel) const
{
	switch (feel) {
		case B_NORMAL_WINDOW_FEEL:
			switch (look) {
				case B_TITLED_WINDOW_LOOK:
					return B_TITLED_WINDOW;

				case B_DOCUMENT_WINDOW_LOOK:
					return B_DOCUMENT_WINDOW;

				case B_BORDERED_WINDOW_LOOK:
					return B_BORDERED_WINDOW;

				default:
					return B_UNTYPED_WINDOW;
			}
			break;

		case B_MODAL_APP_WINDOW_FEEL:
			if (look == B_MODAL_WINDOW_LOOK)
				return B_MODAL_WINDOW;
			break;

		case B_FLOATING_APP_WINDOW_FEEL:
			if (look == B_FLOATING_WINDOW_LOOK)
				return B_FLOATING_WINDOW;
			break;

		default:
			return B_UNTYPED_WINDOW;
	}

	return B_UNTYPED_WINDOW;
}


void
KWindow::_DecomposeType(window_type type, window_look* _look,
	window_feel* _feel) const
{
	switch (type) {
		case B_DOCUMENT_WINDOW:
			*_look = B_DOCUMENT_WINDOW_LOOK;
			*_feel = B_NORMAL_WINDOW_FEEL;
			break;

		case B_MODAL_WINDOW:
			*_look = B_MODAL_WINDOW_LOOK;
			*_feel = B_MODAL_APP_WINDOW_FEEL;
			break;

		case B_FLOATING_WINDOW:
			*_look = B_FLOATING_WINDOW_LOOK;
			*_feel = B_FLOATING_APP_WINDOW_FEEL;
			break;

		case B_BORDERED_WINDOW:
			*_look = B_BORDERED_WINDOW_LOOK;
			*_feel = B_NORMAL_WINDOW_FEEL;
			break;

		case B_TITLED_WINDOW:
		case B_UNTYPED_WINDOW:
		default:
			*_look = B_TITLED_WINDOW_LOOK;
			*_feel = B_NORMAL_WINDOW_FEEL;
			break;
	}
}


KView*
KWindow::CurrentFocus() const
{
	return fFocus;
}



void
KWindow::MoveBy(float dx, float dy)
{
	if ((dx != 0.0f || dy != 0.0f) && Lock()) {
		MoveTo(fFrame.left + dx, fFrame.top + dy);
		Unlock();
	}
}


void
KWindow::MoveTo(BPoint point)
{
	MoveTo(point.x, point.y);
}


void
KWindow::MoveTo(float x, float y)
{
	if (!Lock())
		return;

	x = roundf(x);
	y = roundf(y);

	if (fFrame.left != x || fFrame.top != y) {
		fLink->StartMessage(AS_WINDOW_MOVE);// tododo
		fLink->Attach<float>(x);
		fLink->Attach<float>(y);

		status_t status;
		if (fLink->FlushWithReply(status) == B_OK && status == B_OK)
			fFrame.OffsetTo(x, y);
	}

	Unlock();
}


void
KWindow::ResizeBy(float dx, float dy)
{
	if (Lock()) {
		ResizeTo(fFrame.Width() + dx, fFrame.Height() + dy);
		Unlock();
	}
}


void
KWindow::ResizeTo(float width, float height)
{
	if (!Lock())
		return;

	width = roundf(width);
	height = roundf(height);

	// stay in minimum & maximum frame limits
	if (width < fMinWidth)
		width = fMinWidth;
	else if (width > fMaxWidth)
		width = fMaxWidth;

	if (height < fMinHeight)
		height = fMinHeight;
	else if (height > fMaxHeight)
		height = fMaxHeight;

	if (width != fFrame.Width() || height != fFrame.Height()) {
		fLink->StartMessage(AS_WINDOW_RESIZE);// tododo
		fLink->Attach<float>(width);
		fLink->Attach<float>(height);

		status_t status;
		if (fLink->FlushWithReply(status) == B_OK && status == B_OK) {
			fFrame.right = fFrame.left + width;
			fFrame.bottom = fFrame.top + height;
			_AdoptResize();
		}
	}

	Unlock();
}


void
KWindow::ResizeToPreferred()
{
	BAutolock locker(this);
	Layout(false);

	float width = fTopView->PreferredSize().width;
	width = std::min(width, fTopView->MaxSize().width);
	width = std::max(width, fTopView->MinSize().width);

	float height = fTopView->PreferredSize().height;
	height = std::min(width, fTopView->MaxSize().height);
	height = std::max(width, fTopView->MinSize().height);

	if (GetLayout()->HasHeightForWidth())
		GetLayout()->GetHeightForWidth(width, NULL, NULL, &height);

	ResizeTo(width, height);
}


void
KWindow::CenterIn(const BRect& rect)
{
	BAutolock locker(this);

	// Set size limits now if needed
	UpdateSizeLimits();

	MoveTo(KLayoutUtils::AlignInFrame(rect, Size(),
		BAlignment(B_ALIGN_HORIZONTAL_CENTER,
			B_ALIGN_VERTICAL_CENTER)).LeftTop());
	MoveOnScreen(B_DO_NOT_RESIZE_TO_FIT | B_MOVE_IF_PARTIALLY_OFFSCREEN);
}


void
KWindow::CenterOnScreen()
{
	CenterIn(BScreen(this).Frame());
}


// Centers the window on the screen with the passed in id.
void
KWindow::CenterOnScreen(screen_id id)
{
	CenterIn(BScreen(id).Frame());
}


void
KWindow::MoveOnScreen(uint32 flags)
{
	// Set size limits now if needed
	UpdateSizeLimits();

	BRect screenFrame = BScreen(this).Frame();
	BRect frame = Frame();

	float borderWidth;
	float tabHeight;
	_GetDecoratorSize(&borderWidth, &tabHeight);

	frame.InsetBy(-borderWidth, -borderWidth);
	frame.top -= tabHeight;

	if ((flags & B_DO_NOT_RESIZE_TO_FIT) == 0) {
		// Make sure the window fits on the screen
		if (frame.Width() > screenFrame.Width())
			frame.right -= frame.Width() - screenFrame.Width();
		if (frame.Height() > screenFrame.Height())
			frame.bottom -= frame.Height() - screenFrame.Height();

		BRect innerFrame = frame;
		innerFrame.top += tabHeight;
		innerFrame.InsetBy(borderWidth, borderWidth);
		ResizeTo(innerFrame.Width(), innerFrame.Height());
	}

	if (((flags & B_MOVE_IF_PARTIALLY_OFFSCREEN) == 0
			&& !screenFrame.Contains(frame))
		|| !frame.Intersects(screenFrame)) {
		// Off and away
		CenterOnScreen();
		return;
	}

	// Move such that the upper left corner, and most of the window
	// will be visible.
	float left = frame.left;
	if (left < screenFrame.left)
		left = screenFrame.left;
	else if (frame.right > screenFrame.right)
		left = std::max(0.f, screenFrame.right - frame.Width());

	float top = frame.top;
	if (top < screenFrame.top)
		top = screenFrame.top;
	else if (frame.bottom > screenFrame.bottom)
		top = std::max(0.f, screenFrame.bottom - frame.Height());

	if (top != frame.top || left != frame.left)
		MoveTo(left + borderWidth, top + tabHeight + borderWidth);
}



void
KWindow::Show()
{
debug_printf("[KWindow] {Show} \n");

	bool runCalled = true;
	if (Lock()) {
		fShowLevel--;

debug_printf("[KWindow] {Show} before _SendShowOrHideMessage\n");
		_SendShowOrHideMessage();
debug_printf("[KWindow] {Show} after _SendShowOrHideMessage\n");

		runCalled = fRunCalled;

		Unlock();
	}

	if (!runCalled) {
		// This is the fist time Show() is called, which implicitly runs the
		// looper. NOTE: The window is still locked if it has not been
		// run yet, so accessing members is safe.
		if (fLink->SenderPort() < B_OK) {
			// We don't have valid app_server connection; there is no point
			// in starting our looper
			fThread = B_ERROR;
			return;
		} else
			Run();
	}

debug_printf("[KWindow] {Show}ends\n");
}


void
KWindow::Hide()
{
	if (Lock()) {
		// If we are minimized and are about to be hidden, unminimize
		if (IsMinimized() && fShowLevel == 0)
			Minimize(false);

		fShowLevel++;

		_SendShowOrHideMessage();

		Unlock();
	}
}


bool
KWindow::IsHidden() const
{
	return fShowLevel > 0;
}


bool
KWindow::QuitRequested()
{
	return BLooper::QuitRequested();
}


thread_id
KWindow::Run()
{
	return BLooper::Run();
}


void
KWindow::SetLayout(KLayout* layout)
{
	// Adopt layout's colors for fTopView
	if (layout != NULL)
		fTopView->AdoptViewColors(layout->View());

	fTopView->SetLayout(layout);
}


KLayout*
KWindow::GetLayout() const
{
	return fTopView->GetLayout();
}



void
KWindow::InvalidateLayout(bool descendants)
{
	fTopView->InvalidateLayout(descendants);
}


void
KWindow::Layout(bool force)
{
debug_printf("[KWindow]{Layout}start\n");
	UpdateSizeLimits();

	// Do the actual layout
	fTopView->Layout(force);

debug_printf("[KWindow]{Layout}end\n");
}


bool
KWindow::IsOffscreenWindow() const
{
	return fOffscreen;
}


status_t
KWindow::GetSupportedSuites(BMessage* data)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t status = data->AddString("suites", "suite/vnd.Be-window");
	if (status == B_OK) {
		BPropertyInfo propertyInfo(sWindowPropInfo, sWindowValueInfo);

		status = data->AddFlat("messages", &propertyInfo);
		if (status == B_OK)
			status = BLooper::GetSupportedSuites(data);
	}

	return status;
}



BHandler*
KWindow::ResolveSpecifier(BMessage* message, int32 index, BMessage* specifier,
	int32 what, const char* property)
{
	if (message->what == B_WINDOW_MOVE_BY
		|| message->what == B_WINDOW_MOVE_TO)
		return this;

	BPropertyInfo propertyInfo(sWindowPropInfo);
	if (propertyInfo.FindMatch(message, index, specifier, what, property) >= 0) {
		if (strcmp(property, "View") == 0) {
			// we will NOT pop the current specifier
			return fTopView;
		} else if (strcmp(property, "MenuBar") == 0) {
			if (fKeyMenuBar) {
				message->PopSpecifier();
				return fKeyMenuBar;
			} else {
				BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
				replyMsg.AddInt32("error", B_NAME_NOT_FOUND);
				replyMsg.AddString("message",
					"This window doesn't have a main MenuBar");
				message->SendReply(&replyMsg);
				return NULL;
			}
		} else
			return this;
	}

	return BLooper::ResolveSpecifier(message, index, specifier, what, property);
}


void
KWindow::Flush() const
{
	if (const_cast<KWindow*>(this)->Lock()) {
		fLink->Flush();
		const_cast<KWindow*>(this)->Unlock();
	}
}


void
KWindow::Sync() const
{
	if (!const_cast<KWindow*>(this)->Lock())
		return;

	fLink->StartMessage(AS_SYNC);// tododo

	// waiting for the reply is the actual syncing
	int32 code;
	fLink->FlushWithReply(code);

	const_cast<KWindow*>(this)->Unlock();
}



void
KWindow::DisableUpdates()
{
	if (Lock()) {
		fLink->StartMessage(AS_DISABLE_UPDATES);// tododo
		fLink->Flush();
		Unlock();
	}
}


void
KWindow::EnableUpdates()
{
	if (Lock()) {
		fLink->StartMessage(AS_ENABLE_UPDATES);// tododo
		fLink->Flush();
		Unlock();
	}
}


void
KWindow::BeginViewTransaction()
{
	if (Lock()) {
		fInTransaction = true;
		Unlock();
	}
}


void
KWindow::EndViewTransaction()
{
	if (Lock()) {
		if (fInTransaction)
			fLink->Flush();
		fInTransaction = false;
		Unlock();
	}
}


bool
KWindow::InViewTransaction() const
{
	BAutolock locker(const_cast<KWindow*>(this));
	return fInTransaction;
}


void
KWindow::_CreateTopView()
{
	debug_printf("[KWindow]{_CreateTopView} _CreateTopView(): enter\n");

debug_printf("[KWindow]{_CreateTopView} before B_ORIGIN fFrame.left =%f...\n",fFrame.left);
debug_printf("[KWindow]{_CreateTopView} before || fFrame.top =%f...\n",fFrame.top);
debug_printf("[KWindow]{_CreateTopView} before || fFrame.right =%f...\n",fFrame.right);
debug_printf("[KWindow]{_CreateTopView} before || fFrame.bottom =%f...\n",fFrame.bottom);

	BRect frame = fFrame.OffsetToCopy(B_ORIGIN);

debug_printf("[KWindow]{_CreateTopView} after B_ORIGIN frame.left =%f...\n",frame.left);
debug_printf("[KWindow]{_CreateTopView} after || frame.top =%f...\n",frame.top);
debug_printf("[KWindow]{_CreateTopView} after || frame.right =%f...\n",frame.right);
debug_printf("[KWindow]{_CreateTopView} after || frame.bottom =%f...\n",frame.bottom);


	// TODO: what to do here about std::nothrow?
	fTopView = new KView(frame, "fTopView", B_FOLLOW_ALL, B_WILL_DRAW);
	fTopView->fTopLevelView = true;

	//inhibit check_lock()
	fLastViewToken = _get_object_token_(fTopView);

	// set fTopView's owner, add it to window's eligible handler list
	// and also set its next handler to be this window.

	//STRACE(("Calling setowner fTopView = %p this = %p.\n",
	//	fTopView, this));

	fTopView->_SetOwner(this);

	// we can't use AddChild() because this is the top view
	fTopView->_CreateSelf();
	debug_printf("[KWindow] {_CreateTopView} BuildTopView ended\n");
}


/*!
	\brief Determines the target of a message received for the
		focus view.
*/
BHandler*
KWindow::_DetermineTarget(BMessage* message, BHandler* target)
{
	if (target == NULL)
		target = this;

	switch (message->what) {
		case B_KEY_DOWN:
		case B_KEY_UP:
		{
			// if we have a default button, it might want to hear
			// about pressing the <enter> key
			const int32 kNonLockModifierKeys = B_SHIFT_KEY | B_COMMAND_KEY
				| B_CONTROL_KEY | B_OPTION_KEY | B_MENU_KEY;
			int32 rawChar;
			if (DefaultButton() != NULL
				&& message->FindInt32("raw_char", &rawChar) == B_OK
				&& rawChar == B_ENTER
				&& (modifiers() & kNonLockModifierKeys) == 0)
				return DefaultButton();

			// supposed to fall through
		}
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
		case B_MODIFIERS_CHANGED:
			// these messages should be dispatched by the focus view
			if (CurrentFocus() != NULL)
				return CurrentFocus();
			break;

		case B_MOUSE_DOWN:
		case B_MOUSE_UP:
		case B_MOUSE_MOVED:
		case B_MOUSE_WHEEL_CHANGED:
		case B_MOUSE_IDLE:
			// is there a token of the view that is currently under the mouse?
			int32 token;
			if (message->FindInt32("_view_token", &token) == B_OK) {
				KView* view = _FindView(token);
				if (view != NULL)
					return view;
			}

			// if there is no valid token in the message, we try our
			// luck with the last target, if available
			if (fLastMouseMovedView != NULL)
				return fLastMouseMovedView;
			break;

		case B_PULSE:
		case B_QUIT_REQUESTED:
			// TODO: test whether R5 will let KView dispatch these messages
			return this;

		case _MESSAGE_DROPPED_:
			if (fLastMouseMovedView != NULL)
				return fLastMouseMovedView;
			break;

		default:
			break;
	}

	return target;
}


/*!	\brief Determines whether or not this message has targeted the focus view.

	This will return \c false only if the message did not go to the preferred
	handler, or if the packed message does not contain address the focus view
	at all.
*/
bool
KWindow::_IsFocusMessage(BMessage* message)
{
	BMessage::Private messagePrivate(message);
	if (!messagePrivate.UsePreferredTarget())
		return false;

	bool feedFocus;
	if (message->HasInt32("_token")
		&& (message->FindBool("_feed_focus", &feedFocus) != B_OK || !feedFocus))
		return false;

	return true;
}


/*!	\brief Distributes the message to its intended targets. This is done for
		all messages that should go to the preferred handler.

	Returns \c true in case the message should still be dispatched
*/
bool
KWindow::_UnpackMessage(unpack_cookie& cookie, BMessage** _message,
	BHandler** _target, bool* _usePreferred)
{
	if (cookie.message == NULL)
		return false;

	if (cookie.index == 0 && !cookie.tokens_scanned) {
		// We were called the first time for this message

		if (!*_usePreferred) {
			// only consider messages targeted at the preferred handler
			cookie.message = NULL;
			return true;
		}

		// initialize our cookie
		cookie.message = *_message;
		cookie.focus = *_target;

		if (cookie.focus != NULL)
			cookie.focus_token = _get_object_token_(*_target);

		if (fLastMouseMovedView != NULL && cookie.message->what == B_MOUSE_MOVED)
			cookie.last_view_token = _get_object_token_(fLastMouseMovedView);

		*_usePreferred = false;
	}

	_DequeueAll();

	// distribute the message to all targets specified in the
	// message directly (but not to the focus view)

	for (int32 token; !cookie.tokens_scanned
			&& cookie.message->FindInt32("_token", cookie.index, &token)
				== B_OK;
			cookie.index++) {
		// focus view is preferred and should get its message directly
		if (token == cookie.focus_token) {
			cookie.found_focus = true;
			continue;
		}
		if (token == cookie.last_view_token)
			continue;

		KView* target = _FindView(token);
		if (target == NULL)
			continue;

		*_message = new BMessage(*cookie.message);
		// the secondary copies of the message should not be treated as focus
		// messages, otherwise there will be unintended side effects, i.e.
		// keyboard shortcuts getting processed multiple times.
		(*_message)->RemoveName("_feed_focus");
		*_target = target;
		cookie.index++;
		return true;
	}

	cookie.tokens_scanned = true;

	// if there is a last mouse moved view, and the new focus is
	// different, the previous view wants to get its B_EXITED_VIEW
	// message
	if (cookie.last_view_token != B_NULL_TOKEN && fLastMouseMovedView != NULL
		&& fLastMouseMovedView != cookie.focus) {
		*_message = new BMessage(*cookie.message);
		*_target = fLastMouseMovedView;
		cookie.last_view_token = B_NULL_TOKEN;
		return true;
	}

	bool dispatchToFocus = true;

	// check if the focus token is still valid (could have been removed in the mean time)
	BHandler* handler;
	if (gDefaultTokens.GetToken(cookie.focus_token, B_HANDLER_TOKEN, (void**)&handler) != B_OK
		|| handler->Looper() != this)
		dispatchToFocus = false;

	if (dispatchToFocus && cookie.index > 0) {
		// should this message still be dispatched by the focus view?
		bool feedFocus;
		if (!cookie.found_focus
			&& (cookie.message->FindBool("_feed_focus", &feedFocus) != B_OK
				|| feedFocus == false))
			dispatchToFocus = false;
	}

	if (!dispatchToFocus) {
		delete cookie.message;
		cookie.message = NULL;
		return false;
	}

	*_message = cookie.message;
	*_target = cookie.focus;
	*_usePreferred = true;
	cookie.message = NULL;
	return true;
}


/*!	Some messages don't get to the window in a shape an application should see.
	This method is supposed to give a message the last grinding before
	it's acceptable for the receiving application.
*/
void
KWindow::_SanitizeMessage(BMessage* message, BHandler* target, bool usePreferred)
{
	if (target == NULL)
		return;

	switch (message->what) {
		case B_MOUSE_MOVED:
		case B_MOUSE_UP:
		case B_MOUSE_DOWN:
		{
			BPoint where;
			if (message->FindPoint("screen_where", &where) != B_OK)
				break;

			KView* view = dynamic_cast<KView*>(target);

			if (view == NULL || message->what == B_MOUSE_MOVED) {
				// add local window coordinates, only
				// for regular mouse moved messages
				message->AddPoint("where", ConvertFromScreen(where));
			}

			if (view != NULL) {
				// add local view coordinates
				BPoint viewWhere = view->ConvertFromScreen(where);
				if (message->what != B_MOUSE_MOVED) {
					// Yep, the meaning of "where" is different
					// for regular mouse moved messages versus
					// mouse up/down!
					message->AddPoint("where", viewWhere);
				}
				message->AddPoint("be:view_where", viewWhere);

				if (message->what == B_MOUSE_MOVED) {
					// is there a token of the view that is currently under
					// the mouse?
					KView* viewUnderMouse = NULL;
					int32 token;
					if (message->FindInt32("_view_token", &token) == B_OK)
						viewUnderMouse = _FindView(token);

					// add transit information
					uint32 transit
						= _TransitForMouseMoved(view, viewUnderMouse);
					message->AddInt32("be:transit", transit);

					if (usePreferred)
						fLastMouseMovedView = viewUnderMouse;
				}
			}
			break;
		}

		case B_MOUSE_IDLE:
		{
			// App Server sends screen coordinates, convert the point to
			// local view coordinates, then add the point in be:view_where
			BPoint where;
			if (message->FindPoint("screen_where", &where) != B_OK)
				break;

			KView* view = dynamic_cast<KView*>(target);
			if (view != NULL) {
				// add local view coordinates
				message->AddPoint("be:view_where",
					view->ConvertFromScreen(where));
			}
			break;
		}

		case _MESSAGE_DROPPED_:
		{
			uint32 originalWhat;
			if (message->FindInt32("_original_what",
					(int32*)&originalWhat) == B_OK) {
				message->what = originalWhat;
				message->RemoveName("_original_what");
			}
			break;
		}
	}
}


/*!
	This is called by KView::GetMouse() when a B_MOUSE_MOVED message
	is removed from the queue.
	It allows the window to update the last mouse moved view, and
	let it decide if this message should be kept. It will also remove
	the message from the queue.
	You need to hold the message queue lock when calling this method!

	\return true if this message can be used to get the mouse data from,
	\return false if this is not meant for the public.
*/
bool
KWindow::_StealMouseMessage(BMessage* message, bool& deleteMessage)
{
	BMessage::Private messagePrivate(message);
	if (!messagePrivate.UsePreferredTarget()) {
		// this message is targeted at a specific handler, so we should
		// not steal it
		return false;
	}

	int32 token;
	if (message->FindInt32("_token", 0, &token) == B_OK) {
		// This message has other targets, so we can't remove it;
		// just prevent it from being sent to the preferred handler
		// again (if it should have gotten it at all).
		bool feedFocus;
		if (message->FindBool("_feed_focus", &feedFocus) != B_OK || !feedFocus)
			return false;

		message->RemoveName("_feed_focus");
		deleteMessage = false;
	} else {
		deleteMessage = true;

		if (message->what == B_MOUSE_MOVED) {
			// We need to update the last mouse moved view, as this message
			// won't make it to _SanitizeMessage() anymore.
			KView* viewUnderMouse = NULL;
			int32 token;
			if (message->FindInt32("_view_token", &token) == B_OK)
				viewUnderMouse = _FindView(token);

			// Don't remove important transit messages!
			uint32 transit = _TransitForMouseMoved(fLastMouseMovedView,
				viewUnderMouse);
			if (transit == B_ENTERED_VIEW || transit == B_EXITED_VIEW)
				deleteMessage = false;
		}

		if (deleteMessage) {
			// The message is only thought for the preferred handler, so we
			// can just remove it.
			MessageQueue()->RemoveMessage(message);
		}
	}

	return true;
}




/*!
	Resizes the top view to match the window size. This will also
	adapt the size of all its child views as needed.
	This method has to be called whenever the frame of the window
	changes.
*/
void
KWindow::_AdoptResize()
{
debug_printf("[KWindow]{_AdoptResize}\n");
	// Resize views according to their resize modes - this
	// saves us some server communication, as the server
	// does the same with our views on its side.

	int32 deltaWidth = (int32)(fFrame.Width() - fTopView->Bounds().Width());
	int32 deltaHeight = (int32)(fFrame.Height() - fTopView->Bounds().Height());
	if (deltaWidth == 0 && deltaHeight == 0)
		return;

	fTopView->_ResizeBy(deltaWidth, deltaHeight);

debug_printf("[KWindow]{_AdoptResize}end\n");
}



void
KWindow::_SetFocus(KView* focusView, bool notifyInputServer)
{
	if (fFocus == focusView)
		return;

	// we notify the input server if we are passing focus
	// from a view which has the B_INPUT_METHOD_AWARE to a one
	// which does not, or vice-versa
	if (notifyInputServer && fActive) {
		bool inputMethodAware = false;
		if (focusView)
			inputMethodAware = focusView->Flags() & B_INPUT_METHOD_AWARE;
		BMessage msg(inputMethodAware ? IS_FOCUS_IM_AWARE_VIEW : IS_UNFOCUS_IM_AWARE_VIEW);
		BMessenger messenger(focusView);
		BMessage reply;
		if (focusView)
			msg.AddMessenger("view", messenger);
		_control_input_server_(&msg, &reply);
	}

	fFocus = focusView;
	SetPreferredHandler(focusView);
}



uint32
KWindow::_TransitForMouseMoved(KView* view, KView* viewUnderMouse) const
{
	uint32 transit;
	if (viewUnderMouse == view) {
		// the mouse is over the target view
		if (fLastMouseMovedView != view)
			transit = B_ENTERED_VIEW;
		else
			transit = B_INSIDE_VIEW;
	} else {
		// the mouse is not over the target view
		if (view == fLastMouseMovedView)
			transit = B_EXITED_VIEW;
		else
			transit = B_OUTSIDE_VIEW;
	}
	return transit;
}


/*!	Forwards the key to the switcher
*/
void
KWindow::_Switcher(int32 rawKey, uint32 modifiers, bool repeat)
{
	// only send the first key press, no repeats
	if (repeat)
		return;

	BMessenger jaatbar(k_kDeskbarSignature);
	if (!jaatbar.IsValid()) {
		// TODO: have some kind of fallback-handling in case the Deskbar is
		// not available?
		return;
	}

	BMessage message('TASK');
	message.AddInt32("key", rawKey);
	message.AddInt32("modifiers", modifiers);
	message.AddInt64("when", system_time());
	message.AddInt32("team", Team());
	jaatbar.SendMessage(&message);
}


/*!	Handles keyboard input before it gets forwarded to the target handler.
	This includes shortcut evaluation, keyboard navigation, etc.

	\return handled if true, the event was already handled, and will not
		be forwarded to the target handler.

	TODO: must also convert the incoming key to the font encoding of the target
*/
bool
KWindow::_HandleKeyDown(BMessage* event)
{
	// Only handle special functions when the event targeted the active focus
	// view
	if (!_IsFocusMessage(event))
		return false;

	const char* bytes = NULL;
	if (event->FindString("bytes", &bytes) != B_OK)
		return false;

	char key = bytes[0];

	uint32 modifiers;
	if (event->FindInt32("modifiers", (int32*)&modifiers) != B_OK)
		modifiers = 0;

	// handle KMenuBar key
	if (key == B_ESCAPE && (modifiers & B_COMMAND_KEY) != 0
		&& fKeyMenuBar != NULL) {
		fKeyMenuBar->StartMenuBar(0, true, false, NULL);
		return true;
	}

	// Keyboard navigation through views
	// (B_OPTION_KEY makes BTextViews and friends navigable, even in editing
	// mode)
	if (key == B_TAB && (modifiers & B_OPTION_KEY) != 0) {
		_KeyboardNavigation();
		return true;
	}

	int32 rawKey;
	event->FindInt32("key", &rawKey);

	// Deskbar's Switcher
	if ((key == B_TAB || rawKey == 0x11) && (modifiers & B_CONTROL_KEY) != 0) {
		_Switcher(rawKey, modifiers, event->HasInt32("be:key_repeat"));
		return true;
	}

	// Optionally close window when the escape key is pressed
	if (key == B_ESCAPE && (Flags() & B_CLOSE_ON_ESCAPE) != 0) {
		BMessage message(B_QUIT_REQUESTED);
		message.AddBool("shortcut", true);

		PostMessage(&message);
		return true;
	}

	// PrtScr key takes a screenshot
	if (key == B_FUNCTION_KEY && rawKey == B_PRINT_KEY) {
		// With no modifier keys the best way to get a screenshot is by
		// calling the screenshot CLI
		if (modifiers == 0) {
			be_roster->Launch("application/x-vnd.haiku-screenshot-cli");
			return true;
		}

		// Prepare a message based on the modifier keys pressed and launch the
		// screenshot GUI
		BMessage message(B_ARGV_RECEIVED);
		int32 argc = 1;
		message.AddString("argv", "Screenshot");
		if ((modifiers & B_CONTROL_KEY) != 0) {
			argc++;
			message.AddString("argv", "--clipboard");
		}
		if ((modifiers & B_SHIFT_KEY) != 0) {
			argc++;
			message.AddString("argv", "--silent");
		}
		message.AddInt32("argc", argc);
		be_roster->Launch("application/x-vnd.haiku-screenshot", &message);
		return true;
	}

	// Handle shortcuts
	if ((modifiers & B_COMMAND_KEY) != 0) {
		// Command+q has been pressed, so, we will quit
		// the shortcut mechanism doesn't allow handlers outside the window
		if (!fNoQuitShortcut && (key == 'Q' || key == 'q')) {
			BMessage message(B_QUIT_REQUESTED);
			message.AddBool("shortcut", true);

			be_app->PostMessage(&message);
			// eat the event
			return true;
		}

		// Send Command+Left and Command+Right to textview if it has focus
		if (key == B_LEFT_ARROW || key == B_RIGHT_ARROW) {
			// check key before doing expensive dynamic_cast
			KTextView* textView = dynamic_cast<KTextView*>(CurrentFocus());
			if (textView != NULL) {
				textView->KeyDown(bytes, modifiers);
				// eat the event
				return true;
			}
		}

		// Pretend that the user opened a menu, to give the subclass a
		// chance to update it's menus. This may install new shortcuts,
		// which is why we have to call it here, before trying to find
		// a shortcut for the given key.
		MenusBeginning();

		Shortcut* shortcut = _FindShortcut(key, modifiers);
		if (shortcut != NULL) {
			// TODO: would be nice to move this functionality to
			//	a Shortcut::Invoke() method - but since KMenu::InvokeItem()
			//	(and KMenuItem::Invoke()) are private, I didn't want
			//	to mess with them (KMenuItem::Invoke() is public in
			//	Dano/Zeta, though, maybe we should just follow their
			//	example)
			if (shortcut->MenuItem() != NULL) {
				KMenu* menu = shortcut->MenuItem()->Menu();
				if (menu != NULL)
					KMenuPrivate(menu).InvokeItem(shortcut->MenuItem(), true);
			} else {
				BHandler* target = shortcut->Target();
				if (target == NULL)
					target = CurrentFocus();

				if (shortcut->Message() != NULL) {
					BMessage message(*shortcut->Message());

					if (message.ReplaceInt64("when", system_time()) != B_OK)
						message.AddInt64("when", system_time());
					if (message.ReplaceBool("shortcut", true) != B_OK)
						message.AddBool("shortcut", true);

					PostMessage(&message, target);
				}
			}
		}

		MenusEnded();

		// we always eat the event if the command key was pressed
		return true;
	}

	// TODO: convert keys to the encoding of the target view

	return false;
}


bool
KWindow::_HandleUnmappedKeyDown(BMessage* event)
{
	// Only handle special functions when the event targeted the active focus
	// view
	if (!_IsFocusMessage(event))
		return false;

	uint32 modifiers;
	int32 rawKey;
	if (event->FindInt32("modifiers", (int32*)&modifiers) != B_OK
		|| event->FindInt32("key", &rawKey))
		return false;

	// Deskbar's Switcher
	if (rawKey == 0x11 && (modifiers & B_CONTROL_KEY) != 0) {
		_Switcher(rawKey, modifiers, event->HasInt32("be:key_repeat"));
		return true;
	}

	return false;
}


void
KWindow::_KeyboardNavigation()
{
	BMessage* message = CurrentMessage();
	if (message == NULL)
		return;

	const char* bytes;
	uint32 modifiers;
	if (message->FindString("bytes", &bytes) != B_OK || bytes[0] != B_TAB)
		return;

	message->FindInt32("modifiers", (int32*)&modifiers);

	KView* nextFocus;
	int32 jumpGroups = (modifiers & B_OPTION_KEY) != 0
		? B_NAVIGABLE_JUMP : B_NAVIGABLE;
	if (modifiers & B_SHIFT_KEY)
		nextFocus = _FindPreviousNavigable(fFocus, jumpGroups);
	else
		nextFocus = _FindNextNavigable(fFocus, jumpGroups);

	if (nextFocus != NULL && nextFocus != fFocus)
		nextFocus->MakeFocus(true);
}


/*!
	\brief Return the position of the window centered horizontally to the passed
           in \a frame and vertically 3/4 from the top of \a frame.

	If the window is on the borders

	\param width The width of the window.
	\param height The height of the window.
	\param frame The \a frame to center the window in.

	\return The new window position.
*/
BPoint
KWindow::AlertPosition(const BRect& frame)
{
	float width = Bounds().Width();
	float height = Bounds().Height();

	BPoint point(frame.left + (frame.Width() / 2.0f) - (width / 2.0f),
		frame.top + (frame.Height() / 4.0f) - ceil(height / 3.0f));

	BRect screenFrame = BScreen(this).Frame(); //tododo
	if (frame == screenFrame) {
		// reference frame is screen frame, skip the below adjustments
		return point;
	}

	float borderWidth;
	float tabHeight;
	_GetDecoratorSize(&borderWidth, &tabHeight);

	// clip the x position within the horizontal edges of the screen
	if (point.x < screenFrame.left + borderWidth)
		point.x = screenFrame.left + borderWidth;
	else if (point.x + width > screenFrame.right - borderWidth)
		point.x = screenFrame.right - borderWidth - width;

	// lower the window down if it is covering the window tab
	float tabPosition = frame.LeftTop().y + tabHeight + borderWidth;
	if (point.y < tabPosition)
		point.y = tabPosition;

	// clip the y position within the vertical edges of the screen
	if (point.y < screenFrame.top + borderWidth)
		point.y = screenFrame.top + borderWidth;
	else if (point.y + height > screenFrame.bottom - borderWidth)
		point.y = screenFrame.bottom - borderWidth - height;

	return point;
}


BMessage*
KWindow::ConvertToMessage(void* raw, int32 code)
{
	return BLooper::ConvertToMessage(raw, code);
}


KWindow::Shortcut*
KWindow::_FindShortcut(uint32 key, uint32 modifiers)
{
	int32 count = fShortcuts.CountItems();

	key = Shortcut::PrepareKey(key);
	modifiers = Shortcut::PrepareModifiers(modifiers);

	for (int32 index = 0; index < count; index++) {
		Shortcut* shortcut = (Shortcut*)fShortcuts.ItemAt(index);

		if (shortcut->Matches(key, modifiers))
			return shortcut;
	}

	return NULL;
}


KView*
KWindow::_FindView(int32 token)
{
	BHandler* handler;
	if (gDefaultTokens.GetToken(token, B_HANDLER_TOKEN,
			(void**)&handler) != B_OK) {
		return NULL;
	}

	// the view must belong to us in order to be found by this method
	KView* view = dynamic_cast<KView*>(handler);
	if (view != NULL && view->Window() == this)
		return view;

	return NULL;
}


KView*
KWindow::_FindView(KView* view, BPoint point) const
{
	// point is assumed to be already in view's coordinates
	if (!view->IsHidden(view) && view->Bounds().Contains(point)) {
		if (view->fFirstChild == NULL)
			return view;
		else {
			KView* child = view->fFirstChild;
			while (child != NULL) {
				BPoint childPoint = point - child->Frame().LeftTop();
				KView* subView  = _FindView(child, childPoint);
				if (subView != NULL)
					return subView;

				child = child->fNextSibling;
			}
		}
		return view;
	}
	return NULL;
}


KView*
KWindow::_FindNextNavigable(KView* focus, uint32 flags)
{
	if (focus == NULL)
		focus = fTopView;

	KView* nextFocus = focus;

	// Search the tree for views that accept focus (depth search)
	while (true) {
		if (nextFocus->fFirstChild)
			nextFocus = nextFocus->fFirstChild;
		else if (nextFocus->fNextSibling)
			nextFocus = nextFocus->fNextSibling;
		else {
			// go to the nearest parent with a next sibling
			while (!nextFocus->fNextSibling && nextFocus->fParent) {
				nextFocus = nextFocus->fParent;
			}

			if (nextFocus == fTopView) {
				// if we started with the top view, we traversed the whole tree already
				if (nextFocus == focus)
					return NULL;

				nextFocus = nextFocus->fFirstChild;
			} else
				nextFocus = nextFocus->fNextSibling;
		}

		if (nextFocus == focus || nextFocus == NULL) {
			// When we get here it means that the hole tree has been
			// searched and there is no view with B_NAVIGABLE(_JUMP) flag set!
			return NULL;
		}

		if (!nextFocus->IsHidden() && (nextFocus->Flags() & flags) != 0)
			return nextFocus;
	}
}


KView*
KWindow::_FindPreviousNavigable(KView* focus, uint32 flags)
{
	if (focus == NULL)
		focus = fTopView;

	KView* previousFocus = focus;

	// Search the tree for the previous view that accept focus
	while (true) {
		if (previousFocus->fPreviousSibling) {
			// find the last child in the previous sibling
			previousFocus = _LastViewChild(previousFocus->fPreviousSibling);
		} else {
			previousFocus = previousFocus->fParent;
			if (previousFocus == fTopView)
				previousFocus = _LastViewChild(fTopView);
		}

		if (previousFocus == focus || previousFocus == NULL) {
			// When we get here it means that the hole tree has been
			// searched and there is no view with B_NAVIGABLE(_JUMP) flag set!
			return NULL;
		}

		if (!previousFocus->IsHidden() && (previousFocus->Flags() & flags) != 0)
			return previousFocus;
	}
}


/*!
	Returns the last child in a view hierarchy.
	Needed only by _FindPreviousNavigable().
*/
KView*
KWindow::_LastViewChild(KView* parent)
{
	while (true) {
		KView* last = parent->fFirstChild;
		if (last == NULL)
			return parent;

		while (last->fNextSibling) {
			last = last->fNextSibling;
		}

		parent = last;
	}
}


void
KWindow::SetIsFilePanel(bool isFilePanel)
{
	fIsFilePanel = isFilePanel;
}


bool
KWindow::IsFilePanel() const
{
	return fIsFilePanel;
}


void
KWindow::_GetDecoratorSize(float* _borderWidth, float* _tabHeight) const
{
	// fallback in case retrieving the decorator settings fails
	// (highly unlikely)
	float borderWidth = 5.0;
	float tabHeight = 21.0;

	BMessage settings;
	if (GetDecoratorSettings(&settings) == B_OK) {
		BRect tabRect;
		if (settings.FindRect("tab frame", &tabRect) == B_OK)
			tabHeight = tabRect.Height();
		settings.FindFloat("border width", &borderWidth);
	} else {
		// probably no-border window look
		if (fLook == B_NO_BORDER_WINDOW_LOOK) {
			borderWidth = 0.0;
			tabHeight = 0.0;
		}
		// else use fall-back values from above
	}

	if (_borderWidth != NULL)
		*_borderWidth = borderWidth;
	if (_tabHeight != NULL)
		*_tabHeight = tabHeight;
}


void
KWindow::_SendShowOrHideMessage()
{
debug_printf("[KWindow] {_SendShowOrHideMessage} \n");

	fLink->StartMessage(AS_SHOW_OR_HIDE_WINDOW); // tododo
	fLink->Attach<int32>(fShowLevel);
	fLink->Flush();
}


//	#pragma mark - C++ binary compatibility kludge


extern "C" void
_ReservedWindow1__7KWindow(KWindow* window, BLayout* layout)
{
	// SetLayout()
	perform_data_set_layout data;
	data.layout = layout;
	window->Perform(PERFORM_CODE_SET_LAYOUT, &data);
}


void KWindow::_ReservedWindow2() {}
void KWindow::_ReservedWindow3() {}
void KWindow::_ReservedWindow4() {}
void KWindow::_ReservedWindow5() {}
void KWindow::_ReservedWindow6() {}
void KWindow::_ReservedWindow7() {}
void KWindow::_ReservedWindow8() {}

