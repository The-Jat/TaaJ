/*
 * Copyright 2007-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_MENU_H
#define _K_MENU_H


#include <InterfaceDefs.h>
#include <List.h>
#include <Laminate.h>


class KMenu;
class KMenuBar;
class KMenuItem;


namespace BPrivate {
	class KMenuWindow;
	class KExtraMenuData;
	class KTriggerList;
	class KMenuPrivate;
}

enum k_menu_layout {
	K_ITEMS_IN_ROW = 0,
	K_ITEMS_IN_COLUMN,
	K_ITEMS_IN_MATRIX
};

struct k_menu_info {
	float		font_size;
	font_family	f_family;
	font_style	f_style;
	rgb_color	background_color;
	int32		separator;
	bool		click_to_open;
	bool		triggers_always_shown;
};

status_t get_menu_info(k_menu_info* info);
status_t set_menu_info(k_menu_info* info);

typedef bool (*k_menu_tracking_hook)(KMenu* menu, void* state);


class KMenu : public KView {
public:
								KMenu(const char* name,
									k_menu_layout layout = K_ITEMS_IN_COLUMN);
								KMenu(const char* name, float width,
									float height);
								KMenu(BMessage* archive);

	virtual						~KMenu();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual void				AttachedToWindow();
	virtual void				DetachedFromWindow();
	virtual void				AllAttached();
	virtual void				AllDetached();

	virtual void				Draw(BRect updateRect);

	virtual void				MessageReceived(BMessage* message);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual void				ResizeToPreferred();
	virtual	void				DoLayout();
	virtual	void				FrameMoved(BPoint where);
	virtual	void				FrameResized(float width, float height);

			void				InvalidateLayout();

	virtual void				MakeFocus(bool focus = true);

			bool				AddItem(KMenuItem* item);
			bool				AddItem(KMenuItem* item, int32 index);
			bool				AddItem(KMenuItem* item, BRect frame);
			bool				AddItem(KMenu* menu);
			bool				AddItem(KMenu* menu, int32 index);
			bool				AddItem(KMenu* menu, BRect frame);
			bool				AddList(BList* list, int32 index);

			bool				AddSeparatorItem();

			bool				RemoveItem(KMenuItem* item);
			KMenuItem*			RemoveItem(int32 index);
			bool				RemoveItems(int32 index, int32 count,
									bool deleteItems = false);
			bool				RemoveItem(KMenu* menu);

			KMenuItem*			ItemAt(int32 index) const;
			KMenu*				SubmenuAt(int32 index) const;
			int32				CountItems() const;
			int32				IndexOf(KMenuItem* item) const;
			int32				IndexOf(KMenu* menu) const;
			KMenuItem*			FindItem(uint32 command) const;
			KMenuItem*			FindItem(const char* name) const;

	virtual	status_t			SetTargetForItems(BHandler* target);
	virtual	status_t			SetTargetForItems(BMessenger messenger);
	virtual	void				SetEnabled(bool enable);
	virtual	void				SetRadioMode(bool on);
	virtual	void				SetTriggersEnabled(bool enable);
	virtual	void				SetMaxContentWidth(float maxWidth);

			void				SetLabelFromMarked(bool on);
			bool				IsLabelFromMarked();
			bool				IsEnabled() const;
			bool				IsRadioMode() const;
			bool				AreTriggersEnabled() const;
			bool				IsRedrawAfterSticky() const;
			float				MaxContentWidth() const;

			KMenuItem*			FindMarked();
			int32				FindMarkedIndex();

			KMenu*				Supermenu() const;
			KMenuItem*			Superitem() const;


	virtual BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual status_t			GetSupportedSuites(BMessage* data);

	virtual status_t			Perform(perform_code d, void* arg);

protected:
								KMenu(BRect frame, const char* name,
									uint32 resizeMask, uint32 flags,
									k_menu_layout layout, bool resizeToFit);

	virtual	void				LayoutInvalidated(bool descendants);

	virtual	BPoint				ScreenLocation();

			void				SetItemMargins(float left, float top,
									float right, float bottom);
			void				GetItemMargins(float* _left, float* _top,
									float* _right, float* _bottom) const;

			k_menu_layout			Layout() const;

	virtual	void				Show();
			void				Show(bool selectFirstItem);
			void				Hide();
			KMenuItem*			Track(bool startOpened = false,
									BRect* specialRect = NULL);

public:
	enum add_state {
		B_INITIAL_ADD,
		B_PROCESSING,
		B_ABORT
	};
	virtual	bool				AddDynamicItem(add_state state);
	virtual	void				DrawBackground(BRect updateRect);

			void				SetTrackingHook(k_menu_tracking_hook hook,
									void* state);

private:
	friend class KMenuBar;
	friend class KSeparatorItem;
	friend class BPrivate::KMenuPrivate;
	friend status_t _init_interface_kit_();
	friend status_t	set_menu_info(k_menu_info* info);
	friend status_t	get_menu_info(k_menu_info* info);

	struct LayoutData;

	virtual	void				_ReservedMenu3();
	virtual	void				_ReservedMenu4();
	virtual	void				_ReservedMenu5();
	virtual	void				_ReservedMenu6();

			KMenu&				operator=(const KMenu& other);

			void				_InitData(BMessage* archive);
			bool				_Show(bool selectFirstItem = false,
									bool keyDown = false);
			void				_Hide();
			KMenuItem*			_Track(int* action, long start = -1);
			void				_ScriptReceived(BMessage* message);
			void				_ItemScriptReceived(BMessage* message,
									KMenuItem* item);
			status_t			_ResolveItemSpecifier(const BMessage& specifier,
									int32 what, KMenuItem*& item,
									int32 *index = NULL);
			status_t			_InsertItemAtSpecifier(
									const BMessage& specifier, int32 what,
									KMenuItem* item);

			void				_UpdateNavigationArea(BPoint position,
									BRect& navAreaRectAbove,
									BRect& navAreaBelow);

			void				_UpdateStateOpenSelect(KMenuItem* item,
									BPoint position, BRect& navAreaRectAbove,
									BRect& navAreaBelow,
									bigtime_t& selectedTime,
									bigtime_t& navigationAreaTime);
			void				_UpdateStateClose(KMenuItem* item,
									const BPoint& where,
									const uint32& buttons);

			bool				_AddItem(KMenuItem* item, int32 index);
			bool				_RemoveItems(int32 index, int32 count,
									KMenuItem* item, bool deleteItems = false);
			bool				_RelayoutIfNeeded();
			void				_LayoutItems(int32 index);
			BSize				_ValidatePreferredSize();
			void				_ComputeLayout(int32 index, bool bestFit,
									bool moveItems, float* width,
									float* height);
			void				_ComputeColumnLayout(int32 index, bool bestFit,
									bool moveItems, BRect* override, BRect& outRect);
			void				_ComputeRowLayout(int32 index, bool bestFit,
									bool moveItems, BRect& outRect);
			void				_ComputeMatrixLayout(BRect& outRect);

			BRect				_CalcFrame(BPoint where, bool* scrollOn);

protected:
			void				DrawItems(BRect updateRect);

private:
			bool				_OverSuper(BPoint loc);
			bool				_OverSubmenu(KMenuItem* item, BPoint loc);
			BPrivate::KMenuWindow* _MenuWindow();
			void				_DeleteMenuWindow();
			KMenuItem*			_HitTestItems(BPoint where,
									BPoint slop = B_ORIGIN) const;
			BRect				_Superbounds() const;
			void				_CacheFontInfo();

			void				_ItemMarked(KMenuItem* item);
			void				_Install(KWindow* target);
			void				_Uninstall();
			void				_SelectItem(KMenuItem* item,
									bool showSubmenu = true,
									bool selectFirstItem = false,
									bool keyDown = false);
			bool				_SelectNextItem(KMenuItem* item, bool forward);
			KMenuItem*			_NextItem(KMenuItem* item, bool forward) const;
			void				_SetIgnoreHidden(bool ignoreHidden)
									{ fIgnoreHidden = ignoreHidden; }
			void				_SetStickyMode(bool on);
			bool				_IsStickyMode() const;

			// Methods to get the current modifier keycode
			void				_GetShiftKey(uint32 &value) const;
			void				_GetControlKey(uint32 &value) const;
			void				_GetCommandKey(uint32 &value) const;
			void				_GetOptionKey(uint32 &value) const;
			void				_GetMenuKey(uint32 &value) const;

			void				_CalcTriggers();
			bool				_ChooseTrigger(const char* title, int32& index,
									uint32& trigger,
									BPrivate::KTriggerList& triggers);
			void				_UpdateWindowViewSize(const bool &updatePosition);
			bool				_AddDynamicItems(bool keyDown = false);
			bool				_OkToProceed(KMenuItem* item,
									bool keyDown = false);

			bool				_CustomTrackingWantsToQuit();

			int					_State(KMenuItem** _item = NULL) const;
			void				_InvokeItem(KMenuItem* item, bool now = false);
			void				_QuitTracking(bool onlyThis = true);

	static	k_menu_info			sMenuInfo;

			// Variables to keep track of what code is currently assigned to
			// each modifier key
	static	uint32				sShiftKey;
	static	uint32				sControlKey;
	static	uint32				sOptionKey;
	static	uint32				sCommandKey;
	static	uint32				sMenuKey;

			KMenuItem*			fChosenItem;
			BList				fItems;
			BRect				fPad;
			KMenuItem*			fSelected;
			BPrivate::KMenuWindow* fCachedMenuWindow;
			KMenu*				fSuper;
			KMenuItem*			fSuperitem;
			BRect				fSuperbounds;
			float				fAscent;
			float				fDescent;
			float				fFontHeight;
			uint32				fState;
			k_menu_layout			fLayout;
			BRect*				fExtraRect;
			float				fMaxContentWidth;
			BPoint*				fInitMatrixSize;
			BPrivate::KExtraMenuData* fExtraMenuData;

			LayoutData*			fLayoutData;

			int32				_reserved;

			char				fTrigger;
			bool				fResizeToFit;
			bool				fUseCachedMenuLayout;
			bool				fEnabled;
			bool				fDynamicName;
			bool				fRadioMode;
			bool				fTrackNewBounds;
			bool				fStickyMode;
			bool				fIgnoreHidden;
			bool				fTriggerEnabled;
			bool				fHasSubmenus;
			bool				fAttachAborted;
};

#endif // _MENU_H
