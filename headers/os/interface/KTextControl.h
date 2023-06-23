/*
 * Copyright 2006-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__TEXT_CONTROL_H
#define	K__TEXT_CONTROL_H


#include <KControl.h>
#include <KTextView.h>

//#include"KTextInput.h"


class KLayoutItem;
namespace BPrivate {
	class _KTextInput_;
}


class KTextControl : public KControl {
public:
								KTextControl(BRect frame, const char* name,
									const char* label, const char* initialText,
									BMessage* message,
									uint32 resizeMask = B_FOLLOW_LEFT_TOP,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								KTextControl(const char* name,
									const char* label, const char* initialText,
									BMessage* message,
									uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
								KTextControl(const char* label,
									const char* initialText,
									BMessage* message);
	virtual						~KTextControl();

								KTextControl(BMessage* archive);
	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				SetText(const char* text);
			const char*			Text() const;
			int32				TextLength() const;
			void				MarkAsInvalid(bool invalid);

	virtual	void				SetValue(int32 value);
	virtual	status_t			Invoke(BMessage* message = NULL);

			KTextView*			TextView() const;

	virtual	void				SetModificationMessage(BMessage* message);
			BMessage*			ModificationMessage() const;

	virtual	void				SetAlignment(alignment label, alignment text);
			void				GetAlignment(alignment* _label,
									alignment* _text) const;
	virtual	void				SetDivider(float position);
			float				Divider() const;

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				AttachedToWindow();
	virtual	void				MakeFocus(bool focus = true);
	virtual	void				SetEnabled(bool enable);
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual	void				WindowActivated(bool active);

	virtual	void				GetPreferredSize(float* _width,
									float* _height);
	virtual	void				ResizeToPreferred();

	virtual	void				MessageReceived(BMessage* message);
	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);

	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				DetachedFromWindow();

	virtual	void				AllAttached();
	virtual	void				AllDetached();
	virtual	status_t			GetSupportedSuites(BMessage* data);
	virtual	void				SetFlags(uint32 flags);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	BAlignment			LayoutAlignment();

			KLayoutItem*		CreateLabelLayoutItem();
			KLayoutItem*		CreateTextViewLayoutItem();

protected:
	virtual	status_t			AllArchived(BMessage* into) const;
	virtual	status_t			AllUnarchived(const BMessage* from);

	virtual	void				LayoutInvalidated(bool descendants);
	virtual	void				DoLayout();

	virtual	status_t			SetIcon(const KBitmap* icon, uint32 flags = 0);

private:
	// FBC padding and forbidden methods
	virtual	status_t			Perform(perform_code d, void* arg);

	virtual	void				_ReservedTextControl1();
	virtual	void				_ReservedTextControl2();
	virtual	void				_ReservedTextControl3();
	virtual	void				_ReservedTextControl4();

			KTextControl&		operator=(const KTextControl& other);

private:
	class LabelLayoutItem;
	class TextViewLayoutItem;
	struct LayoutData;

	friend class _KTextInput_;
	friend class LabelLayoutItem;
	friend class TextViewLayoutItem;

			void				_CommitValue();
			void				_UpdateTextViewColors(bool enable);
			void				_InitData(const char* label,
									const BMessage* archive = NULL);
			void				_InitText(const char* initialText,
									const BMessage* archive = NULL);
			void				_ValidateLayout();
			void				_LayoutTextView();
			void				_UpdateFrame();

			void				_ValidateLayoutData();

private:
			BPrivate::_KTextInput_* fText;
			BMessage*			fModificationMessage;
			alignment			fLabelAlign;
			float				fDivider;

			LayoutData*			fLayoutData;
			uint32				fLook;

			uint32				_reserved[8];
};


#endif	// _TEXT_CONTROL_H
