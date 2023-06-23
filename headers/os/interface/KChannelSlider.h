/*
 * Copyright 2009-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_CHANNEL_SLIDER_H
#define _K_CHANNEL_SLIDER_H


#include <KChannelControl.h>


class KChannelSlider : public KChannelControl {
public:
								KChannelSlider(BRect area, const char* name,
									const char* label, BMessage* message,
									int32 channels = 1,
									uint32 resizeMode = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_WILL_DRAW);
								KChannelSlider(BRect area, const char* name,
									const char* label, BMessage* message,
									orientation orientation,
									int32 channels = 1,
									uint32 resizeMode = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_WILL_DRAW);
								KChannelSlider(const char* name,
									const char* label, BMessage* message,
									orientation orientation,
									int32 channels = 1,
									uint32 flags = B_WILL_DRAW);
								KChannelSlider(BMessage* archive);
	virtual						~KChannelSlider();

	static	BArchivable*		Instantiate(BMessage* from);
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;

	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				DetachedFromWindow();
	virtual	void				AllDetached();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				WindowActivated(bool state);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				KeyUp(const char* bytes, int32 numBytes);
	virtual	void				FrameResized(float width, float height);

	virtual	void				SetFont(const BFont* font,
									uint32 mask = B_FONT_ALL);
	virtual	void				MakeFocus(bool focusState = true);

	virtual	void				GetPreferredSize(float* _width, float* _height);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual	void				SetEnabled(bool on);

	virtual	orientation			Orientation() const;
			void				SetOrientation(orientation orientation);

	virtual	int32				MaxChannelCount() const;
	virtual	bool				SupportsIndividualLimits() const;

	virtual	void				DrawChannel(KView* into, int32 channel,
									BRect area, bool pressed);

	virtual	void				DrawGroove(KView* into, int32 channel,
									BPoint leftTop, BPoint rightBottom);

	virtual	void				DrawThumb(KView* into, int32 channel,
									BPoint where, bool pressed);

	virtual	const KBitmap*		ThumbFor(int32 channel, bool pressed);
	virtual	BRect				ThumbFrameFor(int32 channel);
	virtual	float				ThumbDeltaFor(int32 channel);
	virtual	float				ThumbRangeFor(int32 channel);

private:
	// FBC padding
								KChannelSlider(const KChannelSlider&);
			KChannelSlider&		operator=(const KChannelSlider&);


	virtual	void				_Reserved_KChannelSlider_0(void*, ...);
	virtual	void				_Reserved_KChannelSlider_1(void*, ...);
	virtual	void				_Reserved_KChannelSlider_2(void*, ...);
	virtual	void				_Reserved_KChannelSlider_3(void*, ...);
	virtual	void				_Reserved_KChannelSlider_4(void*, ...);
	virtual	void				_Reserved_KChannelSlider_5(void*, ...);
	virtual	void				_Reserved_KChannelSlider_6(void*, ...);
	virtual	void				_Reserved_KChannelSlider_7(void*, ...);

private:
			void				_InitData();
			void				_FinishChange(bool update = false);
			void				_UpdateFontDimens();
			void				_DrawThumbs();
			void				_DrawGrooveFrame(KView* where,
									const BRect& area);
			void				_MouseMovedCommon(BPoint point, BPoint point2);

private:
			float				fBaseLine;
			float				fLineFeed;
			KBitmap*			fLeftKnob;
			KBitmap*			fMidKnob;
			KBitmap*			fRightKnob;
			KBitmap*			fBacking;
			KView*				fBackingView;
			bool				fIsVertical;
			bool				_padding_[3];
			BPoint				fClickDelta;

			int32				fCurrentChannel;
			bool				fAllChannels;
			int32*				fInitialValues;
			float				fMinPoint;
			int32				fFocusChannel;

			uint32				_reserved_[12];
};


#endif // _CHANNEL_SLIDER_H
