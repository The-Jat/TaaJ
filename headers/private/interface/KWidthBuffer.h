/*
 * Copyright 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef K___WIDTHBUFFER_H
#define K___WIDTHBUFFER_H


#include <Locker.h>
#include <KTextView.h>

#include "TextViewSupportBuffer.h"


class BFont;


namespace BPrivate {


class KTextGapBuffer;


struct _width_table_ {
	BFont font;				// corresponding font
	int32 hashCount;		// number of hashed items
	int32 tableCount;		// size of table
	void* widths;			// width table
};


class KWidthBuffer : public _BTextViewSupportBuffer_<_width_table_> {
public:
								KWidthBuffer();
	virtual						~KWidthBuffer();

			float				StringWidth(const char* inText,
									int32 fromOffset, int32 length,
									const BFont* inStyle);
			float				StringWidth(KTextGapBuffer& gapBuffer,
									int32 fromOffset, int32 length,
									const BFont* inStyle);

private:
			bool				FindTable(const BFont* font, int32* outIndex);
			int32				InsertTable(const BFont* font);

			bool				GetEscapement(uint32 value, int32 index,
									float* escapement);
			float				HashEscapements(const char* chars,
									int32 numChars, int32 numBytes,
									int32 tableIndex, const BFont* font);

	static	uint32				Hash(uint32);

private:
			BLocker				fLock;
};


extern KWidthBuffer* k_gWidthBuffer;


} // namespace BPrivate


using BPrivate::KWidthBuffer;


#if __GNUC__ < 3
//! NetPositive binary compatibility support

class _BWidthBuffer_ : public _BTextViewSupportBuffer_<BPrivate::_width_table_> {
	_BWidthBuffer_();
	virtual ~_BWidthBuffer_();
};

extern
_BWidthBuffer_* gCompatibilityWidthBuffer;

#endif // __GNUC__ < 3


#endif // __WIDTHBUFFER_H
