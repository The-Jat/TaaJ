/*
 * Copyright 2006, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef K__BITMAP_PRIVATE_H
#define K__BITMAP_PRIVATE_H


#include <KBitmap.h>
#include <OS.h>


// This structure is placed in the client/server shared memory area.

struct overlay_client_data {
	sem_id	lock;
	uint8*	buffer;
};


void k_reconnect_bitmaps_to_app_server();


class KBitmap::Private {
public:
								Private(KBitmap* bitmap);
			void				ReconnectToAppServer();
private:
			KBitmap*			fBitmap;
};

#endif // _BITMAP_PRIVATE_H
