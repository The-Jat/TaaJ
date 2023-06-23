/*
 * Copyright 2006-2010, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_K_VIEW_LAYOUT_ITEM_H
#define	_K_VIEW_LAYOUT_ITEM_H

#include <KLayoutItem.h>


class KViewLayoutItem : public KLayoutItem {
public:
								KViewLayoutItem(KView* view);
								KViewLayoutItem(BMessage* from);
	virtual						~KViewLayoutItem();

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	BAlignment			Alignment();

	virtual	void				SetExplicitMinSize(BSize size);
	virtual	void				SetExplicitMaxSize(BSize size);
	virtual	void				SetExplicitPreferredSize(BSize size);
	virtual	void				SetExplicitAlignment(BAlignment alignment);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

	virtual	bool				HasHeightForWidth();
	virtual	void				GetHeightForWidth(float width, float* min,
									float* max, float* preferred);

	virtual	KView*				View();

	virtual	void				Relayout(bool immediate = false);

	virtual	status_t			Archive(BMessage* into, bool deep = true) const;
	virtual status_t			AllArchived(BMessage* into) const;
	virtual status_t			AllUnarchived(const BMessage* from);
	static	BArchivable*		Instantiate(BMessage* from);

protected:
	virtual	void				LayoutInvalidated(bool children);
	virtual void				AncestorVisibilityChanged(bool shown);

private:
			KView*				fView;
			int32				fAncestorsVisible;
};

#endif	//	_VIEW_LAYOUT_ITEM_H
