/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__ABSTRACT_LAYOUT_H
#define	K__ABSTRACT_LAYOUT_H

#include <Alignment.h>
#include <KLayout.h>
#include <Size.h>

class KAbstractLayout : public KLayout {
public:
								KAbstractLayout();
								KAbstractLayout(BMessage* from);
	virtual						~KAbstractLayout();

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	BAlignment			Alignment();

	virtual	void				SetExplicitMinSize(BSize size);
	virtual	void				SetExplicitMaxSize(BSize size);
	virtual	void				SetExplicitPreferredSize(BSize size);
	virtual	void				SetExplicitAlignment(BAlignment alignment);

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual BRect				Frame();
	virtual	void				SetFrame(BRect frame);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual status_t			Archive(BMessage* into, bool deep = true) const;

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	// Archiving hook methods
	virtual	status_t			AllArchived(BMessage* archive) const;
	virtual	status_t			AllUnarchived(const BMessage* from);

	virtual status_t			ItemArchived(BMessage* into, KLayoutItem* item,
									int32 index) const;
	virtual	status_t			ItemUnarchived(const BMessage* from,
									KLayoutItem* item, int32 index);

	virtual	bool				ItemAdded(KLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(KLayoutItem* item, int32 fromIndex);
	virtual	void				LayoutInvalidated(bool children);
	virtual	void				OwnerChanged(KView* was);

	// KLayoutItem hook methods
	virtual	void				AttachedToLayout();
	virtual void				DetachedFromLayout(KLayout* layout);
	virtual	void				AncestorVisibilityChanged(bool shown);

private:
	virtual	void				_ReservedAbstractLayout1();
	virtual	void				_ReservedAbstractLayout2();
	virtual	void				_ReservedAbstractLayout3();
	virtual	void				_ReservedAbstractLayout4();
	virtual	void				_ReservedAbstractLayout5();
	virtual	void				_ReservedAbstractLayout6();
	virtual	void				_ReservedAbstractLayout7();
	virtual	void				_ReservedAbstractLayout8();
	virtual	void				_ReservedAbstractLayout9();
	virtual	void				_ReservedAbstractLayout10();

	// forbidden methods
								KAbstractLayout(const KAbstractLayout&);
			void				operator =(const KAbstractLayout&);

			struct	Proxy;
			struct	ViewProxy;
			struct	DataProxy;

			Proxy*				fExplicitData;
			uint32				_reserved[4];
};

#endif	//	_ABSTRACT_LAYOUT_ITEM_H
