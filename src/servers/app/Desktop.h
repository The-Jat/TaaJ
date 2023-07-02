/*
 * Copyright 2001-2020, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrian Oanca, adioanca@cotty.iren.ro
 *		Stephan Aßmus, superstippi@gmx.de
 *		Axel Dörfler, axeld@pinc-software.de
 *		Andrej Spielmann, andrej.spielmann@seh.ox.ac.uk
 *		Brecht Machiels, brecht@mos6581.org
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Joseph Groover, looncraz@looncraz.net
 *		Tri-Edge AI
 *		Jacob Secunda, secundja@gmail.com
 */
#ifndef DESKTOP_H
#define DESKTOP_H


#include <AutoDeleter.h>
#include <Autolock.h>
#include <InterfaceDefs.h>
#include <List.h>
#include <Menu.h>
#include <ObjectList.h>
#include <Region.h>
#include <String.h>
#include <Window.h>

#include <ServerProtocolStructs.h>

#include "CursorManager.h"
#include "DelayedMessage.h"
#include "DesktopListener.h"
#include "DesktopSettings.h"
#include "KDesktopSettings.h" //khidki
#include "EventDispatcher.h"
#include "MessageLooper.h"
#include "MultiLocker.h"
#include "Screen.h"
#include "ScreenManager.h"
#include "ServerCursor.h"
#include "StackAndTile.h"
#include "KStackAndTile.h"//khidki
#include "VirtualScreen.h"
#include "WindowList.h"
#include "KWindowList.h"//khidki
#include "Workspace.h"
#include "WorkspacePrivate.h"


#include"drawing/Painter/Painter.h"//mak
#include "Point.h"//mak
using namespace Gfx;

class BMessage;

class DecorAddOn;
class K_DecorAddOn; // khidki
class DrawingEngine;
class HWInterface;
class ServerApp;
class Window;
class K_Window;//khidki
class WorkspacesView;
class K_WorkspacesView;//khidki
struct server_read_only_memory;

namespace BPrivate {
	class LinkSender;
};


class Desktop : public DesktopObservable, public K_DesktopObservable, public MessageLooper,
	public ScreenOwner {
public:
								Desktop(uid_t userID,
									const char* targetScreen);
	virtual						~Desktop();

			void				RegisterListener(DesktopListener* listener);

void	K_RegisterListener(K_DesktopListener* listener);//khidki


			status_t			Init();

void get_Painter();//mak
void get_Buffer();//mak
void draw_something();//mak
void draw_physical_pixel(const Int_Point& physical_position, int color);//mak
void draw_line(const Int_Point& p1, const Int_Point& p2, int col, int thickness);
void bg_change(rgb_color);//mak

			uid_t				UserID() const { return fUserID; }
			const char*			TargetScreen() { return fTargetScreen; }
	virtual port_id				MessagePort() const { return fMessagePort; }
			area_id				SharedReadOnlyArea() const
									{ return fSharedReadOnlyArea; }

			::EventDispatcher&	EventDispatcher() { return fEventDispatcher; }

			void				BroadcastToAllApps(int32 code);
			void				BroadcastToAllWindows(int32 code);

void	K_BroadcastToAllWindows(int32 code);//khidki

			int32				GetAllWindowTargets(DelayedMessage& message);
			int32				GetAllAppTargets(DelayedMessage& message);

int32	K_GetAllWindowTargets(DelayedMessage& message);//khidki

			filter_result		KeyEvent(uint32 what, int32 key,
									int32 modifiers);

filter_result	K_KeyEvent(uint32 what, int32 key, int32 modifiers);//khidki
	// Locking
			bool				LockSingleWindow()
									{ return fWindowLock.ReadLock(); }
			void				UnlockSingleWindow()
									{ fWindowLock.ReadUnlock(); }

			bool				LockAllWindows()
									{ return fWindowLock.WriteLock(); }
			void				UnlockAllWindows()
									{ fWindowLock.WriteUnlock(); }

			const MultiLocker&	WindowLocker() { return fWindowLock; }

	// Mouse and cursor methods

			void				SetCursor(ServerCursor* cursor);
			ServerCursorReference Cursor() const;
			void				SetManagementCursor(ServerCursor* newCursor);

			void				SetLastMouseState(const BPoint& position,
									int32 buttons, Window* windowUnderMouse);
									
//khidki start
void	K_SetLastMouseState(const BPoint& position,
									int32 buttons, K_Window* windowUnderMouse);
//end

									// for use by the mouse filter only
									// both mouse position calls require
									// the Desktop object to be locked
									// already
			void				GetLastMouseState(BPoint* position,
									int32* buttons) const;
									// for use by ServerWindow

			CursorManager&		GetCursorManager() { return fCursorManager; }

	// Screen and drawing related methods

			status_t			SetScreenMode(int32 workspace, int32 id,
									const display_mode& mode, bool makeDefault);
			status_t			GetScreenMode(int32 workspace, int32 id,
									display_mode& mode);
			status_t			GetScreenFrame(int32 workspace, int32 id,
									BRect& frame);
			void				RevertScreenModes(uint32 workspaces);

			status_t			SetBrightness(int32 id, float brightness);

			MultiLocker&		ScreenLocker() { return fScreenLock; }

			status_t			LockDirectScreen(team_id team);
			status_t			UnlockDirectScreen(team_id team);

			const ::VirtualScreen& VirtualScreen() const
									{ return fVirtualScreen; }
			DrawingEngine*		GetDrawingEngine() const
									{ return fVirtualScreen.DrawingEngine(); }
			::HWInterface*		HWInterface() const
									{ return fVirtualScreen.HWInterface(); }

			void				RebuildAndRedrawAfterWindowChange(
									Window* window, BRegion& dirty);
									// the window lock must be held when calling
									// this function

void	K_RebuildAndRedrawAfterWindowChange(K_Window* window, BRegion& dirty);//khidki

	// ScreenOwner implementation
	virtual	void				ScreenRemoved(Screen* screen) {}
	virtual	void				ScreenAdded(Screen* screen) {}
	virtual	void				ScreenChanged(Screen* screen);
	virtual	bool				ReleaseScreen(Screen* screen) { return false; }

	// Workspace methods

			void				SetWorkspaceAsync(int32 index,
									bool moveFocusWindow = false);

//khidki start
void		K_SetWorkspaceAsync(int32 index,
									bool moveFocusWindow = false);
//end

			void				SetWorkspace(int32 index,
									bool moveFocusWindow = false);
			int32				CurrentWorkspace()
									{ return fCurrentWorkspace; }
			Workspace::Private&	WorkspaceAt(int32 index)
									{ return fWorkspaces[index]; }
			status_t			SetWorkspacesLayout(int32 columns, int32 rows);
			BRect				WorkspaceFrame(int32 index) const;

			void				StoreWorkspaceConfiguration(int32 index);

			void				AddWorkspacesView(WorkspacesView* view);
			void				RemoveWorkspacesView(WorkspacesView* view);


//khidki start
void	K_AddWorkspacesView(K_WorkspacesView* view);
void	K_RemoveWorkspacesView(K_WorkspacesView* view);

//end

	// Window methods

			void				SelectWindow(Window* window);
void	K_SelectWindow(K_Window* window);//khidki

			void				ActivateWindow(Window* window);
void	K_ActivateWindow(K_Window* window);//khidki

			void				SendWindowBehind(Window* window,
									Window* behindOf = NULL,
									bool sendStack = true);
//khidki start
void	K_SendWindowBehind(K_Window* window,
									K_Window* behindOf = NULL,
									bool sendStack = true);
//end


			void				ShowWindow(Window* window);
void	K_ShowWindow(K_Window* window);//khidki
			void				HideWindow(Window* window,
									bool fromMinimize = false);
void	K_HideWindow(K_Window* window, bool fromMinimize = false);//khidki

			void				MinimizeWindow(Window* window, bool minimize);
void	K_MinimizeWindow(K_Window* window, bool minimize);//khidki

			void				MoveWindowBy(Window* window, float x, float y,
									int32 workspace = -1);
			void				ResizeWindowBy(Window* window, float x,
									float y);
void	K_MoveWindowBy(K_Window* window, float x, float y, int32 workspace = -1);//khidki
void	K_ResizeWindowBy(K_Window* window, float x, float y);//khidki
			void				SetWindowOutlinesDelta(Window* window,
									BPoint delta);
			bool				SetWindowTabLocation(Window* window,
									float location, bool isShifting);
			bool				SetWindowDecoratorSettings(Window* window,
									const BMessage& settings);

			void				SetWindowWorkspaces(Window* window,
									uint32 workspaces);

//khidki start
void	K_SetWindowOutlinesDelta(K_Window* window, BPoint delta);
bool	K_SetWindowTabLocation(K_Window* window, float location, bool isShifting);
bool	K_SetWindowDecoratorSettings(K_Window* window,
									const BMessage& settings);
void	K_SetWindowWorkspaces(K_Window* window,
		uint32 workspaces);
//khidki end

			void				AddWindow(Window* window);
void	K_AddWindow(K_Window* window);//khidki
			void				RemoveWindow(Window* window);
void				K_RemoveWindow(K_Window* window);// khidki

			bool				AddWindowToSubset(Window* subset,
									Window* window);
			void				RemoveWindowFromSubset(Window* subset,
									Window* window);
//khidki start
bool				K_AddWindowToSubset(K_Window* subset,
									K_Window* window);
void				K_RemoveWindowFromSubset(K_Window* subset,
									K_Window* window);
//end

			void				FontsChanged(Window* window);
			void				ColorUpdated(Window* window, color_which which,
									rgb_color color);

void	K_FontsChanged(K_Window* window);//khidki
void	K_ColorUpdated(K_Window* window, color_which which, rgb_color color);//khidki

			void				SetWindowLook(Window* window, window_look look);
			void				SetWindowFeel(Window* window, window_feel feel);
			void				SetWindowFlags(Window* window, uint32 flags);
			void				SetWindowTitle(Window* window,
									const char* title);

void	K_SetWindowLook(K_Window* window, window_look look);//khidki
void	K_SetWindowFeel(K_Window* window, window_feel feel);//khidki
void	K_SetWindowFlags(K_Window* window, uint32 flags);//khidki
void	K_SetWindowTitle(K_Window* window, const char* title);//khidki

			Window*				FocusWindow() const { return fFocus; }
			Window*				FrontWindow() const { return fFront; }
			Window*				BackWindow() const { return fBack; }

K_Window*	K_FocusWindow() const { return k_fFocus; }//khidki
K_Window*	K_FrontWindow() const { return k_fFront; }//khidki
K_Window*	K_BackWindow() const { return k_fBack; }//khidki

			Window*				WindowAt(BPoint where);
K_Window*	K_WindowAt(BPoint where);//khidki

			Window*				MouseEventWindow() const
									{ return fMouseEventWindow; }
			void				SetMouseEventWindow(Window* window);

			void				SetViewUnderMouse(const Window* window,
									int32 viewToken);
			int32				ViewUnderMouse(const Window* window);

//khidki start
K_Window*	K_MouseEventWindow() const
									{ return k_fMouseEventWindow; }
void	K_SetMouseEventWindow(K_Window* window);
void	K_SetViewUnderMouse(const K_Window* window,
			int32 viewToken);
int32	K_ViewUnderMouse(const K_Window* window);//khidki
//khidki end

			EventTarget*		KeyboardEventTarget();
//khidki start
EventTarget*	K_KeyboardEventTarget();
//end

			void				SetFocusWindow(Window* window = NULL);
			void				SetFocusLocked(const Window* window);

			Window*				FindWindowByClientToken(int32 token,
									team_id teamID);
			EventTarget*		FindTarget(BMessenger& messenger);

void	K_SetFocusWindow(K_Window* window = NULL);//khidki
void	K_SetFocusLocked(const K_Window* window);//khidki
K_Window*	K_FindWindowByClientToken(int32 token, team_id teamID);//khidki

			void				MarkDirty(BRegion& region);
			void				Redraw();
			void				RedrawBackground();

void	K_MarkDirty(BRegion& region);//khdiki
void	K_Redraw();//khidki
void	K_RedrawBackground();//khidki

			bool				ReloadDecor(DecorAddOn* oldDecor);
//khidki start
bool	K_ReloadDecor(K_DecorAddOn* oldDecor);
//end

			BRegion&			BackgroundRegion()
									{ return fBackgroundRegion; }

			void				MinimizeApplication(team_id team);
			void				BringApplicationToFront(team_id team);
			void				WindowAction(int32 windowToken, int32 action);
void	K_MinimizeApplication(team_id team);//khidki
void	K_BringApplicationToFront(team_id team);//khidki
void	K_WindowAction(int32 windowToken, int32 action);//khidki

			void				WriteWindowList(team_id team,
									BPrivate::LinkSender& sender);
			void				WriteWindowInfo(int32 serverToken,
									BPrivate::LinkSender& sender);
			void				WriteApplicationOrder(int32 workspace,
									BPrivate::LinkSender& sender);
			void				WriteWindowOrder(int32 workspace,
									BPrivate::LinkSender& sender);
//khidki start
void		K_WriteWindowList(team_id team,
									BPrivate::LinkSender& sender);
void		K_WriteWindowInfo(int32 serverToken,
									BPrivate::LinkSender& sender);
void		K_WriteApplicationOrder(int32 workspace,
									BPrivate::LinkSender& sender);
void		K_WriteWindowOrder(int32 workspace,
									BPrivate::LinkSender& sender);
//end

			//! The window lock must be held when accessing a window list!
			WindowList&			CurrentWindows();
			WindowList&			AllWindows();
K_WindowList&		K_CurrentWindows();//khidki
K_WindowList&		K_AllWindows();//khidki

			Window*				WindowForClientLooperPort(port_id port);
K_Window*	K_WindowForClientLooperPort(port_id port);//khidki

			StackAndTile*		GetStackAndTile() { return &fStackAndTile; }
//khidki start
K_StackAndTile*		K_GetStackAndTile() { return &k_fStackAndTile; }
//end

private:
			WindowList&			_Windows(int32 index);
K_WindowList&		K__Windows(int32 index);//khidki

			void				_FlushPendingColors();
void	K__FlushPendingColors();//khidki

			void				_LaunchInputServer();
			void				_GetLooperName(char* name, size_t size);
			void				_PrepareQuit();
			void				_DispatchMessage(int32 code,
									BPrivate::LinkReceiver &link);

			void				_UpdateFloating(int32 previousWorkspace = -1,
									int32 nextWorkspace = -1,
									Window* mouseEventWindow = NULL);
			void				_UpdateBack();
			void				_UpdateFront(bool updateFloating = true);
			void				_UpdateFronts(bool updateFloating = true);
			bool				_WindowHasModal(Window* window) const;
			bool				_WindowCanHaveFocus(Window* window) const;

			void				_WindowChanged(Window* window);
			void				_WindowRemoved(Window* window);

			void				_ShowWindow(Window* window,
									bool affectsOtherWindows = true);
			void				_HideWindow(Window* window);

			void				_UpdateSubsetWorkspaces(Window* window,
									int32 previousIndex = -1,
									int32 newIndex = -1);
			void				_ChangeWindowWorkspaces(Window* window,
									uint32 oldWorkspaces, uint32 newWorkspaces);
			void				_BringWindowsToFront(WindowList& windows,
									int32 list, bool wereVisible);
			Window*				_LastFocusSubsetWindow(Window* window);
			bool				_CheckSendFakeMouseMoved(
									const Window* lastWindowUnderMouse);
			void				_SendFakeMouseMoved(Window* window = NULL);

			Screen*				_DetermineScreenFor(BRect frame);
			void				_RebuildClippingForAllWindows(
									BRegion& stillAvailableOnScreen);
			void				_TriggerWindowRedrawing(
									BRegion& newDirtyRegion);
			void				_SetBackground(BRegion& background);
void				_SetBackground(BRegion& background, rgb_color col);// custom
			status_t			_ActivateApp(team_id team);

			void				_SuspendDirectFrameBufferAccess();
			void				_ResumeDirectFrameBufferAccess();

			void				_ScreenChanged(Screen* screen);
			void				_SetCurrentWorkspaceConfiguration();
			void				_SetWorkspace(int32 index,
									bool moveFocusWindow = false);

//khidki start
void	K__UpdateFloating(int32 previousWorkspace = -1,
				int32 nextWorkspace = -1,
				K_Window* mouseEventWindow = NULL);
void	K__UpdateBack();
void	K__UpdateFront(bool updateFloating = true);
void	K__UpdateFronts(bool updateFloating = true);
bool	K__WindowHasModal(K_Window* window) const;
bool	K__WindowCanHaveFocus(K_Window* window) const;
void	K__WindowChanged(K_Window* window);
void	K__WindowRemoved(K_Window* window);
void	K__ShowWindow(K_Window* window,
			bool affectsOtherWindows = true);
void		K__HideWindow(K_Window* window);
void	K__UpdateSubsetWorkspaces(K_Window* window,
	int32 previousIndex = -1,
	int32 newIndex = -1);
void		K__ChangeWindowWorkspaces(K_Window* window,
		uint32 oldWorkspaces, uint32 newWorkspaces);
void	K__BringWindowsToFront(K_WindowList& windows,
		int32 list, bool wereVisible);
K_Window*	K__LastFocusSubsetWindow(K_Window* window);
bool	K__CheckSendFakeMouseMoved(
		const K_Window* lastWindowUnderMouse);
void	K__SendFakeMouseMoved(K_Window* window = NULL);
void	K__RebuildClippingForAllWindows(BRegion& stillAvailableOnScreen);
void	K__TriggerWindowRedrawing(BRegion& newDirtyRegion);
//end

private:
	friend class DesktopSettings;
	friend class K_DesktopSettings; //khidki
	friend class LockedDesktopSettings;
	friend class K_LockedDesktopSettings; //khidki
Painter *fPainter;//mak
RenderingBuffer* fBuffer;//mak
public:
DrawingEngine* fDrawingEngine;//mak
unsigned int* ptr;//mak
unsigned int* fb_ptr;//mak
int scr_width;//mak
int scr_height;//mak

bool IsKWindow;// to know that whether called from KWindow or BWindow...

private://mak

			uid_t				fUserID;
			char*				fTargetScreen;
			::VirtualScreen		fVirtualScreen;
			ObjectDeleter<DesktopSettingsPrivate>
								fSettings;
			// khidki start
			ObjectDeleter<K_DesktopSettingsPrivate>
								khidkiSettings;
			// khidki end
			port_id				fMessagePort;
			::EventDispatcher	fEventDispatcher;
			area_id				fSharedReadOnlyArea;
			server_read_only_memory* fServerReadOnlyMemory;
			area_id				kSharedReadOnlyArea;// khidki
			server_read_only_memory* kServerReadOnlyMemory; //khidki

			BLocker				fApplicationsLock;
			BObjectList<ServerApp> fApplications;

			sem_id				fShutdownSemaphore;
			int32				fShutdownCount;

			::Workspace::Private fWorkspaces[kMaxWorkspaces];
			MultiLocker			fScreenLock;
			BLocker				fDirectScreenLock;
			team_id				fDirectScreenTeam;
			int32				fCurrentWorkspace;
			int32				fPreviousWorkspace;

			WindowList			fAllWindows;
			WindowList			fSubsetWindows;
			WindowList			fFocusList;
			Window*				fLastWorkspaceFocus[kMaxWorkspaces];

//khidki start
K_WindowList			k_fAllWindows;
K_WindowList			k_fSubsetWindows;
K_WindowList			k_fFocusList;
K_Window*			k_fLastWorkspaceFocus[kMaxWorkspaces];
//khidki end

			BObjectList<WorkspacesView> fWorkspacesViews;
			BLocker				fWorkspacesLock;
//khidki start
BObjectList<K_WorkspacesView> k_fWorkspacesViews;
BLocker	k_fWorkspacesLock;
//end

			CursorManager		fCursorManager;
			ServerCursorReference fCursor;
			ServerCursorReference fManagementCursor;

			MultiLocker			fWindowLock;

			BRegion				fBackgroundRegion;
			BRegion				fScreenRegion;

			Window*				fMouseEventWindow;
K_Window*	k_fMouseEventWindow;//khidki
			const Window*		fWindowUnderMouse;
const K_Window*	k_fWindowUnderMouse;//khidki
			const Window*		fLockedFocusWindow;
const K_Window*	k_fLockedFocusWindow;//khidki
			int32				fViewUnderMouse;
			BPoint				fLastMousePosition;
			int32				fLastMouseButtons;

			Window*				fFocus;
			Window*				fFront;
			Window*				fBack;
K_Window*	k_fFocus;//khidki
K_Window*	k_fFront;//khidki
K_Window*	k_fBack;//khidki

			StackAndTile		fStackAndTile;
K_StackAndTile		k_fStackAndTile;//khidki

			BMessage			fPendingColors;
};

#endif	// DESKTOP_H
