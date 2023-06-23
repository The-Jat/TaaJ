/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	K__ALERT_H
#define	K__ALERT_H


#include <vector>

#include <Khidki.h>


// enum for flavors of alert
enum alert_type {
	B_EMPTY_ALERT = 0,
	B_INFO_ALERT,
	B_IDEA_ALERT,
	B_WARNING_ALERT,
	B_STOP_ALERT
};

enum button_spacing {
	B_EVEN_SPACING = 0,
	B_OFFSET_SPACING
};


class KBitmap;
class KButton;
class KGroupLayout;
class BInvoker;
class KTextView;
class KTAlertView;


class KAlert : public KWindow {
public:
								KAlert();
								KAlert(const char* title, const char* text,
									const char* button1,
									const char* button2 = NULL,
									const char* button3 = NULL,
									button_width width = B_WIDTH_AS_USUAL,
									alert_type type = B_INFO_ALERT);
								KAlert(const char* title, const char* text,
									const char* button1, const char* button2,
									const char* button3, button_width width,
									button_spacing spacing,
									alert_type type = B_INFO_ALERT);
								KAlert(BMessage* data);
	virtual						~KAlert();

	// Archiving
	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

	// KAlert guts
			alert_type			Type() const;
			void				SetType(alert_type type);
			void				SetIcon(KBitmap* bitmap);
			void				SetText(const char* text);
			void				SetButtonSpacing(button_spacing spacing);
			void				SetButtonWidth(button_width width);

			void				SetShortcut(int32 buttonIndex, char key);
			char				Shortcut(int32 buttonIndex) const;

			int32				Go();
			status_t			Go(BInvoker* invoker);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				FrameResized(float newWidth, float newHeight);

			void				AddButton(const char* label, char key = 0);
			int32				CountButtons() const;
			KButton*			ButtonAt(int32 index) const;

			KTextView*			TextView() const;

	virtual BHandler*			ResolveSpecifier(BMessage* message, int32 index,
									BMessage* specifier, int32 form,
									const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual void				DispatchMessage(BMessage* message,
									BHandler* handler);
	virtual	void				Quit();
	virtual	bool				QuitRequested();

	virtual status_t			Perform(perform_code d, void* arg);

	static	BPoint				AlertPosition(float width, float height);

private:
	virtual	void				_ReservedAlert1();
	virtual	void				_ReservedAlert2();
	virtual	void				_ReservedAlert3();

			void				_Init(const char* text, const char* button1,
									const char* button2, const char* button3,
									button_width width, button_spacing spacing,
									alert_type type);
			KBitmap*			_CreateTypeIcon();
			KButton*			_CreateButton(int32 which, const char* label);
			void				_Prepare();

private:
			sem_id				fAlertSem;
			int32				fAlertValue;
			KTAlertView*			fIconView;
			KTextView*			fTextView;
			KGroupLayout*		fButtonLayout;
			std::vector<KButton*> fButtons;
			std::vector<char>	fKeys;
			uint8				fType;
			uint8				fButtonSpacing;
			uint8				fButtonWidth;
			BInvoker*			fInvoker;
			uint32				_reserved[1];
};


#endif	// _ALERT_H
