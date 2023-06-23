/*
 * Copyright 2006-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#ifndef __K__MENU_PRIVATE_H
#define __K__MENU_PRIVATE_H


#include <KMenu.h>


enum k_menu_states {
	K_MENU_STATE_TRACKING = 0,
	K_MENU_STATE_TRACKING_SUBMENU = 1,
	K_MENU_STATE_KEY_TO_SUBMENU = 2,
	K_MENU_STATE_KEY_LEAVE_SUBMENU = 3,
	K_MENU_STATE_CLOSED = 5
};


class KBitmap;
class KMenu;
class KWindow;


namespace BPrivate {

extern const char* k_kEmptyMenuLabel;

class KMenuPrivate {
public:
								KMenuPrivate(KMenu* menu);

			k_menu_layout			Layout() const;
			void				SetLayout(k_menu_layout layout);

			void				ItemMarked(KMenuItem* item);
			void				CacheFontInfo();

			float				FontHeight() const;
			float				Ascent() const;
			BRect				Padding() const;
			void				GetItemMargins(float*, float*, float*, float*)
									const;
			void				SetItemMargins(float, float, float, float);

			int					State(KMenuItem** item = NULL) const;

			void				Install(KWindow* window);
			void				Uninstall();
			void				SetSuper(KMenu* menu);
			void				SetSuperItem(KMenuItem* item);
			void				InvokeItem(KMenuItem* item, bool now = false);
			void				QuitTracking(bool thisMenuOnly = true);
			bool				HasSubmenus() { return fMenu->fHasSubmenus; }

	static	status_t			CreateBitmaps();
	static	void				DeleteBitmaps();

	static	const KBitmap*		MenuItemShift();
	static	const KBitmap*		MenuItemControl();
	static	const KBitmap*		MenuItemOption();
	static	const KBitmap*		MenuItemCommand();
	static	const KBitmap*		MenuItemMenu();

private:
			KMenu*				fMenu;

	static	KBitmap*			sMenuItemShift;
	static	KBitmap*			sMenuItemControl;
	static	KBitmap*			sMenuItemOption;
	static	KBitmap*			sMenuItemAlt;
	static	KBitmap*			sMenuItemMenu;

};

};	// namespace BPrivate


// Note: since sqrt is slow, we don't use it and return the square of the
// distance
#define square(x) ((x) * (x))
static inline float
k_point_distance(const BPoint &pointA, const BPoint &pointB)
{
	return square(pointA.x - pointB.x) + square(pointA.y - pointB.y);
}
#undef square


#endif	// __MENU_PRIVATE_H
