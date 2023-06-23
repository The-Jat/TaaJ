/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_PICTURE_PRIVATE_H
#define _K_PICTURE_PRIVATE_H


#include <Picture.h>
#include <OS.h>


void k_reconnect_pictures_to_app_server();


class KPicture::Private {
public:
								Private(KPicture* picture);
			void				ReconnectToAppServer();
private:
			KPicture*			fPicture;
};


#endif // _PICTURE_PRIVATE_H
