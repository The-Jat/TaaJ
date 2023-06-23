/*
 * Copyright 2006-2007, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef K__MENU_ITEM_H
#define K__MENU_ITEM_H


#include <Archivable.h>
#include <InterfaceDefs.h>
#include <Invoker.h>
#include <KMenu.h>


class BMessage;
class KWindow;

namespace BPrivate {
	class KMenuItemPrivate;
}

class KMenuItem : public BArchivable, public BInvoker {
public:
								KMenuItem(const char* label, BMessage* message,
									char shortcut = 0, uint32 modifiers = 0);
								KMenuItem(KMenu* menu,
									BMessage* message = NULL);
								KMenuItem(BMessage* data);
	virtual						~KMenuItem();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				SetLabel(const char* name);
	virtual	void				SetEnabled(bool enable);
	virtual	void				SetMarked(bool mark);
	virtual	void				SetTrigger(char trigger);
	virtual	void				SetShortcut(char shortcut, uint32 modifiers);

			const char*			Label() const;
			bool				IsEnabled() const;
			bool				IsMarked() const;
			char				Trigger() const;
			char				Shortcut(uint32* _modifiers = NULL) const;

			KMenu*				Submenu() const;
			KMenu*				Menu() const;
			BRect				Frame() const;

protected:
	virtual	void				GetContentSize(float* _width, float* _height);
	virtual	void				TruncateLabel(float maxWidth, char* newLabel);
	virtual	void				DrawContent();
	virtual	void				Draw();
	virtual	void				Highlight(bool highlight);
			bool				IsSelected() const;
			BPoint				ContentLocation() const;

private:
	friend class KMenu;
	friend class KPopUpMenu;
	friend class KMenuBar;
	friend class BPrivate::KMenuItemPrivate;

	virtual	void				_ReservedMenuItem1();
	virtual	void				_ReservedMenuItem2();
	virtual	void				_ReservedMenuItem3();
	virtual	void				_ReservedMenuItem4();

			void				Install(KWindow* window);
			void				Uninstall();
			void				SetSuper(KMenu* superMenu);
			void				Select(bool select);
			void				SetAutomaticTrigger(int32 index,
									uint32 trigger);

protected:
	virtual	status_t			Invoke(BMessage* message = NULL);

private:
								KMenuItem(const KMenuItem& other);
			KMenuItem&			operator=(const KMenuItem& other);

private:
			void				_InitData();
			void				_InitMenuData(KMenu* menu);

			bool				_IsActivated();
			rgb_color			_LowColor();
			rgb_color			_HighColor();

			void				_DrawMarkSymbol();
			void				_DrawShortcutSymbol(bool);
			void				_DrawSubmenuSymbol();
			void				_DrawControlChar(char shortcut, BPoint where);

private:
			char*				fLabel;
			KMenu*				fSubmenu;
			KWindow*			fWindow;
			KMenu*				fSuper;
			BRect				fBounds;
			uint32				fModifiers;
			float				fCachedWidth;
			int16				fTriggerIndex;
			char				fUserTrigger;
			char				fShortcutChar;
			bool				fMark;
			bool				fEnabled;
			bool				fSelected;
			uint32				fTrigger;

			uint32				_reserved[3];
};

// KSeparatorItem now has its own declaration file, but for source
// compatibility we're exporting that class from here too.
#include <KSeparatorItem.h>

#endif // _MENU_ITEM_H
