/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <KGridLayoutBuilder.h>

#include <new>

#include <KSpaceLayoutItem.h>


using std::nothrow;


// constructor
KGridLayoutBuilder::KGridLayoutBuilder(float horizontalSpacing,
		float verticalSpacing)
	: fLayout((new KGridView(horizontalSpacing, verticalSpacing))
					->GridLayout())
{
}

// constructor
KGridLayoutBuilder::KGridLayoutBuilder(KGridLayout* layout)
	: fLayout(layout)
{
}


// constructor
KGridLayoutBuilder::KGridLayoutBuilder(KGridView* view)
	: fLayout(view->GridLayout())
{
}

// GridLayout
KGridLayout*
KGridLayoutBuilder::GridLayout() const
{
	return fLayout;
}

// View
KView*
KGridLayoutBuilder::View() const
{
	return fLayout->Owner();
}

// GetGridLayout
KGridLayoutBuilder&
KGridLayoutBuilder::GetGridLayout(KGridLayout** _layout)
{
	*_layout = fLayout;
	return *this;
}

// GetView
KGridLayoutBuilder&
KGridLayoutBuilder::GetView(KView** _view)
{
	*_view = fLayout->Owner();
	return *this;
}

// Add
KGridLayoutBuilder&
KGridLayoutBuilder::Add(KView* view, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	fLayout->AddView(view, column, row, columnCount, rowCount);
	return *this;
}

// Add
KGridLayoutBuilder&
KGridLayoutBuilder::Add(KLayoutItem* item, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	fLayout->AddItem(item, column, row, columnCount, rowCount);
	return *this;
}

// SetColumnWeight
KGridLayoutBuilder&
KGridLayoutBuilder::SetColumnWeight(int32 column, float weight)
{
	fLayout->SetColumnWeight(column, weight);
	return *this;
}

// SetRowWeight
KGridLayoutBuilder&
KGridLayoutBuilder::SetRowWeight(int32 row, float weight)
{
	fLayout->SetRowWeight(row, weight);
	return *this;
}

// SetInsets
KGridLayoutBuilder&
KGridLayoutBuilder::SetInsets(float left, float top, float right, float bottom)
{
	fLayout->SetInsets(left, top, right, bottom);
	return *this;
}
	
// cast operator KGridLayout*
KGridLayoutBuilder::operator KGridLayout*()
{
	return fLayout;
}

