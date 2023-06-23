/*
 * Copyright 2010, Haiku, Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <KAbstractLayout.h>
#include <KLayoutUtils.h>
#include <Message.h>
#include <Laminate.h>
#include <ViewPrivate.h>


namespace {
	const char* const kSizesField = "KAbstractLayout:sizes";
		// kSizesField == {min, max, preferred}
	const char* const kAlignmentField = "KAbstractLayout:alignment";
	const char* const kFrameField = "KAbstractLayout:frame";
	const char* const kVisibleField = "KAbstractLayout:visible";

	enum proxy_type { VIEW_PROXY_TYPE, DATA_PROXY_TYPE }; 
}


struct KAbstractLayout::Proxy {

	Proxy(proxy_type type)
		:
		type(type)
	{
	}

	virtual ~Proxy()
	{
	}

	virtual	BSize		MinSize() const = 0;
	virtual void		SetMinSize(const BSize&) = 0;

	virtual	BSize		MaxSize() const = 0;
	virtual void		SetMaxSize(const BSize&) = 0;

	virtual	BSize		PreferredSize() const = 0;
	virtual void		SetPreferredSize(const BSize&) = 0;

	virtual	BAlignment	Alignment() const = 0;
	virtual	void		SetAlignment(const BAlignment&) = 0;

	virtual	BRect		Frame() const = 0;
	virtual	void		SetFrame(const BRect& frame) = 0;

	virtual bool		IsVisible(bool ancestorHidden) const = 0;
	virtual	void		SetVisible(bool visible) = 0;

	virtual	status_t	AddDataToArchive(BMessage* archive,
							bool ancestorHidden) = 0;
	virtual	status_t	RestoreDataFromArchive(const BMessage* archive) = 0;

			proxy_type	type;
};


struct KAbstractLayout::DataProxy : Proxy {

	DataProxy()
		:
		Proxy(DATA_PROXY_TYPE),
		minSize(),
		maxSize(),
		preferredSize(),
		alignment(),
		frame(-1, -1, 0, 0),
		visible(true)
	{
	}

	BSize MinSize() const
	{
		return minSize;
	}

	void SetMinSize(const BSize& min)
	{
		minSize = min;
	}

	BSize MaxSize() const
	{
		return maxSize;
	}

	void SetMaxSize(const BSize& max)
	{
		maxSize = max;
	}

	BSize PreferredSize() const
	{
		return preferredSize;
	}

	void SetPreferredSize(const BSize& preferred)
	{
		preferredSize = preferred;
	}

	BAlignment Alignment() const
	{
		return this->alignment;
	}

	void SetAlignment(const BAlignment& align)
	{
		this->alignment = align;
	}

	BRect Frame() const
	{
		return frame;
	}

	void SetFrame(const BRect& frame)
	{
		if (frame == this->frame)
			return;
		this->frame = frame;
	}

	bool IsVisible(bool) const
	{
		return visible;
	}

	void SetVisible(bool visible)
	{
		this->visible = visible;
	}

	status_t AddDataToArchive(BMessage* archive, bool ancestorHidden)
	{
		status_t err = archive->AddSize(kSizesField, minSize);
		if (err == B_OK)
			err = archive->AddSize(kSizesField, maxSize);
		if (err == B_OK)
			err = archive->AddSize(kSizesField, preferredSize);
		if (err == B_OK)
			err = archive->AddAlignment(kAlignmentField, alignment);
		if (err == B_OK)
			err = archive->AddRect(kFrameField, frame);
		if (err == B_OK)
			err = archive->AddBool(kVisibleField, visible);

		return err;
	}

	status_t RestoreDataFromArchive(const BMessage* archive)
	{
		status_t err = archive->FindSize(kSizesField, 0, &minSize);
		if (err == B_OK)
			err = archive->FindSize(kSizesField, 1, &maxSize);
		if (err == B_OK)
			err = archive->FindSize(kSizesField, 2, &preferredSize);
		if (err == B_OK)
			err = archive->FindAlignment(kAlignmentField, &alignment);
		if (err == B_OK)
			err = archive->FindRect(kFrameField, &frame);
		if (err == B_OK)
			err = archive->FindBool(kVisibleField, &visible);

		return err;
	}

	BSize		minSize;
	BSize		maxSize;
	BSize		preferredSize;
	BAlignment	alignment;
	BRect		frame;
	bool		visible;
};


struct KAbstractLayout::ViewProxy : Proxy {
	ViewProxy(KView* target)
		:
		Proxy(VIEW_PROXY_TYPE),
		view(target)
	{
	}

	BSize MinSize() const
	{
		return view->ExplicitMinSize();
	}

	void SetMinSize(const BSize& min)
	{
		view->SetExplicitMinSize(min);
	}

	BSize MaxSize() const
	{
		return view->ExplicitMaxSize();
	}

	void SetMaxSize(const BSize& min)
	{
		view->SetExplicitMaxSize(min);
	}

	BSize PreferredSize() const
	{
		return view->ExplicitPreferredSize();
	}

	void SetPreferredSize(const BSize& preferred)
	{
		view->SetExplicitPreferredSize(preferred);
	}

	BAlignment Alignment() const
	{
		return view->ExplicitAlignment();
	}

	void SetAlignment(const BAlignment& alignment)
	{
		view->SetExplicitAlignment(alignment);
	}

	BRect Frame() const
	{
		return view->Frame();
	}

	void SetFrame(const BRect& frame)
	{
		view->MoveTo(frame.LeftTop());
		view->ResizeTo(frame.Width(), frame.Height());
	}

	bool IsVisible(bool ancestorsVisible) const
	{
		int16 showLevel = KView::KPrivate(view).ShowLevel();
		return (showLevel - (ancestorsVisible ? 0 : 1)) <= 0;
	}

	void SetVisible(bool visible)
	{
		// No need to check that we are not re-hiding, that is done
		// for us.
		if (visible)
			view->Show();
		else
			view->Hide();
	}

	status_t AddDataToArchive(BMessage* archive, bool ancestorHidden)
	{
		return B_OK;
	}

	status_t RestoreDataFromArchive(const BMessage* archive)
	{
		return B_OK;
	}

	KView*	view;
};


KAbstractLayout::KAbstractLayout()
	:
	fExplicitData(new KAbstractLayout::DataProxy())
{
}


KAbstractLayout::KAbstractLayout(BMessage* from)
	:
	KLayout(BUnarchiver::PrepareArchive(from)),
	fExplicitData(new DataProxy())
{
	BUnarchiver(from).Finish();
}


KAbstractLayout::~KAbstractLayout()
{
	delete fExplicitData;
}


BSize
KAbstractLayout::MinSize()
{
	return KLayoutUtils::ComposeSize(fExplicitData->MinSize(), BaseMinSize());
}


BSize
KAbstractLayout::MaxSize()
{
	return KLayoutUtils::ComposeSize(fExplicitData->MaxSize(), BaseMaxSize());
}


BSize
KAbstractLayout::PreferredSize()
{
	return KLayoutUtils::ComposeSize(fExplicitData->PreferredSize(),
		BasePreferredSize());
}


BAlignment
KAbstractLayout::Alignment()
{
	return KLayoutUtils::ComposeAlignment(fExplicitData->Alignment(),
		BaseAlignment());
}


void
KAbstractLayout::SetExplicitMinSize(BSize size)
{
	fExplicitData->SetMinSize(size);
}


void
KAbstractLayout::SetExplicitMaxSize(BSize size)
{
	fExplicitData->SetMaxSize(size);
}


void
KAbstractLayout::SetExplicitPreferredSize(BSize size)
{
	fExplicitData->SetPreferredSize(size);
}


void
KAbstractLayout::SetExplicitAlignment(BAlignment alignment)
{
	fExplicitData->SetAlignment(alignment);
}


BSize
KAbstractLayout::BaseMinSize()
{
	return BSize(0, 0);
}


BSize
KAbstractLayout::BaseMaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


BSize
KAbstractLayout::BasePreferredSize()
{
	return BSize(0, 0);
}


BAlignment
KAbstractLayout::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


BRect
KAbstractLayout::Frame()
{
	return fExplicitData->Frame();
}


void
KAbstractLayout::SetFrame(BRect frame)
{
	if (frame != fExplicitData->Frame()) {
		fExplicitData->SetFrame(frame);
		if (!Owner())
			Relayout();
	}
}


bool
KAbstractLayout::IsVisible()
{
	return fExplicitData->IsVisible(AncestorsVisible());
}


void
KAbstractLayout::SetVisible(bool visible)
{
	if (visible != fExplicitData->IsVisible(AncestorsVisible())) {
		fExplicitData->SetVisible(visible);
		if (Layout())
			Layout()->InvalidateLayout(false);
		VisibilityChanged(visible);
	}
}


status_t
KAbstractLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = KLayout::Archive(into, deep);

	return archiver.Finish(err);
}


status_t
KAbstractLayout::AllArchived(BMessage* archive) const
{
	return KLayout::AllArchived(archive);
}


status_t
KAbstractLayout::AllUnarchived(const BMessage* from)
{
	status_t err = fExplicitData->RestoreDataFromArchive(from);
	if (err != B_OK)
		return err;
		
	return KLayout::AllUnarchived(from);
}


status_t
KAbstractLayout::ItemArchived(BMessage* into, KLayoutItem* item,
	int32 index) const
{
	return KLayout::ItemArchived(into, item, index);
}


status_t
KAbstractLayout::ItemUnarchived(const BMessage* from, KLayoutItem* item,
	int32 index)
{
	return KLayout::ItemUnarchived(from, item, index);
}


bool
KAbstractLayout::ItemAdded(KLayoutItem* item, int32 atIndex)
{
	return KLayout::ItemAdded(item, atIndex);
}


void
KAbstractLayout::ItemRemoved(KLayoutItem* item, int32 fromIndex)
{
	KLayout::ItemRemoved(item, fromIndex);
}


void
KAbstractLayout::LayoutInvalidated(bool children)
{
	KLayout::LayoutInvalidated(children);
}


void
KAbstractLayout::OwnerChanged(KView* was)
{
	if (was) {
		static_cast<ViewProxy*>(fExplicitData)->view = Owner();
		return;
	}

	delete fExplicitData;
	fExplicitData = new ViewProxy(Owner());
}


void
KAbstractLayout::AttachedToLayout()
{
	KLayout::AttachedToLayout();
}


void
KAbstractLayout::DetachedFromLayout(KLayout* layout)
{
	KLayout::DetachedFromLayout(layout);
}


void
KAbstractLayout::AncestorVisibilityChanged(bool shown)
{
	if (AncestorsVisible() == shown)
		return;

	if (KView* owner = Owner()) {
		if (shown)
			owner->Show();
		else
			owner->Hide();
	}
	KLayout::AncestorVisibilityChanged(shown);
}


// Binary compatibility stuff


status_t
KAbstractLayout::Perform(perform_code code, void* _data)
{
	return KLayout::Perform(code, _data);
}


void KAbstractLayout::_ReservedAbstractLayout1() {}
void KAbstractLayout::_ReservedAbstractLayout2() {}
void KAbstractLayout::_ReservedAbstractLayout3() {}
void KAbstractLayout::_ReservedAbstractLayout4() {}
void KAbstractLayout::_ReservedAbstractLayout5() {}
void KAbstractLayout::_ReservedAbstractLayout6() {}
void KAbstractLayout::_ReservedAbstractLayout7() {}
void KAbstractLayout::_ReservedAbstractLayout8() {}
void KAbstractLayout::_ReservedAbstractLayout9() {}
void KAbstractLayout::_ReservedAbstractLayout10() {}

