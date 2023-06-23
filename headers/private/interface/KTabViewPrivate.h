/*
 * Copyright 2015, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef _K_TABVIEW_PRIVATE_H
#define _K_TABVIEW_PRIVATE_H


#include <KTabView.h>


class KTab::Private {
public:
								Private(KTab* tab)
									:
									fTab(tab)
								{
								}

			void				SetTabView(KTabView* tabView)
									{ fTab->fTabView = tabView; }

private:
			KTab* fTab;
};


#endif	/* TABVIEW_PRIVATE_H */
