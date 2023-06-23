/*
 * Copyright 2001-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */


#include "KLineBuffer.h"


KTextView::LineBuffer::LineBuffer()
	:	_BTextViewSupportBuffer_<KSTELine>(20, 2)
{
}


KTextView::LineBuffer::~LineBuffer()
{
}


void
KTextView::LineBuffer::InsertLine(KSTELine* inLine, int32 index)
{
	InsertItemsAt(1, index, inLine);
}


void
KTextView::LineBuffer::RemoveLines(int32 index, int32 count)
{
	RemoveItemsAt(count, index);
}


void
KTextView::LineBuffer::RemoveLineRange(int32 fromOffset, int32 toOffset)
{
	int32 fromLine = OffsetToLine(fromOffset);
	int32 toLine = OffsetToLine(toOffset);

	int32 count = toLine - fromLine;
	if (count > 0)
		RemoveLines(fromLine + 1, count);

	BumpOffset(fromOffset - toOffset, fromLine + 1);
}


int32
KTextView::LineBuffer::OffsetToLine(int32 offset) const
{
	int32 minIndex = 0;
	int32 maxIndex = fItemCount - 1;
	int32 index = 0;

	while (minIndex < maxIndex) {
		index = (minIndex + maxIndex) >> 1;
		if (offset >= fBuffer[index].offset) {
			if (offset < fBuffer[index + 1].offset)
				break;
			else
				minIndex = index + 1;
		} else
			maxIndex = index;
	}

	return index;
}


int32
KTextView::LineBuffer::PixelToLine(float pixel) const
{
	int32 minIndex = 0;
	int32 maxIndex = fItemCount - 1;
	int32 index = 0;

	while (minIndex < maxIndex) {
		index = (minIndex + maxIndex) >> 1;
		if (pixel >= fBuffer[index].origin) {
			if (pixel < fBuffer[index + 1].origin)
				break;
			else
				minIndex = index + 1;
		} else
			maxIndex = index;
	}

	return index;
}


void
KTextView::LineBuffer::BumpOrigin(float delta, int32 index)
{
	for (long i = index; i < fItemCount; i++)
		fBuffer[i].origin += delta;
}


void
KTextView::LineBuffer::BumpOffset(int32 delta, int32 index)
{
	for (long i = index; i < fItemCount; i++)
		fBuffer[i].offset += delta;
}


float
KTextView::LineBuffer::MaxWidth() const
{
	if (fItemCount == 0)
		return 0;

	float maxWidth = 0;
	KSTELine* line = &fBuffer[0];
	for (int32 i = 0; i < fItemCount; i++) {
		if (maxWidth < line->width)
			maxWidth = line->width;
		line++;
	}
	return maxWidth;
}
