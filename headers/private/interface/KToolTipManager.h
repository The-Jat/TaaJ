/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_TOOL_TIP_MANAGER_H
#define _K_TOOL_TIP_MANAGER_H


#include <Locker.h>
#include <Messenger.h>
#include <Point.h>


class KToolTip;


class KToolTipManager {
public:
	static	KToolTipManager*	Manager();

			void				ShowTip(KToolTip* tip, BPoint where,
									void* owner);
			void				HideTip();

			void				SetShowDelay(bigtime_t time);
			bigtime_t			ShowDelay() const;
			void				SetHideDelay(bigtime_t time);
			bigtime_t			HideDelay() const;

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ fLock.Unlock(); }

private:
								KToolTipManager();
	virtual						~KToolTipManager();

	static	void				_InitSingleton();

private:
			BLocker				fLock;
			BMessenger			fWindow;

			bigtime_t			fShowDelay;
			bigtime_t			fHideDelay;

	static	KToolTipManager*	sDefaultInstance;
};


#endif	// _TOOL_TIP_MANAGER_H
