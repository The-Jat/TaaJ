/*
 * Copyright 2003-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

#ifndef K___UNDOBUFFER_H
#define K___UNDOBUFFER_H

#include <KTextView.h>


class BClipboard;


// UndoBuffer
class KTextView::UndoBuffer {
public:
								UndoBuffer(KTextView* view, k_undo_state state);
	virtual						~UndoBuffer();

			void				Undo(BClipboard* clipboard);
			k_undo_state			State(bool* _isRedo) const;

protected:
	virtual	void				UndoSelf(BClipboard* clipboard);
	virtual	void				RedoSelf(BClipboard* clipboard);
	
			KTextView*			fTextView;
			int32				fStart;
			int32				fEnd;

			char*				fTextData;
			int32				fTextLength;
			k_text_run_array*		fRunArray;
			int32				fRunArrayLength;

			bool				fRedo;

private:
			k_undo_state			fState;
};


// CutUndoBuffer
class KTextView::CutUndoBuffer : public KTextView::UndoBuffer {
public:
								CutUndoBuffer(KTextView* textView);
	virtual						~CutUndoBuffer();

protected:
	virtual	void				RedoSelf(BClipboard* clipboard);
};


// PasteUndoBuffer
class KTextView::PasteUndoBuffer : public KTextView::UndoBuffer {
public:
								PasteUndoBuffer(KTextView* textView,
									const char* text, int32 textLength,
									k_text_run_array* runArray,
									int32 runArrayLen);
	virtual						~PasteUndoBuffer();

protected:
	virtual	void				UndoSelf(BClipboard* clipboard);
	virtual	void				RedoSelf(BClipboard* clipboard);

private:
			char*				fPasteText;
			int32				fPasteTextLength;
			k_text_run_array*		fPasteRunArray;
};


// ClearUndoBuffer
class KTextView::ClearUndoBuffer : public KTextView::UndoBuffer {
public:
								ClearUndoBuffer(KTextView* textView);
	virtual						~ClearUndoBuffer();

protected:
	virtual	void				RedoSelf(BClipboard* clipboard);
};


// DropUndoBuffer
class KTextView::DropUndoBuffer : public KTextView::UndoBuffer {
public:
								DropUndoBuffer(KTextView* textView,
									char const* text, int32 textLength,
									k_text_run_array* runArray,
									int32 runArrayLength, int32 location,
									bool internalDrop);
	virtual						~DropUndoBuffer();

protected:
	virtual	void				UndoSelf(BClipboard* clipboard);
	virtual	void				RedoSelf(BClipboard* clipboard);

private:
			char*				fDropText;
			int32				fDropTextLength;
			k_text_run_array*		fDropRunArray;
	
			int32				fDropLocation;
			bool				fInternalDrop;
};


// TypingUndoBuffer
class KTextView::TypingUndoBuffer : public KTextView::UndoBuffer {
public:
								TypingUndoBuffer(KTextView* textView);
	virtual						~TypingUndoBuffer();

			void				InputCharacter(int32 length);
			void				BackwardErase();
			void				ForwardErase();

protected:
	virtual	void				RedoSelf(BClipboard* clipboard);
	virtual	void				UndoSelf(BClipboard* clipboard);

private:
			void				_Reset();
	
			char*				fTypedText;
			int32				fTypedStart;
			int32				fTypedEnd;
			int32				fUndone;
};

#endif //__UNDOBUFFER_H
