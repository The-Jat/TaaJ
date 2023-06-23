/*
 * Copyright 2009-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_TOOL_TIP_WINDOW_H
#define _K_TOOL_TIP_WINDOW_H


#include <Khidki.h>


namespace BPrivate {


class KToolTipWindow : public KWindow {
public:
							KToolTipWindow(KToolTip* tip, BPoint where,
								void* owner);

	virtual	void			MessageReceived(BMessage* message);

private:
			void*			fOwner;
};


}	// namespace BPrivate


#endif	// TOOL_TIP_WINDOW_H
