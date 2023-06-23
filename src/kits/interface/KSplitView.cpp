/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * Copyright 2015, Haiku, Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <KSplitView.h>

#include <stdio.h>

#include <Archivable.h>
#include <KControlLook.h>
#include <Cursor.h>

#include "KSplitLayout.h"


KSplitView::KSplitView(orientation orientation, float spacing)
	:
	KView(NULL,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_INVALIDATE_AFTER_LAYOUT,
		fSplitLayout = new KSplitLayout(orientation, spacing))
{
}


KSplitView::KSplitView(BMessage* from)
	:
	KView(BUnarchiver::PrepareArchive(from)),
	fSplitLayout(NULL)
{
	BUnarchiver(from).Finish();
}


KSplitView::~KSplitView()
{
}


void
KSplitView::SetInsets(float left, float top, float right, float bottom)
{
	left = KControlLook::ComposeSpacing(left);
	top = KControlLook::ComposeSpacing(top);
	right = KControlLook::ComposeSpacing(right);
	bottom = KControlLook::ComposeSpacing(bottom);

	fSplitLayout->SetInsets(left, top, right, bottom);
}


void
KSplitView::SetInsets(float horizontal, float vertical)
{
	horizontal = KControlLook::ComposeSpacing(horizontal);
	vertical = KControlLook::ComposeSpacing(vertical);
	fSplitLayout->SetInsets(horizontal, vertical, horizontal, vertical);
}


void
KSplitView::SetInsets(float insets)
{
	insets = KControlLook::ComposeSpacing(insets);
	fSplitLayout->SetInsets(insets, insets, insets, insets);
}


void
KSplitView::GetInsets(float* left, float* top, float* right,
	float* bottom) const
{
	fSplitLayout->GetInsets(left, top, right, bottom);
}


float
KSplitView::Spacing() const
{
	return fSplitLayout->Spacing();
}


void
KSplitView::SetSpacing(float spacing)
{
	fSplitLayout->SetSpacing(spacing);
}


orientation
KSplitView::Orientation() const
{
	return fSplitLayout->Orientation();
}


void
KSplitView::SetOrientation(orientation orientation)
{
	fSplitLayout->SetOrientation(orientation);
}


float
KSplitView::SplitterSize() const
{
	return fSplitLayout->SplitterSize();
}


void
KSplitView::SetSplitterSize(float size)
{
	fSplitLayout->SetSplitterSize(size);
}


int32
KSplitView::CountItems() const
{
	return fSplitLayout->CountItems();
}


float
KSplitView::ItemWeight(int32 index) const
{
	return fSplitLayout->ItemWeight(index);
}


float
KSplitView::ItemWeight(KLayoutItem* item) const
{
	return fSplitLayout->ItemWeight(item);
}


void
KSplitView::SetItemWeight(int32 index, float weight, bool invalidateLayout)
{
	fSplitLayout->SetItemWeight(index, weight, invalidateLayout);
}


void
KSplitView::SetItemWeight(KLayoutItem* item, float weight)
{
	fSplitLayout->SetItemWeight(item, weight);
}


bool
KSplitView::IsCollapsible(int32 index) const
{
	return fSplitLayout->IsCollapsible(index);
}


void
KSplitView::SetCollapsible(bool collapsible)
{
	fSplitLayout->SetCollapsible(collapsible);
}


void
KSplitView::SetCollapsible(int32 index, bool collapsible)
{
	fSplitLayout->SetCollapsible(index, collapsible);
}


void
KSplitView::SetCollapsible(int32 first, int32 last, bool collapsible)
{
	fSplitLayout->SetCollapsible(first, last, collapsible);
}


bool
KSplitView::IsItemCollapsed(int32 index) const
{
	return fSplitLayout->IsItemCollapsed(index);
}


void
KSplitView::SetItemCollapsed(int32 index, bool collapsed)
{
	fSplitLayout->SetItemCollapsed(index, collapsed);
}


void
KSplitView::AddChild(KView* child, KView* sibling)
{
	KView::AddChild(child, sibling);
}


bool
KSplitView::AddChild(KView* child, float weight)
{
	return fSplitLayout->AddView(child, weight);
}


bool
KSplitView::AddChild(int32 index, KView* child, float weight)
{
	return fSplitLayout->AddView(index, child, weight);
}


bool
KSplitView::AddChild(KLayoutItem* child)
{
	return fSplitLayout->AddItem(child);
}


bool
KSplitView::AddChild(KLayoutItem* child, float weight)
{
	return fSplitLayout->AddItem(child, weight);
}


bool
KSplitView::AddChild(int32 index, KLayoutItem* child, float weight)
{
	return fSplitLayout->AddItem(index, child, weight);
}


void
KSplitView::AttachedToWindow()
{
	AdoptParentColors();
}


void
KSplitView::Draw(BRect updateRect)
{
	// draw the splitters
	int32 draggedSplitterIndex = fSplitLayout->DraggedSplitter();
	int32 count = fSplitLayout->CountItems();
	for (int32 i = 0; i < count - 1; i++) {
		BRect frame = fSplitLayout->SplitterItemFrame(i);
		DrawSplitter(frame, updateRect, Orientation(),
			draggedSplitterIndex == i);
	}
}


void
KSplitView::DrawAfterChildren(BRect r)
{
	return KView::DrawAfterChildren(r);
}


void
KSplitView::MouseDown(BPoint where)
{
	SetMouseEventMask(B_POINTER_EVENTS,
		B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS);

	if (fSplitLayout->StartDraggingSplitter(where))
		Invalidate();
}


void
KSplitView::MouseUp(BPoint where)
{
	if (fSplitLayout->StopDraggingSplitter()) {
		Relayout();
		Invalidate();
	}
}


void
KSplitView::MouseMoved(BPoint where, uint32 transit, const BMessage* message)
{
	BCursor cursor(B_CURSOR_ID_SYSTEM_DEFAULT);

	int32 splitterIndex = fSplitLayout->DraggedSplitter();

	if (splitterIndex >= 0 || fSplitLayout->IsAboveSplitter(where)) {
		if (Orientation() == B_VERTICAL)
			cursor = BCursor(B_CURSOR_ID_RESIZE_NORTH_SOUTH);
		else
			cursor = BCursor(B_CURSOR_ID_RESIZE_EAST_WEST);
	}

	if (splitterIndex >= 0) {
		BRect oldFrame = fSplitLayout->SplitterItemFrame(splitterIndex);
		if (fSplitLayout->DragSplitter(where)) {
			Invalidate(oldFrame);
			Invalidate(fSplitLayout->SplitterItemFrame(splitterIndex));
		}
	}

	SetViewCursor(&cursor, true);
}


void
KSplitView::MessageReceived(BMessage* message)
{
	return KView::MessageReceived(message);
}


void
KSplitView::SetLayout(KLayout* layout)
{
	// not allowed
}


status_t
KSplitView::Archive(BMessage* into, bool deep) const
{
	return KView::Archive(into, deep);
}


status_t
KSplitView::AllArchived(BMessage* archive) const
{
	return KView::AllArchived(archive);
}


status_t
KSplitView::AllUnarchived(const BMessage* from)
{
	status_t err = KView::AllUnarchived(from);
	if (err == B_OK) {
		fSplitLayout = dynamic_cast<KSplitLayout*>(GetLayout());
		if (!fSplitLayout && GetLayout())
			return B_BAD_TYPE;
		else if (!fSplitLayout)
			return B_ERROR;
	}
	return err;
}


BArchivable*
KSplitView::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KSplitView"))
		return new KSplitView(from);
	return NULL;
}


void
KSplitView::DrawSplitter(BRect frame, const BRect& updateRect,
	orientation orientation, bool pressed)
{
	_DrawDefaultSplitter(this, frame, updateRect, orientation, pressed);
}


void
KSplitView::_DrawDefaultSplitter(KView* view, BRect frame,
	const BRect& updateRect, orientation orientation, bool pressed)
{
	uint32 flags = pressed ? KControlLook::B_ACTIVATED : 0;
	k_be_control_look->DrawSplitter(view, frame, updateRect, view->ViewColor(),
		orientation, flags, 0);
}


status_t
KSplitView::Perform(perform_code d, void* arg)
{
	return KView::Perform(d, arg);
}


void KSplitView::_ReservedSplitView1() {}
void KSplitView::_ReservedSplitView2() {}
void KSplitView::_ReservedSplitView3() {}
void KSplitView::_ReservedSplitView4() {}
void KSplitView::_ReservedSplitView5() {}
void KSplitView::_ReservedSplitView6() {}
void KSplitView::_ReservedSplitView7() {}
void KSplitView::_ReservedSplitView8() {}
void KSplitView::_ReservedSplitView9() {}
void KSplitView::_ReservedSplitView10() {}

