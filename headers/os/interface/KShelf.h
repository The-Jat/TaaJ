/*
 * Copyright 2001-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef K___SHELF_H
#define K___SHELF_H


#include <KDragger.h>
#include <Handler.h>
#include <List.h>
#include <Locker.h>


class BDataIO;
class BPoint;
class KView;
class BEntry;
class _KZombieReplicantView_;
struct entry_ref;

namespace BPrivate {
	struct k_replicant_data;
	class KShelfContainerViewFilter;
};


class KShelf : public BHandler {
public:
								KShelf(KView* view, bool allowDrags = true,
									const char* shelfType = NULL);
								KShelf(const entry_ref* ref, KView* view,
									bool allowDrags = true,
									const char* shelfType = NULL);
								KShelf(BDataIO* stream, KView* view,
									bool allowDrags = true,
									const char* shelfType = NULL);
								KShelf(BMessage* archive);
	virtual						~KShelf();

	static BArchivable*			Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				MessageReceived(BMessage* message);
			status_t			Save();
	virtual	void				SetDirty(bool state);
			bool				IsDirty() const;

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual	status_t			Perform(perform_code code, void* data);

			bool				AllowsDragging() const;
			void				SetAllowsDragging(bool state);
			bool				AllowsZombies() const;
			void				SetAllowsZombies(bool state);
			bool				DisplaysZombies() const;
			void				SetDisplaysZombies(bool state);
			bool				IsTypeEnforced() const;
			void				SetTypeEnforced(bool state);

			status_t			SetSaveLocation(BDataIO* stream);
			status_t			SetSaveLocation(const entry_ref* ref);
			BDataIO*			SaveLocation(entry_ref* ref) const;

			status_t			AddReplicant(BMessage* archive,
									BPoint location);
			status_t			DeleteReplicant(KView* replicant);
			status_t			DeleteReplicant(BMessage* archive);
			status_t			DeleteReplicant(int32 index);
			int32				CountReplicants() const;
			BMessage*			ReplicantAt(int32 index, KView** view = NULL,
									uint32* uid = NULL,
									status_t* perr = NULL) const;
			int32				IndexOf(const KView* replicantView) const;
			int32				IndexOf(const BMessage* archive) const;
			int32				IndexOf(uint32 id) const;

protected:
	virtual	bool				CanAcceptReplicantMessage(
									BMessage* archive) const;
	virtual	bool				CanAcceptReplicantView(BRect,
									KView*, BMessage*) const;
	virtual	BPoint				AdjustReplicantBy(BRect, BMessage*) const;

	virtual	void				ReplicantDeleted(int32 index,
									const BMessage* archive,
									const KView *replicant);

private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedShelf2();
	virtual	void				_ReservedShelf3();
	virtual	void				_ReservedShelf4();
	virtual	void				_ReservedShelf5();
	virtual	void				_ReservedShelf6();
	virtual	void				_ReservedShelf7();
	virtual	void				_ReservedShelf8();

								KShelf(const KShelf& other);
			KShelf&				operator=(const KShelf& other);

private:
	friend class BPrivate::KShelfContainerViewFilter;

			status_t			_Archive(BMessage* data) const;
			void				_InitData(BEntry* entry, BDataIO* stream,
									KView* view, bool allowDrags);
			status_t			_DeleteReplicant(
									BPrivate::k_replicant_data* replicant);
			status_t			_AddReplicant(BMessage* data,
									BPoint* location, uint32 uniqueID);
			KView*				_GetReplicant(BMessage* data, KView* view,
									const BPoint& point, KDragger*& dragger,
									KDragger::relation& relation);
			_KZombieReplicantView_* _CreateZombie(BMessage *data,
									KDragger *&dragger);

			status_t			_GetProperty(BMessage* message,
									BMessage* reply);
	static	void				_GetReplicantData(BMessage* message,
									KView* view, KView*& replicant,
									KDragger*& dragger,
									KDragger::relation& relation);
	static	BArchivable*		_InstantiateObject(BMessage* archive,
									image_id* image);

private:
			KView*				fContainerView;
			BDataIO*			fStream;
			BEntry*				fEntry;
			BList				fReplicants;
			BPrivate::KShelfContainerViewFilter* fFilter;
			uint32				fGenCount;
			bool				fAllowDragging;
			bool				fDirty;
			bool				fDisplayZombies;
			bool				fAllowZombies;
			bool				fTypeEnforced;

			uint32				_reserved[8];
};

#endif	/* _SHELF_H */
