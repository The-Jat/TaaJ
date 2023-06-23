/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _K_VIEW_PORT_H
#define _K_VIEW_PORT_H


#include <Laminate.h>


namespace BPrivate {


class KViewPort : public KView {
public:
								KViewPort(KView* child = NULL);
								KViewPort(KLayoutItem* child);
								KViewPort(const char* name,
									KView* child = NULL);
								KViewPort(const char* name,
									KLayoutItem* child);
	virtual						~KViewPort();

			KView*				ChildView() const;
			void				SetChildView(KView* child);

			KLayoutItem*		ChildItem() const;
			void				SetChildItem(KLayoutItem* child);

private:
			class ViewPortLayout;
			friend class ViewPortLayout;

private:
			void				_Init();

private:
			ViewPortLayout*		fLayout;
			KLayoutItem*		fChild;
};


}	// namespace BPrivate


using ::BPrivate::KViewPort;


#endif	// _VIEW_PORT_H
