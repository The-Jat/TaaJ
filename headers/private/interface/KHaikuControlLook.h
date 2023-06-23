/*
 * Copyright 2009-2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_HAIKU_CONTROL_LOOK_H
#define _K_HAIKU_CONTROL_LOOK_H


#include <KControlLook.h>


class KBitmap;
class KControl;
class BGradientLinear;
class KView;


namespace BPrivate {


class KHaikuControlLook : public KControlLook {
public:
								KHaikuControlLook();
	virtual						~KHaikuControlLook();

	virtual BAlignment			DefaultLabelAlignment() const;
	virtual float				DefaultLabelSpacing() const;

	virtual	float				DefaultItemSpacing() const;

	static	float				ComposeSpacing(float spacing);

	virtual uint32				Flags(KControl* control) const;

	virtual	void				DrawButtonFrame(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawButtonFrame(KView* view, BRect& rect,
									const BRect& updateRect,
									float radius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawButtonFrame(KView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawButtonBackground(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonBackground(KView* view, BRect& rect,
									const BRect& updateRect,
									float radius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonBackground(KView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

	virtual	void				DrawMenuBarBackground(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuFieldFrame(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawMenuFieldFrame(KView* view, BRect& rect,
									const BRect& updateRect,
									float radius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);
	virtual	void				DrawMenuFieldFrame(KView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									const rgb_color& background,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuFieldBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, bool popupIndicator,
									uint32 flags = 0);
	virtual	void				DrawMenuFieldBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									float radius, const rgb_color& base,
									bool popupIndicator, uint32 flags = 0);
	virtual	void				DrawMenuFieldBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									bool popupIndicator, uint32 flags = 0);
	virtual	void				DrawMenuFieldBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawMenuItemBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawStatusBar(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& barColor,
									float progressPosition);

	virtual	void				DrawCheckBox(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0);

	virtual	void				DrawRadioButton(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0);

	virtual	void				DrawScrollBarBorder(KView* view,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawScrollBarButton(KView* view,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									int32 direction, orientation orientation,
									bool down = false);
	virtual	void				DrawScrollBarBackground(KView* view,
									BRect& rect1, BRect& rect2,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawScrollBarBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawScrollBarThumb(KView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation,
									uint32 knobStyle = B_KNOB_NONE);

	virtual	void				DrawScrollViewFrame(KView* view,
									BRect& rect, const BRect& updateRect,
									BRect verticalScrollBarFrame,
									BRect horizontalScrollBarFrame,
									const rgb_color& base,
									border_style borderStyle,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawArrowShape(KView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base, uint32 direction,
									uint32 flags = 0,
									float tint = B_DARKEN_MAX_TINT);

	virtual	rgb_color			SliderBarColor(const rgb_color& base);

	virtual	void				DrawSliderBar(KView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base,
									rgb_color leftFillColor,
									rgb_color rightFillColor,
									float sliderScale, uint32 flags,
									orientation orientation);
	virtual	void				DrawSliderBar(KView* view, BRect rect,
									const BRect& updateRect,
									const rgb_color& base, rgb_color fillColor,
									uint32 flags, orientation orientation);

	virtual	void				DrawSliderThumb(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);

	virtual	void				DrawSliderTriangle(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									orientation orientation);
	virtual	void				DrawSliderTriangle(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									const rgb_color& fill, uint32 flags,
									orientation orientation);

	virtual	void				DrawSliderHashMarks(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, int32 count,
									k_hash_mark_location location,
									uint32 flags, orientation orientation);

	virtual	void				DrawTabFrame(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									border_style borderStyle = B_FANCY_BORDER,
									uint32 side = B_TOP_BORDER);
	virtual	void				DrawActiveTab(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									uint32 side = B_TOP_BORDER,
									int32 index = 0, int32 selected = -1,
									int32 first = 0, int32 last = 0);
	virtual	void				DrawInactiveTab(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									uint32 side = B_TOP_BORDER,
									int32 index = 0, int32 selected = -1,
									int32 first = 0, int32 last = 0);

	virtual	void				DrawSplitter(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									orientation orientation,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	// various borders

	virtual	void				DrawBorder(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									border_style borderStyle, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawRaisedBorder(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawGroupFrame(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base,
									uint32 borders = B_ALL_BORDERS);

	virtual	void				DrawTextControlBorder(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	// aligned labels

	virtual	void				DrawLabel(KView* view, const char* label,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const rgb_color* textColor = NULL);
	virtual	void				DrawLabel(KView* view, const char* label,
									BRect rect, const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const BAlignment& alignment,
									const rgb_color* textColor = NULL);
	// TODO: Would be nice to have a (non-virtual) version of this method
	// which takes an array of labels and locations. That would save some
	// setup with the view graphics state.
	virtual	void				DrawLabel(KView* view, const char* label,
									const rgb_color& base, uint32 flags,
									const BPoint& where,
									const rgb_color* textColor = NULL);

	virtual	void				DrawLabel(KView* view, const char* label,
									const KBitmap* icon, BRect rect,
									const BRect& updateRect,
									const rgb_color& base, uint32 flags,
									const BAlignment& alignment,
									const rgb_color* textColor = NULL);

	virtual	void				GetFrameInsets(frame_type frameType,
									uint32 flags, float& _left, float& _top,
									float& _right, float& _bottom);
	virtual	void				GetBackgroundInsets(
									background_type backgroundType,
									uint32 flags, float& _left, float& _top,
									float& _right, float& _bottom);

	virtual	void				DrawButtonWithPopUpBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonWithPopUpBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									float radius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual	void				DrawButtonWithPopUpBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
	virtual float				GetScrollBarWidth(
									orientation orientation = B_VERTICAL);

protected:
			void				_DrawButtonFrame(KView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									const rgb_color& background,
									float contrast, float brightness = 1.0,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

			void				_DrawOuterResessedFrame(KView* view,
									BRect& rect, const rgb_color& base,
									float contrast = 1.0f,
									float brightness = 1.0f,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

			void				_DrawFrame(KView* view, BRect& rect,
									const rgb_color& left,
									const rgb_color& top,
									const rgb_color& right,
									const rgb_color& bottom,
									uint32 borders = B_ALL_BORDERS);
			void				_DrawFrame(KView* view, BRect& rect,
									const rgb_color& left,
									const rgb_color& top,
									const rgb_color& right,
									const rgb_color& bottom,
									const rgb_color& rightTop,
									const rgb_color& leftBottom,
									uint32 borders = B_ALL_BORDERS);

			void				_DrawButtonBackground(KView* view, BRect& rect,
									const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									bool popupIndicator = false,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
			void				_DrawFlatButtonBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									const rgb_color& base,
									bool popupIndicator = false,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);
			void				_DrawNonFlatButtonBackground(KView* view,
									BRect& rect, const BRect& updateRect,
									BRegion& clipping,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									bool popupIndicator = false,
									uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS,
									orientation orientation = B_HORIZONTAL);

			void				_DrawPopUpMarker(KView* view, const BRect& rect,
									const rgb_color& base, uint32 flags);

			void				_DrawMenuFieldBackgroundOutside(KView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base,
									bool popupIndicator = false,
									uint32 flags = 0);

			void				_DrawMenuFieldBackgroundInside(KView* view,
									BRect& rect, const BRect& updateRect,
									float leftTopRadius,
									float rightTopRadius,
									float leftBottomRadius,
									float rightBottomRadius,
									const rgb_color& base, uint32 flags = 0,
									uint32 borders = B_ALL_BORDERS);

	// Rounded corner methods
			void				_DrawRoundCornerLeftTop(KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeColor,
									const rgb_color& frameColor,
									const rgb_color& bevelColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerFrameLeftTop(KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeColor,
									const rgb_color& frameColor);

			void				_DrawRoundCornerBackgroundLeftTop(KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& bevelColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerRightTop(KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeTopColor,
									const rgb_color& edgeRightColor,
									const rgb_color& frameTopColor,
									const rgb_color& frameRightColor,
									const rgb_color& bevelTopColor,
									const rgb_color& bevelRightColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerFrameRightTop(KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeTopColor,
									const rgb_color& edgeRightColor,
									const rgb_color& frameTopColor,
									const rgb_color& frameRightColor);

			void				_DrawRoundCornerBackgroundRightTop(KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& bevelTopColor,
									const rgb_color& bevelRightColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerLeftBottom(KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeLeftColor,
									const rgb_color& edgeBottomColor,
									const rgb_color& frameLeftColor,
									const rgb_color& frameBottomColor,
									const rgb_color& bevelLeftColor,
									const rgb_color& bevelBottomColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerFrameLeftBottom(KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeLeftColor,
									const rgb_color& edgeBottomColor,
									const rgb_color& frameLeftColor,
									const rgb_color& frameBottomColor);

			void				_DrawRoundCornerBackgroundLeftBottom(KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& bevelLeftColor,
									const rgb_color& bevelBottomColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerRightBottom(KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeColor,
									const rgb_color& frameColor,
									const rgb_color& bevelColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundCornerFrameRightBottom(KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& background,
									const rgb_color& edgeColor,
									const rgb_color& frameColor);

			void				_DrawRoundCornerBackgroundRightBottom(
									KView* view,
									BRect& cornerRect, const BRect& updateRect,
									const rgb_color& bevelColor,
									const BGradientLinear& fillGradient);

			void				_DrawRoundBarCorner(KView* view, BRect& rect,
									const BRect& updateRect,
									const rgb_color& edgeLightColor,
									const rgb_color& edgeShadowColor,
									const rgb_color& frameLightColor,
									const rgb_color& frameShadowColor,
									const rgb_color& fillLightColor,
									const rgb_color& fillShadowColor,
									float leftInset, float topInset,
									float rightInset, float bottomInset,
									orientation orientation);

	// Border color methods
			rgb_color			_EdgeLightColor(const rgb_color& base,
									float contrast, float brightness,
									uint32 flags);

			rgb_color			_EdgeShadowColor(const rgb_color& base,
									float contrast, float brightness,
									uint32 flags);

			rgb_color			_FrameLightColor(const rgb_color& base,
									uint32 flags);

			rgb_color			_FrameShadowColor(const rgb_color& base,
									uint32 flags);

			rgb_color			_BevelLightColor(const rgb_color& base,
									uint32 flags);

			rgb_color			_BevelShadowColor(const rgb_color& base,
									uint32 flags);

	// Background gradient methods
			void				_FillGradient(KView* view, const BRect& rect,
									const rgb_color& base, float topTint,
									float bottomTint,
									orientation orientation = B_HORIZONTAL);

			void				_FillGlossyGradient(KView* view,
									const BRect& rect, const rgb_color& base,
									float topTint, float middle1Tint,
									float middle2Tint, float bottomTint,
									orientation orientation = B_HORIZONTAL);

			void				_MakeGradient(BGradientLinear& gradient,
									const BRect& rect, const rgb_color& base,
									float topTint, float bottomTint,
									orientation orientation = B_HORIZONTAL) const;

			void				_MakeGlossyGradient(BGradientLinear& gradient,
									const BRect& rect, const rgb_color& base,
									float topTint, float middle1Tint,
									float middle2Tint, float bottomTint,
									orientation orientation = B_HORIZONTAL) const;

			void				_MakeButtonGradient(BGradientLinear& gradient,
									BRect& rect, const rgb_color& base,
									uint32 flags, orientation orientation = B_HORIZONTAL) const;

			bool				_RadioButtonAndCheckBoxMarkColor(
									const rgb_color& base, rgb_color& color,
									uint32 flags) const;

private:
			bool				fCachedOutline;
};


} // namespace BPrivate

#endif // _HAIKU_CONTROL_LOOK_H
