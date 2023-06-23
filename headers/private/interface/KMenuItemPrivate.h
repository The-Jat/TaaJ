/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */
#ifndef __K__MENU_ITEM_PRIVATE_H
#define __K__MENU_ITEM_PRIVATE_H


#include <KMenuItem.h>


class KMenu;

namespace BPrivate {

class KMenuItemPrivate {
public:
								KMenuItemPrivate(KMenuItem* menuItem);

			void				SetSubmenu(KMenu* submenu);

			void				Install(KWindow* window);
			void				Uninstall();

private:
			KMenuItem*			fMenuItem;
};

};	// namespace BPrivate


#endif	// __MENU_ITEM_PRIVATE_H
