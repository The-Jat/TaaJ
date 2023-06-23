/*
 * Copyright (c) 2001-2020, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 *		Joseph Groover <looncraz@satx.rr.com>
 
 */

#include "KDecorManager.h"

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <Rect.h>

#include <syslog.h>

#include "AppServer.h"
#include "Desktop.h"
#include "DesktopSettings.h"
#include "ServerConfig.h"
#include "SATDecorator.h"
#include "KWindow.h"

//khidki code
//start
//#define TRACE_DEBUG_SERVER
#ifdef TRACE_DEBUG_SERVER
#	define TTRACE(x) debug_printf x
#else
#	define TTRACE(x) ;
#endif
//end

typedef float get_version(void);
typedef K_DecorAddOn* create_decor_addon(image_id id, const char* name);

// Globals
K_DecorManager k_gDecorManager;


K_DecorAddOn::K_DecorAddOn(image_id id, const char* name)
	:
	fImageID(id),
 	fName(name)
{
}


K_DecorAddOn::~K_DecorAddOn()
{
}


status_t
K_DecorAddOn::InitCheck() const
{
	return B_OK;
}


Decorator*
K_DecorAddOn::AllocateDecorator(Desktop* desktop, DrawingEngine* engine,
	BRect rect, const char* title, window_look look, uint32 flags)
{
debug_printf("[K_DecorAddon]{AllocateDecorator} entered AllocateDecorator ...\n");
	if (!desktop->LockSingleWindow())
		return NULL;

	DesktopSettings settings(desktop);
	Decorator* decorator;
	decorator = _AllocateDecorator(settings, rect, desktop);
	desktop->UnlockSingleWindow();
	if (!decorator)
		return NULL;

	decorator->UpdateColors(settings);
debug_printf("[K_DecorAddon]{AllocateDecorator} after UpdateColors in AllocateDecorator...\n");

	if (decorator->AddTab(settings, title, look, flags) == NULL) {
		delete decorator;
		return NULL;
	}

	decorator->SetDrawingEngine(engine);
	return decorator;
}


K_WindowBehaviour*
K_DecorAddOn::AllocateWindowBehaviour(K_Window* window)
{
	return new (std::nothrow)K_SATWindowBehaviour(window,
		window->Desktop()->K_GetStackAndTile());
}


const K_DesktopListenerList&
K_DecorAddOn::GetDesktopListeners()
{
debug_printf("[K_DecorAddon]{GetDesktopListeners}\n");
	return fDesktopListeners;
}


Decorator*
K_DecorAddOn::_AllocateDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
{
debug_printf("[K_DecorAddon]{_AllocateDecorator} entered _AllocateDecorator ...\n");
	return new (std::nothrow)SATDecorator(settings, rect, desktop);
}


//	#pragma mark -


K_DecorManager::K_DecorManager()
	:
	fDefaultDecor(-1, "Default"),
	fCurrentDecor(&fDefaultDecor),
	fPreviewDecor(NULL),
	fPreviewWindow(NULL),
	fCurrentDecorPath("Default")
{
	_LoadSettingsFromDisk();
}


K_DecorManager::~K_DecorManager()
{
}


Decorator*
K_DecorManager::AllocateDecorator(K_Window* window)
{
debug_printf("[K_DecorManager]{AllocateDecorator} AllocateDecorator entered ...\n");
	// Create a new instance of the current decorator.
	// Ownership is that of the caller

	if (!fCurrentDecor) {
		// We should *never* be here. If we do, it's a bug.
		debug_printf("[K_DecorManager]{AllocateDecorator} !fCurrentDecor...\n");
		debugger("K_DecorManager::AllocateDecorator has a NULL decorator");
		return NULL;
	}

	// Are we previewing a specific decorator?
	if (window == fPreviewWindow) {
	debug_printf("[K_DecorManager]{AllocateDecorator} window == fPreviewWindow ...\n");
		if (fPreviewDecor != NULL) {
		debug_printf("[K_DecorManager]{AllocateDecorator} fPreviewDecor != NULL ...\n");
			return fPreviewDecor->AllocateDecorator(window->Desktop(),
				window->GetDrawingEngine(), window->Frame(), window->Title(),
				window->Look(), window->Flags());
		} else {
			fPreviewWindow = NULL;
		}
	}

	return fCurrentDecor->AllocateDecorator(window->Desktop(),
		window->GetDrawingEngine(), window->Frame(), window->Title(),
		window->Look(), window->Flags());
}


K_WindowBehaviour*
K_DecorManager::AllocateWindowBehaviour(K_Window* window)
{
	if (!fCurrentDecor) {
		// We should *never* be here. If we do, it's a bug.
		debugger("K_DecorManager::AllocateDecorator has a NULL decorator");
		return NULL;
	}

	return fCurrentDecor->AllocateWindowBehaviour(window);
}


void
K_DecorManager::CleanupForWindow(K_Window* window)
{
	// Given window is being deleted, do any cleanup needed
	if (fPreviewWindow == window && window != NULL){
		fPreviewWindow = NULL;

		if (fPreviewDecor != NULL)
			unload_add_on(fPreviewDecor->ImageID());

		fPreviewDecor = NULL;
	}
}


status_t
K_DecorManager::PreviewDecorator(BString path, K_Window* window)
{
	if (fPreviewWindow != NULL && fPreviewWindow != window){
		// Reset other window to current decorator - only one can preview
		K_Window* oldPreviewWindow = fPreviewWindow;
		fPreviewWindow = NULL;
		oldPreviewWindow->ReloadDecor();
	}

	if (window == NULL)
		return B_BAD_VALUE;

	// We have to jump some hoops because the window must be able to
	// delete its decorator before we unload the add-on
	status_t error = B_OK;
	K_DecorAddOn* decorPtr = _LoadDecor(path, error);
	if (decorPtr == NULL)
		return error == B_OK ? B_ERROR : error;

	BRegion border;
	window->GetBorderRegion(&border);

	K_DecorAddOn* oldDecor = fPreviewDecor;
	fPreviewDecor = decorPtr;
	fPreviewWindow = window;
	// After this call, the window has deleted its decorator.
	fPreviewWindow->ReloadDecor();

	BRegion newBorder;
	window->GetBorderRegion(&newBorder);

	border.Include(&newBorder);
	window->Desktop()->K_RebuildAndRedrawAfterWindowChange(window, border);

	if (oldDecor != NULL)
		unload_add_on(oldDecor->ImageID());

	return B_OK;
}


const K_DesktopListenerList&
K_DecorManager::GetDesktopListeners()
{
debug_printf("[K_DecorManager]{GetDesktopListeners} \n");

	return fCurrentDecor->GetDesktopListeners();
}


BString
K_DecorManager::GetCurrentDecorator() const
{
	return fCurrentDecorPath.String();
}


status_t
K_DecorManager::SetDecorator(BString path, Desktop* desktop)
{
	status_t error = B_OK;
	K_DecorAddOn* newDecor = _LoadDecor(path, error);
	if (newDecor == NULL)
		return error == B_OK ? B_ERROR : error;

	K_DecorAddOn* oldDecor = fCurrentDecor;

	BString oldPath = fCurrentDecorPath;
	image_id oldImage = fCurrentDecor->ImageID();

	fCurrentDecor = newDecor;
	fCurrentDecorPath = path.String();

	if (desktop->K_ReloadDecor(oldDecor)) {
		// now safe to unload all old decorator data
		// saves us from deleting oldDecor...
		unload_add_on(oldImage);
		_SaveSettingsToDisk();
		return B_OK;
	}

	// TODO: unloading the newDecor and its image
	// problem is we don't know how many windows failed... or why they failed...
	syslog(LOG_WARNING,
		"app_server:K_DecorManager:SetDecorator:\"%s\" *partly* failed\n",
		fCurrentDecorPath.String());

	fCurrentDecor = oldDecor;
	fCurrentDecorPath = oldPath;
	return B_ERROR;
}


K_DecorAddOn*
K_DecorManager::_LoadDecor(BString _path, status_t& error )
{
	if (_path == "Default") {
		error = B_OK;
		return &fDefaultDecor;
	}

	BEntry entry(_path.String(), true);
	if (!entry.Exists()) {
		error = B_ENTRY_NOT_FOUND;
		return NULL;
	}

	BPath path(&entry);
	image_id image = load_add_on(path.Path());
	if (image < 0) {
		error = B_BAD_IMAGE_ID;
		return NULL;
	}

	create_decor_addon*	createFunc;
	if (get_image_symbol(image, "instantiate_decor_addon", B_SYMBOL_TYPE_TEXT,
			(void**)&createFunc) != B_OK) {
		unload_add_on(image);
		error = B_MISSING_SYMBOL;
		return NULL;
	}

	char name[B_FILE_NAME_LENGTH];
	entry.GetName(name);
	K_DecorAddOn* newDecor = createFunc(image, name);
	if (newDecor == NULL || newDecor->InitCheck() != B_OK) {
		unload_add_on(image);
		error = B_ERROR;
		return NULL;
	}

	return newDecor;
}


static const char* kSettingsDir = "system/app_server";
static const char* kSettingsFile = "decorator_settings";


bool
K_DecorManager::_LoadSettingsFromDisk()
{
	// get the user settings directory
	BPath path;
	status_t error = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	if (error != B_OK)
		return false;

	path.Append(kSettingsDir);
	path.Append(kSettingsFile);
	BFile file(path.Path(), B_READ_ONLY);
	if (file.InitCheck() != B_OK)
		return false;

	BMessage settings;
	if (settings.Unflatten(&file) == B_OK) {
		BString itemPath;
		if (settings.FindString("decorator", &itemPath) == B_OK) {
			status_t error = B_OK;
			K_DecorAddOn* decor = _LoadDecor(itemPath, error);
			if (decor != NULL) {
				fCurrentDecor = decor;
				fCurrentDecorPath = itemPath;
				return true;
			} else {
				//TODO: do something with the reported error
			}
		}
	}

	return false;
}


bool
K_DecorManager::_SaveSettingsToDisk()
{
	// get the user settings directory
	BPath path;
	status_t error = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	if (error != B_OK)
		return false;

	path.Append(kSettingsDir);
	if (create_directory(path.Path(), 777) != B_OK)
		return false;

	path.Append(kSettingsFile);
	BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	if (file.InitCheck() != B_OK)
		return false;

	BMessage settings;
	if (settings.AddString("decorator", fCurrentDecorPath.String()) != B_OK)
		return false;
	if (settings.Flatten(&file) != B_OK)
		return false;

	return true;
}

