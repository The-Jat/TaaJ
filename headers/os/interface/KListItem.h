/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_LIST_ITEM_H
#define _K_LIST_ITEM_H


#include <Archivable.h>
#include <Rect.h>


class BFont;
class BList;
class BMessage;
class KOutlineListView;
class KView;


class KListItem : public BArchivable {
public:
								KListItem(uint32 outlineLevel = 0,
									bool expanded = true);
								KListItem(BMessage* archive);
	virtual						~KListItem();

	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

			float				Height() const;
			float				Width() const;
			bool				IsSelected() const;
			void				Select();
			void				Deselect();

	virtual	void				SetEnabled(bool enabled);
			bool				IsEnabled() const;

			void				SetHeight(float height);
			void				SetWidth(float width);
	virtual	void				DrawItem(KView* owner, BRect frame,
									bool complete = false) = 0;
	virtual	void				Update(KView* owner, const BFont* font);

			bool				IsExpanded() const;
			void				SetExpanded(bool expanded);
			uint32				OutlineLevel() const;
			void				SetOutlineLevel(uint32 level);

	virtual	status_t			Perform(perform_code code, void* arg);

private:
	// FBC padding
	virtual	void				_ReservedListItem1();
	virtual	void				_ReservedListItem2();

	// Forbidden
								KListItem(const KListItem& item);
			KListItem&			operator=(const KListItem& item);

private:
	friend class KOutlineListView;
	friend class KListView;

			bool				HasSubitems() const;

			bool				IsItemVisible() const;
			void				SetItemVisible(bool visible);
	inline	float				Top() const;
	inline	float				Bottom() const;
			void				SetTop(float top);

private:
			float				fTop;
			BList*				fTemporaryList;
			float				fWidth;
			float				fHeight;
			uint32				fLevel;
			bool				fSelected;
			bool				fEnabled;
			bool				fExpanded;
			bool				fHasSubitems : 1;
			bool				fVisible : 1;
};


inline float
KListItem::Top(void) const
{
	return fTop;
}


inline float
KListItem::Bottom(void) const
{
	return fTop + ceilf(fHeight) - 1.0;
}


#include <StringItem.h>
	// to maintain source compatibility


#endif // _LIST_ITEM_H
