/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Erik Jaesler, erik@cgsoftware.com
 *		John Scipione, jscipione@gmail.com
 */


//!	KAlert displays a modal alert window.


#include <KAlert.h>

#include <new>

#include <stdio.h>

#include <KBitmap.h>
#include <KButton.h>
#include <KControlLook.h>
#include <FindDirectory.h>
#include <KIconUtils.h>
#include <KLayoutBuilder.h>
#include <KMenuField.h>
#include <MessageFilter.h>
#include <Path.h>
#include <Resources.h>
#include <Screen.h>
#include <String.h>
#include <Khidki.h>

#include<KLayout.h>//khidki
#include <binary_compatibility/Interface.h>


//#define DEBUG_ALERT
#ifdef DEBUG_ALERT
#	define FTRACE(x) fprintf(x)
#else
#	define FTRACE(x) ;
#endif


class KTAlertView : public KView {
public:
								KTAlertView();
								KTAlertView(BMessage* archive);
								~KTAlertView();

	static	KTAlertView*			Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				GetPreferredSize(float* _width, float* _height);
	virtual	BSize				MaxSize();
	virtual	void				Draw(BRect updateRect);

			void				SetBitmap(KBitmap* Icon)
									{ fIconBitmap = Icon; }
			KBitmap*			Bitmap()
									{ return fIconBitmap; }

private:
			KBitmap*			fIconBitmap;
};


class _KAlertFilter_ : public BMessageFilter {
public:
								_KAlertFilter_(KAlert* Alert);
								~_KAlertFilter_();

	virtual	filter_result		Filter(BMessage* msg, BHandler** target);

private:
			KAlert*				fAlert;
};


static const unsigned int kAlertButtonMsg = 'ALTB';
static const int kSemTimeOut = 50000;

static const int kButtonOffsetSpacing = 62;
static const int kButtonUsualWidth = 55;
static const int kIconStripeWidth = 30;

static const int kWindowMinWidth = 310;
static const int kWindowOffsetMinWidth = 335;


static inline float
icon_layout_scale()
{
	float scale = be_plain_font->Size() / 12;
	return max_c(1, scale);
}


// #pragma mark -


KAlert::KAlert()
	:
	KWindow(BRect(0, 0, 100, 100), "", B_MODAL_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	_Init(NULL, NULL, NULL, NULL, B_WIDTH_FROM_WIDEST, B_EVEN_SPACING,
		B_INFO_ALERT);
}


KAlert::KAlert(const char *title, const char *text, const char *button1,
		const char *button2, const char *button3, button_width width,
		alert_type type)
	:
	KWindow(BRect(0, 0, 100, 100), title, B_MODAL_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	_Init(text, button1, button2, button3, width, B_EVEN_SPACING, type);
}


KAlert::KAlert(const char *title, const char *text, const char *button1,
		const char *button2, const char *button3, button_width width,
		button_spacing spacing, alert_type type)
	:
	KWindow(BRect(0, 0, 100, 100), title, B_MODAL_WINDOW,
		B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	_Init(text, button1, button2, button3, width, spacing, type);
}


KAlert::KAlert(BMessage* data)
	:
	KWindow(data)
{
	fInvoker = NULL;
	fAlertSem = -1;
	fAlertValue = -1;

	fTextView = (KTextView*)FindView("_tv_");

	// TODO: window loses default button on dearchive!
	// TODO: ButtonAt() doesn't work afterwards (also affects shortcuts)

	KTAlertView* view = (KTAlertView*)FindView("_master_");
	if (view)
		view->SetBitmap(_CreateTypeIcon());

	// Get keys
	char key;
	for (int32 i = 0; i < 3; ++i) {
		if (data->FindInt8("_but_key", i, (int8*)&key) == B_OK)
			fKeys[i] = key;
	}

	AddCommonFilter(new(std::nothrow) _KAlertFilter_(this));
}


KAlert::~KAlert()
{
	// Probably not necessary, but it makes me feel better.
	if (fAlertSem >= B_OK)
		delete_sem(fAlertSem);
}


BArchivable*
KAlert::Instantiate(BMessage* data)
{
	if (!validate_instantiation(data, "KAlert"))
		return NULL;

	return new(std::nothrow) KAlert(data);
}


status_t
KAlert::Archive(BMessage* data, bool deep) const
{
	status_t ret = KWindow::Archive(data, deep);

	// Stow the text
	if (ret == B_OK)
		ret = data->AddString("_text", fTextView->Text());

	// Stow the alert type
	if (ret == B_OK)
		ret = data->AddInt32("_atype", fType);

	// Stow the button width
	if (ret == B_OK)
		ret = data->AddInt32("_but_width", fButtonWidth);

	// Stow the shortcut keys
	if (fKeys[0] || fKeys[1] || fKeys[2]) {
		// If we have any to save, we must save something for everyone so it
		// doesn't get confusing on the unarchive.
		if (ret == B_OK)
			ret = data->AddInt8("_but_key", fKeys[0]);
		if (ret == B_OK)
			ret = data->AddInt8("_but_key", fKeys[1]);
		if (ret == B_OK)
			ret = data->AddInt8("_but_key", fKeys[2]);
	}

	return ret;
}


alert_type
KAlert::Type() const
{
	return (alert_type)fType;
}


void
KAlert::SetType(alert_type type)
{
	fType = type;
}


void
KAlert::SetText(const char* text)
{
	TextView()->SetText(text);
}


void
KAlert::SetIcon(KBitmap* bitmap)
{
	fIconView->SetBitmap(bitmap);
}


void
KAlert::SetButtonSpacing(button_spacing spacing)
{
	fButtonSpacing = spacing;
}


void
KAlert::SetButtonWidth(button_width width)
{
	fButtonWidth = width;
}


void
KAlert::SetShortcut(int32 index, char key)
{
	if (index >= 0 && (size_t)index < fKeys.size())
		fKeys[index] = key;
}


char
KAlert::Shortcut(int32 index) const
{
	if (index >= 0 && (size_t)index < fKeys.size())
		return fKeys[index];

	return 0;
}


int32
KAlert::Go()
{
	fAlertSem = create_sem(0, "AlertSem");
	if (fAlertSem < 0) {
		Quit();
		return -1;
	}

	// Get the originating window, if it exists
	KWindow* window = dynamic_cast<KWindow*>(
		BLooper::LooperForThread(find_thread(NULL)));

	_Prepare();
	Show();

	if (window != NULL) {
		status_t status;
		for (;;) {
			do {
				status = acquire_sem_etc(fAlertSem, 1, B_RELATIVE_TIMEOUT,
					kSemTimeOut);
				// We've (probably) had our time slice taken away from us
			} while (status == B_INTERRUPTED);

			if (status == B_BAD_SEM_ID) {
				// Semaphore was finally nuked in MessageReceived
				break;
			}
			window->UpdateIfNeeded();
		}
	} else {
		// No window to update, so just hang out until we're done.
		while (acquire_sem(fAlertSem) == B_INTERRUPTED) {
		}
	}

	// Have to cache the value since we delete on Quit()
	int32 value = fAlertValue;
	if (Lock())
		Quit();

	return value;
}


status_t
KAlert::Go(BInvoker* invoker)
{
	fInvoker = invoker;
	_Prepare();
	Show();
	return B_OK;
}


void
KAlert::MessageReceived(BMessage* msg)
{
	if (msg->what != kAlertButtonMsg)
		return KWindow::MessageReceived(msg);

	int32 which;
	if (msg->FindInt32("which", &which) == B_OK) {
		if (fAlertSem < 0) {
			// Semaphore hasn't been created; we're running asynchronous
			if (fInvoker != NULL) {
				BMessage* out = fInvoker->Message();
				if (out && (out->ReplaceInt32("which", which) == B_OK
							|| out->AddInt32("which", which) == B_OK))
					fInvoker->Invoke();
			}
			PostMessage(B_QUIT_REQUESTED);
		} else {
			// Created semaphore means were running synchronously
			fAlertValue = which;

			// TextAlertVar does release_sem() below, and then sets the
			// member var.  That doesn't make much sense to me, since we
			// want to be able to clean up at some point.  Better to just
			// nuke the semaphore now; we don't need it any more and this
			// lets synchronous Go() continue just as well.
			delete_sem(fAlertSem);
			fAlertSem = -1;
		}
	}
}


void
KAlert::FrameResized(float newWidth, float newHeight)
{
	KWindow::FrameResized(newWidth, newHeight);
}


void
KAlert::AddButton(const char* label, char key)
{
	if (label == NULL || label[0] == '\0')
		return;

	KButton* button = _CreateButton(fButtons.size(), label);
	fButtons.push_back(button);
	fKeys.push_back(key);

	SetDefaultButton(button);
	fButtonLayout->AddView(button);
}


int32
KAlert::CountButtons() const
{
	return (int32)fButtons.size();
}


KButton*
KAlert::ButtonAt(int32 index) const
{
	if (index >= 0 && (size_t)index < fButtons.size())
		return fButtons[index];

	return NULL;
}


KTextView*
KAlert::TextView() const
{
	return fTextView;
}


BHandler*
KAlert::ResolveSpecifier(BMessage* msg, int32 index,
	BMessage* specifier, int32 form, const char* property)
{
	return KWindow::ResolveSpecifier(msg, index, specifier, form, property);
}


status_t
KAlert::GetSupportedSuites(BMessage* data)
{
	return KWindow::GetSupportedSuites(data);
}


void
KAlert::DispatchMessage(BMessage* msg, BHandler* handler)
{
	KWindow::DispatchMessage(msg, handler);
}


void
KAlert::Quit()
{
	KWindow::Quit();
}


bool
KAlert::QuitRequested()
{
	return KWindow::QuitRequested();
}


//! This method is deprecated, do not use - use KWindow::CenterIn() instead.
BPoint
KAlert::AlertPosition(float width, float height)
{
	BPoint result(100, 100);

	KWindow* window =
		dynamic_cast<KWindow*>(BLooper::LooperForThread(find_thread(NULL)));

	BScreen screen(window);
	BRect screenFrame(0, 0, 640, 480);
	if (screen.IsValid())
		screenFrame = screen.Frame();

	// Horizontally, we're smack in the middle
	result.x = screenFrame.left + (screenFrame.Width() / 2.0) - (width / 2.0);

	// This is probably sooo wrong, but it looks right on 1024 x 768
	result.y = screenFrame.top + (screenFrame.Height() / 4.0) - ceil(height / 3.0);

	return result;
}


status_t
KAlert::Perform(perform_code code, void* _data)
{
	switch (code) {
		case PERFORM_CODE_SET_LAYOUT:
			k_perform_data_set_layout* data = (k_perform_data_set_layout*)_data;
			KAlert::SetLayout(data->layout);
			return B_OK;
	}

	return KWindow::Perform(code, _data);
}


void KAlert::_ReservedAlert1() {}
void KAlert::_ReservedAlert2() {}
void KAlert::_ReservedAlert3() {}


void
KAlert::_Init(const char* text, const char* button0, const char* button1,
	const char* button2, button_width buttonWidth, button_spacing spacing,
	alert_type type)
{
	fInvoker = NULL;
	fAlertSem = -1;
	fAlertValue = -1;

	fIconView = new KTAlertView();

	fTextView = new KTextView("_tv_");
	fTextView->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	rgb_color textColor = ui_color(B_PANEL_TEXT_COLOR);
	fTextView->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
	fTextView->MakeEditable(false);
	fTextView->MakeSelectable(false);
	fTextView->SetWordWrap(true);
	fTextView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED));

	fButtonLayout = new KGroupLayout(B_HORIZONTAL, B_USE_HALF_ITEM_SPACING);

	SetType(type);
	SetButtonWidth(buttonWidth);
	SetButtonSpacing(spacing);
	SetText(text);

	KLayoutBuilder::Group<>(this, B_HORIZONTAL, 0)
		.Add(fIconView)
		.AddGroup(B_VERTICAL, B_USE_HALF_ITEM_SPACING)
			.SetInsets(B_USE_HALF_ITEM_INSETS)
			.Add(fTextView)
			.AddGroup(B_HORIZONTAL, 0)
				.AddGlue()
				.Add(fButtonLayout);

	AddButton(button0);
	AddButton(button1);
	AddButton(button2);

	AddCommonFilter(new(std::nothrow) _KAlertFilter_(this));
}


KBitmap*
KAlert::_CreateTypeIcon()
{
	if (Type() == B_EMPTY_ALERT)
		return NULL;

	// The icons are in the app_server resources
	KBitmap* icon = NULL;
	BPath path;
	status_t status = find_directory(B_BEOS_SERVERS_DIRECTORY, &path);
	if (status != B_OK) {
		FTRACE((stderr, "KAlert::_CreateTypeIcon() - find_directory "
			"failed: %s\n", strerror(status)));
		return NULL;
	}

	path.Append("app_server");
	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK) {
		FTRACE((stderr, "KAlert::_CreateTypeIcon() - BFile init failed: %s\n",
			strerror(status)));
		return NULL;
	}

	BResources resources;
	status = resources.SetTo(&file);
	if (status != B_OK) {
		FTRACE((stderr, "KAlert::_CreateTypeIcon() - BResources init "
			"failed: %s\n", strerror(status)));
		return NULL;
	}

	// Which icon are we trying to load?
	const char* iconName;
	switch (fType) {
		case B_INFO_ALERT:
			iconName = "info";
			break;
		case B_IDEA_ALERT:
			iconName = "idea";
			break;
		case B_WARNING_ALERT:
			iconName = "warn";
			break;
		case B_STOP_ALERT:
			iconName = "stop";
			break;

		default:
			// Alert type is either invalid or B_EMPTY_ALERT;
			// either way, we're not going to load an icon
			return NULL;
	}

	int32 iconSize = (int32)(32 * icon_layout_scale());
	// Allocate the icon bitmap
	icon = new(std::nothrow) KBitmap(BRect(0, 0, iconSize - 1, iconSize - 1),
		0, B_RGBA32);
	if (icon == NULL || icon->InitCheck() < B_OK) {
		FTRACE((stderr, "KAlert::_CreateTypeIcon() - No memory for bitmap\n"));
		delete icon;
		return NULL;
	}

	// Load the raw icon data
	size_t size = 0;
	const uint8* rawIcon;

	// Try to load vector icon
	rawIcon = (const uint8*)resources.LoadResource(B_VECTOR_ICON_TYPE,
		iconName, &size);
	if (rawIcon != NULL
		&& KIconUtils::GetVectorIcon(rawIcon, size, icon) == B_OK) {
		return icon;
	}

	// Fall back to bitmap icon
	rawIcon = (const uint8*)resources.LoadResource(B_LARGE_ICON_TYPE,
		iconName, &size);
	if (rawIcon == NULL) {
		FTRACE((stderr, "KAlert::_CreateTypeIcon() - Icon resource not found\n"));
		delete icon;
		return NULL;
	}

	// Handle color space conversion
	if (icon->ColorSpace() != B_CMAP8) {
		KIconUtils::ConvertFromCMAP8(rawIcon, iconSize, iconSize,
			iconSize, icon);
	}

	return icon;
}


KButton*
KAlert::_CreateButton(int32 which, const char* label)
{
	BMessage* message = new BMessage(kAlertButtonMsg);
	if (message == NULL)
		return NULL;

	message->AddInt32("which", which);

	char name[32];
	snprintf(name, sizeof(name), "_b%" B_PRId32 "_", which);

	return new(std::nothrow) KButton(name, label, message);
}


/*!	Tweaks the layout according to the configuration.
*/
void
KAlert::_Prepare()
{
	// Must have at least one button
	if (CountButtons() == 0)
		debugger("BAlerts must have at least one button.");

	float fontFactor = be_plain_font->Size() / 11.0f;

	if (fIconView->Bitmap() == NULL)
		fIconView->SetBitmap(_CreateTypeIcon());

	if (fButtonWidth == B_WIDTH_AS_USUAL) {
		float usualWidth = kButtonUsualWidth * fontFactor;

		for (int32 index = 0; index < CountButtons(); index++) {
			KButton* button = ButtonAt(index);
			if (button->MinSize().width < usualWidth)
				button->SetExplicitSize(BSize(usualWidth, B_SIZE_UNSET));
		}
	} else if (fButtonWidth == B_WIDTH_FROM_WIDEST) {
		// Get width of widest label
		float maxWidth = 0;
		for (int32 index = 0; index < CountButtons(); index++) {
			KButton* button = ButtonAt(index);
			float width;
			button->GetPreferredSize(&width, NULL);

			if (width > maxWidth)
				maxWidth = width;
		}
		for (int32 index = 0; index < CountButtons(); index++) {
			KButton* button = ButtonAt(index);
			button->SetExplicitSize(BSize(maxWidth, B_SIZE_UNSET));
		}
	}

	if (fButtonSpacing == B_OFFSET_SPACING && CountButtons() > 1) {
		// Insert some strut
		fButtonLayout->AddItem(1, KSpaceLayoutItem::CreateHorizontalStrut(
			kButtonOffsetSpacing * fontFactor));
	}

	// Position the alert so that it is centered vertically but offset a bit
	// horizontally in the parent window's frame or, if unavailable, the
	// screen frame.
	float minWindowWidth = (fButtonSpacing == B_OFFSET_SPACING
		? kWindowOffsetMinWidth : kWindowMinWidth) * fontFactor;
	GetLayout()->SetExplicitMinSize(BSize(minWindowWidth, B_SIZE_UNSET));

	ResizeToPreferred();

	// Return early if we've already been moved...
	if (Frame().left != 0 && Frame().right != 0)
		return;

	// otherwise center ourselves on-top of parent window/screen
	KWindow* parent = dynamic_cast<KWindow*>(BLooper::LooperForThread(
		find_thread(NULL)));
	const BRect frame = parent != NULL ? parent->Frame()
		: BScreen(this).Frame();

	MoveTo(static_cast<KWindow*>(this)->AlertPosition(frame));
		// Hidden by KAlert::AlertPosition()
}


//	#pragma mark - KTAlertView


KTAlertView::KTAlertView()
	:
	KView("KTAlertView", B_WILL_DRAW),
	fIconBitmap(NULL)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
}


KTAlertView::KTAlertView(BMessage* archive)
	:
	KView(archive),
	fIconBitmap(NULL)
{
}


KTAlertView::~KTAlertView()
{
	delete fIconBitmap;
}


KTAlertView*
KTAlertView::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "KTAlertView"))
		return NULL;

	return new(std::nothrow) KTAlertView(archive);
}


status_t
KTAlertView::Archive(BMessage* archive, bool deep) const
{
	return KView::Archive(archive, deep);
}


void
KTAlertView::GetPreferredSize(float* _width, float* _height)
{
	float scale = icon_layout_scale();

	if (_width != NULL) {
		if (fIconBitmap != NULL)
			*_width = 18 * scale + fIconBitmap->Bounds().Width();
		else
			*_width = 0;
	}
	if (_height != NULL) {
		if (fIconBitmap != NULL)
			*_height = 6 * scale + fIconBitmap->Bounds().Height();
		else
			*_height = 0;
	}
}


BSize
KTAlertView::MaxSize()
{
	return BSize(MinSize().width, B_SIZE_UNLIMITED);
}


void
KTAlertView::Draw(BRect updateRect)
{
	if (fIconBitmap == NULL)
		return;

	// Here's the fun stuff
	BRect stripeRect = Bounds();
	float iconLayoutScale = icon_layout_scale();
	stripeRect.right = kIconStripeWidth * iconLayoutScale;
	SetHighColor(tint_color(ViewColor(), B_DARKEN_1_TINT));
	FillRect(stripeRect);

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	DrawBitmapAsync(fIconBitmap, BPoint(18 * iconLayoutScale,
		6 * iconLayoutScale));
}


//	#pragma mark - _KAlertFilter_


_KAlertFilter_::_KAlertFilter_(KAlert* alert)
	: BMessageFilter(B_KEY_DOWN),
	fAlert(alert)
{
}


_KAlertFilter_::~_KAlertFilter_()
{
}


filter_result
_KAlertFilter_::Filter(BMessage* msg, BHandler** target)
{
	if (msg->what == B_KEY_DOWN) {
		char byte;
		if (msg->FindInt8("byte", (int8*)&byte) == B_OK) {
			for (int i = 0; i < fAlert->CountButtons(); ++i) {
				if (byte == fAlert->Shortcut(i) && fAlert->ButtonAt(i)) {
					char space = ' ';
					fAlert->ButtonAt(i)->KeyDown(&space, 1);

					return B_SKIP_MESSAGE;
				}
			}
		}
	}

	return B_DISPATCH_MESSAGE;
}
