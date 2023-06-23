/*
 * Copyright 2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef K__WINDOW_BEHAVIOUR_H
#define K__WINDOW_BEHAVIOUR_H


#include <Region.h>

#include "Decorator.h"


class BMessage;
class ClickTarget;
class K_Window;


class K_WindowBehaviour {
public:
								K_WindowBehaviour();
	virtual						~K_WindowBehaviour();

	virtual	bool				MouseDown(BMessage* message, BPoint where,
									int32 lastHitRegion, int32& clickCount,
									int32& _hitRegion) = 0;
	virtual	void				MouseUp(BMessage* message, BPoint where) = 0;
	virtual	void				MouseMoved(BMessage *message, BPoint where,
									bool isFake) = 0;

	virtual	void				ModifiersChanged(int32 modifiers);

			bool				IsDragging() const { return fIsDragging; }
			bool				IsResizing() const { return fIsResizing; }

protected:
	/*! The window is going to be moved by delta. This hook should be used to
	implement the magnetic screen border, i.e. alter the delta accordantly.
	\return true if delta has been modified. */
	virtual bool				AlterDeltaForSnap(K_Window* window, BPoint& delta,
									bigtime_t now);

protected:
			bool				fIsResizing : 1;
			bool				fIsDragging : 1;
};


#endif
