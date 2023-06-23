/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include <KMenuItemPrivate.h>

#include <KMenu.h>


namespace BPrivate {

KMenuItemPrivate::KMenuItemPrivate(KMenuItem* menuItem)
	:
	fMenuItem(menuItem)
{
}


void
KMenuItemPrivate::SetSubmenu(KMenu* submenu)
{
	delete fMenuItem->fSubmenu;

	fMenuItem->_InitMenuData(submenu);

	if (fMenuItem->fSuper != NULL) {
		fMenuItem->fSuper->InvalidateLayout();

		if (fMenuItem->fSuper->LockLooper()) {
			fMenuItem->fSuper->Invalidate();
			fMenuItem->fSuper->UnlockLooper();
		}
	}
}


void
KMenuItemPrivate::Install(KWindow* window)
{
	fMenuItem->Install(window);
}


void
KMenuItemPrivate::Uninstall()
{
	fMenuItem->Uninstall();
}


}	// namespace BPrivate
