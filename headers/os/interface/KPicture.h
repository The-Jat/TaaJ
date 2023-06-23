/*
 * Copyright 2001-2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_PICTURE_H
#define	_PICTURE_H


#include <InterfaceDefs.h>
#include <Rect.h>
#include <Archivable.h>


class BDataIO;
class KView;
//class KView;//khidki
struct _KPictureExtent_;


class KPicture : public BArchivable {
public:
								KPicture();
								KPicture(const KPicture& other);
								KPicture(BMessage* data);
	virtual						~KPicture();

	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;
	virtual	status_t			Perform(perform_code code, void* arg);

			status_t			Play(void** callBackTable,
									int32 tableEntries,
									void* userData);

			status_t			Flatten(BDataIO* stream);
			status_t			Unflatten(BDataIO* stream);

	class Private;
private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedPicture1();
	virtual	void				_ReservedPicture2();
	virtual	void				_ReservedPicture3();

			KPicture&			operator=(const KPicture&);

private:
	friend class BWindow;
	friend class KView;
		//friend class KView;//khidki
	friend class BPrintJob;
	friend class Private;

			void				_InitData();
			void				_DisposeData();

			void				_ImportOldData(const void* data, int32 size);

			void				SetToken(int32 token);
			int32				Token() const;

			bool				_AssertLocalCopy();
			bool				_AssertOldLocalCopy();
			bool				_AssertServerCopy();

			status_t			_Upload();
			status_t			_Download();

	// Deprecated API
								KPicture(const void* data, int32 size);
			const void*			Data() const;
			int32				DataSize() const;

			void				Usurp(KPicture* lameDuck);
			KPicture*			StepDown();

private:
			int32				fToken;
			_KPictureExtent_*	fExtent;
			KPicture*			fUsurped;

			uint32				_reserved[3];
};

#endif // _PICTURE_H

