/*
 * Copyright 2006, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_K_LAYOUT_CONTEXT_H
#define	_K_LAYOUT_CONTEXT_H

#include <List.h>

class KLayoutContext;


class KLayoutContextListener {
public:
								KLayoutContextListener();
	virtual						~KLayoutContextListener();

	virtual	void				LayoutContextLeft(KLayoutContext* context) = 0;

private:
	virtual	void				_ReservedLayoutContextListener1();
	virtual	void				_ReservedLayoutContextListener2();
	virtual	void				_ReservedLayoutContextListener3();
	virtual	void				_ReservedLayoutContextListener4();
	virtual	void				_ReservedLayoutContextListener5();

			uint32				_reserved[3];
};


class KLayoutContext {
public:
								KLayoutContext();
								~KLayoutContext();

			void				AddListener(KLayoutContextListener* listener);
			void				RemoveListener(
									KLayoutContextListener* listener);

private:
			BList				fListeners;
			uint32				_reserved[5];
};

#endif	//	_LAYOUT_CONTEXT_H
