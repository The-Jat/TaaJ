/*
 * Copyright 2010-2011 Haiku, Inc. All rights reserved.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 *
 * Distributed under the terms of the MIT License.
 */


#include <KGridLayout.h>

#include <algorithm>
#include <new>
#include <string.h>

#include <KControlLook.h>
#include <KLayoutItem.h>
#include <List.h>
#include <Message.h>

#include "KViewLayoutItem.h"


using std::nothrow;
using std::swap;


enum {
	MAX_COLUMN_ROW_COUNT	= 1024,
};


namespace {
	// a placeholder we put in our grid array to make a cell occupied
	KLayoutItem* const OCCUPIED_GRID_CELL = (KLayoutItem*)0x1;

	const char* const kRowSizesField = "KGridLayout:rowsizes";
		// kRowSizesField = {min, max}
	const char* const kRowWeightField = "KGridLayout:rowweight";
	const char* const kColumnSizesField = "KGridLayout:columnsizes";
		// kColumnSizesField = {min, max}
	const char* const kColumnWeightField = "KGridLayout:columnweight";
	const char* const kItemDimensionsField = "KGridLayout:item:dimensions";
		// kItemDimensionsField = {x, y, width, height}
}


struct KGridLayout::ItemLayoutData {
	Dimensions	dimensions;

	ItemLayoutData()
	{
		dimensions.x = 0;
		dimensions.y = 0;
		dimensions.width = 1;
		dimensions.height = 1;
	}
};


class KGridLayout::RowInfoArray {
public:
	RowInfoArray()
	{
	}

	~RowInfoArray()
	{
		for (int32 i = 0; Info* info = (Info*)fInfos.ItemAt(i); i++)
			delete info;
	}

	int32 Count() const
	{
		return fInfos.CountItems();
	}

	float Weight(int32 index) const
	{
		if (Info* info = _InfoAt(index))
			return info->weight;
		return 1;
	}

	void SetWeight(int32 index, float weight)
	{
		if (Info* info = _InfoAt(index, true))
			info->weight = weight;
	}

	float MinSize(int32 index) const
	{
		if (Info* info = _InfoAt(index))
			return info->minSize;
		return B_SIZE_UNSET;
	}

	void SetMinSize(int32 index, float size)
	{
		if (Info* info = _InfoAt(index, true))
			info->minSize = size;
	}

	float MaxSize(int32 index) const
	{
		if (Info* info = _InfoAt(index))
			return info->maxSize;
		return B_SIZE_UNSET;
	}

	void SetMaxSize(int32 index, float size)
	{
		if (Info* info = _InfoAt(index, true))
			info->maxSize = size;
	}

private:
	struct Info {
		float	weight;
		float	minSize;
		float	maxSize;
	};

	Info* _InfoAt(int32 index) const
	{
		return (Info*)fInfos.ItemAt(index);
	}

	Info* _InfoAt(int32 index, bool resize)
	{
		if (index < 0 || index >= MAX_COLUMN_ROW_COUNT)
			return NULL;

		// resize, if necessary and desired
		int32 count = Count();
		if (index >= count) {
			if (!resize)
				return NULL;

			for (int32 i = count; i <= index; i++) {
				Info* info = new Info;
				info->weight = 1;
				info->minSize = B_SIZE_UNSET;
				info->maxSize = B_SIZE_UNSET;
				fInfos.AddItem(info);
			}
		}

		return _InfoAt(index);
	}

	BList		fInfos;
};


KGridLayout::KGridLayout(float horizontal, float vertical)
	:
	fGrid(NULL),
	fColumnCount(0),
	fRowCount(0),
	fRowInfos(new RowInfoArray),
	fColumnInfos(new RowInfoArray),
	fMultiColumnItems(0),
	fMultiRowItems(0)
{
	SetSpacing(horizontal, vertical);
}


KGridLayout::KGridLayout(BMessage* from)
	:
	KTwoDimensionalLayout(BUnarchiver::PrepareArchive(from)),
	fGrid(NULL),
	fColumnCount(0),
	fRowCount(0),
	fRowInfos(new RowInfoArray),
	fColumnInfos(new RowInfoArray),
	fMultiColumnItems(0),
	fMultiRowItems(0)
{
	BUnarchiver unarchiver(from);
	int32 columns;
	from->GetInfo(kColumnWeightField, NULL, &columns);

	int32 rows;
	from->GetInfo(kRowWeightField, NULL, &rows);

	// sets fColumnCount && fRowCount on success
	if (!_ResizeGrid(columns, rows)) {
		unarchiver.Finish(B_NO_MEMORY);
		return;
	}

	for (int32 i = 0; i < fRowCount; i++) {
		float getter;
		if (from->FindFloat(kRowWeightField, i, &getter) == B_OK)
			fRowInfos->SetWeight(i, getter);

		if (from->FindFloat(kRowSizesField, i * 2, &getter) == B_OK)
			fRowInfos->SetMinSize(i, getter);

		if (from->FindFloat(kRowSizesField, i * 2 + 1, &getter) == B_OK)
			fRowInfos->SetMaxSize(i, getter);
	}

	for (int32 i = 0; i < fColumnCount; i++) {
		float getter;
		if (from->FindFloat(kColumnWeightField, i, &getter) == B_OK)
			fColumnInfos->SetWeight(i, getter);

		if (from->FindFloat(kColumnSizesField, i * 2, &getter) == B_OK)
			fColumnInfos->SetMinSize(i, getter);

		if (from->FindFloat(kColumnSizesField, i * 2 + 1, &getter) == B_OK)
			fColumnInfos->SetMaxSize(i, getter);
	}
}


KGridLayout::~KGridLayout()
{
	delete fRowInfos;
	delete fColumnInfos;

	for (int32 i = 0; i < fColumnCount; i++)
		delete[] fGrid[i];
	delete[] fGrid;
}


int32
KGridLayout::CountColumns() const
{
	return fColumnCount;
}


int32
KGridLayout::CountRows() const
{
	return fRowCount;
}


float
KGridLayout::HorizontalSpacing() const
{
	return fHSpacing;
}


float
KGridLayout::VerticalSpacing() const
{
	return fVSpacing;
}


void
KGridLayout::SetHorizontalSpacing(float spacing)
{
	spacing = KControlLook::ComposeSpacing(spacing);
	if (spacing != fHSpacing) {
		fHSpacing = spacing;

		InvalidateLayout();
	}
}


void
KGridLayout::SetVerticalSpacing(float spacing)
{
	spacing = KControlLook::ComposeSpacing(spacing);
	if (spacing != fVSpacing) {
		fVSpacing = spacing;

		InvalidateLayout();
	}
}


void
KGridLayout::SetSpacing(float horizontal, float vertical)
{
	horizontal = KControlLook::ComposeSpacing(horizontal);
	vertical = KControlLook::ComposeSpacing(vertical);
	if (horizontal != fHSpacing || vertical != fVSpacing) {
		fHSpacing = horizontal;
		fVSpacing = vertical;

		InvalidateLayout();
	}
}


float
KGridLayout::ColumnWeight(int32 column) const
{
	return fColumnInfos->Weight(column);
}


void
KGridLayout::SetColumnWeight(int32 column, float weight)
{
	fColumnInfos->SetWeight(column, weight);
}


float
KGridLayout::MinColumnWidth(int32 column) const
{
	return fColumnInfos->MinSize(column);
}


void
KGridLayout::SetMinColumnWidth(int32 column, float width)
{
	fColumnInfos->SetMinSize(column, width);
}


float
KGridLayout::MaxColumnWidth(int32 column) const
{
	return fColumnInfos->MaxSize(column);
}


void
KGridLayout::SetMaxColumnWidth(int32 column, float width)
{
	fColumnInfos->SetMaxSize(column, width);
}


float
KGridLayout::RowWeight(int32 row) const
{
	return fRowInfos->Weight(row);
}


void
KGridLayout::SetRowWeight(int32 row, float weight)
{
	fRowInfos->SetWeight(row, weight);
}


float
KGridLayout::MinRowHeight(int row) const
{
	return fRowInfos->MinSize(row);
}


void
KGridLayout::SetMinRowHeight(int32 row, float height)
{
	fRowInfos->SetMinSize(row, height);
}


float
KGridLayout::MaxRowHeight(int32 row) const
{
	return fRowInfos->MaxSize(row);
}


void
KGridLayout::SetMaxRowHeight(int32 row, float height)
{
	fRowInfos->SetMaxSize(row, height);
}


KLayoutItem*
KGridLayout::ItemAt(int32 column, int32 row) const
{
	if (column < 0 || column >= CountColumns()
		|| row < 0 || row >= CountRows())
		return NULL;

	return fGrid[column][row];
}


KLayoutItem*
KGridLayout::AddView(KView* child)
{
	return KTwoDimensionalLayout::AddView(child);
}


KLayoutItem*
KGridLayout::AddView(int32 index, KView* child)
{
	return KTwoDimensionalLayout::AddView(index, child);
}


KLayoutItem*
KGridLayout::AddView(KView* child, int32 column, int32 row, int32 columnCount,
	int32 rowCount)
{
	if (!child)
		return NULL;

	KLayoutItem* item = new KViewLayoutItem(child);
	if (!AddItem(item, column, row, columnCount, rowCount)) {
		delete item;
		return NULL;
	}

	return item;
}


bool
KGridLayout::AddItem(KLayoutItem* item)
{
	// find a free spot
	for (int32 row = 0; row < fRowCount; row++) {
		for (int32 column = 0; column < fColumnCount; column++) {
			if (_IsGridCellEmpty(column, row))
				return AddItem(item, column, row, 1, 1);
		}
	}

	// no free spot, start a new column
	return AddItem(item, fColumnCount, 0, 1, 1);
}


bool
KGridLayout::AddItem(int32 index, KLayoutItem* item)
{
	return AddItem(item);
}


bool
KGridLayout::AddItem(KLayoutItem* item, int32 column, int32 row,
	int32 columnCount, int32 rowCount)
{
	if (!_AreGridCellsEmpty(column, row, columnCount, rowCount))
		return false;

	bool success = KTwoDimensionalLayout::AddItem(-1, item);
	if (!success)
		return false;

	// set item dimensions
	if (ItemLayoutData* data = _LayoutDataForItem(item)) {
		data->dimensions.x = column;
		data->dimensions.y = row;
		data->dimensions.width = columnCount;
		data->dimensions.height = rowCount;
	}

	if (!_InsertItemIntoGrid(item)) {
		RemoveItem(item);
		return false;
	}

	if (columnCount > 1)
		fMultiColumnItems++;
	if (rowCount > 1)
		fMultiRowItems++;

	return success;
}


status_t
KGridLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t result = KTwoDimensionalLayout::Archive(into, deep);

	for (int32 i = 0; i < fRowCount && result == B_OK; i++) {
		result = into->AddFloat(kRowWeightField, fRowInfos->Weight(i));
		if (result == B_OK)
			result = into->AddFloat(kRowSizesField, fRowInfos->MinSize(i));
		if (result == B_OK)
			result = into->AddFloat(kRowSizesField, fRowInfos->MaxSize(i));
	}

	for (int32 i = 0; i < fColumnCount && result == B_OK; i++) {
		result = into->AddFloat(kColumnWeightField, fColumnInfos->Weight(i));
		if (result == B_OK)
			result = into->AddFloat(kColumnSizesField, fColumnInfos->MinSize(i));
		if (result == B_OK)
			result = into->AddFloat(kColumnSizesField, fColumnInfos->MaxSize(i));
	}

	return archiver.Finish(result);
}


status_t
KGridLayout::AllArchived(BMessage* into) const
{
	return KTwoDimensionalLayout::AllArchived(into);
}


status_t
KGridLayout::AllUnarchived(const BMessage* from)
{
	return KTwoDimensionalLayout::AllUnarchived(from);
}


BArchivable*
KGridLayout::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KGridLayout"))
		return new KGridLayout(from);
	return NULL;
}


status_t
KGridLayout::ItemArchived(BMessage* into, KLayoutItem* item, int32 index) const
{
	ItemLayoutData* data =	_LayoutDataForItem(item);

	status_t result = into->AddInt32(kItemDimensionsField, data->dimensions.x);
	if (result == B_OK)
		result = into->AddInt32(kItemDimensionsField, data->dimensions.y);

	if (result == B_OK)
		result = into->AddInt32(kItemDimensionsField, data->dimensions.width);

	if (result == B_OK)
		result = into->AddInt32(kItemDimensionsField, data->dimensions.height);

	return result;
}


status_t
KGridLayout::ItemUnarchived(const BMessage* from,
	KLayoutItem* item, int32 index)
{
	ItemLayoutData* data = _LayoutDataForItem(item);
	Dimensions& dimensions = data->dimensions;

	index *= 4;
		// each item stores 4 int32s into kItemDimensionsField
	status_t result = from->FindInt32(kItemDimensionsField, index, &dimensions.x);
	if (result == B_OK)
		result = from->FindInt32(kItemDimensionsField, ++index, &dimensions.y);

	if (result == B_OK)
		result = from->FindInt32(kItemDimensionsField, ++index, &dimensions.width);

	if (result == B_OK) {
		result = from->FindInt32(kItemDimensionsField,
			++index, &dimensions.height);
	}

	if (result != B_OK)
		return result;

	if (!_AreGridCellsEmpty(dimensions.x, dimensions.y,
		dimensions.width, dimensions.height))
		return B_BAD_DATA;

	if (!_InsertItemIntoGrid(item))
		return B_NO_MEMORY;

	if (dimensions.width > 1)
		fMultiColumnItems++;

	if (dimensions.height > 1)
		fMultiRowItems++;

	return result;
}


bool
KGridLayout::ItemAdded(KLayoutItem* item, int32 atIndex)
{
	item->SetLayoutData(new(nothrow) ItemLayoutData);
	return item->LayoutData() != NULL;
}


void
KGridLayout::ItemRemoved(KLayoutItem* item, int32 fromIndex)
{
	ItemLayoutData* data = _LayoutDataForItem(item);
	Dimensions itemDimensions = data->dimensions;
	item->SetLayoutData(NULL);
	delete data;

	if (itemDimensions.width > 1)
		fMultiColumnItems--;

	if (itemDimensions.height > 1)
		fMultiRowItems--;

	// remove the item from the grid
	for (int x = 0; x < itemDimensions.width; x++) {
		for (int y = 0; y < itemDimensions.height; y++)
			fGrid[itemDimensions.x + x][itemDimensions.y + y] = NULL;
	}

	// check whether we can shrink the grid
	if (itemDimensions.x + itemDimensions.width == fColumnCount
		|| itemDimensions.y + itemDimensions.height == fRowCount) {
		int32 columnCount = fColumnCount;
		int32 rowCount = fRowCount;

		// check for empty columns
		bool empty = true;
		for (; columnCount > 0; columnCount--) {
			for (int32 row = 0; empty && row < rowCount; row++)
				empty &= (fGrid[columnCount - 1][row] == NULL);

			if (!empty)
				break;
		}

		// check for empty rows
		empty = true;
		for (; rowCount > 0; rowCount--) {
			for (int32 column = 0; empty && column < columnCount; column++)
				empty &= (fGrid[column][rowCount - 1] == NULL);

			if (!empty)
				break;
		}

		// resize the grid
		if (columnCount != fColumnCount || rowCount != fRowCount)
			_ResizeGrid(columnCount, rowCount);
	}
}


bool
KGridLayout::HasMultiColumnItems()
{
	return fMultiColumnItems > 0;
}


bool
KGridLayout::HasMultiRowItems()
{
	return fMultiRowItems > 0;
}


int32
KGridLayout::InternalCountColumns()
{
	return fColumnCount;
}


int32
KGridLayout::InternalCountRows()
{
	return fRowCount;
}


void
KGridLayout::GetColumnRowConstraints(orientation orientation, int32 index,
	ColumnRowConstraints* constraints)
{
	if (orientation == B_HORIZONTAL) {
		constraints->min = MinColumnWidth(index);
		constraints->max = MaxColumnWidth(index);
		constraints->weight = ColumnWeight(index);
	} else {
		constraints->min = MinRowHeight(index);
		constraints->max = MaxRowHeight(index);
		constraints->weight = RowWeight(index);
	}
}


void
KGridLayout::GetItemDimensions(KLayoutItem* item, Dimensions* dimensions)
{
	if (ItemLayoutData* data = _LayoutDataForItem(item))
		*dimensions = data->dimensions;
}


bool
KGridLayout::_IsGridCellEmpty(int32 column, int32 row)
{
	if (column < 0 || row < 0)
		return false;

	if (column >= fColumnCount || row >= fRowCount)
		return true;

	return (fGrid[column][row] == NULL);
}


bool
KGridLayout::_AreGridCellsEmpty(int32 column, int32 row, int32 columnCount,
	int32 rowCount)
{
	if (column < 0 || row < 0)
		return false;
	int32 toColumn = min_c(column + columnCount, fColumnCount);
	int32 toRow = min_c(row + rowCount, fRowCount);

	for (int32 x = column; x < toColumn; x++) {
		for (int32 y = row; y < toRow; y++) {
			if (fGrid[x][y] != NULL)
				return false;
		}
	}

	return true;
}


bool
KGridLayout::_InsertItemIntoGrid(KLayoutItem* item)
{
	KGridLayout::ItemLayoutData* data = _LayoutDataForItem(item);
	int32 column = data->dimensions.x;
	int32 columnCount = data->dimensions.width;
	int32 row = data->dimensions.y;
	int32 rowCount = data->dimensions.height;

	// resize the grid, if necessary
	int32 newColumnCount = max_c(fColumnCount, column + columnCount);
	int32 newRowCount = max_c(fRowCount, row + rowCount);
	if (newColumnCount > fColumnCount || newRowCount > fRowCount) {
		if (!_ResizeGrid(newColumnCount, newRowCount))
			return false;
	}

	// enter the item in the grid
	for (int32 x = 0; x < columnCount; x++) {
		for (int32 y = 0; y < rowCount; y++) {
			if (x == 0 && y == 0)
				fGrid[column + x][row + y] = item;
			else
				fGrid[column + x][row + y] = OCCUPIED_GRID_CELL;
		}
	}

	return true;
}


bool
KGridLayout::_ResizeGrid(int32 columnCount, int32 rowCount)
{
	if (columnCount == fColumnCount && rowCount == fRowCount)
		return true;

	int32 rowsToKeep = min_c(rowCount, fRowCount);

	// allocate new grid
	KLayoutItem*** grid = new(nothrow) KLayoutItem**[columnCount];
	if (grid == NULL)
		return false;

	memset(grid, 0, sizeof(KLayoutItem**) * columnCount);

	bool success = true;
	for (int32 i = 0; i < columnCount; i++) {
		KLayoutItem** column = new(nothrow) KLayoutItem*[rowCount];
		if (!column) {
			success = false;
			break;
		}
		grid[i] = column;

		memset(column, 0, sizeof(KLayoutItem*) * rowCount);
		if (i < fColumnCount && rowsToKeep > 0)
			memcpy(column, fGrid[i], sizeof(KLayoutItem*) * rowsToKeep);
	}

	// if everything went fine, set the new grid
	if (success) {
		swap(grid, fGrid);
		swap(columnCount, fColumnCount);
		swap(rowCount, fRowCount);
	}

	// delete the old, respectively on error the partially created grid
	for (int32 i = 0; i < columnCount; i++)
		delete[] grid[i];

	delete[] grid;

	return success;
}


KGridLayout::ItemLayoutData*
KGridLayout::_LayoutDataForItem(KLayoutItem* item) const
{
	if (!item)
		return NULL;
	return (ItemLayoutData*)item->LayoutData();
}


status_t
KGridLayout::Perform(perform_code d, void* arg)
{
	return KTwoDimensionalLayout::Perform(d, arg);
}


void KGridLayout::_ReservedGridLayout1() {}
void KGridLayout::_ReservedGridLayout2() {}
void KGridLayout::_ReservedGridLayout3() {}
void KGridLayout::_ReservedGridLayout4() {}
void KGridLayout::_ReservedGridLayout5() {}
void KGridLayout::_ReservedGridLayout6() {}
void KGridLayout::_ReservedGridLayout7() {}
void KGridLayout::_ReservedGridLayout8() {}
void KGridLayout::_ReservedGridLayout9() {}
void KGridLayout::_ReservedGridLayout10() {}
