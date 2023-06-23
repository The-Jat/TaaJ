/*
 * Copyright 2006-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__SPLIT_VIEW_H
#define	K__SPLIT_VIEW_H

#include <Laminate.h>

class KSplitLayout;


class KSplitView : public KView {
public:
								KSplitView(
									orientation orientation = B_HORIZONTAL,
									float spacing = B_USE_DEFAULT_SPACING);
								KSplitView(BMessage* from);
	virtual						~KSplitView();

			void				SetInsets(float left, float top, float right,
									float bottom);
			void				SetInsets(float horizontal, float vertical);
			void				SetInsets(float insets);
			void				GetInsets(float* left, float* top,
									float* right, float* bottom) const;

			float				Spacing() const;
			void				SetSpacing(float spacing);

			orientation			Orientation() const;
			void				SetOrientation(orientation orientation);

			float				SplitterSize() const;
			void				SetSplitterSize(float size);

			int32				CountItems() const;
			float				ItemWeight(int32 index) const;
			float				ItemWeight(KLayoutItem* item) const;
			void				SetItemWeight(int32 index, float weight,
									bool invalidateLayout);
			void				SetItemWeight(KLayoutItem* item, float weight);

			bool				IsCollapsible(int32 index) const;
			void				SetCollapsible(bool collapsible);
			void				SetCollapsible(int32 index, bool collapsible);
			void				SetCollapsible(int32 first, int32 last,
									bool collapsible);

			bool				IsItemCollapsed(int32 index) const;
			void				SetItemCollapsed(int32 index, bool collapsed);

//			void				AddChild(KView* child);
			void				AddChild(KView* child, KView* sibling = NULL);
			bool				AddChild(KView* child, float weight);
			bool				AddChild(int32 index, KView* child,
									float weight);

			bool				AddChild(KLayoutItem* child);
			bool				AddChild(KLayoutItem* child, float weight);
			bool				AddChild(int32 index, KLayoutItem* child,
									float weight);

	virtual void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				DrawAfterChildren(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* message);

	virtual	void				MessageReceived(BMessage* message);


	virtual	void				SetLayout(KLayout* layout);
									// overridden to avoid use

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* from);

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	virtual	status_t			AllArchived(BMessage* into) const;
	virtual status_t			AllUnarchived(const BMessage* from);

	virtual	void				DrawSplitter(BRect frame,
									const BRect& updateRect,
									orientation orientation,
									bool pressed);

private:

	// FBC padding
	virtual	void				_ReservedSplitView1();
	virtual	void				_ReservedSplitView2();
	virtual	void				_ReservedSplitView3();
	virtual	void				_ReservedSplitView4();
	virtual	void				_ReservedSplitView5();
	virtual	void				_ReservedSplitView6();
	virtual	void				_ReservedSplitView7();
	virtual	void				_ReservedSplitView8();
	virtual	void				_ReservedSplitView9();
	virtual	void				_ReservedSplitView10();

	// forbidden methods
								KSplitView(const KSplitView&);
			void				operator =(const KSplitView&);

	static	void				_DrawDefaultSplitter(KView* view, BRect frame,
									const BRect& updateRect,
									orientation orientation,
									bool pressed);

private:
			KSplitLayout*		fSplitLayout;
			uint32				_reserved[3];
};


#endif	// _SPLIT_VIEW_H
