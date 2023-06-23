/*
 * Copyright 2001-2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_CONTROL_H
#define _K_CONTROL_H

#include <Invoker.h>
#include <Message.h>	// For convenience
#include <Laminate.h>


enum {
	K_CONTROL_OFF = 0,
	K_CONTROL_ON = 1,
	K_CONTROL_PARTIALLY_ON = 2
};

class KBitmap;
class KWindow;

namespace BPrivate {
	class KIcon;
};


class KControl : public KView, public BInvoker {
public:
								KControl(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizingMode, uint32 flags);
								KControl(const char* name, const char* label,
									BMessage* message, uint32 flags);
	virtual						~KControl();

								KControl(BMessage* data);
	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

	virtual	void				WindowActivated(bool active);

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				AllAttached();
	virtual	void				AllDetached();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MakeFocus(bool focus = true);

	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage);

	virtual	void				SetLabel(const char* string);
			const char*			Label() const;

	virtual	void				SetValue(int32 value);
			int32				Value() const;

	virtual	void				SetEnabled(bool enabled);
			bool				IsEnabled() const;

	virtual	void				GetPreferredSize(float* _width,
									float* _height);
	virtual	void				ResizeToPreferred();

	virtual	status_t			Invoke(BMessage* message = NULL);
	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* message);

	virtual	status_t			Perform(perform_code d, void* arg);

	virtual	status_t			SetIcon(const KBitmap* bitmap,
									uint32 flags = 0);
			status_t			SetIconBitmap(const KBitmap* bitmap,
									uint32 which, uint32 flags = 0);
			const KBitmap*		IconBitmap(uint32 which) const;

protected:
			bool				IsFocusChanging() const;
			bool				IsTracking() const;
			void				SetTracking(bool state);

			void				SetValueNoUpdate(int32 value);

private:
			struct IconData;

private:
	virtual	void				_ReservedControl2();
	virtual	void				_ReservedControl3();
	virtual	void				_ReservedControl4();

			KControl&			operator=(const KControl&);

			void				InitData(BMessage* data = NULL);

private:
			char*				fLabel;
			int32				fValue;
			bool				fEnabled;
			bool				fFocusChanging;
			bool				fTracking;
			bool				fWantsNav;
			BPrivate::KIcon*	fIcon;

#ifdef B_HAIKU_64_BIT
			uint32				_reserved[2];
#else
			uint32				_reserved[3];
#endif
};

#endif // _CONTROL_H
