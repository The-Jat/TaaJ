/*
 * Copyright 2009, Alexandre Deckner, alex@zappotek.com
 * Distributed under the terms of the MIT License.
 */
#ifndef K_DRAG_TRACKING_FILTER_H
#define K_DRAG_TRACKING_FILTER_H

#include <MessageFilter.h>
#include <Point.h>

class KView;
class BHandler;

namespace BPrivate {

class KDragTrackingFilter : public BMessageFilter {
public:
						KDragTrackingFilter(KView* targetView, uint32 messageWhat);

	filter_result		Filter(BMessage* message, BHandler** _target);

private:
			KView*		fTargetView;
			uint32		fMessageWhat;
			bool		fIsTracking;
			BPoint		fClickPoint;
			uint32		fClickButtons;
};

}	// namespace BPrivate

using BPrivate::KDragTrackingFilter;

#endif	// DRAG_TRACKING_FILTER_H
