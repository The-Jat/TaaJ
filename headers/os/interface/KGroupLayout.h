/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__GROUP_LAYOUT_H
#define	K__GROUP_LAYOUT_H

#include <KTwoDimensionalLayout.h>

class KGroupLayout : public KTwoDimensionalLayout {
public:
								KGroupLayout(orientation orientation,
									float spacing = B_USE_DEFAULT_SPACING);
								KGroupLayout(BMessage* from);
	virtual						~KGroupLayout();

			float				Spacing() const;
			void				SetSpacing(float spacing);

			orientation			Orientation() const;
			void				SetOrientation(orientation orientation);

			float				ItemWeight(int32 index) const;
			void				SetItemWeight(int32 index, float weight);

	virtual	KLayoutItem*		AddView(KView* child);
	virtual	KLayoutItem*		AddView(int32 index, KView* child);
	virtual	KLayoutItem*		AddView(KView* child, float weight);
	virtual	KLayoutItem*		AddView(int32 index, KView* child,
									float weight);

	virtual	bool				AddItem(KLayoutItem* item);
	virtual	bool				AddItem(int32 index, KLayoutItem* item);
	virtual	bool				AddItem(KLayoutItem* item, float weight);
	virtual	bool				AddItem(int32 index, KLayoutItem* item,
									float weight);

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	virtual status_t			AllArchived(BMessage* into) const;
	virtual	status_t			AllUnarchived(const BMessage* from);
	virtual status_t			ItemArchived(BMessage* into, KLayoutItem* item,
									int32 index) const;
	virtual	status_t			ItemUnarchived(const BMessage* from,
									KLayoutItem* item, int32 index);

	virtual	bool				ItemAdded(KLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(KLayoutItem* item, int32 fromIndex);

	virtual	void				PrepareItems(orientation orientation);

	virtual	int32				InternalCountColumns();
	virtual	int32				InternalCountRows();
	virtual	void				GetColumnRowConstraints(
									orientation orientation,
									int32 index,
									ColumnRowConstraints* constraints);
	virtual	void				GetItemDimensions(KLayoutItem* item,
									Dimensions* dimensions);

private:

	// FBC padding
	virtual	void				_ReservedGroupLayout1();
	virtual	void				_ReservedGroupLayout2();
	virtual	void				_ReservedGroupLayout3();
	virtual	void				_ReservedGroupLayout4();
	virtual	void				_ReservedGroupLayout5();
	virtual	void				_ReservedGroupLayout6();
	virtual	void				_ReservedGroupLayout7();
	virtual	void				_ReservedGroupLayout8();
	virtual	void				_ReservedGroupLayout9();
	virtual	void				_ReservedGroupLayout10();

	// forbidden methods
								KGroupLayout(const KGroupLayout&);
			void				operator =(const KGroupLayout&);

			struct ItemLayoutData;

			ItemLayoutData*		_LayoutDataForItem(KLayoutItem* item) const;

			orientation			fOrientation;
			BList				fVisibleItems;

			uint32				_reserved[5];
};

#endif	// _GROUP_LAYOUT_H
