/*
 * Copyright 2010 Haiku, Inc. All rights reserved.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 *
 * Distributed under the terms of the MIT License.
 */


#include <KGridView.h>


KGridView::KGridView(float horizontalSpacing, float verticalSpacing)
	:
	KView(NULL, 0, new KGridLayout(horizontalSpacing, verticalSpacing))
{
	AdoptSystemColors();
}


KGridView::KGridView(const char* name, float horizontalSpacing,
	float verticalSpacing)
	:
	KView(name, 0, new KGridLayout(horizontalSpacing, verticalSpacing))
{
	AdoptSystemColors();
}


KGridView::KGridView(BMessage* from)
	:
	KView(from)
{
}


KGridView::~KGridView()
{
}


void
KGridView::SetLayout(KLayout* layout)
{
	// only BGridLayouts are allowed
	if (!dynamic_cast<KGridLayout*>(layout))
		return;

	KView::SetLayout(layout);
}


KGridLayout*
KGridView::GridLayout() const
{
	return dynamic_cast<KGridLayout*>(GetLayout());
}


BArchivable*
KGridView::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KGridView"))
		return new KGridView(from);
	return NULL;
}


status_t
KGridView::Perform(perform_code code, void* _data)
{
	return KView::Perform(code, _data);
}


void KGridView::_ReservedGridView1() {}
void KGridView::_ReservedGridView2() {}
void KGridView::_ReservedGridView3() {}
void KGridView::_ReservedGridView4() {}
void KGridView::_ReservedGridView5() {}
void KGridView::_ReservedGridView6() {}
void KGridView::_ReservedGridView7() {}
void KGridView::_ReservedGridView8() {}
void KGridView::_ReservedGridView9() {}
void KGridView::_ReservedGridView10() {}
