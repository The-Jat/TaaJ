/*
 * Copyright 2003-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

//!	UndoBuffer and its subclasses handle different types of Undo operations.


#include "KUndoBuffer.h"
#include "utf8_functions.h"

#include <Clipboard.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// TODO: properly document this file


//	#pragma mark - UndoBuffer


KTextView::UndoBuffer::UndoBuffer(KTextView* textView, k_undo_state state)
	:
	fTextView(textView),
	fTextData(NULL),
	fRunArray(NULL),
	fRunArrayLength(0),
	fRedo(false),
	fState(state)
{
	fTextView->GetSelection(&fStart, &fEnd);
	fTextLength = fEnd - fStart;
	
	fTextData = (char*)malloc(fTextLength);
	memcpy(fTextData, fTextView->Text() + fStart, fTextLength);

	if (fTextView->IsStylable())
		fRunArray = fTextView->RunArray(fStart, fEnd, &fRunArrayLength);
}


KTextView::UndoBuffer::~UndoBuffer()
{
	free(fTextData);
	KTextView::FreeRunArray(fRunArray);
}


void
KTextView::UndoBuffer::Undo(BClipboard* clipboard)
{
	fRedo ? RedoSelf(clipboard) : UndoSelf(clipboard);
		
	fRedo = !fRedo;
}


k_undo_state
KTextView::UndoBuffer::State(bool* _isRedo) const
{
	*_isRedo = fRedo;

	return fState;
}


void
KTextView::UndoBuffer::UndoSelf(BClipboard* clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Insert(fTextData, fTextLength, fRunArray);
	fTextView->Select(fStart, fStart);
}


void
KTextView::UndoBuffer::RedoSelf(BClipboard* clipboard)
{
}


//	#pragma mark - CutUndoBuffer


KTextView::CutUndoBuffer::CutUndoBuffer(KTextView* textView)
	: KTextView::UndoBuffer(textView, K_UNDO_CUT)
{
}


KTextView::CutUndoBuffer::~CutUndoBuffer()
{
}


void 
KTextView::CutUndoBuffer::RedoSelf(BClipboard* clipboard)
{
	BMessage* clip = NULL;
	
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fEnd);
	if (clipboard->Lock()) {
		clipboard->Clear();
		if ((clip = clipboard->Data())) {
			clip->AddData("text/plain", B_MIME_TYPE, fTextData, fTextLength);
			if (fRunArray)
				clip->AddData("application/x-vnd.Be-text_run_array",
					B_MIME_TYPE, fRunArray, fRunArrayLength);
			clipboard->Commit();
		}
		clipboard->Unlock();
	}
}


//	#pragma mark - PasteUndoBuffer


KTextView::PasteUndoBuffer::PasteUndoBuffer(KTextView* textView,
		const char* text, int32 textLen, k_text_run_array* runArray,
		int32 runArrayLen)
	: KTextView::UndoBuffer(textView, K_UNDO_PASTE),
	fPasteText(NULL),
	fPasteTextLength(textLen),
	fPasteRunArray(NULL)
{
	fPasteText = (char*)malloc(fPasteTextLength);
	memcpy(fPasteText, text, fPasteTextLength);

	if (runArray)
		fPasteRunArray = KTextView::CopyRunArray(runArray);
}


KTextView::PasteUndoBuffer::~PasteUndoBuffer()
{
	free(fPasteText);
	KTextView::FreeRunArray(fPasteRunArray);
}


void
KTextView::PasteUndoBuffer::UndoSelf(BClipboard* clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fStart + fPasteTextLength);
	fTextView->Insert(fTextData, fTextLength, fRunArray);
	fTextView->Select(fStart, fEnd);
}


void
KTextView::PasteUndoBuffer::RedoSelf(BClipboard* clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fEnd);
	fTextView->Insert(fPasteText, fPasteTextLength, fPasteRunArray);
	fTextView->Select(fStart + fPasteTextLength, fStart + fPasteTextLength);
}


//	#pragma mark - ClearUndoBuffer


KTextView::ClearUndoBuffer::ClearUndoBuffer(KTextView* textView)
	: KTextView::UndoBuffer(textView, K_UNDO_CLEAR)
{
}


KTextView::ClearUndoBuffer::~ClearUndoBuffer()
{
}


void
KTextView::ClearUndoBuffer::RedoSelf(BClipboard* clipboard)
{
	fTextView->Select(fStart, fStart);
	fTextView->Delete(fStart, fEnd);
}


//	#pragma mark - DropUndoBuffer


KTextView::DropUndoBuffer::DropUndoBuffer(KTextView* textView,
		char const* text, int32 textLen, k_text_run_array* runArray,
		int32 runArrayLen, int32 location, bool internalDrop)
	: KTextView::UndoBuffer(textView, K_UNDO_DROP),
	fDropText(NULL),
	fDropTextLength(textLen),
	fDropRunArray(NULL)
{
	fInternalDrop = internalDrop;
	fDropLocation = location;

	fDropText = (char*)malloc(fDropTextLength);
	memcpy(fDropText, text, fDropTextLength);

	if (runArray)
		fDropRunArray = KTextView::CopyRunArray(runArray);

	if (fInternalDrop && fDropLocation >= fEnd)
		fDropLocation -= fDropTextLength;
}


KTextView::DropUndoBuffer::~DropUndoBuffer()
{
	free(fDropText);
	KTextView::FreeRunArray(fDropRunArray);
}


void
KTextView::DropUndoBuffer::UndoSelf(BClipboard* )
{
	fTextView->Select(fDropLocation, fDropLocation);
	fTextView->Delete(fDropLocation, fDropLocation + fDropTextLength);
	if (fInternalDrop) {
		fTextView->Select(fStart, fStart);
		fTextView->Insert(fTextData, fTextLength, fRunArray);
	}
	fTextView->Select(fStart, fEnd);
}


void
KTextView::DropUndoBuffer::RedoSelf(BClipboard* )
{
	if (fInternalDrop) {
		fTextView->Select(fStart, fStart);
		fTextView->Delete(fStart, fEnd);
	}
	fTextView->Select(fDropLocation, fDropLocation);
	fTextView->Insert(fDropText, fDropTextLength, fDropRunArray);
	fTextView->Select(fDropLocation, fDropLocation + fDropTextLength);
}


//	#pragma mark - TypingUndoBuffer


KTextView::TypingUndoBuffer::TypingUndoBuffer(KTextView* textView)
	: KTextView::UndoBuffer(textView, K_UNDO_TYPING),
	fTypedText(NULL),
	fTypedStart(fStart),
	fTypedEnd(fEnd),
	fUndone(0)
{
}


KTextView::TypingUndoBuffer::~TypingUndoBuffer()
{
	free(fTypedText);
}


void
KTextView::TypingUndoBuffer::UndoSelf(BClipboard* clipboard)
{
	int32 len = fTypedEnd - fTypedStart;
	
	free(fTypedText);
	fTypedText = (char*)malloc(len);
	memcpy(fTypedText, fTextView->Text() + fTypedStart, len);
	
	fTextView->Select(fTypedStart, fTypedStart);
	fTextView->Delete(fTypedStart, fTypedEnd);
	fTextView->Insert(fTextData, fTextLength);
	fTextView->Select(fStart, fEnd);
	fUndone++;
}


void
KTextView::TypingUndoBuffer::RedoSelf(BClipboard* clipboard)
{	
	fTextView->Select(fTypedStart, fTypedStart);
	fTextView->Delete(fTypedStart, fTypedStart + fTextLength);
	fTextView->Insert(fTypedText, fTypedEnd - fTypedStart);
	fUndone--;
}


void
KTextView::TypingUndoBuffer::InputCharacter(int32 len)
{
	int32 start, end;
	fTextView->GetSelection(&start, &end);
	
	if (start != fTypedEnd || end != fTypedEnd)
		_Reset();
		
	fTypedEnd += len;
}


void
KTextView::TypingUndoBuffer::_Reset()
{
	free(fTextData);
	fTextView->GetSelection(&fStart, &fEnd);
	fTextLength = fEnd - fStart;
	fTypedStart = fStart;
	fTypedEnd = fStart;
	
	fTextData = (char*)malloc(fTextLength);
	memcpy(fTextData, fTextView->Text() + fStart, fTextLength);
	
	free(fTypedText);
	fTypedText = NULL;
	fRedo = false;
	fUndone = 0;
}


void
KTextView::TypingUndoBuffer::BackwardErase()
{
	int32 start, end;
	fTextView->GetSelection(&start, &end);
	
	const char* text = fTextView->Text();
	int32 charLen = UTF8PreviousCharLen(text + start, text);
	
	if (start != fTypedEnd || end != fTypedEnd) {
		_Reset();
		// if we've got a selection, we're already done
		if (start != end)
			return;
	} 
	
	char* buffer = (char*)malloc(fTextLength + charLen);
	memcpy(buffer + charLen, fTextData, fTextLength);
	
	fTypedStart = start - charLen;
	start = fTypedStart;
	for (int32 x = 0; x < charLen; x++)
		buffer[x] = fTextView->ByteAt(start + x);
	free(fTextData);
	fTextData = buffer;
	
	fTextLength += charLen;
	fTypedEnd -= charLen;
}


void
KTextView::TypingUndoBuffer::ForwardErase()
{
	// TODO: Cleanup
	int32 start, end;

	fTextView->GetSelection(&start, &end);
	
	int32 charLen = UTF8NextCharLen(fTextView->Text() + start);	
	
	if (start != fTypedEnd || end != fTypedEnd || fUndone > 0) {
		_Reset();
		// if we've got a selection, we're already done
		if (fStart == fEnd) {
			free(fTextData);
			fTextLength = charLen;
			fTextData = (char*)malloc(fTextLength);
			
			// store the erased character
			for (int32 x = 0; x < charLen; x++)
				fTextData[x] = fTextView->ByteAt(start + x);
		}
	} else {	
		// Here we need to store the erased text, so we get the text that it's 
		// already in the buffer, and we add the erased character.
		// a realloc + memmove would maybe be cleaner, but that way we spare a
		// copy (malloc + memcpy vs realloc + memmove).
		
		int32 newLength = fTextLength + charLen;
		char* buffer = (char*)malloc(newLength);
		
		// copy the already stored data
		memcpy(buffer, fTextData, fTextLength);
		
		if (fTextLength < newLength) {
			// store the erased character
			for (int32 x = 0; x < charLen; x++)
				buffer[fTextLength + x] = fTextView->ByteAt(start + x);
		}

		fTextLength = newLength;
		free(fTextData);
		fTextData = buffer;
	}
}
