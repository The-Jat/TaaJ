/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__CARD_LAYOUT_H
#define	K__CARD_LAYOUT_H

#include <KAbstractLayout.h>


class KCardLayout : public KAbstractLayout {
public:
								KCardLayout();
								KCardLayout(BMessage* from);
	virtual						~KCardLayout();

			KLayoutItem*		VisibleItem() const;
			int32				VisibleIndex() const;
			void				SetVisibleItem(int32 index);
			void				SetVisibleItem(KLayoutItem* item);

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	virtual	status_t			AllArchived(BMessage* archive) const;
	virtual status_t			AllUnarchived(const BMessage* from);

	virtual status_t			ItemArchived(BMessage* into, KLayoutItem* item,
									int32 index) const;
	virtual	status_t			ItemUnarchived(const BMessage* from,
									KLayoutItem* item, int32 index);

	virtual	void				LayoutInvalidated(bool children = false);
	virtual	void				DoLayout();
	virtual	bool				ItemAdded(KLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(KLayoutItem* item, int32 fromIndex);

private:

			void				_ValidateMinMax();

	// FBC padding
	virtual	void				_ReservedCardLayout1();
	virtual	void				_ReservedCardLayout2();
	virtual	void				_ReservedCardLayout3();
	virtual	void				_ReservedCardLayout4();
	virtual	void				_ReservedCardLayout5();
	virtual	void				_ReservedCardLayout6();
	virtual	void				_ReservedCardLayout7();
	virtual	void				_ReservedCardLayout8();
	virtual	void				_ReservedCardLayout9();
	virtual	void				_ReservedCardLayout10();

	// forbidden methods
								KCardLayout(const KCardLayout&);
			void				operator =(const KCardLayout&);

			BSize				fMin;
			BSize				fMax;
			BSize				fPreferred;
			KLayoutItem*		fVisibleItem;
			bool				fMinMaxValid;

			uint32				_reserved[5];
};

#endif	// _CARD_LAYOUT_H
