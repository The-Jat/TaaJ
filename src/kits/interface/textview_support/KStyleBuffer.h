/*
 * Copyright 2001-2006 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers, mflerackers@androme.be
 *		Stefano Ceccherini, burton666@libero.it
 */


#include <Font.h>
#include <InterfaceDefs.h>
#include <SupportDefs.h>
#include <KTextView.h>

#include "TextViewSupportBuffer.h"


struct KSTEStyle {
	BFont		font;		// font
	rgb_color	color;		// pen color
};


struct KSTEStyleRun {
	long		offset;		// byte offset of first character of run
	KSTEStyle	style;		// style info
};


struct KSTEStyleRange {
	long		count;		// number of style runs
	KSTEStyleRun	runs[1];	// array of count number of runs
};


struct KSTEStyleRecord {
	long		refs;		// reference count for this style
	float		ascent;		// ascent for this style
	float		descent;	// descent for this style
	KSTEStyle	style;		// style info
};


struct STEStyleRunDesc {
	long		offset;		// byte offset of first character of run
	long		index;		// index of corresponding style record
};


// _KStyleRunDescBuffer_ class -------------------------------------------------
class _KStyleRunDescBuffer_ : public _BTextViewSupportBuffer_<STEStyleRunDesc> {
public:
								_KStyleRunDescBuffer_();

			void				InsertDesc(STEStyleRunDesc* inDesc,
								int32 index);
			void				RemoveDescs(int32 index, int32 count = 1);

			int32				OffsetToRun(int32 offset) const;
			void				BumpOffset(int32 delta, int32 index);

			STEStyleRunDesc*	operator[](int32 index) const;
};


inline STEStyleRunDesc*
_KStyleRunDescBuffer_::operator[](int32 index) const
{
	return &fBuffer[index];
}


// _KStyleRecordBuffer_ class --------------------------------------------------
class _KStyleRecordBuffer_ : public _BTextViewSupportBuffer_<KSTEStyleRecord> {
public:
								_KStyleRecordBuffer_();

			int32				InsertRecord(const BFont* inFont,
									const rgb_color* inColor);
			void				CommitRecord(int32 index);
			void				RemoveRecord(int32 index);

			bool				MatchRecord(const BFont* inFont,
									const rgb_color* inColor,
									int32* outIndex);

			KSTEStyleRecord*		operator[](int32 index) const;
};


inline KSTEStyleRecord*
_KStyleRecordBuffer_::operator[](int32 index) const
{
	return &fBuffer[index];
}


// StyleBuffer class --------------------------------------------------------
class KTextView::StyleBuffer {
public:
								StyleBuffer(const BFont* inFont,
									const rgb_color* inColor);

			void				InvalidateNullStyle();
			bool				IsValidNullStyle() const;

			void				SyncNullStyle(int32 offset);
			void				SetNullStyle(uint32 inMode,
									const BFont* inFont,
									const rgb_color* inColor,
									int32 offset = 0);
			void				GetNullStyle(const BFont** font,
									const rgb_color** color) const;

			void				GetStyle(int32 inOffset, BFont* outFont,
									rgb_color* outColor) const;
			void				ContinuousGetStyle(BFont*, uint32*,
									rgb_color*, bool*, int32, int32) const;

			KSTEStyleRange*		AllocateStyleRange(
									const int32 numStyles) const;
			void				SetStyleRange(int32 fromOffset,
									int32 toOffset, int32 textLen,
									uint32 inMode, const BFont* inFont,
									const rgb_color* inColor);
			KSTEStyleRange*		GetStyleRange(int32 startOffset,
									int32 endOffset) const;

			void				RemoveStyleRange(int32 fromOffset,
									int32 toOffset);
			void				RemoveStyles(int32 index, int32 count = 1);

			int32				Iterate(int32 fromOffset, int32 length,
									InlineInput* input,
									const BFont** outFont = NULL,
									const rgb_color** outColor = NULL,
									float* outAscent = NULL,
									float* outDescen = NULL,
									uint32* = NULL) const;

			int32				OffsetToRun(int32 offset) const;
			void				BumpOffset(int32 delta, int32 index);

			KSTEStyleRun			operator[](int32 index) const;
			int32				NumRuns() const;

	const	_KStyleRunDescBuffer_&	RunBuffer() const;
	const	_KStyleRecordBuffer_&	RecordBuffer() const;

private:
			_KStyleRunDescBuffer_	fStyleRunDesc;
			_KStyleRecordBuffer_	fStyleRecord;
			bool				fValidNullStyle;
			KSTEStyle			fNullStyle;
};


inline int32
KTextView::StyleBuffer::NumRuns() const
{
	return fStyleRunDesc.ItemCount();
}


inline const _KStyleRunDescBuffer_&
KTextView::StyleBuffer::RunBuffer() const
{
	return fStyleRunDesc;
}


inline const _KStyleRecordBuffer_&
KTextView::StyleBuffer::RecordBuffer() const
{
	return fStyleRecord;
}
