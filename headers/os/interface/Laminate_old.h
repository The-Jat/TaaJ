#ifndef __LAMINATE__H
#define __LAMINATE__H

#include <AffineTransform.h>
#include <Alignment.h>
#include <Font.h>
#include <Handler.h>
#include <InterfaceDefs.h>
#include <Rect.h>
#include <Gradient.h>
#include <View.h>

class BShelf;
class KWindow;
struct _array_data_;
struct _array_hdr_;
struct overlay_restrictions;

namespace BPrivate {
	class ViewState;
};


class KView : public BHandler {
public:
								KView(BRect frame, const char* name,
									uint32 resizingMode, uint32 flags);

			KWindow*			Window() const;
	virtual	void				Draw(BRect updateRect);
	virtual	void				WindowActivated(bool active);

		void				_ParentResizedBy(int32 deltaWidth,
									int32 deltaHeight);

			void				ConvertFromScreen(BPoint* point) const;
			BPoint				ConvertFromScreen(BPoint point) const;
			void				ConvertFromScreen(BRect* rect) const;
			BRect				ConvertFromScreen(BRect rect) const;


				BRect				Bounds() const;
				BRect				Frame() const;
			BPoint				LeftTop() const;
				void				SetViewUIColor(color_which which,
									float tint = B_NO_TINT);

				color_which			ViewUIColor(float* tint = NULL) const;

				

				void				SetHighUIColor(color_which which,
									float tint = B_NO_TINT);
				
				void				SetLowUIColor(color_which which,
									float tint = B_NO_TINT);

			void				PushState();
			void				PopState();

	virtual	void				DrawAfterChildren(BRect updateRect);

			void				Invalidate(BRect invalRect);
			void				Invalidate();

			uint32				Flags() const;
			uint32				ResizingMode() const;
	virtual	void				MakeFocus(bool focus = true);
			bool				IsHidden() const;
			bool				IsHidden(const KView* looking_from) const;

			void				Flush() const;

			BLayoutContext*		LayoutContext() const;

			void				Layout(bool force);
			void				Relayout();
protected:

	virtual	void				DoLayout();

protected:

	virtual	void				LayoutChanged();

private:
			void				_Layout(bool force, BLayoutContext* context);

private:
	struct LayoutData;

	friend class BScrollBar;
	friend class BShelf;
	friend class KWindow;//khidki code

				void				_InitData(BRect frame, const char* name,
									uint32 resizingMode, uint32 flags);
			bool				_CheckOwnerLockAndSwitchCurrent() const;
			void				_CheckLockAndSwitchCurrent() const;
			void				_CheckLock() const;
			void				_SwitchServerCurrentView() const;

			bool				_CheckOwnerLock() const;

			void				_SetOwner(KWindow* newOwner);
			void				_RemoveCommArray();
			void				_ResizeBy(int32 deltaWidth, int32 deltaHeight);

			void				_MoveTo(int32 x, int32 y);

			void				_ConvertFromScreen(BPoint* pt,
									bool checkLock) const;
			void				_ConvertFromParent(BPoint* pt,
									bool checkLock) const;
			void				_Activate(bool state);

			void				_Draw(BRect screenUpdateRect);
			void				_DrawAfterChildren(BRect screenUpdateRect);

			void				_FlushIfNotInTransaction();

			bool				_CreateSelf();

private:
	
			/*uint32				fFlags;
			BPoint				fParentOffset;
			KWindow*			fOwner;
			KView*				fParent;
			KView*				fNextSibling;
			KView*				fPreviousSibling;
			KView*				fFirstChild;
			
			int16				fShowLevel;
			bool				fTopLevelView;
			bool				fNoISInteraction;
			
			BPicture*			fCurrentPicture;
			_array_data_*		fCommArray;
			
			BScrollBar*			fVerScroller;
			BScrollBar*			fHorScroller;
			bool				fIsPrinting;
			bool				fAttached;
			bool				_unused_bool1;
			bool				_unused_bool2;
			
			::BPrivate::ViewState* fState;
			BRect				fBounds;
			BShelf*				fShelf;
			uint32				fEventMask;
			uint32				fEventOptions;
			uint32				fMouseEventOptions;

			LayoutData*			fLayoutData;
			BToolTip*			fToolTip;*/
			
			int32				_unused_int1;

			uint32				fFlags;
			BPoint				fParentOffset;
			KWindow*			fOwner;
			KView*				fParent;
			KView*				fNextSibling;
			KView*				fPreviousSibling;
			KView*				fFirstChild;

			int16				fShowLevel;
			bool				fTopLevelView;
			bool				fNoISInteraction;
			BPicture*			fCurrentPicture;
			_array_data_*		fCommArray;

			BScrollBar*			fVerScroller;
			BScrollBar*			fHorScroller;
			bool				fIsPrinting;
			bool				fAttached;
			bool				_unused_bool1;
			bool				_unused_bool2;
			::BPrivate::ViewState* fState;
			BRect				fBounds;
			BShelf*				fShelf;
			uint32				fEventMask;
			uint32				fEventOptions;
			uint32				fMouseEventOptions;

			LayoutData*			fLayoutData;
			BToolTip*			fToolTip;

			uint32				_reserved[6];

};
#endif
