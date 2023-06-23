/*
 * Copyright 2001-2020 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Frans van Nispen (xlr8@tref.nl)
 */

//! The KTextView derivative owned by an instance of KTextControl.

#ifndef	K__TEXT_CONTROLI_H
#define	K__TEXT_CONTROLI_H


#include <KTextView.h>


class KTextControl;

namespace BPrivate {

class _KTextInput_ : public KTextView {
public:
						_KTextInput_(BRect frame, BRect textRect,
							uint32 resizeMask,
							uint32 flags = B_WILL_DRAW | B_PULSE_NEEDED);
						_KTextInput_(BMessage *data);
virtual					~_KTextInput_();

static	BArchivable*	Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;

virtual	void			MouseDown(BPoint where);
virtual	void			FrameResized(float width, float height);
virtual	void			KeyDown(const char *bytes, int32 numBytes);
virtual	void			MakeFocus(bool focusState = true);

virtual	BSize			MinSize();

		void			SetInitialText();

virtual	void			Paste(BClipboard *clipboard);

protected:

virtual	void			InsertText(const char *inText, int32 inLength,
								   int32 inOffset, const k_text_run_array *inRuns);
virtual	void			DeleteText(int32 fromOffset, int32 toOffset);

private:

		KTextControl	*TextControl();

		char			*fPreviousText;
		bool			fInMouseDown;
};

}	// namespace BPrivate

using namespace BPrivate;


#endif	// _TEXT_CONTROLI_H

