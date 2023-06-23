/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef K__DRAGGER_H
#define K__DRAGGER_H


#include <Locker.h>
#include <List.h>
#include <Laminate.h>

class KBitmap;
class BMessage;
class KPopUpMenu;
class KShelf;

namespace BPrivate {
	struct k_replicant_data;
	class KShelfContainerViewFilter;
};


class KDragger : public KView {
public:
								KDragger(BRect frame, KView* target,
									uint32 resizingMode = B_FOLLOW_NONE,
									uint32 flags = B_WILL_DRAW);
								KDragger(KView* target,
									uint32 flags = B_WILL_DRAW);
								KDragger(BMessage* data);
	virtual						~KDragger();

	static	BArchivable*		 Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data,
									bool deep = true) const;

	virtual void				AttachedToWindow();
	virtual void				DetachedFromWindow();

	virtual void				Draw(BRect updateRect);
	virtual void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual void				MessageReceived(BMessage* message);
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	static	status_t			ShowAllDraggers();
	static	status_t			HideAllDraggers();
	static	bool				AreDraggersDrawn();

	virtual BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual status_t			GetSupportedSuites(BMessage* data);
	virtual status_t			Perform(perform_code code, void* data);

	virtual void				ResizeToPreferred();
	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual void				MakeFocus(bool focus = true);
	virtual void				AllAttached();
	virtual void				AllDetached();

			status_t			SetPopUp(KPopUpMenu* contextMenu);
			KPopUpMenu*			PopUp() const;

			bool				InShelf() const;
			KView*				Target() const;

	virtual	KBitmap*			DragBitmap(BPoint* offset, drawing_mode* mode);

	class Private;

protected:
			bool				IsVisibilityChanging() const;

private:
	friend class BPrivate::KShelfContainerViewFilter;
	friend struct BPrivate::k_replicant_data;
	friend class Private;
	friend class KShelf;

	virtual	void				_ReservedDragger2();
	virtual	void				_ReservedDragger3();
	virtual	void				_ReservedDragger4();

	static	void				_UpdateShowAllDraggers(bool visible);

			KDragger&			operator=(const KDragger& other);

			void				_InitData();
			void				_AddToList();
			void				_RemoveFromList();
			status_t			_DetermineRelationship();
			status_t			_SetViewToDrag(KView* target);
			void				_SetShelf(KShelf* shelf);
			void				_SetZombied(bool state);
			void				_BuildDefaultPopUp();
			void				_ShowPopUp(KView* target, BPoint where);

			enum relation {
				TARGET_UNKNOWN,
				TARGET_IS_CHILD,
				TARGET_IS_PARENT,
				TARGET_IS_SIBLING
			};

			KView*				fTarget;
			relation			fRelation;
			KShelf*				fShelf;
			bool				fTransition;
			bool				fIsZombie;
			char				fErrCount;
			bool				fPopUpIsCustom;
			KBitmap*			fBitmap;
			KPopUpMenu*			fPopUp;
			uint32				_reserved[3];
};

#endif /* _DRAGGER_H */
