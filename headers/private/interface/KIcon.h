/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_INTERFACE__ICON_H_
#define _K_INTERFACE__ICON_H_


#include <InterfaceDefs.h>
#include <ObjectList.h>
#include <Rect.h>


class KBitmap;


namespace BPrivate {


class KIcon {
public:
								KIcon();
								~KIcon();

			status_t			SetTo(const KBitmap* bitmap, uint32 flags = 0);

			bool				SetBitmap(KBitmap* bitmap, uint32 which);
			KBitmap*			Bitmap(uint32 which) const;

			status_t			SetExternalBitmap(const KBitmap* bitmap,
									uint32 which, uint32 flags);

			KBitmap*			CreateBitmap(const BRect& bounds,
									color_space colorSpace, uint32 which);
			KBitmap*			CopyBitmap(const KBitmap& bitmapToClone,
									uint32 which);
			void				DeleteBitmaps();

	// convenience methods for icon owners
	static	status_t			UpdateIcon(const KBitmap* bitmap, uint32 flags,
									KIcon*& _icon);
	static	status_t			SetIconBitmap(const KBitmap* bitmap,
									uint32 which, uint32 flags, KIcon*& _icon);

private:
			typedef BObjectList<KBitmap> BitmapList;

private:
	static	KBitmap*			_ConvertToRGB32(const KBitmap* bitmap,
									bool noAppServerLink = false);
	static	status_t			_TrimBitmap(const KBitmap* bitmap,
									bool keepAspect, KBitmap*& _trimmedBitmap);
			status_t			_MakeBitmaps(const KBitmap* bitmap,
									uint32 flags);

private:
			BitmapList			fEnabledBitmaps;
			BitmapList			fDisabledBitmaps;
};


}	// namespace BPrivate


using BPrivate::KIcon;


#endif	// _INTERFACE__ICON_H_
