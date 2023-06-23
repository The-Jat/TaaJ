/*
 * Copyright 2006-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef K__ICON_UTILS_H
#define K__ICON_UTILS_H


#include <Mime.h>

class KBitmap;
class BNode;


class KIconUtils {
								KIconUtils();
								~KIconUtils();
								KIconUtils(const KIconUtils&);
			KIconUtils&			operator=(const KIconUtils&);

public:
	static	status_t			GetIcon(BNode* node,
									const char* vectorIconAttrName,
									const char* smallIconAttrName,
									const char* largeIconAttrName,
									icon_size size, KBitmap* result);

	static	status_t			GetVectorIcon(BNode* node,
									const char* attrName, KBitmap* result);

	static	status_t			GetVectorIcon(const uint8* buffer,
									size_t size, KBitmap* result);

//khidki start
/*static	status_t	K_GetVectorIcon(const uint8* buffer,
									size_t size, KBitmap* result);

*/
//end

	static	status_t			GetCMAP8Icon(BNode* node,
									const char* smallIconAttrName,
									const char* largeIconAttrName,
									icon_size size, KBitmap* icon);

	static	status_t			ConvertFromCMAP8(KBitmap* source,
									KBitmap* result);
	static	status_t			ConvertToCMAP8(KBitmap* source,
									KBitmap* result);

	static	status_t			ConvertFromCMAP8(const uint8* data,
									uint32 width, uint32 height,
									uint32 bytesPerRow, KBitmap* result);

	static	status_t			ConvertToCMAP8(const uint8* data,
									uint32 width, uint32 height,
									uint32 bytesPerRow, KBitmap* result);
};

#endif	// _ICON_UTILS_H
