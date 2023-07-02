/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 *		Joseph Groover <looncraz@satx.rr.com>
 */
#ifndef K__DECOR_MANAGER_H
#define K__DECOR_MANAGER_H


#include <image.h>
#include <String.h>
#include <Locker.h>
#include <ObjectList.h>
#include <Entry.h>
#include <DecorInfo.h>

#include "KDecorator.h"

class Desktop;
class K_DesktopListener;
class DrawingEngine;
class K_Window;
class K_WindowBehaviour;


typedef BObjectList<K_DesktopListener> K_DesktopListenerList;


// special name to test for use of non-fs-tied default decorator
// this just keeps things clean and simple is all

class K_DecorAddOn {
public:
								K_DecorAddOn(image_id id, const char* name);
	virtual						~K_DecorAddOn();

	virtual status_t			InitCheck() const;

			image_id			ImageID() const { return fImageID; }

			K_Decorator*			AllocateDecorator(Desktop* desktop,
									DrawingEngine* engine, BRect rect,
									const char* title, window_look look,
									uint32 flags);

	virtual	K_WindowBehaviour*	AllocateWindowBehaviour(K_Window* window);

	virtual const K_DesktopListenerList& GetDesktopListeners();

protected:
	virtual	K_Decorator*			_AllocateDecorator(K_DesktopSettings& settings,
									BRect rect, Desktop* desktop);

			K_DesktopListenerList	fDesktopListeners;

private:
			image_id			fImageID;
			BString 			fName;
};


class K_DecorManager {
public:
								K_DecorManager();
								~K_DecorManager();

			K_Decorator*			AllocateDecorator(K_Window *window);
			K_WindowBehaviour*	AllocateWindowBehaviour(K_Window *window);
			void				CleanupForWindow(K_Window *window);

			status_t			PreviewDecorator(BString path, K_Window *window);

			const K_DesktopListenerList& GetDesktopListeners();

			BString 			GetCurrentDecorator() const;
			status_t			SetDecorator(BString path, Desktop *desktop);

private:
			K_DecorAddOn*			_LoadDecor(BString path, status_t &error);
			bool				_LoadSettingsFromDisk();
			bool				_SaveSettingsToDisk();

private:
			K_DecorAddOn			fDefaultDecor;
			K_DecorAddOn*			fCurrentDecor;
			K_DecorAddOn*			fPreviewDecor;

			K_Window*				fPreviewWindow;
			BString				fCurrentDecorPath;
};

extern K_DecorManager k_gDecorManager;

#endif	/* DECOR_MANAGER_H */
