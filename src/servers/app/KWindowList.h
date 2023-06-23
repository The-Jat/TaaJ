/*
 * Copyright (c) 2005-2008, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef __K_WINDOW_LIST_H
#define __K_WINDOW_LIST_H


#include <SupportDefs.h>
#include <Point.h>


class K_Window;


class K_WindowList {
public:
					K_WindowList(int32 index = 0);
					~K_WindowList();

			void	SetIndex(int32 index);
			int32	Index() const { return fIndex; }

			K_Window*	FirstWindow() const { return fFirstWindow; }
			K_Window*	LastWindow() const { return fLastWindow; }

			void	AddWindow(K_Window* window, K_Window* before = NULL);
			void	RemoveWindow(K_Window* window);

			bool	HasWindow(K_Window* window) const;
			bool	ValidateWindow(K_Window* window) const;

			int32	Count() const;
						// O(n)

private:
	int32			fIndex;
	K_Window*			fFirstWindow;
	K_Window*			fLastWindow;
};

enum k_window_lists {
	k_kAllWindowList = 32,
	k_kSubsetList,
	k_kFocusList,
	k_kWorkingList,

	k_kListCount
};

struct k_window_anchor {
	k_window_anchor();

	K_Window*	next;
	K_Window*	previous;
	BPoint	position;
};

extern const BPoint k_kInvalidWindowPosition;

#endif	// WINDOW_LIST_H
