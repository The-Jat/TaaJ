/*
 * Copyright 2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <KCardLayout.h>
#include <KCardView.h>


KCardView::KCardView()
	:
	KView(NULL, 0, new KCardLayout())
{
	AdoptSystemColors();
}


KCardView::KCardView(const char* name)
	:
	KView(name, 0, new KCardLayout())
{
	AdoptSystemColors();
}


KCardView::KCardView(BMessage* from)
	:
	KView(from)
{
	AdoptSystemColors();
}


KCardView::~KCardView()
{
}


void
KCardView::SetLayout(KLayout* layout)
{
	if (dynamic_cast<KCardLayout*>(layout) == NULL)
		return;

	KView::SetLayout(layout);
}


KCardLayout*
KCardView::CardLayout() const
{
	return static_cast<KCardLayout*>(GetLayout());
}


BArchivable*
KCardView::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KCardView"))
		return new KCardView(from);
	return NULL;
}


status_t
KCardView::Perform(perform_code d, void* arg)
{
	return KView::Perform(d, arg);
}


void KCardView::_ReservedCardView1() {}
void KCardView::_ReservedCardView2() {}
void KCardView::_ReservedCardView3() {}
void KCardView::_ReservedCardView4() {}
void KCardView::_ReservedCardView5() {}
void KCardView::_ReservedCardView6() {}
void KCardView::_ReservedCardView7() {}
void KCardView::_ReservedCardView8() {}
void KCardView::_ReservedCardView9() {}
void KCardView::_ReservedCardView10() {}
