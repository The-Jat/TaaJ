/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_STRING_ITEM_H
#define _K_STRING_ITEM_H


#include <KListItem.h>


class KStringItem : public KListItem {
public:
								KStringItem(const char* text,
									uint32 outlineLevel = 0,
									bool expanded = true);
								KStringItem(BMessage* archive);
	virtual						~KStringItem();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				DrawItem(KView* owner, BRect frame,
									bool complete = false);
	virtual	void				SetText(const char* text);
			const char*			Text() const;

	virtual	void				Update(KView* owner, const BFont* font);

	virtual	status_t			Perform(perform_code code, void* arg);

protected:
			float				BaselineOffset() const;

private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedStringItem1();
	virtual	void				_ReservedStringItem2();

								KStringItem(const KStringItem& item);
			KStringItem&		operator=(const KStringItem& item);

private:
			char*				fText;
			float				fBaselineOffset;
			uint32				_reserved[2];
};

#endif	// _STRING_ITEM_H
