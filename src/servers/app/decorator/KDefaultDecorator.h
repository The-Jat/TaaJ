/*
 * Copyright 2001-2020 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Ryan Leavengood, leavengood@gmail.com
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Joseph Groover, looncraz@looncraz.net
 *		Tri-Edge AI
 *		Jacob Secunda, secundja@gmail.com
 */
#ifndef K_DEFAULT_DECORATOR_H
#define K_DEFAULT_DECORATOR_H


#include "KTabDecorator.h"


class Desktop;
class ServerBitmap;


class K_DefaultDecorator: public K_TabDecorator {
public:
								K_DefaultDecorator(K_DesktopSettings& settings,
									BRect frame, Desktop* desktop);
	virtual						~K_DefaultDecorator();

	virtual	void				GetComponentColors(Component component,
									uint8 highlight, ComponentColors _colors,
									K_Decorator::Tab* tab = NULL);

	virtual void				UpdateColors(K_DesktopSettings& settings);

protected:
	virtual	void				_DrawFrame(BRect rect);

	virtual	void				_DrawTab(K_Decorator::Tab* tab, BRect r);
	virtual	void				_DrawTitle(K_Decorator::Tab* tab, BRect r);
	virtual	void				_DrawClose(K_Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawZoom(K_Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawMinimize(K_Decorator::Tab* tab, bool direct,
									BRect rect);
	virtual	void				_DrawResizeKnob(BRect r, bool full,
									const ComponentColors& color);

private:
 			void				_DrawButtonBitmap(ServerBitmap* bitmap,
 									bool direct, BRect rect);
			void				_DrawBlendedRect(DrawingEngine *engine,
									const BRect rect, bool down,
									const ComponentColors& colors);
			ServerBitmap*		_GetBitmapForButton(K_Decorator::Tab* tab,
									Component item, bool down, int32 width,
									int32 height);

			void				_GetComponentColors(Component component,
									ComponentColors _colors,
									K_Decorator::Tab* tab = NULL);
};


#endif	// DEFAULT_DECORATOR_H
