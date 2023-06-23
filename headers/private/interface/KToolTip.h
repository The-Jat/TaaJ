/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_TOOL_TIP_H
#define _K_TOOL_TIP_H


#include <Alignment.h>
#include <Archivable.h>
#include <Point.h>
#include <Referenceable.h>


class KView;
class KTextView;


class KToolTip : public BArchivable, public BReferenceable {
public:
								KToolTip();
								KToolTip(BMessage* archive);
	virtual						~KToolTip();

	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	KView*				View() const = 0;

	virtual void				SetSticky(bool enable);
			bool				IsSticky() const;
	virtual void				SetMouseRelativeLocation(BPoint location);
			BPoint				MouseRelativeLocation() const;
	virtual void				SetAlignment(BAlignment alignment);
			BAlignment			Alignment() const;

	virtual	void				AttachedToWindow();
	virtual void				DetachedFromWindow();

protected:
			bool				Lock();
			void				Unlock();

private:
								KToolTip(const KToolTip& other);
			KToolTip&			operator=(const KToolTip &other);

			void				_InitData();

private:
			bool				fLockedLooper;
			bool				fIsSticky;
			BPoint				fRelativeLocation;
			BAlignment			fAlignment;
};


class KTextToolTip : public KToolTip {
public:
								KTextToolTip(const char* text);
								KTextToolTip(BMessage* archive);
	virtual						~KTextToolTip();

	static	KTextToolTip*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	KView*				View() const;

			const char*			Text() const;
			void				SetText(const char* text);

private:
			void				_InitData(const char* text);

private:
			KTextView*			fTextView;
};


#endif	// _TOOL_TIP_H
