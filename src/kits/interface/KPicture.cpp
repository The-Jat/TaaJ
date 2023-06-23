/*
 * Copyright 2001-2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers, mflerackers@androme.be
 */


// Records a series of drawing instructions that can be "replayed" later.


#include <KPicture.h>

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG 1
#include <ByteOrder.h>
#include <Debug.h>
#include <List.h>
#include <Message.h>

#include <AppServerLink.h>
#include <Autolock.h>
#include <ObjectList.h>
#include <PicturePlayer.h>
#include <ServerProtocol.h>

#include "KPicturePrivate.h"


static BObjectList<KPicture> k_sPictureList;
static BLocker k_sPictureListLock;


void
k_reconnect_pictures_to_app_server()
{
	BAutolock _(k_sPictureListLock);
	for (int32 i = 0; i < k_sPictureList.CountItems(); i++) {
		KPicture::Private picture(k_sPictureList.ItemAt(i));
		picture.ReconnectToAppServer();
	}
}


KPicture::Private::Private(KPicture* picture)
	:
	fPicture(picture)
{
}


void
KPicture::Private::ReconnectToAppServer()
{
	fPicture->_Upload();
}


struct _KPictureExtent_ {
							_KPictureExtent_(int32 size = 0);
							~_KPictureExtent_();

			const void*		Data() const { return fNewData; }
			status_t		ImportData(const void* data, int32 size);

			status_t		Flatten(BDataIO* stream);
			status_t		Unflatten(BDataIO* stream);

			int32			Size() const { return fNewSize; }
			status_t		SetSize(int32 size);

			bool			AddPicture(KPicture* picture)
								{ return fPictures.AddItem(picture); }
			void			DeletePicture(int32 index)
								{ delete static_cast<KPicture*>
									(fPictures.RemoveItem(index)); }

			BList*			Pictures() { return &fPictures; }
			KPicture*		PictureAt(int32 index)
								{ return static_cast<KPicture*>
									(fPictures.ItemAt(index)); }

			int32			CountPictures() const
								{ return fPictures.CountItems(); }

private:
			void*	fNewData;
			int32	fNewSize;

			BList	fPictures;
				// In R5 this is a BArray<KPicture*>
				// which is completely inline.
};


struct picture_header {
	int32 magic1; // version ?
	int32 magic2; // endianess ?
};


KPicture::KPicture()
	:
	fToken(-1),
	fExtent(NULL),
	fUsurped(NULL)
{
	_InitData();
}


KPicture::KPicture(const KPicture& otherPicture)
	:
	fToken(-1),
	fExtent(NULL),
	fUsurped(NULL)
{
	_InitData();

	if (otherPicture.fToken != -1) {
		BPrivate::AppServerLink link;
		link.StartMessage(AS_CLONE_PICTURE);
		link.Attach<int32>(otherPicture.fToken);

		status_t status = B_ERROR;
		if (link.FlushWithReply(status) == B_OK && status == B_OK)
			link.Read<int32>(&fToken);

		if (status < B_OK)
			return;
	}

	if (otherPicture.fExtent->Size() > 0) {
		fExtent->ImportData(otherPicture.fExtent->Data(),
			otherPicture.fExtent->Size());

		for (int32 i = 0; i < otherPicture.fExtent->CountPictures(); i++) {
			KPicture* picture
				= new KPicture(*otherPicture.fExtent->PictureAt(i));
			fExtent->AddPicture(picture);
		}
	}
}


KPicture::KPicture(BMessage* data)
	:
	fToken(-1),
	fExtent(NULL),
	fUsurped(NULL)
{
	_InitData();

	int32 version;
	if (data->FindInt32("_ver", &version) != B_OK)
		version = 0;

	int8 endian;
	if (data->FindInt8("_endian", &endian) != B_OK)
		endian = 0;

	const void* pictureData;
	ssize_t size;
	if (data->FindData("_data", B_RAW_TYPE, &pictureData, &size) != B_OK)
		return;

	// Load sub pictures
	BMessage pictureMessage;
	int32 i = 0;
	while (data->FindMessage("piclib", i++, &pictureMessage) == B_OK) {
		KPicture* picture = new KPicture(&pictureMessage);
		fExtent->AddPicture(picture);
	}

	if (version == 0) {
		// TODO: For now. We'll see if it's worth to support old style data
		debugger("old style KPicture data is not supported");
	} else if (version == 1) {
		fExtent->ImportData(pictureData, size);

//		swap_data(fExtent->fNewData, fExtent->fNewSize);

		if (fExtent->Size() > 0)
			_AssertServerCopy();
	}

	// Do we just free the data now?
	if (fExtent->Size() > 0)
		fExtent->SetSize(0);

	// What with the sub pictures?
	for (i = fExtent->CountPictures() - 1; i >= 0; i--)
		fExtent->DeletePicture(i);
}


KPicture::KPicture(const void* data, int32 size)
{
	_InitData();
	// TODO: For now. We'll see if it's worth to support old style data
	debugger("old style KPicture data is not supported");
}


void
KPicture::_InitData()
{
	fToken = -1;
	fUsurped = NULL;

	fExtent = new (std::nothrow) _KPictureExtent_;

	BAutolock _(k_sPictureListLock);
	k_sPictureList.AddItem(this);
}


KPicture::~KPicture()
{
	BAutolock _(k_sPictureListLock);
	k_sPictureList.RemoveItem(this, false);
	_DisposeData();
}


void
KPicture::_DisposeData()
{
	if (fToken != -1) {
		BPrivate::AppServerLink link;

		link.StartMessage(AS_DELETE_PICTURE);
		link.Attach<int32>(fToken);
		link.Flush();
		SetToken(-1);
	}

	delete fExtent;
	fExtent = NULL;
}


BArchivable*
KPicture::Instantiate(BMessage* data)
{
	if (validate_instantiation(data, "KPicture"))
		return new KPicture(data);

	return NULL;
}


status_t
KPicture::Archive(BMessage* data, bool deep) const
{
	if (!const_cast<KPicture*>(this)->_AssertLocalCopy())
		return B_ERROR;

	status_t err = BArchivable::Archive(data, deep);
	if (err != B_OK)
		return err;

	err = data->AddInt32("_ver", 1);
	if (err != B_OK)
		return err;

	err = data->AddInt8("_endian", B_HOST_IS_BENDIAN);
	if (err != B_OK)
		return err;

	err = data->AddData("_data", B_RAW_TYPE, fExtent->Data(), fExtent->Size());
	if (err != B_OK)
		return err;

	for (int32 i = 0; i < fExtent->CountPictures(); i++) {
		BMessage pictureMessage;

		err = fExtent->PictureAt(i)->Archive(&pictureMessage, deep);
		if (err != B_OK)
			break;

		err = data->AddMessage("piclib", &pictureMessage);
		if (err != B_OK)
			break;
	}

	return err;
}


status_t
KPicture::Perform(perform_code code, void* arg)
{
	return BArchivable::Perform(code, arg);
}


status_t
KPicture::Play(void** callBackTable, int32 tableEntries, void* user)
{
	if (!_AssertLocalCopy())
		return B_ERROR;

	BPrivate::PicturePlayer player(fExtent->Data(), fExtent->Size(),
		fExtent->Pictures());

	return player.Play(callBackTable, tableEntries, user);
}


status_t
KPicture::Flatten(BDataIO* stream)
{
	// TODO: what about endianess?

	if (!_AssertLocalCopy())
		return B_ERROR;

	const picture_header header = { 2, 0 };
	ssize_t bytesWritten = stream->Write(&header, sizeof(header));
	if (bytesWritten < B_OK)
		return bytesWritten;

	if (bytesWritten != (ssize_t)sizeof(header))
		return B_IO_ERROR;

	return fExtent->Flatten(stream);
}


status_t
KPicture::Unflatten(BDataIO* stream)
{
	// TODO: clear current picture data?

	picture_header header;
	ssize_t bytesRead = stream->Read(&header, sizeof(header));
	if (bytesRead < B_OK)
		return bytesRead;

	if (bytesRead != (ssize_t)sizeof(header)
		|| header.magic1 != 2 || header.magic2 != 0)
		return B_BAD_TYPE;

	status_t status = fExtent->Unflatten(stream);
	if (status < B_OK)
		return status;

//	swap_data(fExtent->fNewData, fExtent->fNewSize);

	if (!_AssertServerCopy())
		return B_ERROR;

	// Data is now kept server side, remove the local copy
	if (fExtent->Data() != NULL)
		fExtent->SetSize(0);

	return status;
}


void
KPicture::_ImportOldData(const void* data, int32 size)
{
	// TODO: We don't support old data for now
}


void
KPicture::SetToken(int32 token)
{
	fToken = token;
}


int32
KPicture::Token() const
{
	return fToken;
}


bool
KPicture::_AssertLocalCopy()
{
	if (fExtent->Data() != NULL)
		return true;

	if (fToken == -1)
		return false;

	return _Download() == B_OK;
}


bool
KPicture::_AssertOldLocalCopy()
{
	// TODO: We don't support old data for now

	return false;
}


bool
KPicture::_AssertServerCopy()
{
	if (fToken != -1)
		return true;

	if (fExtent->Data() == NULL)
		return false;

	for (int32 i = 0; i < fExtent->CountPictures(); i++) {
		if (!fExtent->PictureAt(i)->_AssertServerCopy())
			return false;
	}

	return _Upload() == B_OK;
}


status_t
KPicture::_Upload()
{
	if (fExtent == NULL || fExtent->Data() == NULL)
		return B_BAD_VALUE;

	BPrivate::AppServerLink link;

	link.StartMessage(AS_CREATE_PICTURE);
	link.Attach<int32>(fExtent->CountPictures());

	for (int32 i = 0; i < fExtent->CountPictures(); i++) {
		KPicture* picture = fExtent->PictureAt(i);
		if (picture != NULL)
			link.Attach<int32>(picture->fToken);
		else
			link.Attach<int32>(-1);
	}
	link.Attach<int32>(fExtent->Size());
	link.Attach(fExtent->Data(), fExtent->Size());

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK
		&& status == B_OK) {
		link.Read<int32>(&fToken);
	}

	return status;
}


status_t
KPicture::_Download()
{
	ASSERT(fExtent->Data() == NULL);
	ASSERT(fToken != -1);

	BPrivate::AppServerLink link;

	link.StartMessage(AS_DOWNLOAD_PICTURE);
	link.Attach<int32>(fToken);

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		int32 count = 0;
		link.Read<int32>(&count);

		// Read sub picture tokens
		for (int32 i = 0; i < count; i++) {
			KPicture* picture = new KPicture;
			link.Read<int32>(&picture->fToken);
			fExtent->AddPicture(picture);
		}

		int32 size;
		link.Read<int32>(&size);
		status = fExtent->SetSize(size);
		if (status == B_OK)
			link.Read(const_cast<void*>(fExtent->Data()), size);
	}

	return status;
}


const void*
KPicture::Data() const
{
	if (fExtent->Data() == NULL)
		const_cast<KPicture*>(this)->_AssertLocalCopy();

	return fExtent->Data();
}


int32
KPicture::DataSize() const
{
	if (fExtent->Data() == NULL)
		const_cast<KPicture*>(this)->_AssertLocalCopy();

	return fExtent->Size();
}


void
KPicture::Usurp(KPicture* lameDuck)
{
	_DisposeData();

	// Reinitializes the KPicture
	_InitData();

	// Do the Usurping
	fUsurped = lameDuck;
}


KPicture*
KPicture::StepDown()
{
	KPicture* lameDuck = fUsurped;
	fUsurped = NULL;

	return lameDuck;
}


void KPicture::_ReservedPicture1() {}
void KPicture::_ReservedPicture2() {}
void KPicture::_ReservedPicture3() {}


KPicture&
KPicture::operator=(const KPicture&)
{
	return* this;
}


// _KPictureExtent_
_KPictureExtent_::_KPictureExtent_(int32 size)
	:
	fNewData(NULL),
	fNewSize(0)
{
	SetSize(size);
}


_KPictureExtent_::~_KPictureExtent_()
{
	free(fNewData);
	for (int32 i = 0; i < fPictures.CountItems(); i++)
		delete static_cast<KPicture*>(fPictures.ItemAtFast(i));
}


status_t
_KPictureExtent_::ImportData(const void* data, int32 size)
{
	if (data == NULL)
		return B_BAD_VALUE;

	status_t status = B_OK;
	if (Size() != size)
		status = SetSize(size);

	if (status == B_OK)
		memcpy(fNewData, data, size);

	return status;
}


status_t
_KPictureExtent_::Unflatten(BDataIO* stream)
{
	if (stream == NULL)
		return B_BAD_VALUE;

	int32 count = 0;
	ssize_t bytesRead = stream->Read(&count, sizeof(count));
	if (bytesRead < B_OK)
		return bytesRead;
	if (bytesRead != (ssize_t)sizeof(count))
		return B_BAD_DATA;

	for (int32 i = 0; i < count; i++) {
		KPicture* picture = new KPicture;
		status_t status = picture->Unflatten(stream);
		if (status < B_OK) {
			delete picture;
			return status;
		}

		AddPicture(picture);
	}

	int32 size;
	bytesRead = stream->Read(&size, sizeof(size));
	if (bytesRead < B_OK)
		return bytesRead;

	if (bytesRead != (ssize_t)sizeof(size))
		return B_IO_ERROR;

	status_t status = B_OK;
	if (Size() != size)
		status = SetSize(size);

	if (status < B_OK)
		return status;

	bytesRead = stream->Read(fNewData, size);
	if (bytesRead < B_OK)
		return bytesRead;

	if (bytesRead != (ssize_t)size)
		return B_IO_ERROR;

	return B_OK;
}


status_t
_KPictureExtent_::Flatten(BDataIO* stream)
{
	int32 count = fPictures.CountItems();
	ssize_t bytesWritten = stream->Write(&count, sizeof(count));
	if (bytesWritten < B_OK)
		return bytesWritten;

	if (bytesWritten != (ssize_t)sizeof(count))
		return B_IO_ERROR;

	for (int32 i = 0; i < count; i++) {
		status_t status = PictureAt(i)->Flatten(stream);
		if (status < B_OK)
			return status;
	}

	bytesWritten = stream->Write(&fNewSize, sizeof(fNewSize));
	if (bytesWritten < B_OK)
		return bytesWritten;

	if (bytesWritten != (ssize_t)sizeof(fNewSize))
		return B_IO_ERROR;

	bytesWritten = stream->Write(fNewData, fNewSize);
	if (bytesWritten < B_OK)
		return bytesWritten;

	if (bytesWritten != fNewSize)
		return B_IO_ERROR;

	return B_OK;
}


status_t
_KPictureExtent_::SetSize(int32 size)
{
	if (size < 0)
		return B_BAD_VALUE;

	if (size == fNewSize)
		return B_OK;

	if (size == 0) {
		free(fNewData);
		fNewData = NULL;
	} else {
		void* data = realloc(fNewData, size);
		if (data == NULL)
			return B_NO_MEMORY;

		fNewData = data;
	}

	fNewSize = size;
	return B_OK;
}
