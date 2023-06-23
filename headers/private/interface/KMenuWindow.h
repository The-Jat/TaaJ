/*
 * Copyright 2001-2009, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stefano Ceccherini (burton666@libero.it)
 */
#ifndef K_MENU_WINDOW_H
#define K_MENU_WINDOW_H


#include <Khidki.h>

class KMenu;


namespace BPrivate {

class KMenuFrame;
class KMenuScroller;


class KMenuWindow : public KWindow {
public:
							KMenuWindow(const char* name);
	virtual					~KMenuWindow();

	virtual	void			DispatchMessage(BMessage* message,
								BHandler* handler);

			void			AttachMenu(KMenu* menu);
			void			DetachMenu();

			void			AttachScrollers();
			void			DetachScrollers();

			void			SetSmallStep(float step);
			void			GetSteps(float* _smallStep, float* _largeStep) const;
			bool			HasScrollers() const;
			bool			CheckForScrolling(const BPoint& cursor);
			bool			TryScrollBy(const float& step);
			bool			TryScrollTo(const float& where);

private:
			bool			_Scroll(const BPoint& cursor);
			void			_ScrollBy(const float& step);

			KMenu*			fMenu;
			KMenuFrame*		fMenuFrame;
			KMenuScroller*	fUpperScroller;
			KMenuScroller*	fLowerScroller;

			float			fScrollStep;
			float			fValue;
			float			fLimit;
};

}	// namespace BPrivate

#endif	// MENU_WINDOW_H
