/*
 * Copyright 2001-2010, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Adi Oanca <adioanca@gmail.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler <axeld@pinc-software.de>
 *		Brecht Machiels <brecht@mos6581.org>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _K_DEFAULT_WINDOW_BEHAVIOUR_H
#define _K_DEFAULT_WINDOW_BEHAVIOUR_H


#include "KWindowBehaviour.h"

#include "Decorator.h"
#include "MagneticBorder.h"
#include "ServerCursor.h"

#include <AutoDeleter.h>


class Desktop;
class K_Window;


class K_DefaultWindowBehaviour : public K_WindowBehaviour {
public:
								K_DefaultWindowBehaviour(K_Window* window);
	virtual						~K_DefaultWindowBehaviour();

	virtual	bool				MouseDown(BMessage* message, BPoint where,
									int32 lastHitRegion, int32& clickCount,
									int32& _hitRegion);
	virtual	void				MouseUp(BMessage* message, BPoint where);
	virtual	void				MouseMoved(BMessage *message, BPoint where,
									bool isFake);

	virtual	void				ModifiersChanged(int32 modifiers);

protected:
	virtual bool				AlterDeltaForSnap(K_Window* window, BPoint& delta,
									bigtime_t now);
private:
			enum Action {
				ACTION_NONE,
				ACTION_ZOOM,
				ACTION_CLOSE,
				ACTION_MINIMIZE,
				ACTION_TAB,
				ACTION_DRAG,
				ACTION_SLIDE_TAB,
				ACTION_RESIZE,
				ACTION_RESIZE_BORDER
			};

			enum {
				// 1 for the "natural" resize border, -1 for the opposite, so
				// multiplying the movement delta by that value results in the
				// size change.
				LEFT	= -1,
				TOP		= -1,
				NONE	= 0,
				RIGHT	= 1,
				BOTTOM	= 1
			};

			struct State;
			struct MouseTrackingState;
			struct DragState;
			struct ResizeState;
			struct SlideTabState;
			struct ResizeBorderState;
			struct DecoratorButtonState;
			struct ManageWindowState;

			// to keep gcc 2 happy
			friend struct State;
			friend struct MouseTrackingState;
			friend struct DragState;
			friend struct ResizeState;
			friend struct SlideTabState;
			friend struct ResizeBorderState;
			friend struct DecoratorButtonState;
			friend struct ManageWindowState;

private:
			bool				_IsWindowModifier(int32 modifiers) const;
			Decorator::Region	_RegionFor(const BMessage* message,
									int32& tab) const;

			void				_SetBorderHighlights(int8 horizontal,
									int8 vertical, bool active);

			ServerCursor*		_ResizeCursorFor(int8 horizontal,
									int8 vertical);
			void				_SetResizeCursor(int8 horizontal,
									int8 vertical);
			void				_ResetResizeCursor();
	static	void				_ComputeResizeDirection(float x, float y,
									int8& _horizontal, int8& _vertical);

			void				_NextState(State* state);

protected:
			K_Window*				fWindow;
			Desktop*			fDesktop;
			ObjectDeleter<State>
								fState;
			int32				fLastModifiers;

			MagneticBorder		fMagneticBorder;
};


#endif	// DEFAULT_WINDOW_BEHAVIOUR_H
