/*
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


//I left it because it is independent of changed things.

#include <KLayoutContext.h>


// constructor
KLayoutContextListener::KLayoutContextListener()
{
}

// destructor
KLayoutContextListener::~KLayoutContextListener()
{
}


void KLayoutContextListener::_ReservedLayoutContextListener1() {}
void KLayoutContextListener::_ReservedLayoutContextListener2() {}
void KLayoutContextListener::_ReservedLayoutContextListener3() {}
void KLayoutContextListener::_ReservedLayoutContextListener4() {}
void KLayoutContextListener::_ReservedLayoutContextListener5() {}


// #pragma mark -


// constructor
KLayoutContext::KLayoutContext()
{
}

// destructor
KLayoutContext::~KLayoutContext()
{
	// notify the listeners
	for (int32 i = 0;
		 KLayoutContextListener* listener
		 	= (KLayoutContextListener*)fListeners.ItemAt(i);
		 i++) {
		listener->LayoutContextLeft(this);
	}
}

// AddListener
void
KLayoutContext::AddListener(KLayoutContextListener* listener)
{
	if (listener)
		fListeners.AddItem(listener);
}

// RemoveListener
void
KLayoutContext::RemoveListener(KLayoutContextListener* listener)
{
	if (listener)
		fListeners.RemoveItem(listener);
}
