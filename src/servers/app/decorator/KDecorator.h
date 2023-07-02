/*
 * Copyright 2001-2020 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Joseph Groover, looncraz@looncraz.net
 *		Tri-Edge AI
 *		Jacob Secunda, secundja@gmail.com
 */
#ifndef K_DECORATOR_H
#define K_DECORATOR_H


#include <Rect.h>
#include <Region.h>
#include <String.h>
#include <Window.h>
#include <Khidki.h>//khidki

#include "DrawState.h"
#include "MultiLocker.h"

class Desktop;
class K_DesktopSettings;
class DrawingEngine;
class ServerBitmap;
class ServerFont;
class BRegion;


class K_Decorator {
public:
	struct Tab {
							Tab();
		virtual				~Tab() {}

		BRect				tabRect;

		BRect				zoomRect;
		BRect				closeRect;
		BRect				minimizeRect;

		bool				closePressed : 1;
		bool				zoomPressed : 1;
		bool				minimizePressed : 1;

		window_look			look;
		uint32				flags;
		bool				isFocused : 1;

		BString				title;

		uint32				tabOffset;
		float				tabLocation;
		float				textOffset;

		BString				truncatedTitle;
		int32				truncatedTitleLength;

		bool				buttonFocus : 1;

		bool				isHighlighted : 1;

		float				minTabSize;
		float				maxTabSize;

		ServerBitmap*		closeBitmaps[4];
		ServerBitmap*		minimizeBitmaps[4];
		ServerBitmap*		zoomBitmaps[4];
	};

	enum Region {
		REGION_NONE,

		REGION_TAB,

		REGION_CLOSE_BUTTON,
		REGION_ZOOM_BUTTON,
		REGION_MINIMIZE_BUTTON,

		REGION_LEFT_BORDER,
		REGION_RIGHT_BORDER,
		REGION_TOP_BORDER,
		REGION_BOTTOM_BORDER,

		REGION_LEFT_TOP_CORNER,
		REGION_LEFT_BOTTOM_CORNER,
		REGION_RIGHT_TOP_CORNER,
		REGION_RIGHT_BOTTOM_CORNER,

		REGION_COUNT
	};

	enum {
		HIGHLIGHT_NONE,
		HIGHLIGHT_RESIZE_BORDER,

		HIGHLIGHT_USER_DEFINED
	};

								K_Decorator(K_DesktopSettings& settings,
											BRect frame,
											Desktop* desktop);
	virtual						~K_Decorator();

	virtual	K_Decorator::Tab*		AddTab(K_DesktopSettings& settings,
									const char* title, window_look look,
									uint32 flags, int32 index = -1,
									BRegion* updateRegion = NULL);
	virtual	bool				RemoveTab(int32 index,
									BRegion* updateRegion = NULL);
	virtual	bool				MoveTab(int32 from, int32 to, bool isMoving,
									BRegion* updateRegion = NULL);

	virtual int32				TabAt(const BPoint& where) const;
			K_Decorator::Tab*		TabAt(int32 index) const
									{ return fTabList.ItemAt(index); }
			int32				CountTabs() const
									{ return fTabList.CountItems(); }
			void				SetTopTab(int32 tab);

			void				SetDrawingEngine(DrawingEngine *driver);
	inline	DrawingEngine*		GetDrawingEngine() const
									{ return fDrawingEngine; }

			void				FontsChanged(K_DesktopSettings& settings,
									BRegion* updateRegion = NULL);
			void				ColorsChanged(K_DesktopSettings& settings,
									BRegion* updateRegion = NULL);

	virtual void				UpdateColors(K_DesktopSettings& settings) = 0;

			void				SetLook(int32 tab, K_DesktopSettings& settings,
									window_look look,
									BRegion* updateRegion = NULL);
			void				SetFlags(int32 tab, uint32 flags,
									BRegion* updateRegion = NULL);

			window_look			Look(int32 tab) const;
			uint32				Flags(int32 tab) const;

			BRect				BorderRect() const;
			BRect				TitleBarRect() const;
			BRect				TabRect(int32 tab) const;
			BRect				TabRect(K_Decorator::Tab* tab) const;

			void				SetClose(int32 tab, bool pressed);
			void				SetMinimize(int32 tab, bool pressed);
			void				SetZoom(int32 tab, bool pressed);

			const char*			Title(int32 tab) const;
			const char*			Title(K_Decorator::Tab* tab) const;
			void				SetTitle(int32 tab, const char* string,
									BRegion* updateRegion = NULL);

			void				SetFocus(int32 tab, bool focussed);
			bool				IsFocus(int32 tab) const;
			bool				IsFocus(K_Decorator::Tab* tab) const;

	virtual	float				TabLocation(int32 tab) const;
			bool				SetTabLocation(int32 tab, float location,
									bool isShifting,
									BRegion* updateRegion = NULL);
				/*! \return true if tab location updated, false if out of
					bounds or unsupported */

	virtual	Region				RegionAt(BPoint where, int32& tab) const;

			const BRegion&		GetFootprint();
			::Desktop*			GetDesktop();

			void				MoveBy(float x, float y);
			void				MoveBy(BPoint offset);
			void				ResizeBy(float x, float y, BRegion* dirty);
			void				ResizeBy(BPoint offset, BRegion* dirty);
			void				SetOutlinesDelta(BPoint delta, BRegion* dirty);
			bool				IsOutlineResizing() const
									{ return fOutlinesDelta != BPoint(0, 0); }

	virtual	bool				SetRegionHighlight(Region region,
									uint8 highlight, BRegion* dirty,
									int32 tab = -1);
	inline	uint8				RegionHighlight(Region region,
									int32 tab = -1) const;

			bool				SetSettings(const BMessage& settings,
									BRegion* updateRegion = NULL);
	virtual	bool				GetSettings(BMessage* settings) const;

	virtual	void				GetSizeLimits(int32* minWidth, int32* minHeight,
									int32* maxWidth, int32* maxHeight) const;
	virtual	void				ExtendDirtyRegion(Region region, BRegion& dirty);

	virtual	void				Draw(BRect updateRect) = 0;
	virtual	void				Draw() = 0;

	virtual	void				DrawTab(int32 tab);
	virtual	void				DrawTitle(int32 tab);

	virtual	void				DrawClose(int32 tab);
	virtual	void				DrawMinimize(int32 tab);
	virtual	void				DrawZoom(int32 tab);

			rgb_color			UIColor(light_color_scheme_which which);

			float				BorderWidth();
			float				TabHeight();

protected:
	virtual	K_Decorator::Tab*		_AllocateNewTab();

	virtual	void				_DoLayout() = 0;
		//! method for calculating layout for the decorator
	virtual	void				_DoOutlineLayout() = 0;

	virtual	void				_DrawFrame(BRect rect) = 0;
	virtual	void				_DrawOutlineFrame(BRect rect) = 0;
	virtual	void				_DrawTabs(BRect rect);

	virtual	void				_DrawTab(K_Decorator::Tab* tab, BRect rect) = 0;
	virtual	void				_DrawTitle(K_Decorator::Tab* tab,
									BRect rect) = 0;

	virtual	void				_DrawButtons(K_Decorator::Tab* tab,
									const BRect& invalid) = 0;
	virtual	void				_DrawClose(K_Decorator::Tab* tab, bool direct,
									BRect rect) = 0;
	virtual	void				_DrawMinimize(K_Decorator::Tab* tab, bool direct,
									BRect rect) = 0;
	virtual	void				_DrawZoom(K_Decorator::Tab* tab, bool direct,
									BRect rect) = 0;

	virtual	void				_SetTitle(K_Decorator::Tab* tab,
									const char* string,
									BRegion* updateRegion = NULL) = 0;
			int32				_TitleWidth(K_Decorator::Tab* tab) const
									{ return tab->title.CountChars(); }

	virtual	void				_SetFocus(K_Decorator::Tab* tab);
	virtual	bool				_SetTabLocation(K_Decorator::Tab* tab,
									float location, bool isShifting,
									BRegion* updateRegion = NULL);

	virtual	K_Decorator::Tab*		_TabAt(int32 index) const;

	virtual void				_FontsChanged(K_DesktopSettings& settings,
									BRegion* updateRegion = NULL);
	virtual	void				_UpdateFont(K_DesktopSettings& settings) = 0;

	virtual void				_SetLook(K_Decorator::Tab* tab,
									K_DesktopSettings& settings,
									window_look look,
									BRegion* updateRegion = NULL);
	virtual void				_SetFlags(K_Decorator::Tab* tab, uint32 flags,
									BRegion* updateRegion = NULL);

	virtual void				_MoveBy(BPoint offset);
	virtual	void				_ResizeBy(BPoint offset, BRegion* dirty) = 0;

	virtual void				_MoveOutlineBy(BPoint offset);
	virtual void				_ResizeOutlineBy(BPoint offset, BRegion* dirty);
	virtual void				_SetOutlinesDelta(BPoint delta, BRegion* dirty);

	virtual bool				_SetSettings(const BMessage& settings,
									BRegion* updateRegion = NULL);

	virtual	bool				_AddTab(K_DesktopSettings& settings,
									int32 index = -1,
									BRegion* updateRegion = NULL) = 0;
	virtual	bool				_RemoveTab(int32 index,
									BRegion* updateRegion = NULL) = 0;
	virtual	bool				_MoveTab(int32 from, int32 to, bool isMoving,
									BRegion* updateRegion = NULL) = 0;

	virtual	void				_GetFootprint(BRegion* region);
	virtual void				_GetOutlineFootprint(BRegion* region);
			void				_InvalidateFootprint();

			void 				_InvalidateBitmaps();

protected:
	mutable		MultiLocker	fLocker;

			DrawingEngine*		fDrawingEngine;
			DrawState			fDrawState;

			BPoint				fOutlinesDelta;

			// Individual rects for handling window frame
			// rendering the proper way
			BRect				fTitleBarRect;
			BRect				fFrame;
			BRect				fResizeRect;
			BRect				fBorderRect;
			BRect				fOutlineBorderRect;

			BRect				fLeftBorder;
			BRect				fTopBorder;
			BRect				fBottomBorder;
			BRect				fRightBorder;

			BRect				fLeftOutlineBorder;
			BRect				fTopOutlineBorder;
			BRect				fBottomOutlineBorder;
			BRect				fRightOutlineBorder;

			int32				fBorderWidth;
			int32				fOutlineBorderWidth;

			K_Decorator::Tab*		fTopTab;
			BObjectList<K_Decorator::Tab>	fTabList;

private:
			Desktop*			fDesktop;
			BRegion				fFootprint;
			bool				fFootprintValid : 1;

			uint8				fRegionHighlights[REGION_COUNT - 1];
};


uint8
K_Decorator::RegionHighlight(Region region, int32 tab) const
{
	int32 index = (int32)region - 1;
	return index >= 0 && index < REGION_COUNT - 1
		? fRegionHighlights[index] : 0;
}


#endif	// DECORATOR_H
