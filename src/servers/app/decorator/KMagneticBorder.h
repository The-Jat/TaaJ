/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef K_MAGNETIC_BORDRER_H
#define K_MAGNETIC_BORDRER_H


#include <Point.h>
#include <Screen.h>


class Screen;
//class Window;
class K_Window;//khidki


class K_MagneticBorder {
public:
								K_MagneticBorder();

			bool				AlterDeltaForSnap(K_Window* window, BPoint& delta,
									bigtime_t now);
//khidki start
//bool	K_AlterDeltaForSnap(K_Window* window, BPoint& delta, bigtime_t now);
//end

			bool				AlterDeltaForSnap(const Screen* screen,
									BRect& frame, BPoint& delta, bigtime_t now);

private:
			bigtime_t			fLastSnapTime;
};


#endif // MAGNETIC_BORDRER_H
