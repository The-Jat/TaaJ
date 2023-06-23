/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <KGroupLayoutBuilder.h>

#include <new>

#include <KSpaceLayoutItem.h>


using std::nothrow;


// constructor
KGroupLayoutBuilder::KGroupLayoutBuilder(orientation orientation,
	float spacing)
	: fRootLayout((new KGroupView(orientation, spacing))->GroupLayout())
{
	_PushLayout(fRootLayout);
}

// constructor
KGroupLayoutBuilder::KGroupLayoutBuilder(KGroupLayout* layout)
	: fRootLayout(layout)
{
	_PushLayout(fRootLayout);
}

// constructor
KGroupLayoutBuilder::KGroupLayoutBuilder(KGroupView* view)
	: fRootLayout(view->GroupLayout())
{
	_PushLayout(fRootLayout);
}

// RootLayout
KGroupLayout*
KGroupLayoutBuilder::RootLayout() const
{
	return fRootLayout;
}

// TopLayout
KGroupLayout*
KGroupLayoutBuilder::TopLayout() const
{
	int32 count = fLayoutStack.CountItems();
	return (count > 0
		? (KGroupLayout*)fLayoutStack.ItemAt(count - 1) : NULL);
}

// GetTopLayout
KGroupLayoutBuilder&
KGroupLayoutBuilder::GetTopLayout(KGroupLayout** _layout)
{
	*_layout = TopLayout();
	return *this;
}

// TopView
KView*
KGroupLayoutBuilder::TopView() const
{
	if (KGroupLayout* layout = TopLayout())
		return layout->Owner();
	return NULL;
}

// GetTopView
KGroupLayoutBuilder&
KGroupLayoutBuilder::GetTopView(KView** _view)
{
	if (KGroupLayout* layout = TopLayout())
		*_view = layout->Owner();
	else
		*_view = NULL;

	return *this;
}

// Add
KGroupLayoutBuilder&
KGroupLayoutBuilder::Add(KView* view)
{
	if (KGroupLayout* layout = TopLayout())
		layout->AddView(view);
	return *this;
}

// Add
KGroupLayoutBuilder&
KGroupLayoutBuilder::Add(KView* view, float weight)
{
	if (KGroupLayout* layout = TopLayout())
		layout->AddView(view, weight);
	return *this;
}

// Add
KGroupLayoutBuilder&
KGroupLayoutBuilder::Add(KLayoutItem* item)
{
	if (KGroupLayout* layout = TopLayout())
		layout->AddItem(item);
	return *this;
}

// Add
KGroupLayoutBuilder&
KGroupLayoutBuilder::Add(KLayoutItem* item, float weight)
{
	if (KGroupLayout* layout = TopLayout())
		layout->AddItem(item, weight);
	return *this;
}

// AddGroup
KGroupLayoutBuilder&
KGroupLayoutBuilder::AddGroup(orientation orientation, float spacing,
	float weight)
{
	if (KGroupLayout* layout = TopLayout()) {
		KGroupView* group = new(nothrow) KGroupView(orientation, spacing);
		if (group) {
			if (layout->AddView(group, weight))
				_PushLayout(group->GroupLayout());
			else
				delete group;
		}
	}

	return *this;
}

// End
KGroupLayoutBuilder&
KGroupLayoutBuilder::End()
{
	_PopLayout();
	return *this;
}

// AddGlue
KGroupLayoutBuilder&
KGroupLayoutBuilder::AddGlue(float weight)
{
	if (KGroupLayout* layout = TopLayout())
		layout->AddItem(KSpaceLayoutItem::CreateGlue(), weight);

	return *this;
}

// AddStrut
KGroupLayoutBuilder&
KGroupLayoutBuilder::AddStrut(float size)
{
	if (KGroupLayout* layout = TopLayout()) {
		if (layout->Orientation() == B_HORIZONTAL)
			layout->AddItem(KSpaceLayoutItem::CreateHorizontalStrut(size));
		else
			layout->AddItem(KSpaceLayoutItem::CreateVerticalStrut(size));
	}

	return *this;
}

// SetInsets
KGroupLayoutBuilder& 
KGroupLayoutBuilder::SetInsets(float left, float top, float right, float bottom)
{
	if (KGroupLayout* layout = TopLayout())
		layout->SetInsets(left, top, right, bottom);

	return *this;
}
	
// cast operator KGroupLayout*
KGroupLayoutBuilder::operator KGroupLayout*()
{
	return fRootLayout;
}

// _PushLayout
bool
KGroupLayoutBuilder::_PushLayout(KGroupLayout* layout)
{
	return fLayoutStack.AddItem(layout);
}

// _PopLayout
void
KGroupLayoutBuilder::_PopLayout()
{
	int32 count = fLayoutStack.CountItems();
	if (count > 0)
		fLayoutStack.RemoveItem(count - 1);
}
