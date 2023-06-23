/*
 * Copyright 2009-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__LAYOUT_BUILDER_H
#define	K__LAYOUT_BUILDER_H


#include <new>

#include <KCardLayout.h>
#include <KCardView.h>
#include <KGridLayout.h>
#include <KGridView.h>
#include <KGroupLayout.h>
#include <KGroupView.h>
#include <KMenu.h>
#include <KMenuField.h>
#include <KMenuItem.h>
#include <KSpaceLayoutItem.h>
#include <KSplitView.h>
#include <KTextControl.h>
#include <Khidki.h>


namespace KLayoutBuilder {

template<typename ParentBuilder> class KBase;
template<typename ParentBuilder = void*> class Group;
template<typename ParentBuilder = void*> class Grid;
template<typename ParentBuilder = void*> class Split;
template<typename ParentBuilder = void*> class Cards;
template<typename ParentBuilder = void*> class Menu;
template<typename ParentBuilder = void*> class MenuItem;


template<typename ParentBuilder>
class KBase {
protected:
	inline						KBase();

public:
	inline	void				SetParent(ParentBuilder* parent);
		// conceptually private
	inline	ParentBuilder&		End();

protected:
			ParentBuilder*		fParent;
};


template<typename ParentBuilder>
class Group : public KBase<ParentBuilder> {
public:
	typedef Group<ParentBuilder>	ThisBuilder;
	typedef Group<ThisBuilder>		GroupBuilder;
	typedef Grid<ThisBuilder>		GridBuilder;
	typedef Split<ThisBuilder>		SplitBuilder;
	typedef Cards<ThisBuilder>		CardBuilder;

public:
	inline						Group(orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
	inline						Group(KWindow* window,
									orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
	inline						Group(KView* view,
									orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
	inline						Group(KGroupLayout* layout);
	inline						Group(KGroupView* view);

	inline	KGroupLayout*		Layout() const;
	inline	KView*				View() const;
	inline	ThisBuilder&		GetLayout(KGroupLayout** _layout);
	inline	ThisBuilder&		GetView(KView** _view);

	inline	ThisBuilder&		Add(KView* view);
	inline	ThisBuilder&		Add(KView* view, float weight);
	inline	ThisBuilder&		Add(KLayoutItem* item);
	inline	ThisBuilder&		Add(KLayoutItem* item, float weight);

	inline	GroupBuilder		AddGroup(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	GroupBuilder		AddGroup(KGroupView* groupView,
									float weight = 1.0f);
	inline	GroupBuilder		AddGroup(KGroupLayout* groupLayout,
									float weight = 1.0f);

	inline	GridBuilder			AddGrid(float horizontal
										= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	GridBuilder			AddGrid(KGridLayout* gridLayout,
									float weight = 1.0f);
	inline	GridBuilder			AddGrid(KGridView* gridView,
									float weight = 1.0f);

	inline	SplitBuilder		AddSplit(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	SplitBuilder		AddSplit(KSplitView* splitView,
									float weight = 1.0f);

	inline	CardBuilder			AddCards(float weight = 1.0f);
	inline	CardBuilder			AddCards(KCardLayout* cardLayout,
									float weight = 1.0f);
	inline	CardBuilder			AddCards(KCardView* cardView,
									float weight = 1.0f);

	inline	ThisBuilder&		AddGlue(float weight = 1.0f);
	inline	ThisBuilder&		AddStrut(float size);

	inline	ThisBuilder&		SetInsets(float left, float top, float right,
									float bottom);
	inline	ThisBuilder&		SetInsets(float horizontal, float vertical);
	inline	ThisBuilder&		SetInsets(float insets);

	inline	ThisBuilder&		SetExplicitMinSize(BSize size);
	inline	ThisBuilder&		SetExplicitMaxSize(BSize size);
	inline	ThisBuilder&		SetExplicitPreferredSize(BSize size);
	inline	ThisBuilder&		SetExplicitAlignment(BAlignment alignment);

	inline						operator KGroupLayout*();

private:
			KGroupLayout*		fLayout;
};


template<typename ParentBuilder>
class Grid : public KBase<ParentBuilder> {
public:
	typedef Grid<ParentBuilder>		ThisBuilder;
	typedef Group<ThisBuilder>		GroupBuilder;
	typedef Grid<ThisBuilder>		GridBuilder;
	typedef Split<ThisBuilder>		SplitBuilder;
	typedef Cards<ThisBuilder>		CardBuilder;

public:
	inline						Grid(float horizontal
										= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
	inline						Grid(KWindow* window,
									float horizontal = B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
	inline						Grid(KView* view,
									float horizontal = B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
	inline						Grid(KGridLayout* layout);
	inline						Grid(KGridView* view);

	inline	KGridLayout*		Layout() const;
	inline	KView*				View() const;
	inline	ThisBuilder&		GetLayout(KGridLayout** _layout);
	inline	ThisBuilder&		GetView(KView** _view);

	inline	ThisBuilder&		Add(KView* view, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	ThisBuilder&		Add(KLayoutItem* item, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	ThisBuilder&		AddMenuField(KMenuField* menuField,
									int32 column, int32 row,
									alignment labelAlignment
										= B_ALIGN_HORIZONTAL_UNSET,
									int32 labelColumnCount = 1,
									int32 fieldColumnCount = 1,
									int32 rowCount = 1);
	inline	ThisBuilder&		AddTextControl(KTextControl* textControl,
									int32 column, int32 row,
									alignment labelAlignment
										= B_ALIGN_HORIZONTAL_UNSET,
									int32 labelColumnCount = 1,
									int32 textColumnCount = 1,
									int32 rowCount = 1);

	inline	GroupBuilder		AddGroup(orientation orientation,
									float spacing, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	GroupBuilder		AddGroup(KGroupView* groupView,	int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);
	inline	GroupBuilder		AddGroup(KGroupLayout* groupLayout,
									int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);

	inline	GridBuilder			AddGrid(float horizontalSpacing,
									float verticalSpacing, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);
	inline	GridBuilder			AddGrid(KGridLayout* gridLayout,
									int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	GridBuilder			AddGrid(KGridView* gridView,
									int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);

	inline	SplitBuilder		AddSplit(orientation orientation,
									float spacing, int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	SplitBuilder		AddSplit(KSplitView* splitView, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);

	inline	CardBuilder			AddCards(int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);
	inline	CardBuilder			AddCards(KCardLayout* cardLayout, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);
	inline	CardBuilder			AddCards(KCardView* cardView, int32 column,
									int32 row, int32 columnCount = 1,
									int32 rowCount = 1);

	inline	ThisBuilder&		AddGlue(int32 column, int32 row,
									int32 columnCount = 1, int32 rowCount = 1);

	inline	ThisBuilder&		SetHorizontalSpacing(float spacing);
	inline	ThisBuilder&		SetVerticalSpacing(float spacing);
	inline	ThisBuilder&		SetSpacing(float horizontal, float vertical);

	inline	ThisBuilder&		SetColumnWeight(int32 column, float weight);
	inline	ThisBuilder&		SetRowWeight(int32 row, float weight);

	inline	ThisBuilder&		SetInsets(float left, float top, float right,
									float bottom);
	inline	ThisBuilder&		SetInsets(float horizontal, float vertical);
	inline	ThisBuilder&		SetInsets(float insets);

	inline	ThisBuilder&		SetExplicitMinSize(BSize size);
	inline	ThisBuilder&		SetExplicitMaxSize(BSize size);
	inline	ThisBuilder&		SetExplicitPreferredSize(BSize size);
	inline	ThisBuilder&		SetExplicitAlignment(BAlignment alignment);

	inline						operator KGridLayout*();

private:
			KGridLayout*		fLayout;
};


template<typename ParentBuilder>
class Split : public KBase<ParentBuilder> {
public:
	typedef Split<ParentBuilder>	ThisBuilder;
	typedef Group<ThisBuilder>		GroupBuilder;
	typedef Grid<ThisBuilder>		GridBuilder;
	typedef Split<ThisBuilder>		SplitBuilder;
	typedef Cards<ThisBuilder>		CardBuilder;

public:
	inline						Split(orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
	inline						Split(KSplitView* view);

	inline	KSplitView*			View() const;
	inline	ThisBuilder&		GetView(KView** _view);
	inline	ThisBuilder&		GetSplitView(KSplitView** _view);

	inline	ThisBuilder&		Add(KView* view);
	inline	ThisBuilder&		Add(KView* view, float weight);
	inline	ThisBuilder&		Add(KLayoutItem* item);
	inline	ThisBuilder&		Add(KLayoutItem* item, float weight);

	inline	GroupBuilder		AddGroup(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	GroupBuilder		AddGroup(KGroupView* groupView,
									float weight = 1.0f);
	inline	GroupBuilder		AddGroup(KGroupLayout* groupLayout,
									float weight = 1.0f);

	inline	GridBuilder			AddGrid(float horizontal
											= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	GridBuilder			AddGrid(KGridView* gridView,
									float weight = 1.0f);
	inline	GridBuilder			AddGrid(KGridLayout* gridLayout,
									float weight = 1.0f);

	inline	SplitBuilder		AddSplit(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING,
									float weight = 1.0f);
	inline	SplitBuilder		AddSplit(KSplitView* splitView,
									float weight = 1.0f);

	inline	CardBuilder			AddCards(float weight = 1.0f);
	inline	CardBuilder			AddCards(KCardLayout* cardLayout,
									float weight = 1.0f);
	inline	CardBuilder			AddCards(KCardView* cardView,
									float weight = 1.0f);

	inline	ThisBuilder&		SetCollapsible(bool collapsible);
	inline	ThisBuilder&		SetCollapsible(int32 index, bool collapsible);
	inline	ThisBuilder&		SetCollapsible(int32 first, int32 last,
									bool collapsible);

	inline	ThisBuilder&		SetInsets(float left, float top, float right,
									float bottom);
	inline	ThisBuilder&		SetInsets(float horizontal, float vertical);
	inline	ThisBuilder&		SetInsets(float insets);

	inline						operator KSplitView*();

private:
			KSplitView*			fView;
};

template<typename ParentBuilder>
class Cards : public KBase<ParentBuilder> {
public:
	typedef Cards<ParentBuilder>	ThisBuilder;
	typedef Group<ThisBuilder>		GroupBuilder;
	typedef Grid<ThisBuilder>		GridBuilder;
	typedef Split<ThisBuilder>		SplitBuilder;
	typedef Cards<ThisBuilder>		CardBuilder;

public:
	inline						Cards();
	inline						Cards(KWindow* window);
	inline						Cards(KView* view);
	inline						Cards(KCardLayout* layout);
	inline						Cards(KCardView* view);

	inline	KCardLayout*		Layout() const;
	inline	KView*				View() const;
	inline	ThisBuilder&		GetLayout(KCardLayout** _layout);
	inline	ThisBuilder&		GetView(KView** _view);

	inline	ThisBuilder&		Add(KView* view);
	inline	ThisBuilder&		Add(KLayoutItem* item);

	inline	GroupBuilder		AddGroup(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING);
	inline	GroupBuilder		AddGroup(KGroupView* groupView);
	inline	GroupBuilder		AddGroup(KGroupLayout* groupLayout);

	inline	GridBuilder			AddGrid(float horizontal
										= B_USE_DEFAULT_SPACING,
									float vertical = B_USE_DEFAULT_SPACING);
	inline	GridBuilder			AddGrid(KGridLayout* gridLayout);
	inline	GridBuilder			AddGrid(KGridView* gridView);

	inline	SplitBuilder		AddSplit(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING);
	inline	SplitBuilder		AddSplit(KSplitView* splitView);

	inline	CardBuilder			AddCards();
	inline	CardBuilder			AddCards(KCardLayout* cardLayout);
	inline	CardBuilder			AddCards(KCardView* cardView);

	inline	ThisBuilder&		SetExplicitMinSize(BSize size);
	inline	ThisBuilder&		SetExplicitMaxSize(BSize size);
	inline	ThisBuilder&		SetExplicitPreferredSize(BSize size);
	inline	ThisBuilder&		SetExplicitAlignment(BAlignment alignment);

	inline	ThisBuilder&		SetVisibleItem(int32 index);

	inline						operator KCardLayout*();

private:
			KCardLayout*		fLayout;
};


template<typename ParentBuilder>
class Menu : public KBase<ParentBuilder> {
public:
	typedef Menu<ParentBuilder>		ThisBuilder;
	typedef MenuItem<ParentBuilder>	ItemBuilder;
	typedef Menu<ThisBuilder>		MenuBuilder;

public:
	inline						Menu(KMenu* menu);

	inline	ThisBuilder&		GetMenu(KMenu*& _menu);

	inline	ItemBuilder			AddItem(KMenuItem* item);
	inline	ItemBuilder			AddItem(KMenu* menu);
	inline	ItemBuilder			AddItem(const char* label, BMessage* message,
									char shortcut = 0, uint32 modifiers = 0);
	inline	ItemBuilder			AddItem(const char* label, uint32 messageWhat,
									char shortcut = 0, uint32 modifiers = 0);

	inline	MenuBuilder			AddMenu(KMenu* menu);
	inline	MenuBuilder			AddMenu(const char* title,
									k_menu_layout layout = K_ITEMS_IN_COLUMN);

	inline	ThisBuilder&		AddSeparator();

private:
			KMenu*				fMenu;
};


template<typename ParentBuilder>
class MenuItem : public Menu<ParentBuilder> {
public:
	typedef MenuItem<ParentBuilder>	ThisBuilder;

public:
	inline						MenuItem(ParentBuilder* parentBuilder,
									KMenu* menu, KMenuItem* item);

	inline	ThisBuilder&		GetItem(KMenuItem*& _item);

	inline	ThisBuilder&		SetEnabled(bool enabled);

private:
			KMenuItem*			fMenuItem;
};


// #pragma mark - KBase


template<typename ParentBuilder>
KBase<ParentBuilder>::KBase()
	:
	fParent(NULL)
{
}


template<typename ParentBuilder>
void
KBase<ParentBuilder>::SetParent(ParentBuilder* parent)
{
	fParent = parent;
}


template<typename ParentBuilder>
ParentBuilder&
KBase<ParentBuilder>::End()
{
	return *fParent;
}


// #pragma mark - Group


template<typename ParentBuilder>
Group<ParentBuilder>::Group(orientation orientation, float spacing)
	:
	fLayout((new KGroupView(orientation, spacing))->GroupLayout())
{
}


template<typename ParentBuilder>
Group<ParentBuilder>::Group(KWindow* window, orientation orientation,
		float spacing)
	:
	fLayout(new KGroupLayout(orientation, spacing))
{
	window->SetLayout(fLayout);
	fLayout->Owner()->AdoptSystemColors();
}


template<typename ParentBuilder>
Group<ParentBuilder>::Group(KView* view, orientation orientation,
		float spacing)
	:
	fLayout(new KGroupLayout(orientation, spacing))
{

	if (view->HasDefaultColors())
		view->AdoptSystemColors();

	view->SetLayout(fLayout);
}


template<typename ParentBuilder>
Group<ParentBuilder>::Group(KGroupLayout* layout)
	:
	fLayout(layout)
{
}


template<typename ParentBuilder>
Group<ParentBuilder>::Group(KGroupView* view)
	:
	fLayout(view->GroupLayout())
{
}


template<typename ParentBuilder>
KGroupLayout*
Group<ParentBuilder>::Layout() const
{
	return fLayout;
}


template<typename ParentBuilder>
KView*
Group<ParentBuilder>::View() const
{
	return fLayout->Owner();
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::GetLayout(KGroupLayout** _layout)
{
	*_layout = fLayout;
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::GetView(KView** _view)
{
	*_view = fLayout->Owner();
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::Add(KView* view)
{
	fLayout->AddView(view);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::Add(KView* view, float weight)
{
	fLayout->AddView(view, weight);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::Add(KLayoutItem* item)
{
	fLayout->AddItem(item);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::Add(KLayoutItem* item, float weight)
{
	fLayout->AddItem(item, weight);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GroupBuilder
Group<ParentBuilder>::AddGroup(orientation orientation, float spacing,
		float weight)
{
	GroupBuilder builder(new KGroupLayout(orientation, spacing));
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GroupBuilder
Group<ParentBuilder>::AddGroup(KGroupView* groupView, float weight)
{
	GroupBuilder builder(groupView);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GroupBuilder
Group<ParentBuilder>::AddGroup(KGroupLayout* groupLayout, float weight)
{
	GroupBuilder builder(groupLayout);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GridBuilder
Group<ParentBuilder>::AddGrid(float horizontalSpacing,
	float verticalSpacing, float weight)
{
	GridBuilder builder(new KGridLayout(horizontalSpacing, verticalSpacing));
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GridBuilder
Group<ParentBuilder>::AddGrid(KGridLayout* gridLayout, float weight)
{
	GridBuilder builder(gridLayout);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::GridBuilder
Group<ParentBuilder>::AddGrid(KGridView* gridView, float weight)
{
	GridBuilder builder(gridView);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::SplitBuilder
Group<ParentBuilder>::AddSplit(orientation orientation, float spacing,
		float weight)
{
	SplitBuilder builder(orientation, spacing);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::SplitBuilder
Group<ParentBuilder>::AddSplit(KSplitView* splitView, float weight)
{
	SplitBuilder builder(splitView);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::CardBuilder
Group<ParentBuilder>::AddCards(float weight)
{
	CardBuilder builder;
	builder.SetParent(this);
	fLayout->AddView(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::CardBuilder
Group<ParentBuilder>::AddCards(KCardLayout* cardLayout, float weight)
{
	CardBuilder builder(cardLayout);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::CardBuilder
Group<ParentBuilder>::AddCards(KCardView* cardView, float weight)
{
	CardBuilder builder(cardView);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::AddGlue(float weight)
{
	fLayout->AddItem(KSpaceLayoutItem::CreateGlue(), weight);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::AddStrut(float size)
{
	if (fLayout->Orientation() == B_HORIZONTAL)
		fLayout->AddItem(KSpaceLayoutItem::CreateHorizontalStrut(size));
	else
		fLayout->AddItem(KSpaceLayoutItem::CreateVerticalStrut(size));

	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetInsets(float left, float top, float right,
	float bottom)
{
	fLayout->SetInsets(left, top, right, bottom);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetInsets(float horizontal, float vertical)
{
	fLayout->SetInsets(horizontal, vertical);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetInsets(float insets)
{
	fLayout->SetInsets(insets);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetExplicitMinSize(BSize size)
{
	fLayout->SetExplicitMinSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetExplicitMaxSize(BSize size)
{
	fLayout->SetExplicitMaxSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetExplicitPreferredSize(BSize size)
{
	fLayout->SetExplicitPreferredSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Group<ParentBuilder>::ThisBuilder&
Group<ParentBuilder>::SetExplicitAlignment(BAlignment alignment)
{
	fLayout->SetExplicitAlignment(alignment);
	return *this;
}


template<typename ParentBuilder>
Group<ParentBuilder>::operator KGroupLayout*()
{
	return fLayout;
}


// #pragma mark - Grid


template<typename ParentBuilder>
Grid<ParentBuilder>::Grid(float horizontalSpacing, float verticalSpacing)
	:
	fLayout((new KGridView(horizontalSpacing, verticalSpacing))->GridLayout())
{
}


template<typename ParentBuilder>
Grid<ParentBuilder>::Grid(KWindow* window, float horizontalSpacing,
	float verticalSpacing)
	:
	fLayout(new KGridLayout(horizontalSpacing, verticalSpacing))
{
	window->SetLayout(fLayout);
	fLayout->Owner()->AdoptSystemColors();
}


template<typename ParentBuilder>
Grid<ParentBuilder>::Grid(KView* view, float horizontalSpacing,
	float verticalSpacing)
	:
	fLayout(new KGridLayout(horizontalSpacing, verticalSpacing))
{
	if (view->HasDefaultColors())
		view->AdoptSystemColors();

	view->SetLayout(fLayout);
}


template<typename ParentBuilder>
Grid<ParentBuilder>::Grid(KGridLayout* layout)
	:
	fLayout(layout)
{
}


template<typename ParentBuilder>
Grid<ParentBuilder>::Grid(KGridView* view)
	:
	fLayout(view->GridLayout())
{
}


template<typename ParentBuilder>
KGridLayout*
Grid<ParentBuilder>::Layout() const
{
	return fLayout;
}


template<typename ParentBuilder>
KView*
Grid<ParentBuilder>::View() const
{
	return fLayout->Owner();
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::GetLayout(KGridLayout** _layout)
{
	*_layout = fLayout;
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::GetView(KView** _view)
{
	*_view = fLayout->Owner();
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::Add(KView* view, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	fLayout->AddView(view, column, row, columnCount, rowCount);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::Add(KLayoutItem* item, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	fLayout->AddItem(item, column, row, columnCount, rowCount);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::AddMenuField(KMenuField* menuField, int32 column,
	int32 row, alignment labelAlignment, int32 labelColumnCount,
	int32 fieldColumnCount, int32 rowCount)
{
	KLayoutItem* item = menuField->CreateLabelLayoutItem();
	item->SetExplicitAlignment(
		BAlignment(labelAlignment, B_ALIGN_VERTICAL_UNSET));
	fLayout->AddItem(item, column, row, labelColumnCount, rowCount);
	fLayout->AddItem(menuField->CreateMenuBarLayoutItem(),
		column + labelColumnCount, row, fieldColumnCount, rowCount);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::AddTextControl(KTextControl* textControl, int32 column,
	int32 row, alignment labelAlignment, int32 labelColumnCount,
	int32 textColumnCount, int32 rowCount)
{
	KLayoutItem* item = textControl->CreateLabelLayoutItem();
	item->SetExplicitAlignment(
		BAlignment(labelAlignment, B_ALIGN_VERTICAL_UNSET));
	fLayout->AddItem(item, column, row, labelColumnCount, rowCount);
	fLayout->AddItem(textControl->CreateTextViewLayoutItem(),
		column + labelColumnCount, row, textColumnCount, rowCount);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::GroupBuilder
Grid<ParentBuilder>::AddGroup(orientation orientation, float spacing,
		int32 column, int32 row, int32 columnCount, int32 rowCount)
{
	GroupBuilder builder(new KGroupLayout(orientation, spacing));
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::GroupBuilder
Grid<ParentBuilder>::AddGroup(KGroupView* groupView, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	GroupBuilder builder(groupView);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::GroupBuilder
Grid<ParentBuilder>::AddGroup(KGroupLayout* groupLayout, int32 column,
	int32 row, int32 columnCount, int32 rowCount)
{
	GroupBuilder builder(groupLayout);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::GridBuilder
Grid<ParentBuilder>::AddGrid(float horizontalSpacing, float verticalSpacing,
	int32 column, int32 row, int32 columnCount, int32 rowCount)
{
	GridBuilder builder(new KGridLayout(horizontalSpacing, verticalSpacing));
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::GridBuilder
Grid<ParentBuilder>::AddGrid(KGridView* gridView, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	GridBuilder builder(gridView);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::SplitBuilder
Grid<ParentBuilder>::AddSplit(orientation orientation, float spacing,
	int32 column, int32 row, int32 columnCount, int32 rowCount)
{
	SplitBuilder builder(orientation, spacing);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::SplitBuilder
Grid<ParentBuilder>::AddSplit(KSplitView* splitView, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	SplitBuilder builder(splitView);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::CardBuilder
Grid<ParentBuilder>::AddCards(int32 column, int32 row, int32 columnCount,
	int32 rowCount)
{
	CardBuilder builder;
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::CardBuilder
Grid<ParentBuilder>::AddCards(KCardLayout* cardLayout, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	CardBuilder builder(cardLayout);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::CardBuilder
Grid<ParentBuilder>::AddCards(KCardView* cardView, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	CardBuilder builder(cardView);
	builder.SetParent(this);
	fLayout->AddView(builder.View(), column, row, columnCount, rowCount);
	return builder;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::AddGlue(int32 column, int32 row, int32 columnCount,
	int32 rowCount)
{
	fLayout->AddItem(KSpaceLayoutItem::CreateGlue(), column, row, columnCount,
		rowCount);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetHorizontalSpacing(float spacing)
{
	fLayout->SetHorizontalSpacing(spacing);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetVerticalSpacing(float spacing)
{
	fLayout->SetVerticalSpacing(spacing);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetSpacing(float horizontal, float vertical)
{
	fLayout->SetSpacing(horizontal, vertical);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetColumnWeight(int32 column, float weight)
{
	fLayout->SetColumnWeight(column, weight);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetRowWeight(int32 row, float weight)
{
	fLayout->SetRowWeight(row, weight);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetInsets(float left, float top, float right,
	float bottom)
{
	fLayout->SetInsets(left, top, right, bottom);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetInsets(float horizontal, float vertical)
{
	fLayout->SetInsets(horizontal, vertical);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetInsets(float insets)
{
	fLayout->SetInsets(insets);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetExplicitMinSize(BSize size)
{
	fLayout->SetExplicitMinSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetExplicitMaxSize(BSize size)
{
	fLayout->SetExplicitMaxSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetExplicitPreferredSize(BSize size)
{
	fLayout->SetExplicitPreferredSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Grid<ParentBuilder>::ThisBuilder&
Grid<ParentBuilder>::SetExplicitAlignment(BAlignment alignment)
{
	fLayout->SetExplicitAlignment(alignment);
	return *this;
}


template<typename ParentBuilder>
Grid<ParentBuilder>::operator KGridLayout*()
{
	return fLayout;
}


// #pragma mark - Split


template<typename ParentBuilder>
Split<ParentBuilder>::Split(orientation orientation, float spacing)
	:
	fView(new KSplitView(orientation, spacing))
{
}


template<typename ParentBuilder>
Split<ParentBuilder>::Split(KSplitView* view)
	:
	fView(view)
{
}


template<typename ParentBuilder>
KSplitView*
Split<ParentBuilder>::View() const
{
	return fView;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::GetView(KView** _view)
{
	*_view = fView;
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::GetSplitView(KSplitView** _view)
{
	*_view = fView;
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::Add(KView* view)
{
	fView->AddChild(view);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::Add(KView* view, float weight)
{
	fView->AddChild(view, weight);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::Add(KLayoutItem* item)
{
	fView->AddChild(item);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::Add(KLayoutItem* item, float weight)
{
	fView->AddChild(item, weight);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GroupBuilder
Split<ParentBuilder>::AddGroup(orientation orientation, float spacing,
		float weight)
{
	GroupBuilder builder(new KGroupLayout(orientation, spacing));
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GroupBuilder
Split<ParentBuilder>::AddGroup(KGroupView* groupView, float weight)
{
	GroupBuilder builder(groupView);
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GroupBuilder
Split<ParentBuilder>::AddGroup(KGroupLayout* groupLayout, float weight)
{
	GroupBuilder builder(groupLayout);
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GridBuilder
Split<ParentBuilder>::AddGrid(float horizontalSpacing, float verticalSpacing,
	float weight)
{
	GridBuilder builder(new KGridLayout(horizontalSpacing, verticalSpacing));
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GridBuilder
Split<ParentBuilder>::AddGrid(KGridView* gridView, float weight)
{
	GridBuilder builder(gridView);
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::GridBuilder
Split<ParentBuilder>::AddGrid(KGridLayout* layout, float weight)
{
	GridBuilder builder(layout);
	builder.SetParent(this);
	fView->AddChild(builder.Layout(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::SplitBuilder
Split<ParentBuilder>::AddSplit(orientation orientation, float spacing,
		float weight)
{
	SplitBuilder builder(orientation, spacing);
	builder.SetParent(this);
	fView->AddChild(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::CardBuilder
Split<ParentBuilder>::AddCards(float weight)
{
	CardBuilder builder;
	builder.SetParent(this);
	fView->AddChild(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::CardBuilder
Split<ParentBuilder>::AddCards(KCardLayout* cardLayout, float weight)
{
	CardBuilder builder(cardLayout);
	builder.SetParent(this);
	fView->AddChild(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::CardBuilder
Split<ParentBuilder>::AddCards(KCardView* cardView, float weight)
{
	CardBuilder builder(cardView);
	builder.SetParent(this);
	fView->AddChild(builder.View(), weight);
	return builder;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetCollapsible(bool collapsible)
{
	fView->SetCollapsible(collapsible);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetCollapsible(int32 index, bool collapsible)
{
	fView->SetCollapsible(index, collapsible);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetCollapsible(int32 first, int32 last, bool collapsible)
{
	fView->SetCollapsible(first, last, collapsible);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetInsets(float left, float top, float right,
	float bottom)
{
	fView->SetInsets(left, top, right, bottom);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetInsets(float horizontal, float vertical)
{
	fView->SetInsets(horizontal, vertical);
	return *this;
}


template<typename ParentBuilder>
typename Split<ParentBuilder>::ThisBuilder&
Split<ParentBuilder>::SetInsets(float insets)
{
	fView->SetInsets(insets);
	return *this;
}


template<typename ParentBuilder>
Split<ParentBuilder>::operator KSplitView*()
{
	return fView;
}


// #pragma mark - Cards


template<typename ParentBuilder>
Cards<ParentBuilder>::Cards()
	:
	fLayout((new KCardView())->CardLayout())
{
}


template<typename ParentBuilder>
Cards<ParentBuilder>::Cards(KWindow* window)
	:
	fLayout(new KCardLayout())
{
	window->SetLayout(fLayout);

	fLayout->Owner()->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


template<typename ParentBuilder>
Cards<ParentBuilder>::Cards(KView* view)
	:
	fLayout(new KCardLayout())
{
	view->SetLayout(fLayout);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


template<typename ParentBuilder>
Cards<ParentBuilder>::Cards(KCardLayout* layout)
	:
	fLayout(layout)
{
}


template<typename ParentBuilder>
Cards<ParentBuilder>::Cards(KCardView* view)
	:
	fLayout(view->CardLayout())
{
}


template<typename ParentBuilder>
KCardLayout*
Cards<ParentBuilder>::Layout() const
{
	return fLayout;
}


template<typename ParentBuilder>
KView*
Cards<ParentBuilder>::View() const
{
	return fLayout->Owner();
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::GetLayout(KCardLayout** _layout)
{
	*_layout = fLayout;
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::GetView(KView** _view)
{
	*_view = fLayout->Owner();
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::Add(KView* view)
{
	fLayout->AddView(view);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::Add(KLayoutItem* item)
{
	fLayout->AddItem(item);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GroupBuilder
Cards<ParentBuilder>::AddGroup(orientation orientation, float spacing)
{
	GroupBuilder builder(new KGroupLayout(orientation, spacing));
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GroupBuilder
Cards<ParentBuilder>::AddGroup(KGroupView* groupView)
{
	GroupBuilder builder(groupView);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GroupBuilder
Cards<ParentBuilder>::AddGroup(KGroupLayout* groupLayout)
{
	GroupBuilder builder(groupLayout);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GridBuilder
Cards<ParentBuilder>::AddGrid(float horizontal, float vertical)
{
	GridBuilder builder(horizontal, vertical);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GridBuilder
Cards<ParentBuilder>::AddGrid(KGridLayout* gridLayout)
{
	GridBuilder builder(gridLayout);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::GridBuilder
Cards<ParentBuilder>::AddGrid(KGridView* gridView)
{
	GridBuilder builder(gridView);
	builder.SetParent(this);
	fLayout->AddItem(builder.Layout());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::SplitBuilder
Cards<ParentBuilder>::AddSplit(orientation orientation, float spacing)
{
	SplitBuilder builder(orientation, spacing);
	builder.SetParent(this);
	fLayout->AddView(builder.View());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::SplitBuilder
Cards<ParentBuilder>::AddSplit(KSplitView* splitView)
{
	SplitBuilder builder(splitView);
	builder.SetParent(this);
	fLayout->AddView(builder.View());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::CardBuilder
Cards<ParentBuilder>::AddCards()
{
	CardBuilder builder;
	builder.SetParent(this);
	fLayout->AddView(builder.View());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::CardBuilder
Cards<ParentBuilder>::AddCards(KCardLayout* cardLayout)
{
	CardBuilder builder(cardLayout);
	builder.SetParent(this);
	fLayout->AddView(builder.View());
	return builder;
}

template<typename ParentBuilder>
typename Cards<ParentBuilder>::CardBuilder
Cards<ParentBuilder>::AddCards(KCardView* cardView)
{
	CardBuilder builder(cardView);
	builder.SetParent(this);
	fLayout->AddView(builder.View());
	return builder;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::SetExplicitMinSize(BSize size)
{
	fLayout->SetExplicitMinSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::SetExplicitMaxSize(BSize size)
{
	fLayout->SetExplicitMaxSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::SetExplicitPreferredSize(BSize size)
{
	fLayout->SetExplicitPreferredSize(size);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::SetExplicitAlignment(BAlignment alignment)
{
	fLayout->SetExplicitAlignment(alignment);
	return *this;
}


template<typename ParentBuilder>
typename Cards<ParentBuilder>::ThisBuilder&
Cards<ParentBuilder>::SetVisibleItem(int32 item)
{
	fLayout->SetVisibleItem(item);
	return *this;
}


template<typename ParentBuilder>
Cards<ParentBuilder>::operator KCardLayout*()
{
	return fLayout;
}


// #pragma mark - Menu


template<typename ParentBuilder>
Menu<ParentBuilder>::Menu(KMenu* menu)
	:
	fMenu(menu)
{
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ThisBuilder&
Menu<ParentBuilder>::GetMenu(KMenu*& _menu)
{
	_menu = fMenu;
	return *this;
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ItemBuilder
Menu<ParentBuilder>::AddItem(KMenuItem* item)
{
	fMenu->AddItem(item);
	return MenuItem<ParentBuilder>(this->fParent, fMenu, item);
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ItemBuilder
Menu<ParentBuilder>::AddItem(KMenu* menu)
{
	if (!fMenu->AddItem(menu))
		throw std::bad_alloc();

	return MenuItem<ParentBuilder>(this->fParent, fMenu,
		fMenu->ItemAt(fMenu->CountItems() - 1));
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ItemBuilder
Menu<ParentBuilder>::AddItem(const char* label, BMessage* message,
	char shortcut, uint32 modifiers)
{
	KMenuItem* item = new KMenuItem(label, message, shortcut, modifiers);
	if (!fMenu->AddItem(item)) {
		delete item;
		item = NULL;
	}

	return MenuItem<ParentBuilder>(this->fParent, fMenu, item);
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ItemBuilder
Menu<ParentBuilder>::AddItem(const char* label, uint32 messageWhat,
	char shortcut, uint32 modifiers)
{
	BMessage* message = new BMessage(messageWhat);
	KMenuItem* item;
	try {
		item = new KMenuItem(label, message, shortcut, modifiers);
	} catch (...) {
		delete message;
		throw;
	}

	if (!fMenu->AddItem(item)) {
		delete item;
		item = NULL;
	}

	return MenuItem<ParentBuilder>(this->fParent, fMenu, item);
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::ThisBuilder&
Menu<ParentBuilder>::AddSeparator()
{
	fMenu->AddSeparatorItem();
	return *this;
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::MenuBuilder
Menu<ParentBuilder>::AddMenu(KMenu* menu)
{
	if (!fMenu->AddItem(menu))
		throw std::bad_alloc();

	MenuBuilder builder(menu);
	builder.SetParent(this);
	return builder;
}


template<typename ParentBuilder>
typename Menu<ParentBuilder>::MenuBuilder
Menu<ParentBuilder>::AddMenu(const char* title, k_menu_layout layout)
{
	KMenu* menu = new KMenu(title, layout);
	if (!fMenu->AddItem(menu)) {
		delete menu;
		throw std::bad_alloc();
	}

	MenuBuilder builder(menu);
	builder.SetParent(this);
	return builder;
}


// #pragma mark - MenuItem


template<typename ParentBuilder>
MenuItem<ParentBuilder>::MenuItem(ParentBuilder* parentBuilder, KMenu* menu,
	KMenuItem* item)
	:
	Menu<ParentBuilder>(menu),
	fMenuItem(item)
{
	this->SetParent(parentBuilder);
}


template<typename ParentBuilder>
typename MenuItem<ParentBuilder>::ThisBuilder&
MenuItem<ParentBuilder>::GetItem(KMenuItem*& _item)
{
	_item = fMenuItem;
	return *this;
}


template<typename ParentBuilder>
typename MenuItem<ParentBuilder>::ThisBuilder&
MenuItem<ParentBuilder>::SetEnabled(bool enabled)
{
	fMenuItem->SetEnabled(enabled);
	return *this;
}


}	// namespace KLayoutBuilder


#endif	// _LAYOUT_BUILDER_H
