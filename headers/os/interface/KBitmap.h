/*
 * Copyright 2001-2007, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__BITMAP_H
#define	K__BITMAP_H


#include <Archivable.h>
#include <InterfaceDefs.h>
#include <Rect.h>

#include<Bitmap.h>

//class KView;
class KView;//khidki
class KWindow;
namespace BPrivate {
	class BPrivateScreen;
}
/*
enum {
	K_BITMAP_CLEAR_TO_WHITE				= 0x00000001,
	K_BITMAP_ACCEPTS_VIEWS				= 0x00000002,
	K_BITMAP_IS_AREA					= 0x00000004,
	K_BITMAP_IS_LOCKED					= 0x00000008 | K_BITMAP_IS_AREA,
	K_BITMAP_IS_CONTIGUOUS				= 0x00000010 | K_BITMAP_IS_LOCKED,
	K_BITMAP_IS_OFFSCREEN				= 0x00000020,
		// Offscreen but non-overlay bitmaps are not supported on Haiku,
		// but appearantly never were on BeOS either! The accelerant API
		// would need to be extended to so that the app_server can ask
		// the graphics driver to reserve memory for a bitmap and for this
		// to make any sense, an accelerated blit from this memory into
		// the framebuffer needs to be added to the API as well.
	K_BITMAP_WILL_OVERLAY				= 0x00000040 | K_BITMAP_IS_OFFSCREEN,
	K_BITMAP_RESERVE_OVERLAY_CHANNEL	= 0x00000080,

	// Haiku extensions:
	K_BITMAP_NO_SERVER_LINK				= 0x00000100,
		// Cheap to create, object will manage memory itself,
		// no BApplication needs to run, but one can't draw such
		// a KBitmap.
};
*/
#define K_ANY_BYTES_PER_ROW	-1


class KBitmap : public BArchivable {
public:
								KBitmap(BRect bounds, uint32 flags,
									color_space colorSpace,
									int32 bytesPerRow = K_ANY_BYTES_PER_ROW,
									screen_id screenID = B_MAIN_SCREEN_ID);
								KBitmap(BRect bounds, color_space colorSpace,
									bool acceptsViews = false,
									bool needsContiguous = false);
								KBitmap(const KBitmap& source, uint32 flags);
								KBitmap(const KBitmap& source);
								KBitmap(const KBitmap* source,
									bool acceptsViews = false,
									bool needsContiguous = false);
	virtual						~KBitmap();

	// Archiving
								KBitmap(BMessage* data);
	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

			status_t			InitCheck() const;
			bool				IsValid() const;

			status_t			LockBits(uint32* state = NULL);
			void				UnlockBits();

			area_id				Area() const;
			void*				Bits() const;
			int32				BitsLength() const;
			int32				BytesPerRow() const;
			color_space			ColorSpace() const;
			BRect				Bounds() const;

			status_t			SetDrawingFlags(uint32 flags);
			uint32				Flags() const;

			void				SetBits(const void* data, int32 length,
									int32 offset, color_space colorSpace);

	// not part of the R5 API
			status_t			ImportBits(const void* data, int32 length,
									int32 bpr, int32 offset,
									color_space colorSpace);
			status_t			ImportBits(const void* data, int32 length,
									int32 bpr, color_space colorSpace,
									BPoint from, BPoint to, int32 width,
									int32 height);
			status_t			ImportBits(const KBitmap* bitmap);
			status_t			ImportBits(const KBitmap* bitmap, BPoint from,
									BPoint to, int32 width, int32 height);

			status_t			GetOverlayRestrictions(
									overlay_restrictions* restrictions) const;

	// to mimic a KWindow
	virtual	void				AddChild(KView* view);
	virtual	bool				RemoveChild(KView* view);
			int32				CountChildren() const;
			KView*				ChildAt(int32 index) const;
			KView*				FindView(const char* viewName) const;
			KView*				FindView(BPoint point) const;
			bool				Lock();
			void				Unlock();
			bool				IsLocked() const;

			KBitmap&			operator=(const KBitmap& source);

	class Private;
private:
	//friend class KView;
		friend class KView;//khidki
	friend class BApplication;
	friend class ::BPrivate::BPrivateScreen;
	friend class Private;

	virtual	status_t			Perform(perform_code d, void* arg);
	virtual	void				_ReservedBitmap1();
	virtual	void				_ReservedBitmap2();
	virtual	void				_ReservedBitmap3();

			int32				_ServerToken() const;
			void				_InitObject(BRect bounds,
									color_space colorSpace, uint32 flags,
									int32 bytesPerRow, screen_id screenID);
			void				_CleanUp();
			void				_AssertPointer();

			void				_ReconnectToAppServer();

private:
			uint8*				fBasePointer;
			int32				fSize;
			color_space			fColorSpace;
			BRect				fBounds;
			int32				fBytesPerRow;
			KWindow*			fWindow;
			int32				fServerToken;
			int32				fAreaOffset;
			uint8				unused;
			area_id				fArea;
			area_id				fServerArea;
			uint32				fFlags;
			status_t			fInitError;
};

#endif	// _BITMAP_H
