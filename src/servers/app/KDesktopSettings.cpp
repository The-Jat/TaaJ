/*
 * Copyright 2005-2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Andrej Spielmann, <andrej.spielmann@seh.ox.ac.uk>
 *		Joseph Groover <looncraz@looncraz.net>
 */


#include "KDesktopSettings.h"
#include "KDesktopSettingsPrivate.h"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include <DefaultColors.h>
#include <InterfaceDefs.h>
#include <ServerReadOnlyMemory.h>

#include "Desktop.h"
#include "FontCache.h"
#include "FontCacheEntry.h"
#include "GlobalFontManager.h"
#include "GlobalSubpixelSettings.h"
#include "ServerConfig.h"


K_DesktopSettingsPrivate::K_DesktopSettingsPrivate(server_read_only_memory* shared)
	:
	fShared(*shared)
{
	// if the on-disk settings are not complete, the defaults will be kept
	_SetDefaults();
	_Load();
}


K_DesktopSettingsPrivate::~K_DesktopSettingsPrivate()
{
}


void
K_DesktopSettingsPrivate::_SetDefaults()
{
	fPlainFont = *gFontManager->DefaultPlainFont();
	fBoldFont = *gFontManager->DefaultBoldFont();
	fFixedFont = *gFontManager->DefaultFixedFont();

	fMouseMode = B_NORMAL_MOUSE;
	fFocusFollowsMouseMode = B_NORMAL_FOCUS_FOLLOWS_MOUSE;
	fAcceptFirstClick = true;
	fShowAllDraggers = true;

	// init scrollbar info
	fScrollBarInfo.proportional = true;
	fScrollBarInfo.double_arrows = false;
	fScrollBarInfo.knob = 0;
		// look of the knob (R5: (0, 1, 2), 1 = default)
		// change default = 0 (no knob) in Haiku
	fScrollBarInfo.min_knob_size = 15;

	// init menu info
	strlcpy(fMenuInfo.f_family, fPlainFont.Family(), B_FONT_FAMILY_LENGTH);
	strlcpy(fMenuInfo.f_style, fPlainFont.Style(), B_FONT_STYLE_LENGTH);
	fMenuInfo.font_size = fPlainFont.Size();
	fMenuInfo.background_color.set_to(216, 216, 216);

	fMenuInfo.separator = 0;
		// look of the separator (R5: (0, 1, 2), default 0)
	fMenuInfo.click_to_open = true; // always true
	fMenuInfo.triggers_always_shown = false;

	fWorkspacesColumns = 2;
	fWorkspacesRows = 2;

	memcpy((void*)fShared.colors, BPrivate::defaultLightModeColors,
		sizeof(rgb_color) * kLightColorSchemeWhichCount);

	gSubpixelAntialiasing = true;
	gDefaultHintingMode = HINTING_MODE_ON;
	gSubpixelAverageWeight = 120;
	gSubpixelOrderingRGB = true;
}


status_t
K_DesktopSettingsPrivate::_GetPath(BPath& path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status < B_OK)
		return status;

	status = path.Append("system/app_server");
	if (status < B_OK)
		return status;

	return create_directory(path.Path(), 0755);
}


status_t
K_DesktopSettingsPrivate::_Load()
{
	// TODO: add support for old app_server_settings file as well

	BPath basePath;
	status_t status = _GetPath(basePath);
	if (status < B_OK)
		return status;

	// read workspaces settings

	BPath path(basePath);
	path.Append("workspaces");

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		BMessage settings;
		status = settings.Unflatten(&file);
		if (status == B_OK) {
			int32 columns;
			int32 rows;
			if (settings.FindInt32("columns", &columns) == B_OK
				&& settings.FindInt32("rows", &rows) == B_OK) {
				_ValidateWorkspacesLayout(columns, rows);
				fWorkspacesColumns = columns;
				fWorkspacesRows = rows;
			}

			int32 i = 0;
			while (i < kMaxWorkspaces && settings.FindMessage("workspace",
					i, &fWorkspaceMessages[i]) == B_OK) {
				i++;
			}
		}
	}

	// read font settings

	path = basePath;
	path.Append("fonts");

	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		BMessage settings;
		status = settings.Unflatten(&file);
		if (status != B_OK) {
			fFontSettingsLoadStatus = status;
		} else if (gFontManager->Lock()) {
			const char* family;
			const char* style;
			float size;

			if (settings.FindString("plain family", &family) == B_OK
				&& settings.FindString("plain style", &style) == B_OK
				&& settings.FindFloat("plain size", &size) == B_OK) {
				FontStyle* fontStyle = gFontManager->GetStyle(family, style);
				fPlainFont.SetStyle(fontStyle);
				fPlainFont.SetSize(size);
			}

			if (settings.FindString("bold family", &family) == B_OK
				&& settings.FindString("bold style", &style) == B_OK
				&& settings.FindFloat("bold size", &size) == B_OK) {
				FontStyle* fontStyle = gFontManager->GetStyle(family, style);
				fBoldFont.SetStyle(fontStyle);
				fBoldFont.SetSize(size);
			}

			if (settings.FindString("fixed family", &family) == B_OK
				&& settings.FindString("fixed style", &style) == B_OK
				&& settings.FindFloat("fixed size", &size) == B_OK) {
				FontStyle* fontStyle = gFontManager->GetStyle(family, style);
				if (fontStyle != NULL && (fontStyle->IsFixedWidth()
						|| fontStyle->IsFullAndHalfFixed()))
					fFixedFont.SetStyle(fontStyle);
				fFixedFont.SetSize(size);
			}

			int32 hinting;
			if (settings.FindInt32("hinting", &hinting) == B_OK)
				gDefaultHintingMode = hinting;

			gFontManager->Unlock();
		} else
			fFontSettingsLoadStatus = EWOULDBLOCK;
	} else
		fFontSettingsLoadStatus = status;

	// read mouse settings

	path = basePath;
	path.Append("mouse");

	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		BMessage settings;
		status = settings.Unflatten(&file);
		if (status == B_OK) {
			int32 mode;
			if (settings.FindInt32("mode", &mode) == B_OK)
				fMouseMode = (mode_mouse)mode;

			int32 focusFollowsMouseMode;
			if (settings.FindInt32("focus follows mouse mode",
					&focusFollowsMouseMode) == B_OK) {
				fFocusFollowsMouseMode
					= (mode_focus_follows_mouse)focusFollowsMouseMode;
			}

			bool acceptFirstClick;
			if (settings.FindBool("accept first click", &acceptFirstClick)
					== B_OK) {
				fAcceptFirstClick = acceptFirstClick;
			}
		}
	}

	// read appearance settings

	path = basePath;
	path.Append("appearance");

	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		BMessage settings;
		status = settings.Unflatten(&file);
		if (status == B_OK) {
			// menus
			float fontSize;
			if (settings.FindFloat("font size", &fontSize) == B_OK)
				fMenuInfo.font_size = fontSize;

			const char* fontFamily;
			if (settings.FindString("font family", &fontFamily) == B_OK)
				strlcpy(fMenuInfo.f_family, fontFamily, B_FONT_FAMILY_LENGTH);

			const char* fontStyle;
			if (settings.FindString("font style", &fontStyle) == B_OK)
				strlcpy(fMenuInfo.f_style, fontStyle, B_FONT_STYLE_LENGTH);

			rgb_color bgColor;
			if (settings.FindInt32("bg color", (int32*)&bgColor) == B_OK)
				fMenuInfo.background_color = bgColor;

			int32 separator;
			if (settings.FindInt32("separator", &separator) == B_OK)
				fMenuInfo.separator = separator;

			bool clickToOpen;
			if (settings.FindBool("click to open", &clickToOpen) == B_OK)
				fMenuInfo.click_to_open = clickToOpen;

			bool triggersAlwaysShown;
			if (settings.FindBool("triggers always shown", &triggersAlwaysShown)
					 == B_OK) {
				fMenuInfo.triggers_always_shown = triggersAlwaysShown;
			}

			// scrollbars
			bool proportional;
			if (settings.FindBool("proportional", &proportional) == B_OK)
				fScrollBarInfo.proportional = proportional;

			bool doubleArrows;
			if (settings.FindBool("double arrows", &doubleArrows) == B_OK)
				fScrollBarInfo.double_arrows = doubleArrows;

			int32 knob;
			if (settings.FindInt32("knob", &knob) == B_OK)
				fScrollBarInfo.knob = knob;

			int32 minKnobSize;
			if (settings.FindInt32("min knob size", &minKnobSize) == B_OK)
				fScrollBarInfo.min_knob_size = minKnobSize;

			// subpixel font rendering
			bool subpix;
			if (settings.FindBool("subpixel antialiasing", &subpix) == B_OK)
				gSubpixelAntialiasing = subpix;

			int8 averageWeight;
			if (settings.FindInt8("subpixel average weight", &averageWeight)
					== B_OK) {
				gSubpixelAverageWeight = averageWeight;
			}

			bool subpixelOrdering;
			if (settings.FindBool("subpixel ordering", &subpixelOrdering)
					== B_OK) {
				gSubpixelOrderingRGB = subpixelOrdering;
			}

			const char* controlLook;
			if (settings.FindString("control look", &controlLook) == B_OK) {
				fControlLook = controlLook;
			}

			// colors
			for (int32 i = 0; i < kLightColorSchemeWhichCount; i++) {
				char colorName[12];
				snprintf(colorName, sizeof(colorName), "color%" B_PRId32,
					(int32)index_to_light_color_scheme_which(i));

				if (settings.FindInt32(colorName, (int32*)&fShared.colors[i]) != B_OK) {
					// Set obviously bad value so the Appearance app can detect it
					fShared.colors[i] = B_TRANSPARENT_COLOR;
				}
			}
		}
	}

	// read dragger settings

	path = basePath;
	path.Append("dragger");

	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status == B_OK) {
		BMessage settings;
		status = settings.Unflatten(&file);
		if (status == B_OK) {
			if (settings.FindBool("show", &fShowAllDraggers) != B_OK)
				fShowAllDraggers = true;
		}
	}

	return B_OK;
}


status_t
K_DesktopSettingsPrivate::Save(uint32 mask)
{
#if TEST_MODE
	return B_OK;
#endif

	BPath basePath;
	status_t status = _GetPath(basePath);
	if (status != B_OK)
		return status;

	if (mask & k_kWorkspacesSettings) {
		BPath path(basePath);
		if (path.Append("workspaces") == B_OK) {
			BMessage settings('asws');
			settings.AddInt32("columns", fWorkspacesColumns);
			settings.AddInt32("rows", fWorkspacesRows);

			for (int32 i = 0; i < kMaxWorkspaces; i++) {
				settings.AddMessage("workspace", &fWorkspaceMessages[i]);
			}

			BFile file;
			status = file.SetTo(path.Path(), B_CREATE_FILE | B_ERASE_FILE
				| B_READ_WRITE);
			if (status == B_OK) {
				status = settings.Flatten(&file, NULL);
			}
		}
	}

	if (mask & k_kFontSettings) {
		BPath path(basePath);
		if (path.Append("fonts") == B_OK) {
			BMessage settings('asfn');

			settings.AddString("plain family", fPlainFont.Family());
			settings.AddString("plain style", fPlainFont.Style());
			settings.AddFloat("plain size", fPlainFont.Size());

			settings.AddString("bold family", fBoldFont.Family());
			settings.AddString("bold style", fBoldFont.Style());
			settings.AddFloat("bold size", fBoldFont.Size());

			settings.AddString("fixed family", fFixedFont.Family());
			settings.AddString("fixed style", fFixedFont.Style());
			settings.AddFloat("fixed size", fFixedFont.Size());

			settings.AddInt32("hinting", gDefaultHintingMode);

			BFile file;
			status = file.SetTo(path.Path(), B_CREATE_FILE | B_ERASE_FILE
				| B_READ_WRITE);
			if (status == B_OK) {
				status = settings.Flatten(&file, NULL);
			}
		}
	}

	if (mask & k_kMouseSettings) {
		BPath path(basePath);
		if (path.Append("mouse") == B_OK) {
			BMessage settings('asms');
			settings.AddInt32("mode", (int32)fMouseMode);
			settings.AddInt32("focus follows mouse mode",
				(int32)fFocusFollowsMouseMode);
			settings.AddBool("accept first click", fAcceptFirstClick);

			BFile file;
			status = file.SetTo(path.Path(), B_CREATE_FILE | B_ERASE_FILE
				| B_READ_WRITE);
			if (status == B_OK) {
				status = settings.Flatten(&file, NULL);
			}
		}
	}

	if (mask & k_kDraggerSettings) {
		BPath path(basePath);
		if (path.Append("dragger") == B_OK) {
			BMessage settings('asdg');
			settings.AddBool("show", fShowAllDraggers);

			BFile file;
			status = file.SetTo(path.Path(), B_CREATE_FILE | B_ERASE_FILE
				| B_READ_WRITE);
			if (status == B_OK) {
				status = settings.Flatten(&file, NULL);
			}
		}
	}

	if (mask & k_kAppearanceSettings) {
		BPath path(basePath);
		if (path.Append("appearance") == B_OK) {
			BMessage settings('aslk');
			settings.AddFloat("font size", fMenuInfo.font_size);
			settings.AddString("font family", fMenuInfo.f_family);
			settings.AddString("font style", fMenuInfo.f_style);
			settings.AddInt32("bg color",
				(const int32&)fMenuInfo.background_color);
			settings.AddInt32("separator", fMenuInfo.separator);
			settings.AddBool("click to open", fMenuInfo.click_to_open);
			settings.AddBool("triggers always shown",
				fMenuInfo.triggers_always_shown);

			settings.AddBool("proportional", fScrollBarInfo.proportional);
			settings.AddBool("double arrows", fScrollBarInfo.double_arrows);
			settings.AddInt32("knob", fScrollBarInfo.knob);
			settings.AddInt32("min knob size", fScrollBarInfo.min_knob_size);

			settings.AddBool("subpixel antialiasing", gSubpixelAntialiasing);
			settings.AddInt8("subpixel average weight", gSubpixelAverageWeight);
			settings.AddBool("subpixel ordering", gSubpixelOrderingRGB);

			settings.AddString("control look", fControlLook);

			for (int32 i = 0; i < kLightColorSchemeWhichCount; i++) {
				char colorName[12];
				snprintf(colorName, sizeof(colorName), "color%" B_PRId32,
					(int32)index_to_light_color_scheme_which(i));
				settings.AddInt32(colorName, (const int32&)fShared.colors[i]);
			}

			BFile file;
			status = file.SetTo(path.Path(), B_CREATE_FILE | B_ERASE_FILE
				| B_READ_WRITE);
			if (status == B_OK) {
				status = settings.Flatten(&file, NULL);
			}
		}
	}

	return status;
}


void
K_DesktopSettingsPrivate::SetDefaultPlainFont(const ServerFont &font)
{
	fPlainFont = font;
	Save(k_kFontSettings);
}


const ServerFont &
K_DesktopSettingsPrivate::DefaultPlainFont() const
{
	return fPlainFont;
}


void
K_DesktopSettingsPrivate::SetDefaultBoldFont(const ServerFont &font)
{
	fBoldFont = font;
	Save(k_kFontSettings);
}


const ServerFont &
K_DesktopSettingsPrivate::DefaultBoldFont() const
{
	return fBoldFont;
}


void
K_DesktopSettingsPrivate::SetDefaultFixedFont(const ServerFont &font)
{
	fFixedFont = font;
	Save(k_kFontSettings);
}


const ServerFont &
K_DesktopSettingsPrivate::DefaultFixedFont() const
{
	return fFixedFont;
}


void
K_DesktopSettingsPrivate::SetScrollBarInfo(const scroll_bar_info& info)
{
	fScrollBarInfo = info;
	Save(k_kAppearanceSettings);
}


const scroll_bar_info&
K_DesktopSettingsPrivate::ScrollBarInfo() const
{
	return fScrollBarInfo;
}


void
K_DesktopSettingsPrivate::SetMenuInfo(const menu_info& info)
{
	fMenuInfo = info;
	// Also update the ui_color
	SetUIColor(K_MENU_BACKGROUND_COLOR, info.background_color);
		// SetUIColor already saves the settings
}


const menu_info&
K_DesktopSettingsPrivate::MenuInfo() const
{
	return fMenuInfo;
}


void
K_DesktopSettingsPrivate::SetMouseMode(const mode_mouse mode)
{
	fMouseMode = mode;
	Save(k_kMouseSettings);
}


void
K_DesktopSettingsPrivate::SetFocusFollowsMouseMode(mode_focus_follows_mouse mode)
{
	fFocusFollowsMouseMode = mode;
	Save(k_kMouseSettings);
}


mode_mouse
K_DesktopSettingsPrivate::MouseMode() const
{
	return fMouseMode;
}


mode_focus_follows_mouse
K_DesktopSettingsPrivate::FocusFollowsMouseMode() const
{
	return fFocusFollowsMouseMode;
}


void
K_DesktopSettingsPrivate::SetAcceptFirstClick(const bool acceptFirstClick)
{
	fAcceptFirstClick = acceptFirstClick;
	Save(k_kMouseSettings);
}


bool
K_DesktopSettingsPrivate::AcceptFirstClick() const
{
	return fAcceptFirstClick;
}


void
K_DesktopSettingsPrivate::SetShowAllDraggers(bool show)
{
	fShowAllDraggers = show;
	Save(k_kDraggerSettings);
}


bool
K_DesktopSettingsPrivate::ShowAllDraggers() const
{
	return fShowAllDraggers;
}


void
K_DesktopSettingsPrivate::SetWorkspacesLayout(int32 columns, int32 rows)
{
	_ValidateWorkspacesLayout(columns, rows);
	fWorkspacesColumns = columns;
	fWorkspacesRows = rows;

	Save(k_kWorkspacesSettings);
}


int32
K_DesktopSettingsPrivate::WorkspacesCount() const
{
	return fWorkspacesColumns * fWorkspacesRows;
}


int32
K_DesktopSettingsPrivate::WorkspacesColumns() const
{
	return fWorkspacesColumns;
}


int32
K_DesktopSettingsPrivate::WorkspacesRows() const
{
	return fWorkspacesRows;
}


void
K_DesktopSettingsPrivate::SetWorkspacesMessage(int32 index, BMessage& message)
{
	if (index < 0 || index >= kMaxWorkspaces)
		return;

	fWorkspaceMessages[index] = message;
}


const BMessage*
K_DesktopSettingsPrivate::WorkspacesMessage(int32 index) const
{
	if (index < 0 || index >= kMaxWorkspaces)
		return NULL;

	return &fWorkspaceMessages[index];
}


void
K_DesktopSettingsPrivate::SetUIColor(light_color_scheme_which which, const rgb_color color,
									bool* changed)
{
	int32 index = light_color_scheme_which_to_index(which);
	if (index < 0 || index >= kLightColorSchemeWhichCount)
		return;

	if (changed != NULL)
		*changed = fShared.colors[index] != color;

	fShared.colors[index] = color;
	// TODO: deprecate the background_color member of the menu_info struct,
	// otherwise we have to keep this duplication...
	if (which == K_MENU_BACKGROUND_COLOR)
		fMenuInfo.background_color = color;

	Save(k_kAppearanceSettings);
}


void
K_DesktopSettingsPrivate::SetUIColors(const BMessage& colors, bool* changed)
{
	int32 count = colors.CountNames(B_RGB_32_BIT_TYPE);
	if (count <= 0)
		return;

	int32 index = 0;
	int32 colorIndex = 0;
	char* name = NULL;
	type_code type;
	rgb_color color;
	light_color_scheme_which which = K_NO_COLOR;

	while (colors.GetInfo(B_RGB_32_BIT_TYPE, index, &name, &type) == B_OK) {
		which = get_light_color_scheme_which_from_name(name);
		colorIndex = light_color_scheme_which_to_index(which);
		if (colorIndex < 0 || colorIndex >= kLightColorSchemeWhichCount
			|| colors.FindColor(name, &color) != B_OK) {
			if (changed != NULL)
				changed[index] = false;

			++index;
			continue;
		}

		if (changed != NULL)
			changed[index] = fShared.colors[colorIndex] != color;

		fShared.colors[colorIndex] = color;

		if (which == (int32)B_MENU_BACKGROUND_COLOR)
			fMenuInfo.background_color = color;

		++index;
	}

	Save(k_kAppearanceSettings);
}


rgb_color
K_DesktopSettingsPrivate::UIColor(light_color_scheme_which which) const
{
	static const rgb_color invalidColor = {0, 0, 0, 0};
	int32 index = light_color_scheme_which_to_index(which);
	if (index < 0 || index >= kLightColorSchemeWhichCount)
		return invalidColor;

	return fShared.colors[index];
}


void
K_DesktopSettingsPrivate::SetSubpixelAntialiasing(bool subpix)
{
	gSubpixelAntialiasing = subpix;
	Save(k_kAppearanceSettings);
}


bool
K_DesktopSettingsPrivate::SubpixelAntialiasing() const
{
	return gSubpixelAntialiasing;
}


void
K_DesktopSettingsPrivate::SetHinting(uint8 hinting)
{
	gDefaultHintingMode = hinting;
	Save(k_kFontSettings);
}


uint8
K_DesktopSettingsPrivate::Hinting() const
{
	return gDefaultHintingMode;
}


void
K_DesktopSettingsPrivate::SetSubpixelAverageWeight(uint8 averageWeight)
{
	gSubpixelAverageWeight = averageWeight;
	Save(k_kAppearanceSettings);
}


uint8
K_DesktopSettingsPrivate::SubpixelAverageWeight() const
{
	return gSubpixelAverageWeight;
}


void
K_DesktopSettingsPrivate::SetSubpixelOrderingRegular(bool subpixelOrdering)
{
	gSubpixelOrderingRGB = subpixelOrdering;
	Save(k_kAppearanceSettings);
}


bool
K_DesktopSettingsPrivate::IsSubpixelOrderingRegular() const
{
	return gSubpixelOrderingRGB;
}


status_t
K_DesktopSettingsPrivate::SetControlLook(const char* path)
{
	fControlLook = path;
	return Save(k_kAppearanceSettings);
}


const BString&
K_DesktopSettingsPrivate::ControlLook() const
{
	return fControlLook;
}


void
K_DesktopSettingsPrivate::_ValidateWorkspacesLayout(int32& columns,
	int32& rows) const
{
	if (columns < 1)
		columns = 1;
	if (rows < 1)
		rows = 1;

	if (columns * rows > kMaxWorkspaces) {
		// Revert to defaults in case of invalid settings
		columns = 2;
		rows = 2;
	}
}


//	#pragma mark - read access


K_DesktopSettings::K_DesktopSettings(Desktop* desktop)
	:
	fSettings(desktop->khidkiSettings.Get())
{

}


void
K_DesktopSettings::GetDefaultPlainFont(ServerFont &font) const
{
	font = fSettings->DefaultPlainFont();
}


void
K_DesktopSettings::GetDefaultBoldFont(ServerFont &font) const
{
	font = fSettings->DefaultBoldFont();
}


void
K_DesktopSettings::GetDefaultFixedFont(ServerFont &font) const
{
	font = fSettings->DefaultFixedFont();
}


void
K_DesktopSettings::GetScrollBarInfo(scroll_bar_info& info) const
{
	info = fSettings->ScrollBarInfo();
}


void
K_DesktopSettings::GetMenuInfo(menu_info& info) const
{
	info = fSettings->MenuInfo();
}


mode_mouse
K_DesktopSettings::MouseMode() const
{
	return fSettings->MouseMode();
}


mode_focus_follows_mouse
K_DesktopSettings::FocusFollowsMouseMode() const
{
	return fSettings->FocusFollowsMouseMode();
}


bool
K_DesktopSettings::AcceptFirstClick() const
{
	return fSettings->AcceptFirstClick();
}


bool
K_DesktopSettings::ShowAllDraggers() const
{
	return fSettings->ShowAllDraggers();
}


int32
K_DesktopSettings::WorkspacesCount() const
{
	return fSettings->WorkspacesCount();
}


int32
K_DesktopSettings::WorkspacesColumns() const
{
	return fSettings->WorkspacesColumns();
}


int32
K_DesktopSettings::WorkspacesRows() const
{
	return fSettings->WorkspacesRows();
}


const BMessage*
K_DesktopSettings::WorkspacesMessage(int32 index) const
{
	return fSettings->WorkspacesMessage(index);
}


rgb_color
K_DesktopSettings::UIColor(light_color_scheme_which which) const
{
	return fSettings->UIColor(which);
}


bool
K_DesktopSettings::SubpixelAntialiasing() const
{
	return fSettings->SubpixelAntialiasing();
}


uint8
K_DesktopSettings::Hinting() const
{
	return fSettings->Hinting();
}


uint8
K_DesktopSettings::SubpixelAverageWeight() const
{
	return fSettings->SubpixelAverageWeight();
}


bool
K_DesktopSettings::IsSubpixelOrderingRegular() const
{
	// True corresponds to RGB, false means BGR
	return fSettings->IsSubpixelOrderingRegular();
}


const BString&
K_DesktopSettings::ControlLook() const
{
	return fSettings->ControlLook();
}

//	#pragma mark - write access


K_LockedDesktopSettings::K_LockedDesktopSettings(Desktop* desktop)
	:
	K_DesktopSettings(desktop),
	fDesktop(desktop)
{
#if DEBUG
	if (desktop->fWindowLock.IsReadLocked())
		debugger("desktop read locked when trying to change settings");
#endif

	fDesktop->LockAllWindows();
}


K_LockedDesktopSettings::~K_LockedDesktopSettings()
{
	fDesktop->UnlockAllWindows();
}


void
K_LockedDesktopSettings::SetDefaultPlainFont(const ServerFont &font)
{
	fSettings->SetDefaultPlainFont(font);
}


void
K_LockedDesktopSettings::SetDefaultBoldFont(const ServerFont &font)
{
	fSettings->SetDefaultBoldFont(font);
	fDesktop->BroadcastToAllWindows(AS_SYSTEM_FONT_CHANGED);
}


void
K_LockedDesktopSettings::SetDefaultFixedFont(const ServerFont &font)
{
	fSettings->SetDefaultFixedFont(font);
}


void
K_LockedDesktopSettings::SetScrollBarInfo(const scroll_bar_info& info)
{
	fSettings->SetScrollBarInfo(info);
}


void
K_LockedDesktopSettings::SetMenuInfo(const menu_info& info)
{
	fSettings->SetMenuInfo(info);
}


void
K_LockedDesktopSettings::SetMouseMode(const mode_mouse mode)
{
	fSettings->SetMouseMode(mode);
}


void
K_LockedDesktopSettings::SetFocusFollowsMouseMode(mode_focus_follows_mouse mode)
{
	fSettings->SetFocusFollowsMouseMode(mode);
}


void
K_LockedDesktopSettings::SetAcceptFirstClick(const bool acceptFirstClick)
{
	fSettings->SetAcceptFirstClick(acceptFirstClick);
}


void
K_LockedDesktopSettings::SetShowAllDraggers(bool show)
{
	fSettings->SetShowAllDraggers(show);
}


void
K_LockedDesktopSettings::SetUIColors(const BMessage& colors, bool* changed)
{
	fSettings->SetUIColors(colors, &changed[0]);
}


void
K_LockedDesktopSettings::SetSubpixelAntialiasing(bool subpix)
{
	fSettings->SetSubpixelAntialiasing(subpix);
}


void
K_LockedDesktopSettings::SetHinting(uint8 hinting)
{
	fSettings->SetHinting(hinting);
}


void
K_LockedDesktopSettings::SetSubpixelAverageWeight(uint8 averageWeight)
{
	fSettings->SetSubpixelAverageWeight(averageWeight);
}

void
K_LockedDesktopSettings::SetSubpixelOrderingRegular(bool subpixelOrdering)
{
	fSettings->SetSubpixelOrderingRegular(subpixelOrdering);
}


status_t
K_LockedDesktopSettings::SetControlLook(const char* path)
{
	return fSettings->SetControlLook(path);
}

