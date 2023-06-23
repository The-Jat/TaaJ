/*
 * Copyright 2001-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Marc Flerackers, mflerackers@androme.be
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _K_BMC_PRIVATE_H
#define _K_BMC_PRIVATE_H


#include <BeBuild.h>
#include <KMenuBar.h>
#include <MessageFilter.h>


static const float k_kVMargin = 2.0f;


class K_BMCFilter_ : public BMessageFilter {
public:
								K_BMCFilter_(KMenuField* menuField, uint32 what);
	virtual						~K_BMCFilter_();

	virtual	filter_result		Filter(BMessage* message, BHandler** handler);

private:
			K_BMCFilter_&		operator=(const K_BMCFilter_&);

			KMenuField*			fMenuField;
};


class K_BMCMenuBar_ : public KMenuBar {
public:
								K_BMCMenuBar_(BRect frame, bool fixedSize,
									KMenuField* menuField);
								K_BMCMenuBar_(KMenuField* menuField);
								K_BMCMenuBar_(BMessage* data);
	virtual						~K_BMCMenuBar_();

	static	BArchivable*		Instantiate(BMessage* data);

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);
	virtual	void				MakeFocus(bool focused = true);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				SetMaxContentWidth(float width);
	virtual	void				SetEnabled(bool enabled);

			void				TogglePopUpMarker(bool show)
									{ fShowPopUpMarker = show; }
			bool				IsPopUpMarkerShown() const
									{ return fShowPopUpMarker; }

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

private:
								K_BMCMenuBar_&operator=(const K_BMCMenuBar_&);

			void				_Init();

			KMenuField*			fMenuField;
			bool				fFixedSize;	
			bool				fShowPopUpMarker;
			float				fPreviousWidth;
};


#endif // _BMC_PRIVATE_H
