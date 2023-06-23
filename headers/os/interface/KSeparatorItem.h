/*
 * Copyright 2001-2014 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_SEPARATOR_ITEM_H
#define _K_SEPARATOR_ITEM_H


#include <KMenuItem.h>

class BMessage;


class KSeparatorItem : public KMenuItem {
public:
								KSeparatorItem();
								KSeparatorItem(BMessage* data);
	virtual						~KSeparatorItem();

	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data,
									bool deep = true) const;

	virtual	void				SetEnabled(bool enable);

protected:
	virtual	void				GetContentSize(float* _width, float* _height);
	virtual	void				Draw();

private:
	// FBC padding, reserved and forbidden
	virtual	void				_ReservedSeparatorItem1();
	virtual	void				_ReservedSeparatorItem2();

			KSeparatorItem&		operator=(const KSeparatorItem& other);

			uint32				_reserved[1];
};


#endif // _SEPARATOR_ITEM_H
