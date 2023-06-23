/*
 * Copyright 2010-2015 Haiku, Inc. All rights reserved.
 * Copyright 2006, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 *
 * Distributed under the terms of the MIT License.
 */


#include <KGroupView.h>


KGroupView::KGroupView(orientation orientation, float spacing)
	:
	KView(NULL, 0, new KGroupLayout(orientation, spacing))
{
	AdoptSystemColors();
}


KGroupView::KGroupView(const char* name, orientation orientation,
	float spacing)
	:
	KView(name, 0, new KGroupLayout(orientation, spacing))
{
	AdoptSystemColors();
}


KGroupView::KGroupView(BMessage* from)
	:
	KView(from)
{
	AdoptSystemColors();
}


KGroupView::~KGroupView()
{
}


void
KGroupView::SetLayout(KLayout* layout)
{
	// only BGroupLayouts are allowed
	if (!dynamic_cast<KGroupLayout*>(layout))
		return;

	KView::SetLayout(layout);
}


BArchivable*
KGroupView::Instantiate(BMessage* from)
{
	if (validate_instantiation(from, "KGroupView"))
		return new KGroupView(from);

	return NULL;
}


KGroupLayout*
KGroupView::GroupLayout() const
{
	return dynamic_cast<KGroupLayout*>(GetLayout());
}


status_t
KGroupView::Perform(perform_code code, void* _data)
{
	return KView::Perform(code, _data);
}


void KGroupView::_ReservedGroupView1() {}
void KGroupView::_ReservedGroupView2() {}
void KGroupView::_ReservedGroupView3() {}
void KGroupView::_ReservedGroupView4() {}
void KGroupView::_ReservedGroupView5() {}
void KGroupView::_ReservedGroupView6() {}
void KGroupView::_ReservedGroupView7() {}
void KGroupView::_ReservedGroupView8() {}
void KGroupView::_ReservedGroupView9() {}
void KGroupView::_ReservedGroupView10() {}
