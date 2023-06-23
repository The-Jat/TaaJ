/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <KToolTip.h>

#include <new>

#include <Message.h>
#include <KTextView.h>
#include <KToolTipManager.h>


KToolTip::KToolTip()
{
	_InitData();
}


KToolTip::KToolTip(BMessage* archive)
{
	_InitData();

	bool sticky;
	if (archive->FindBool("sticky", &sticky) == B_OK)
		fIsSticky = sticky;

	// TODO!
}


KToolTip::~KToolTip()
{
}


status_t
KToolTip::Archive(BMessage* archive, bool deep) const
{
	status_t status = BArchivable::Archive(archive, deep);

	if (fIsSticky)
		status = archive->AddBool("sticky", fIsSticky);

	// TODO!
	return status;
}


void
KToolTip::SetSticky(bool enable)
{
	fIsSticky = enable;
}


bool
KToolTip::IsSticky() const
{
	return fIsSticky;
}


void
KToolTip::SetMouseRelativeLocation(BPoint location)
{
	fRelativeLocation = location;
}


BPoint
KToolTip::MouseRelativeLocation() const
{
	return fRelativeLocation;
}


void
KToolTip::SetAlignment(BAlignment alignment)
{
	fAlignment = alignment;
}


BAlignment
KToolTip::Alignment() const
{
	return fAlignment;
}


void
KToolTip::AttachedToWindow()
{
}


void
KToolTip::DetachedFromWindow()
{
}


bool
KToolTip::Lock()
{
	bool lockedLooper;
	while (true) {
		lockedLooper = View()->LockLooper();
		if (!lockedLooper) {
			KToolTipManager* manager = KToolTipManager::Manager();
			manager->Lock();

			if (View()->Window() != NULL) {
				manager->Unlock();
				continue;
			}
		}
		break;
	}

	fLockedLooper = lockedLooper;
	return true;
}


void
KToolTip::Unlock()
{
	if (fLockedLooper)
		View()->UnlockLooper();
	else
		KToolTipManager::Manager()->Unlock();
}


void
KToolTip::_InitData()
{
	fIsSticky = false;
	fRelativeLocation = BPoint(20, 20);
	fAlignment = BAlignment(B_ALIGN_RIGHT, B_ALIGN_BOTTOM);
}


//	#pragma mark -


KTextToolTip::KTextToolTip(const char* text)
{
	_InitData(text);
}


KTextToolTip::KTextToolTip(BMessage* archive)
{
	// TODO!
}


KTextToolTip::~KTextToolTip()
{
	delete fTextView;
}


/*static*/ KTextToolTip*
KTextToolTip::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "KTextToolTip"))
		return NULL;

	return new(std::nothrow) KTextToolTip(archive);
}


status_t
KTextToolTip::Archive(BMessage* archive, bool deep) const
{
	status_t status = KToolTip::Archive(archive, deep);
	// TODO!

	return status;
}


KView*
KTextToolTip::View() const
{
	return fTextView;
}


const char*
KTextToolTip::Text() const
{
	return fTextView->Text();
}


void
KTextToolTip::SetText(const char* text)
{
	if (!Lock())
		return;

	fTextView->SetText(text);
	fTextView->InvalidateLayout();

	Unlock();
}


void
KTextToolTip::_InitData(const char* text)
{
	fTextView = new KTextView("tool tip text");
	fTextView->SetText(text);
	fTextView->MakeEditable(false);
	fTextView->SetViewUIColor(B_TOOL_TIP_BACKGROUND_COLOR);
	rgb_color color = ui_color(B_TOOL_TIP_TEXT_COLOR);
	fTextView->SetFontAndColor(NULL, 0, &color);
	fTextView->SetWordWrap(false);
}

