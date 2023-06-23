/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <KViewPort.h>

#include <algorithm>

#include <KAbstractLayout.h>
#include <KScrollBar.h>

#include "KViewLayoutItem.h"


namespace BPrivate {


// #pragma mark - ViewPortLayout


class KViewPort::ViewPortLayout : public KAbstractLayout {
public:
	ViewPortLayout(KViewPort* viewPort)
		:
		KAbstractLayout(),
		fViewPort(viewPort),
		fHasViewChild(false),
		fIsCacheValid(false),
		fMin(),
		fMax(),
		fPreferred()
	{
	}

	KView* ChildView() const
	{
		if (!fHasViewChild)
			return NULL;
		if (KViewLayoutItem* item = dynamic_cast<KViewLayoutItem*>(ItemAt(0)))
			return item->View();
		return NULL;
	}

	void SetChildView(KView* view)
	{
		_UnsetChild();

		if (view != NULL && AddView(0, view) != NULL)
			fHasViewChild = true;
	}

	KLayoutItem* ChildItem() const
	{
		return ItemAt(0);
	}

	void SetChildItem(KLayoutItem* item)
	{
		_UnsetChild();

		if (item != NULL)
			AddItem(0, item);
	}

	virtual BSize BaseMinSize()
	{
		_ValidateMinMax();
		return fMin;
	}

	virtual BSize BaseMaxSize()
	{
		_ValidateMinMax();
		return fMax;
	}

	virtual BSize BasePreferredSize()
	{
		_ValidateMinMax();
		return fPreferred;
	}

	virtual BAlignment BaseAlignment()
	{
		return KAbstractLayout::BaseAlignment();
	}

	virtual bool HasHeightForWidth()
	{
		_ValidateMinMax();
		return false;
		// TODO: Support height-for-width!
	}

	virtual void GetHeightForWidth(float width, float* min, float* max,
		float* preferred)
	{
		if (!HasHeightForWidth())
			return;

		// TODO: Support height-for-width!
	}

	virtual void LayoutInvalidated(bool children)
	{
		fIsCacheValid = false;
	}

	virtual void DoLayout()
	{
		_ValidateMinMax();

		KLayoutItem* child = ItemAt(0);
		if (child == NULL)
			return;

		// Determine the layout area: LayoutArea() will only give us the size
		// of the view port's frame.
		BSize viewSize = LayoutArea().Size();
		BSize layoutSize = viewSize;

		BSize childMin = child->MinSize();
		BSize childMax = child->MaxSize();

		// apply the maximum constraints
		layoutSize.width = std::min(layoutSize.width, childMax.width);
		layoutSize.height = std::min(layoutSize.height, childMax.height);

		// apply the minimum constraints
		layoutSize.width = std::max(layoutSize.width, childMin.width);
		layoutSize.height = std::max(layoutSize.height, childMin.height);

		// TODO: Support height-for-width!

		child->AlignInFrame(BRect(BPoint(0, 0), layoutSize));

		_UpdateScrollBar(fViewPort->ScrollBar(B_HORIZONTAL), viewSize.width,
			layoutSize.width);
		_UpdateScrollBar(fViewPort->ScrollBar(B_VERTICAL), viewSize.height,
			layoutSize.height);
	}

private:
	void _UnsetChild()
	{
		if (CountItems() > 0) {
			KLayoutItem* item = RemoveItem((int32)0);
			if (fHasViewChild)
				delete item;
			fHasViewChild = false;
		}
	}

	void _ValidateMinMax()
	{
		if (fIsCacheValid)
			return;

		if (KLayoutItem* child = ItemAt(0)) {
			fMin = child->MinSize();
			if (_IsHorizontallyScrollable())
				fMin.width = -1;
			if (_IsVerticallyScrollable())
				fMin.height = -1;
			fMax = child->MaxSize();
			fPreferred = child->PreferredSize();
			// TODO: Support height-for-width!
		} else {
			fMin.Set(-1, -1);
			fMax.Set(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
			fPreferred.Set(20, 20);
		}

		fIsCacheValid = true;
	}

	bool _IsHorizontallyScrollable() const
	{
		return fViewPort->ScrollBar(B_HORIZONTAL) != NULL;
	}

	bool _IsVerticallyScrollable() const
	{
		return fViewPort->ScrollBar(B_VERTICAL) != NULL;
	}

	void _UpdateScrollBar(KScrollBar* scrollBar, float viewPortSize,
		float dataSize)
	{
		if (scrollBar == NULL)
			return;

		if (viewPortSize < dataSize) {
			scrollBar->SetRange(0, dataSize - viewPortSize);
			scrollBar->SetProportion(viewPortSize / dataSize);
			float smallStep;
			scrollBar->GetSteps(&smallStep, NULL);
			scrollBar->SetSteps(smallStep, viewPortSize);
		} else {
			scrollBar->SetRange(0, 0);
			scrollBar->SetProportion(1);
		}
	}

private:
	KViewPort*	fViewPort;
	bool		fHasViewChild;
	bool		fIsCacheValid;
	BSize		fMin;
	BSize		fMax;
	BSize		fPreferred;
};


// #pragma mark - KViewPort


KViewPort::KViewPort(KView* child)
	:
	KView(NULL, 0),
	fChild(NULL)
{
	_Init();
	SetChildView(child);
}


KViewPort::KViewPort(KLayoutItem* child)
	:
	KView(NULL, 0),
	fChild(NULL)
{
	_Init();
	SetChildItem(child);
}


KViewPort::KViewPort(const char* name, KView* child)
	:
	KView(name, 0),
	fChild(NULL)
{
	_Init();
	SetChildView(child);
}


KViewPort::KViewPort(const char* name, KLayoutItem* child)
	:
	KView(name, 0),
	fChild(NULL)
{
	_Init();
	SetChildItem(child);
}


KViewPort::~KViewPort()
{
}


KView*
KViewPort::ChildView() const
{
	return fLayout->ChildView();
}


void
KViewPort::SetChildView(KView* child)
{
	fLayout->SetChildView(child);
	InvalidateLayout();
}


KLayoutItem*
KViewPort::ChildItem() const
{
	return fLayout->ChildItem();
}


void
KViewPort::SetChildItem(KLayoutItem* child)
{
	fLayout->SetChildItem(child);
	InvalidateLayout();
}


void
KViewPort::_Init()
{
	fLayout = new ViewPortLayout(this);
	SetLayout(fLayout);
}


}	// namespace BPrivate


using ::BPrivate::KViewPort;
