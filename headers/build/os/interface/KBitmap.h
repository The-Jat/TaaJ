//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Distributed under the terms of the MIT license.
//
//	File Name:		Bitmap.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	KBitmap objects represent off-screen windows that
//					contain bitmap data.
//------------------------------------------------------------------------------

#ifndef	K__BITMAP_H
#define	K__BITMAP_H

#include <Archivable.h>
#include <InterfaceDefs.h>
#include <Rect.h>

#include<Bitmap.h>
/*
enum {
	K_BITMAP_CLEAR_TO_WHITE				= 0x00000001,
	K_BITMAP_ACCEPTS_VIEWS				= 0x00000002,
	K_BITMAP_IS_AREA					= 0x00000004,
	K_BITMAP_IS_LOCKED					= 0x00000008 | K_BITMAP_IS_AREA,
	K_BITMAP_IS_CONTIGUOUS				= 0x00000010 | K_BITMAP_IS_LOCKED,
	K_BITMAP_IS_OFFSCREEN				= 0x00000020,
	K_BITMAP_WILL_OVERLAY				= 0x00000040 | K_BITMAP_IS_OFFSCREEN,
	K_BITMAP_RESERVE_OVERLAY_CHANNEL	= 0x00000080,
	K_BITMAP_NO_SERVER_LINK				= 0x00000100
};*/

#define K_ANY_BYTES_PER_ROW	-1

//----------------------------------------------------------------//
//----- KBitmap class --------------------------------------------//

class KBitmap : public BArchivable {
public:
	KBitmap(BRect bounds, uint32 flags, color_space colorSpace,
			int32 bytesPerRow = K_ANY_BYTES_PER_ROW,
			screen_id screenID = B_MAIN_SCREEN_ID);
	KBitmap(BRect bounds, color_space colorSpace, bool acceptsViews = false,
			bool needsContiguous = false);
	KBitmap(const KBitmap *source, bool acceptsViews = false,
			bool needsContiguous = false);
	virtual ~KBitmap();

	// Archiving
	KBitmap(BMessage *data);
	static BArchivable *Instantiate(BMessage *data);
	virtual status_t Archive(BMessage *data, bool deep = true) const;

	status_t InitCheck() const;
	bool IsValid() const;

	status_t LockBits(uint32 *state = NULL);
	void UnlockBits();

	area_id Area() const;
	void *Bits() const;
	int32 BitsLength() const;
	int32 BytesPerRow() const;
	color_space ColorSpace() const;
	BRect Bounds() const;

	void SetBits(const void *data, int32 length, int32 offset,
				 color_space colorSpace);

	// not part of the R5 API
	status_t ImportBits(const void *data, int32 length, int32 bpr,
						int32 offset, color_space colorSpace);
	status_t ImportBits(const KBitmap *bitmap);

	status_t GetOverlayRestrictions(overlay_restrictions *restrictions) const;

//----- Private or reserved -----------------------------------------//
	
	virtual status_t Perform(perform_code d, void *arg);

private:
	virtual void _ReservedBitmap1();
	virtual void _ReservedBitmap2();
	virtual void _ReservedBitmap3();

	KBitmap(const KBitmap &);
	KBitmap &operator=(const KBitmap &);

	char *get_shared_pointer() const;
	int32 get_server_token() const;
	void InitObject(BRect bounds, color_space colorSpace, uint32 flags,
					int32 bytesPerRow, screen_id screenID);
	void CleanUp();

	void AssertPtr();

	void		*fBasePtr;
	int32		fSize;
	color_space	fColorSpace;
	BRect		fBounds;
	int32		fBytesPerRow;
	int32		fServerToken;
	int32		fToken;
	uint8		unused;
	area_id		fArea;
	area_id		fOrigArea;
	uint32		fFlags;
	status_t	fInitError;
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif	// _BITMAP_H
