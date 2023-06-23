/*
 * Copyright 2003-2015, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_MENU_BAR_H
#define _K_MENU_BAR_H


#include <InterfaceDefs.h>
#include <KMenu.h>
#include <OS.h>


enum k_menu_bar_border {
	K_BORDER_FRAME,
	K_BORDER_CONTENTS,
	K_BORDER_EACH_ITEM
};

class KMenu;
//class BWindow;
class KWindow;//khidki
class KMenuItem;
class KMenuField;


class KMenuBar : public KMenu {
public:
								KMenuBar(BRect frame, const char* name,
									uint32 resizingMode = B_FOLLOW_LEFT_RIGHT
										| B_FOLLOW_TOP,
									k_menu_layout layout = K_ITEMS_IN_ROW,
									bool resizeToFit = true);
								KMenuBar(const char* name,
									k_menu_layout layout = K_ITEMS_IN_ROW,
									uint32 flags = B_WILL_DRAW
										| B_FRAME_EVENTS);
								KMenuBar(BMessage* archive);
	virtual						~KMenuBar();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual void				AllAttached();
	virtual void				AllDetached();

	virtual	void				WindowActivated(bool state);
	virtual void				MakeFocus(bool state = true);

	virtual void				ResizeToPreferred();
	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual	void				Show();
	virtual	void				Hide();

	virtual	void				Draw(BRect updateRect);

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual status_t			GetSupportedSuites(BMessage* data);

	virtual	void				SetBorder(k_menu_bar_border border);
			k_menu_bar_border		Border() const;
			void				SetBorders(uint32 borders);
			uint32				Borders() const;

	virtual status_t			Perform(perform_code code, void* data);

protected:
			void				StartMenuBar(int32 menuIndex,
									bool sticky = true, bool showMenu = false,
									BRect* special_rect = NULL);

private:
	//friend class BWindow;
		friend class KWindow;//khidki
	friend class KMenuField;
	friend class KMenu;

	virtual	void				_ReservedMenuBar1();
	virtual	void				_ReservedMenuBar2();
	virtual	void				_ReservedMenuBar3();
	virtual	void				_ReservedMenuBar4();

			KMenuBar			&operator=(const KMenuBar &);

	static	int32				_TrackTask(void *arg);
			KMenuItem*			_Track(int32 *action, int32 startIndex = -1,
									bool showMenu = false);
			void				_StealFocus();
			void				_RestoreFocus();
			void				_InitData(k_menu_layout layout);

			k_menu_bar_border		fBorder;
			thread_id			fTrackingPID;
			int32				fPrevFocusToken;
			sem_id				fMenuSem;
			BRect*				fLastBounds;
			uint32				fBorders;
			uint32				_reserved[1];

			bool				fTracking;
};


#endif /* _MENU_BAR_H */
