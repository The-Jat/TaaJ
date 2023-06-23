/*
 * Copyright 2001-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */


/*!	KBitmap objects represent off-screen windows that
	contain bitmap data.
*/


#include <KBitmap.h>

#include <algorithm>
#include <limits.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <GraphicsDefs.h>
#include <Locker.h>
#include <Laminate.h>
#include <Khidki.h>

#include <ApplicationPrivate.h>
#include <AppServerLink.h>
#include <Autolock.h>
#include <ObjectList.h>
#include <ServerMemoryAllocator.h>
#include <ServerProtocol.h>

#include "ColorConversion.h"
#include "KBitmapPrivate.h"


using namespace BPrivate;


//khidki code
//start
//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TTRACE(x) debug_printf x
#else
#	define TTRACE(x) ;
#endif
//end

static BObjectList<KBitmap> k_sBitmapList;
static BLocker k_sBitmapListLock;


void
k_reconnect_bitmaps_to_app_server()
{
	BAutolock _(k_sBitmapListLock);
	for (int32 i = 0; i < k_sBitmapList.CountItems(); i++) {
		KBitmap::Private bitmap(k_sBitmapList.ItemAt(i));
		bitmap.ReconnectToAppServer();
	}
}


KBitmap::Private::Private(KBitmap* bitmap)
	:
	fBitmap(bitmap)
{
}


void
KBitmap::Private::ReconnectToAppServer()
{
	fBitmap->_ReconnectToAppServer();
}


/*!	\brief Returns the number of bytes per row needed to store the actual
		   bitmap data (not including any padding) given a color space and a
		   row width.
	\param colorSpace The color space.
	\param width The width.
	\return The number of bytes per row needed to store data for a row, or
			0, if the color space is not supported.
*/
static inline int32
k_get_raw_bytes_per_row(color_space colorSpace, int32 width)
{
	int32 bpr = 0;
	switch (colorSpace) {
		// supported
		case B_RGBA64: case B_RGBA64_BIG:
			bpr = 8 * width;
			break;
		case B_RGB48: case B_RGB48_BIG:
			bpr = 6 * width;
			break;
		case B_RGB32: case B_RGBA32:
		case B_RGB32_BIG: case B_RGBA32_BIG:
		case B_UVL32: case B_UVLA32:
		case B_LAB32: case B_LABA32:
		case B_HSI32: case B_HSIA32:
		case B_HSV32: case B_HSVA32:
		case B_HLS32: case B_HLSA32:
		case B_CMY32: case B_CMYA32: case B_CMYK32:
			bpr = 4 * width;
			break;
		case B_RGB24: case B_RGB24_BIG:
		case B_UVL24: case B_LAB24: case B_HSI24:
		case B_HSV24: case B_HLS24: case B_CMY24:
			bpr = 3 * width;
			break;
		case B_RGB16:		case B_RGB15:		case B_RGBA15:
		case B_RGB16_BIG:	case B_RGB15_BIG:	case B_RGBA15_BIG:
			bpr = 2 * width;
			break;
		case B_CMAP8: case B_GRAY8:
			bpr = width;
			break;
		case B_GRAY1:
			bpr = (width + 7) / 8;
			break;
		case B_YCbCr422: case B_YUV422:
			bpr = (width + 3) / 4 * 8;
			break;
		case B_YCbCr411: case B_YUV411:
			bpr = (width + 3) / 4 * 6;
			break;
		case B_YCbCr444: case B_YUV444:
			bpr = (width + 3) / 4 * 12;
			break;
		case B_YCbCr420: case B_YUV420:
			bpr = (width + 3) / 4 * 6;
			break;
		case B_YUV9:
			bpr = (width + 15) / 16 * 18;
			break;
		// unsupported
		case B_NO_COLOR_SPACE:
		case B_YUV12:
			break;
	}
	return bpr;
}


namespace BPrivate {

/*!	\brief Returns the number of bytes per row needed to store the bitmap
		   data (including any padding) given a color space and a row width.
	\param colorSpace The color space.
	\param width The width.
	\return The number of bytes per row needed to store data for a row, or
			0, if the color space is not supported.
*/
int32
k_get_bytes_per_row(color_space colorSpace, int32 width)
{
	int32 bpr = k_get_raw_bytes_per_row(colorSpace, width);
	// align to int32
	bpr = (bpr + 3) & 0x7ffffffc;
	return bpr;
}

}	// namespace BPrivate


//	#pragma mark -


/*!	\brief Creates and initializes a KBitmap.
	\param bounds The bitmap dimensions.
	\param flags Creation flags.
	\param colorSpace The bitmap's color space.
	\param bytesPerRow The number of bytes per row the bitmap should use.
		   \c B_ANY_BYTES_PER_ROW to let the constructor choose an appropriate
		   value.
	\param screenID ???
*/
KBitmap::KBitmap(BRect bounds, uint32 flags, color_space colorSpace,
		int32 bytesPerRow, screen_id screenID)
	:
	fBasePointer(NULL),
	fSize(0),
	fColorSpace(B_NO_COLOR_SPACE),
	fBounds(0, 0, -1, -1),
	fBytesPerRow(0),
	fWindow(NULL),
	fServerToken(-1),
	fAreaOffset(-1),
	fArea(-1),
	fServerArea(-1),
	fFlags(0),
	fInitError(B_NO_INIT)
{
	_InitObject(bounds, colorSpace, flags, bytesPerRow, screenID);
}


/*!	\brief Creates and initializes a KBitmap.
	\param bounds The bitmap dimensions.
	\param colorSpace The bitmap's color space.
	\param acceptsViews \c true, if the bitmap shall accept BViews, i.e. if
		   it shall be possible to attach KView to the bitmap and draw into
		   it.
	\param needsContiguous If \c true a physically contiguous chunk of memory
		   will be allocated.
*/
KBitmap::KBitmap(BRect bounds, color_space colorSpace, bool acceptsViews,
		bool needsContiguous)
	:
	fBasePointer(NULL),
	fSize(0),
	fColorSpace(B_NO_COLOR_SPACE),
	fBounds(0, 0, -1, -1),
	fBytesPerRow(0),
	fWindow(NULL),
	fServerToken(-1),
	fAreaOffset(-1),
	fArea(-1),
	fServerArea(-1),
	fFlags(0),
	fInitError(B_NO_INIT)
{
debug_printf("[KBitmap] constructor\n");

	int32 flags = (acceptsViews ? B_BITMAP_ACCEPTS_VIEWS : 0)
		| (needsContiguous ? B_BITMAP_IS_CONTIGUOUS : 0);
	_InitObject(bounds, colorSpace, flags, K_ANY_BYTES_PER_ROW,
		B_MAIN_SCREEN_ID);

debug_printf("[KBitmap] constructor end\n");
}


/*!	\brief Creates a KBitmap as a clone of another bitmap.
	\param source The source bitmap.
	\param acceptsViews \c true, if the bitmap shall accept BViews, i.e. if
		   it shall be possible to attach KView to the bitmap and draw into
		   it.
	\param needsContiguous If \c true a physically contiguous chunk of memory
		   will be allocated.
*/
KBitmap::KBitmap(const KBitmap* source, bool acceptsViews, bool needsContiguous)
	:
	fBasePointer(NULL),
	fSize(0),
	fColorSpace(B_NO_COLOR_SPACE),
	fBounds(0, 0, -1, -1),
	fBytesPerRow(0),
	fWindow(NULL),
	fServerToken(-1),
	fAreaOffset(-1),
	fArea(-1),
	fServerArea(-1),
	fFlags(0),
	fInitError(B_NO_INIT)
{
	if (source && source->IsValid()) {
		int32 flags = (acceptsViews ? B_BITMAP_ACCEPTS_VIEWS : 0)
			| (needsContiguous ? B_BITMAP_IS_CONTIGUOUS : 0);
		_InitObject(source->Bounds(), source->ColorSpace(), flags,
			source->BytesPerRow(), B_MAIN_SCREEN_ID);
		if (InitCheck() == B_OK) {
			memcpy(Bits(), source->Bits(), min_c(BitsLength(),
				source->BitsLength()));
		}
	}
}


KBitmap::KBitmap(const KBitmap& source, uint32 flags)
	:
	fBasePointer(NULL),
	fSize(0),
	fColorSpace(B_NO_COLOR_SPACE),
	fBounds(0, 0, -1, -1),
	fBytesPerRow(0),
	fWindow(NULL),
	fServerToken(-1),
	fAreaOffset(-1),
	fArea(-1),
	fServerArea(-1),
	fFlags(0),
	fInitError(B_NO_INIT)
{
debug_printf("[KBitmap] constructor\n");

	if (!source.IsValid())
		return;

	_InitObject(source.Bounds(), source.ColorSpace(), flags,
		source.BytesPerRow(), B_MAIN_SCREEN_ID);

	if (InitCheck() == B_OK)
		memcpy(Bits(), source.Bits(), min_c(BitsLength(), source.BitsLength()));

debug_printf("[KBitmap] constructor end\n");
}


KBitmap::KBitmap(const KBitmap& source)
	:
	fBasePointer(NULL),
	fSize(0),
	fColorSpace(B_NO_COLOR_SPACE),
	fBounds(0, 0, -1, -1),
	fBytesPerRow(0),
	fWindow(NULL),
	fServerToken(-1),
	fAreaOffset(-1),
	fArea(-1),
	fServerArea(-1),
	fFlags(0),
	fInitError(B_NO_INIT)
{
	*this = source;
}


/*!	\brief Frees all resources associated with this object.
*/
KBitmap::~KBitmap()
{
	_CleanUp();
}


/*!	\brief Unarchives a bitmap from a BMessage.
	\param data The archive.
*/
KBitmap::KBitmap(BMessage* data)
	:
	BArchivable(data),
	fBasePointer(NULL),
	fSize(0),
	fColorSpace(B_NO_COLOR_SPACE),
	fBounds(0, 0, -1, -1),
	fBytesPerRow(0),
	fWindow(NULL),
	fServerToken(-1),
	fAreaOffset(-1),
	fArea(-1),
	fServerArea(-1),
	fFlags(0),
	fInitError(B_NO_INIT)
{
	int32 flags;
	if (data->FindInt32("_bmflags", &flags) != B_OK) {
		// this bitmap is archived in some archaic format
		flags = 0;

		bool acceptsViews;
		if (data->FindBool("_view_ok", &acceptsViews) == B_OK && acceptsViews)
			flags |= B_BITMAP_ACCEPTS_VIEWS;

		bool contiguous;
		if (data->FindBool("_contiguous", &contiguous) == B_OK && contiguous)
			flags |= B_BITMAP_IS_CONTIGUOUS;
	}

	int32 rowBytes;
	if (data->FindInt32("_rowbytes", &rowBytes) != B_OK) {
		rowBytes = -1;
			// bytes per row are computed in InitObject(), then
	}

	BRect bounds;
	color_space cspace;
	if (data->FindRect("_frame", &bounds) == B_OK
		&& data->FindInt32("_cspace", (int32*)&cspace) == B_OK) {
		_InitObject(bounds, cspace, flags, rowBytes, B_MAIN_SCREEN_ID);
	}

	if (InitCheck() == B_OK) {
		ssize_t size;
		const void* buffer;
		if (data->FindData("_data", B_RAW_TYPE, &buffer, &size) == B_OK) {
			if (size == BitsLength()) {
				_AssertPointer();
				memcpy(fBasePointer, buffer, size);
			}
		}
	}

	if ((fFlags & B_BITMAP_ACCEPTS_VIEWS) != 0) {
		BMessage message;
		int32 i = 0;

		while (data->FindMessage("_views", i++, &message) == B_OK) {
			if (KView* view
					= dynamic_cast<KView*>(instantiate_object(&message)))
				AddChild(view);
		}
	}
}


/*!	\brief Instantiates a KBitmap from an archive.
	\param data The archive.
	\return A bitmap reconstructed from the archive or \c NULL, if an error
			occured.
*/
BArchivable*
KBitmap::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "KBitmap"))
		return new KBitmap(data);

	return NULL;
}


/*!	\brief Archives the KBitmap object.
	\param data The archive.
	\param deep \c true, if child object shall be archived as well, \c false
		   otherwise.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
status_t
KBitmap::Archive(BMessage* data, bool deep) const
{
	status_t ret = BArchivable::Archive(data, deep);

	if (ret == B_OK)
		ret = data->AddRect("_frame", fBounds);

	if (ret == B_OK)
		ret = data->AddInt32("_cspace", (int32)fColorSpace);

	if (ret == B_OK)
		ret = data->AddInt32("_bmflags", fFlags);

	if (ret == B_OK)
		ret = data->AddInt32("_rowbytes", fBytesPerRow);

	if (ret == B_OK && deep) {
		if ((fFlags & B_BITMAP_ACCEPTS_VIEWS) != 0) {
			BMessage views;
			for (int32 i = 0; i < CountChildren(); i++) {
				if (ChildAt(i)->Archive(&views, deep))
					ret = data->AddMessage("_views", &views);
				views.MakeEmpty();
				if (ret < B_OK)
					break;
			}
		}
	}
	// Note: R5 does not archive the data if B_BITMAP_IS_CONTIGUOUS is
	// true and it does save all formats as B_RAW_TYPE and it does save
	// the data even if B_BITMAP_ACCEPTS_VIEWS is set (as opposed to
	// the BeBook)
	if (ret == B_OK) {
		const_cast<KBitmap*>(this)->_AssertPointer();
		ret = data->AddData("_data", B_RAW_TYPE, fBasePointer, fSize);
	}
	return ret;
}


/*!	\brief Returns the result from the construction.
	\return \c B_OK, if the object is properly initialized, an error code
			otherwise.
*/
status_t
KBitmap::InitCheck() const
{
	return fInitError;
}


/*!	\brief Returns whether or not the KBitmap object is valid.
	\return \c true, if the object is properly initialized, \c false otherwise.
*/
bool
KBitmap::IsValid() const
{
	return InitCheck() == B_OK;
}


/*!	\brief Locks the bitmap bits so that they cannot be relocated.

	This is currently only used for overlay bitmaps - whenever you
	need to access their Bits(), you have to lock them first.
	On resolution change overlay bitmaps can be relocated in memory;
	using this call prevents you from accessing an invalid pointer
	and clobbering memory that doesn't belong you.
*/
status_t
KBitmap::LockBits(uint32* state)
{
	// TODO: how do we fill the "state"?
	//	It would be more or less useful to report what kind of bitmap
	//	we got (ie. overlay, placeholder, or non-overlay)
	if ((fFlags & B_BITMAP_WILL_OVERLAY) != 0) {
		overlay_client_data* data = (overlay_client_data*)fBasePointer;

		status_t status;
		do {
			status = acquire_sem(data->lock);
		} while (status == B_INTERRUPTED);

		if (data->buffer == NULL) {
			// the app_server does not grant us access to the frame buffer
			// right now - let's release the lock and fail
			release_sem_etc(data->lock, 1, B_DO_NOT_RESCHEDULE);
			return B_BUSY;
		}
		return status;
	}

	// NOTE: maybe this is used to prevent the app_server from
	// drawing the bitmap yet?
	// axeld: you mean for non overlays?

	return B_OK;
}


/*!	\brief Unlocks the bitmap's buffer again.
	Counterpart to LockBits(), see there for comments.
*/
void
KBitmap::UnlockBits()
{
	if ((fFlags & B_BITMAP_WILL_OVERLAY) == 0)
		return;

	overlay_client_data* data = (overlay_client_data*)fBasePointer;
	release_sem_etc(data->lock, 1, B_DO_NOT_RESCHEDULE);
}


/*! \brief Returns the ID of the area the bitmap data reside in.
	\return The ID of the area the bitmap data reside in.
*/
area_id
KBitmap::Area() const
{
	const_cast<KBitmap*>(this)->_AssertPointer();
	return fArea;
}


/*!	\brief Returns the pointer to the bitmap data.
	\return The pointer to the bitmap data.
*/
void*
KBitmap::Bits() const
{
	const_cast<KBitmap*>(this)->_AssertPointer();

	if ((fFlags & B_BITMAP_WILL_OVERLAY) != 0) {
		overlay_client_data* data = (overlay_client_data*)fBasePointer;
		return data->buffer;
	}

	return (void*)fBasePointer;
}


/*!	\brief Returns the size of the bitmap data.
	\return The size of the bitmap data.
*/
int32
KBitmap::BitsLength() const
{
	return fSize;
}


/*!	\brief Returns the number of bytes used to store a row of bitmap data.
	\return The number of bytes used to store a row of bitmap data.
*/
int32
KBitmap::BytesPerRow() const
{
	return fBytesPerRow;
}


/*!	\brief Returns the bitmap's color space.
	\return The bitmap's color space.
*/
color_space
KBitmap::ColorSpace() const
{
	return fColorSpace;
}


/*!	\brief Returns the bitmap's dimensions.
	\return The bitmap's dimensions.
*/
BRect
KBitmap::Bounds() const
{
	return fBounds;
}


/*!	\brief Returns the bitmap's creating flags.

	This method informs about which flags have been used to create the
	bitmap. It would for example tell you whether this is an overlay
	bitmap. If bitmap creation succeeded, all flags are fulfilled.

	\return The bitmap's creation flags.
*/
uint32
KBitmap::Flags() const
{
	return fFlags;
}


/*!	\brief Assigns data to the bitmap.

	Data are directly written into the bitmap's data buffer, being converted
	beforehand, if necessary. Some conversions work rather unintuitively:
	- \c B_RGB32: The source buffer is supposed to contain \c B_RGB24_BIG
	  data without padding at the end of the rows.
	- \c B_RGB32: The source buffer is supposed to contain \c B_CMAP8
	  data without padding at the end of the rows.
	- other color spaces: The source buffer is supposed to contain data
	  according to the specified color space being rowwise padded to int32.

	The currently supported source/target color spaces are
	\c B_RGB{32,24,16,15}[_BIG], \c B_CMAP8 and \c B_GRAY{8,1}.

	\note As this methods is apparently a bit strange to use, Haiku introduces
		  ImportBits() methods, which are recommended to be used instead.

	\param data The data to be copied.
	\param length The length in bytes of the data to be copied.
	\param offset The offset (in bytes) relative to beginning of the bitmap
		   data specifying the position at which the source data shall be
		   written.
	\param colorSpace Color space of the source data.
*/
void
KBitmap::SetBits(const void* data, int32 length, int32 offset,
	color_space colorSpace)
{
	status_t error = (InitCheck() == B_OK ? B_OK : B_NO_INIT);
	// check params
	if (error == B_OK && (data == NULL || offset > fSize || length < 0))
		error = B_BAD_VALUE;
	int32 width = 0;
	if (error == B_OK)
		width = fBounds.IntegerWidth() + 1;
	int32 inBPR = -1;
	// tweaks to mimic R5 behavior
	if (error == B_OK) {
		if (colorSpace == B_RGB32) {
			// B_RGB32 means actually unpadded B_RGB24_BIG
			colorSpace = B_RGB24_BIG;
			inBPR = width * 3;
		} else if (colorSpace == B_CMAP8 && fColorSpace != B_CMAP8) {
			// If in color space is B_CMAP8, but the bitmap's is another one,
			// ignore source data row padding.
			inBPR = width;
		}

		// call the sane method, which does the actual work
		error = ImportBits(data, length, inBPR, offset, colorSpace);
	}
}


/*!	\brief Assigns data to the bitmap.

	Data are directly written into the bitmap's data buffer, being converted
	beforehand, if necessary. Unlike for SetBits(), the meaning of
	\a colorSpace is exactly the expected one here, i.e. the source buffer
	is supposed to contain data of that color space. \a bpr specifies how
	many bytes the source contains per row. \c B_ANY_BYTES_PER_ROW can be
	supplied, if standard padding to int32 is used.

	The currently supported source/target color spaces are
	\c B_RGB{32,24,16,15}[_BIG], \c B_CMAP8 and \c B_GRAY{8,1}.

	\param data The data to be copied.
	\param length The length in bytes of the data to be copied.
	\param bpr The number of bytes per row in the source data.
	\param offset The offset (in bytes) relative to beginning of the bitmap
		   data specifying the position at which the source data shall be
		   written.
	\param colorSpace Color space of the source data.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a data, invalid \a bpr or \a offset, or
	  unsupported \a colorSpace.
*/
status_t
KBitmap::ImportBits(const void* data, int32 length, int32 bpr, int32 offset,
	color_space colorSpace)
{
	_AssertPointer();

	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (!data || offset > fSize || length < 0)
		return B_BAD_VALUE;

	int32 width = fBounds.IntegerWidth() + 1;
	if (bpr <= 0) {
		if (bpr == K_ANY_BYTES_PER_ROW)
			bpr = k_get_bytes_per_row(colorSpace, width);
		else
			return B_BAD_VALUE;
	}

	return BPrivate::ConvertBits(data, (uint8*)fBasePointer + offset, length,
		fSize - offset, bpr, fBytesPerRow, colorSpace, fColorSpace, width,
		fBounds.IntegerHeight() + 1);
}


/*!	\brief Assigns data to the bitmap.

	Allows for a BPoint offset in the source and in the bitmap. The region
	of the source at \a from extending \a width and \a height is assigned
	(and converted if necessary) to the bitmap at \a to.

	The currently supported source/target color spaces are
	\c B_RGB{32,24,16,15}[_BIG], \c B_CMAP8 and \c B_GRAY{8,1}.

	\param data The data to be copied.
	\param length The length in bytes of the data to be copied.
	\param bpr The number of bytes per row in the source data.
	\param colorSpace Color space of the source data.
	\param from The offset in the source where reading should begin.
	\param to The offset in the bitmap where the source should be written.
	\param width The width (in pixels) to be imported.
	\param height The height (in pixels) to be imported.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a data, invalid \a bpr, unsupported
	  \a colorSpace or invalid width/height.
*/
status_t
KBitmap::ImportBits(const void* data, int32 length, int32 bpr,
	color_space colorSpace, BPoint from, BPoint to, int32 width, int32 height)
{
	_AssertPointer();

	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (!data || length < 0 || width < 0 || height < 0)
		return B_BAD_VALUE;

	if (bpr <= 0) {
		if (bpr == K_ANY_BYTES_PER_ROW)
			bpr = k_get_bytes_per_row(colorSpace, fBounds.IntegerWidth() + 1);
		else
			return B_BAD_VALUE;
	}

	return BPrivate::ConvertBits(data, fBasePointer, length, fSize, bpr,
		fBytesPerRow, colorSpace, fColorSpace, from, to, width, height);
}


/*!	\briefly Assigns another bitmap's data to this bitmap.

	The supplied bitmap must have the exactly same dimensions as this bitmap.
	Its data is converted to the color space of this bitmap.

	The currently supported source/target color spaces are
	\c B_RGB{32,24,16,15}[_BIG], \c B_CMAP8 and \c B_GRAY{8,1}.

	\param bitmap The source bitmap.
	\return
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a bitmap, or \a bitmap has other dimensions,
	  or the conversion from or to one of the color spaces is not supported.
*/
status_t
KBitmap::ImportBits(const KBitmap* bitmap)
{
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (!bitmap || bitmap->InitCheck() != B_OK || bitmap->Bounds() != fBounds)
		return B_BAD_VALUE;

	return ImportBits(bitmap->Bits(), bitmap->BitsLength(),
		bitmap->BytesPerRow(), 0, bitmap->ColorSpace());
}


/*!	\brief Assigns data to the bitmap.

	Allows for a BPoint offset in the source and in the bitmap. The region
	of the source at \a from extending \a width and \a height is assigned
	(and converted if necessary) to the bitmap at \a to. The source bitmap is
	clipped to the bitmap and they don't need to have the same dimensions.

	The currently supported source/target color spaces are
	\c B_RGB{32,24,16,15}[_BIG], \c B_CMAP8 and \c B_GRAY{8,1}.

	\param bitmap The source bitmap.
	\param from The offset in the source where reading should begin.
	\param to The offset in the bitmap where the source should be written.
	\param width The width (in pixels) to be imported.
	\param height The height (in pixels) to be imported.
	- \c B_OK: Everything went fine.
	- \c B_BAD_VALUE: \c NULL \a bitmap, the conversion from or to one of
	  the color spaces is not supported, or invalid width/height.
*/
status_t
KBitmap::ImportBits(const KBitmap* bitmap, BPoint from, BPoint to, int32 width,
	int32 height)
{
	if (InitCheck() != B_OK)
		return B_NO_INIT;

	if (!bitmap || bitmap->InitCheck() != B_OK)
		return B_BAD_VALUE;

	return ImportBits(bitmap->Bits(), bitmap->BitsLength(),
		bitmap->BytesPerRow(), bitmap->ColorSpace(), from, to, width, height);
}


/*!	\brief Returns the overlay_restrictions structure for this bitmap
*/
status_t
KBitmap::GetOverlayRestrictions(overlay_restrictions* restrictions) const
{
	if ((fFlags & B_BITMAP_WILL_OVERLAY) == 0)
		return B_BAD_TYPE;

	BPrivate::AppServerLink link;

	link.StartMessage(AS_GET_BITMAP_OVERLAY_RESTRICTIONS);
	link.Attach<int32>(fServerToken);

	status_t status;
	if (link.FlushWithReply(status) < B_OK)
		return status;

	link.Read(restrictions, sizeof(overlay_restrictions));
	return B_OK;
}


/*!	\brief Adds a KView to the bitmap's view hierarchy.

	The bitmap must accept views and the supplied view must not be child of
	another parent.

	\param view The view to be added.
*/
void
KBitmap::AddChild(KView* view)
{
	if (fWindow != NULL)
		fWindow->AddChild(view);
}


/*!	\brief Removes a KView from the bitmap's view hierarchy.
	\param view The view to be removed.
*/
bool
KBitmap::RemoveChild(KView* view)
{
	return fWindow != NULL ? fWindow->RemoveChild(view) : false;
}


/*!	\brief Returns the number of BViews currently belonging to the bitmap.
	\return The number of BViews currently belonging to the bitmap.
*/
int32
KBitmap::CountChildren() const
{
	return fWindow != NULL ? fWindow->CountChildren() : 0;
}


/*!	\brief Returns the KView at a certain index in the bitmap's list of views.
	\param index The index of the KView to be returned.
	\return The KView at index \a index or \c NULL, if the index is out of
			range.
*/
KView*
KBitmap::ChildAt(int32 index) const
{
	return fWindow != NULL ? fWindow->ChildAt(index) : NULL;
}


/*!	\brief Returns a bitmap's KView with a certain name.
	\param name The name of the KView to be returned.
	\return The KView with the name \a name or \c NULL, if the bitmap doesn't
	know a view with that name.
*/
KView*
KBitmap::FindView(const char* viewName) const
{
	return fWindow != NULL ? fWindow->FindView(viewName) : NULL;
}


/*!	\brief Returns a bitmap's KView at a certain location.
	\param point The location.
	\return The KView with located at \a point or \c NULL, if the bitmap
	doesn't know a view at this location.
*/
KView*
KBitmap::FindView(BPoint point) const
{
	return fWindow != NULL ? fWindow->FindView(point) : NULL;
}


/*!	\brief Locks the off-screen window that belongs to the bitmap.

	The bitmap must accept views, if locking should work.

	\return \c true, if the lock was acquired successfully, \c false
			otherwise.
*/
bool
KBitmap::Lock()
{
	return fWindow != NULL ? fWindow->Lock() : false;
}


/*!	\brief Unlocks the off-screen window that belongs to the bitmap.

	The bitmap must accept views, if locking should work.
*/
void
KBitmap::Unlock()
{
	if (fWindow != NULL)
		fWindow->Unlock();
}


/*!	\brief Returns whether or not the bitmap's off-screen window is locked.

	The bitmap must accept views, if locking should work.

	\return \c true, if the caller owns a lock , \c false otherwise.
*/
bool
KBitmap::IsLocked() const
{
	return fWindow != NULL ? fWindow->IsLocked() : false;
}


KBitmap&
KBitmap::operator=(const KBitmap& source)
{
	_CleanUp();
	fInitError = B_NO_INIT;

	if (!source.IsValid())
		return *this;

	_InitObject(source.Bounds(), source.ColorSpace(), source.Flags(),
		source.BytesPerRow(), B_MAIN_SCREEN_ID);
	if (InitCheck() == B_OK)
		memcpy(Bits(), source.Bits(), min_c(BitsLength(), source.BitsLength()));

	return *this;
}


status_t
KBitmap::Perform(perform_code d, void* arg)
{
	return BArchivable::Perform(d, arg);
}

// FBC
void KBitmap::_ReservedBitmap1() {}
void KBitmap::_ReservedBitmap2() {}
void KBitmap::_ReservedBitmap3() {}


#if 0
// get_shared_pointer
/*!	\brief ???
*/
char*
KBitmap::get_shared_pointer() const
{
	return NULL;	// not implemented
}
#endif

int32
KBitmap::_ServerToken() const
{
	return fServerToken;
}


/*!	\brief Initializes the bitmap.
	\param bounds The bitmap dimensions.
	\param colorSpace The bitmap's color space.
	\param flags Creation flags.
	\param bytesPerRow The number of bytes per row the bitmap should use.
		   \c B_ANY_BYTES_PER_ROW to let the constructor choose an appropriate
		   value.
	\param screenID ???
*/
void
KBitmap::_InitObject(BRect bounds, color_space colorSpace, uint32 flags,
	int32 bytesPerRow, screen_id screenID)
{
//printf("KBitmap::InitObject(bounds: BRect(%.1f, %.1f, %.1f, %.1f), format: %ld, flags: %ld, bpr: %ld\n",
//	   bounds.left, bounds.top, bounds.right, bounds.bottom, colorSpace, flags, bytesPerRow);

	// TODO: Should we handle rounding of the "bounds" here? How does R5 behave?

debug_printf("[KBitmap]{_InitObject}\n");

	status_t error = B_OK;

#ifdef RUN_WITHOUT_APP_SERVER
	flags |= B_BITMAP_NO_SERVER_LINK;
#endif	// RUN_WITHOUT_APP_SERVER

	_CleanUp();

	// check params
	if (!bounds.IsValid() || !bitmaps_support_space(colorSpace, NULL)) {
	debug_printf("[KBitmap]{_InitObject}!bounds.IsValid() || !bitmaps_support_space(colorSpace, NULL)\n");
		error = B_BAD_VALUE;
	} else {
	debug_printf("[KBitmap]{_InitObject}else part\n");
		// bounds is in floats and might be valid but much larger than what we
		// can handle the size could not be expressed in int32
		double realSize = bounds.Width() * bounds.Height();
		if (realSize > (double)(INT_MAX / 4)) {
			fprintf(stderr, "bitmap bounds is much too large: "
				"BRect(%.1f, %.1f, %.1f, %.1f)\n",
				bounds.left, bounds.top, bounds.right, bounds.bottom);
			error = B_BAD_VALUE;
		}
	}
	if (error == B_OK) {
	debug_printf("[KBitmap]{_InitObject}error==B_OK\n");
		int32 bpr = k_get_bytes_per_row(colorSpace, bounds.IntegerWidth() + 1);
		if (bytesPerRow < 0)
			bytesPerRow = bpr;
		else if (bytesPerRow < bpr)
// NOTE: How does R5 behave?
			error = B_BAD_VALUE;
	}
	// allocate the bitmap buffer
	if (error == B_OK) {
	debug_printf("[KBitmap]{_InitObject}error==B_OK 2\n");
		// TODO: Let the app_server return the size when it allocated the bitmap
		int32 size = bytesPerRow * (bounds.IntegerHeight() + 1);

		if ((flags & B_BITMAP_NO_SERVER_LINK) != 0) {
		debug_printf("[KBitmap]{_InitObject}flags & B_BITMAP_NO_SERVER_LINK) != 0)\n");
			fBasePointer = (uint8*)malloc(size);
			if (fBasePointer) {
				fSize = size;
				fColorSpace = colorSpace;
				fBounds = bounds;
				fBytesPerRow = bytesPerRow;
				fFlags = flags;
			} else
				error = B_NO_MEMORY;
		} else {
				debug_printf("[KBitmap]{_InitObject}flags & B_BITMAP_NO_SERVER_LINK) != 0) else part\n");
			// Ask the server (via our owning application) to create a bitmap.
			BPrivate::AppServerLink link;

			// Attach Data:
			// 1) BRect bounds
			// 2) color_space space
			// 3) int32 bitmap_flags
			// 4) int32 bytes_per_row
			// 5) int32 screen_id::id
			link.StartMessage(AS_CREATE_BITMAP);
			link.Attach<BRect>(bounds);
			link.Attach<color_space>(colorSpace);
			link.Attach<uint32>(flags);
			link.Attach<int32>(bytesPerRow);
			link.Attach<int32>(screenID.id);

			if (link.FlushWithReply(error) == B_OK && error == B_OK) {
			debug_printf("[KBitmap]{_InitObject} link.FlushWithReply(error) == B_OK && error == B_OK\n");
				// server side success
				// Get token
				link.Read<int32>(&fServerToken);

				uint8 allocationFlags;
				link.Read<uint8>(&allocationFlags);
				link.Read<area_id>(&fServerArea);
				link.Read<int32>(&fAreaOffset);

				BPrivate::ServerMemoryAllocator* allocator
					= BApplication::Private::ServerAllocator();

				if ((allocationFlags & kNewAllocatorArea) != 0) {
					error = allocator->AddArea(fServerArea, fArea,
						fBasePointer, size);
				} else {
					error = allocator->AreaAndBaseFor(fServerArea, fArea,
						fBasePointer);
					if (error == B_OK)
						fBasePointer += fAreaOffset;
				}

				if ((allocationFlags & kFramebuffer) != 0) {
					// The base pointer will now point to an overlay_client_data
					// structure bytes per row might be modified to match
					// hardware constraints
					link.Read<int32>(&bytesPerRow);
					size = bytesPerRow * (bounds.IntegerHeight() + 1);
				}

				if (fServerArea >= B_OK) {
					fSize = size;
					fColorSpace = colorSpace;
					fBounds = bounds;
					fBytesPerRow = bytesPerRow;
					fFlags = flags;
				} else
					error = fServerArea;
			}

			if (error < B_OK) {
				fBasePointer = NULL;
				fServerToken = -1;
				fArea = -1;
				fServerArea = -1;
				fAreaOffset = -1;
				// NOTE: why not "0" in case of error?
				fFlags = flags;
			} else {
				BAutolock _(k_sBitmapListLock);
				k_sBitmapList.AddItem(this);
			}
		}
		fWindow = NULL;
	}

	fInitError = error;

	if (fInitError == B_OK) {
		// clear to white if the flags say so.
		if (flags & (B_BITMAP_CLEAR_TO_WHITE | B_BITMAP_ACCEPTS_VIEWS)) {
			if (fColorSpace == B_CMAP8) {
				// "255" is the "transparent magic" index for B_CMAP8 bitmaps
				// use the correct index for "white"
				memset(fBasePointer, 65, fSize);
			} else {
				// should work for most colorspaces
				memset(fBasePointer, 0xff, fSize);
			}
		}
		// TODO: Creating an offscreen window with a non32 bit bitmap
		// copies the current content of the bitmap to a back buffer.
		// So at this point the bitmap has to be already cleared to white.
		// Better move the above code to the server so the problem looks more
		// clear.
		if (flags & B_BITMAP_ACCEPTS_VIEWS) {
		debug_printf("[KBitmap]{_InitObject}B_BITMAP_ACCEPT_VIEWS\n");
			fWindow = new(std::nothrow) KWindow(Bounds(), fServerToken);
			if (fWindow) {
			debug_printf("[KBitmap]{_InitObject} fWindow\n");
				// A KWindow starts life locked and is unlocked
				// in Show(), but this window is never shown and
				// it's message loop is never started.
				fWindow->Unlock();
			} else
				fInitError = B_NO_MEMORY;
		}
	}

debug_printf("[KBitmap]{_InitObject}end\n");
}


/*!	\brief Cleans up any memory allocated by the bitmap and
		informs the server to do so as well (if needed).
*/
void
KBitmap::_CleanUp()
{
	if (fWindow != NULL) {
		if (fWindow->Lock())
			delete fWindow;
		fWindow = NULL;
			// this will leak fWindow if it couldn't be locked
	}

	if (fBasePointer == NULL)
		return;

	if ((fFlags & B_BITMAP_NO_SERVER_LINK) != 0) {
		free(fBasePointer);
	} else if (fServerToken != -1) {
		BPrivate::AppServerLink link;
		// AS_DELETE_BITMAP:
		// Attached Data:
		//	1) int32 server token
		link.StartMessage(AS_DELETE_BITMAP);
		link.Attach<int32>(fServerToken);
		link.Flush();

		// The server areas are deleted via kMsgDeleteServerMemoryArea message

		fArea = -1;
		fServerToken = -1;
		fAreaOffset = -1;

		BAutolock _(k_sBitmapListLock);
		k_sBitmapList.RemoveItem(this);
	}
	fBasePointer = NULL;
}


void
KBitmap::_AssertPointer()
{
	if (fBasePointer == NULL && fServerArea >= B_OK && fAreaOffset == -1) {
		// We lazily clone our own areas - if the bitmap is part of the usual
		// server memory area, or is a B_BITMAP_NO_SERVER_LINK bitmap, it
		// already has its data.
		fArea = clone_area("shared bitmap area", (void**)&fBasePointer,
			B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, fServerArea);
	}
}


void
KBitmap::_ReconnectToAppServer()
{
	BPrivate::AppServerLink link;

	link.StartMessage(AS_RECONNECT_BITMAP);
	link.Attach<BRect>(fBounds);
	link.Attach<color_space>(fColorSpace);
	link.Attach<uint32>(fFlags);
	link.Attach<int32>(fBytesPerRow);
	link.Attach<int32>(0);
	link.Attach<int32>(fArea);
	link.Attach<int32>(fAreaOffset);

	status_t error;
	if (link.FlushWithReply(error) == B_OK && error == B_OK) {
		// server side success
		// Get token
		link.Read<int32>(&fServerToken);

		link.Read<area_id>(&fServerArea);
	}
}
