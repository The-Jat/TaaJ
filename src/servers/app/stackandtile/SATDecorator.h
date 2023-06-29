/*
 * Copyright 2010-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef SAT_DECORATOR_H
#define SAT_DECORATOR_H


#include "DecorManager.h"
#include "DefaultDecorator.h"
#include "KDefaultDecorator.h"
#include "DefaultWindowBehaviour.h"
#include "KDefaultWindowBehaviour.h"//khidki
#include "StackAndTile.h"
#include "KStackAndTile.h"//khidki


class Desktop;


class SATDecorator : public DefaultDecorator {
public:
			enum {
				HIGHLIGHT_STACK_AND_TILE = HIGHLIGHT_USER_DEFINED
			};

public:
								SATDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop);

protected:
	virtual	void				UpdateColors(DesktopSettings& settings);
	virtual	void				GetComponentColors(Component component,
									uint8 highlight, ComponentColors _colors,
									Decorator::Tab* tab = NULL);

private:
				rgb_color		fHighlightTabColor;
				rgb_color		fHighlightTabColorLight;
				rgb_color		fHighlightTabColorBevel;
				rgb_color		fHighlightTabColorShadow;
};


class SATWindowBehaviour : public DefaultWindowBehaviour {
public:
								SATWindowBehaviour(Window* window,
									StackAndTile* sat);

protected:
	virtual bool				AlterDeltaForSnap(Window* window, BPoint& delta,
									bigtime_t now);

private:
			StackAndTile*		fStackAndTile;
};

//khidki start
class K_SATDecorator : public K_DefaultDecorator {
public:
			enum {
				HIGHLIGHT_STACK_AND_TILE = HIGHLIGHT_USER_DEFINED
			};

public:
								K_SATDecorator(DesktopSettings& settings,
									BRect frame, Desktop* desktop);

protected:
	virtual	void				UpdateColors(DesktopSettings& settings);
	virtual	void				GetComponentColors(Component component,
									uint8 highlight, ComponentColors _colors,
									K_Decorator::Tab* tab = NULL);

private:
				rgb_color		fHighlightTabColor;
				rgb_color		fHighlightTabColorLight;
				rgb_color		fHighlightTabColorBevel;
				rgb_color		fHighlightTabColorShadow;
};
//end


//khidki start

class K_SATWindowBehaviour : public K_DefaultWindowBehaviour {
public:
								K_SATWindowBehaviour(K_Window* window,
									K_StackAndTile* sat);

protected:
	virtual bool				AlterDeltaForSnap(K_Window* window, BPoint& delta,
									bigtime_t now);

private:
			K_StackAndTile*		fStackAndTile;
};

//end


#endif
