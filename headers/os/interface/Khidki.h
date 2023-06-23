#ifndef	_KHIDKI_H
#define	_KHIDKI_H

#include "Window.h"
#include <Looper.h>
#include <StorageDefs.h>

#include <Laminate.h>
//#include <View.h>
//#include "Aayat.h"


class KButton;
class KMenuBar;
class KMenuItem;
class BMessage;
class BMessageRunner;
class BMessenger;
class KView;

namespace BPrivate {
	class PortLink;
};
//using namespace Gfx;

/*extern enum k_window_type; {
	K_UNTYPED_WINDOW					= 0,
	K_TITLED_WINDOW						= 1,
	K_MODAL_WINDOW						= 3,
	K_DOCUMENT_WINDOW					= 11,
	K_BORDERED_WINDOW					= 20,
	K_FLOATING_WINDOW					= 21
};*/

/*extern enum k_window_look; {
	K_BORDERED_WINDOW_LOOK				= 20,
	K_NO_BORDER_WINDOW_LOOK				= 19,
	K_TITLED_WINDOW_LOOK				= 1,
	K_DOCUMENT_WINDOW_LOOK				= 11,
	K_MODAL_WINDOW_LOOK					= 3,
	K_FLOATING_WINDOW_LOOK				= 7
};
*/
/*extern enum k_window_feel;{
	K_NORMAL_WINDOW_FEEL				= 0,
	K_MODAL_SUBSET_WINDOW_FEEL			= 2,
	K_MODAL_APP_WINDOW_FEEL				= 1,
	K_MODAL_ALL_WINDOW_FEEL				= 3,
	K_FLOATING_SUBSET_WINDOW_FEEL		= 5,
	K_FLOATING_APP_WINDOW_FEEL			= 4,
	K_FLOATING_ALL_WINDOW_FEEL			= 6
};*/

/*extern enum k_window_alignment; {
	K_BYTE_ALIGNMENT	= 0,
	K_PIXEL_ALIGNMENT	= 1
};
*/
// window flags
/*enum {
	K_NOT_MOVABLE						= 0x00000001,
	K_NOT_CLOSABLE						= 0x00000020,
	K_NOT_ZOOMABLE						= 0x00000040,
	K_NOT_MINIMIZABLE					= 0x00004000,
	K_NOT_RESIZABLE						= 0x00000002,
	K_NOT_H_RESIZABLE					= 0x00000004,
	K_NOT_V_RESIZABLE					= 0x00000008,
	K_AVOID_FRONT						= 0x00000080,
	K_AVOID_FOCUS						= 0x00002000,
	K_WILL_ACCEPT_FIRST_CLICK			= 0x00000010,
	K_OUTLINE_RESIZE					= 0x00001000,
	K_NO_WORKSPACE_ACTIVATION			= 0x00000100,
	K_NOT_ANCHORED_ON_ACTIVATE			= 0x00020000,
	K_ASYNCHRONOUS_CONTROLS				= 0x00080000,
	K_QUIT_ON_WINDOW_CLOSE				= 0x00100000,
	K_SAME_POSITION_IN_ALL_WORKSPACES	= 0x00200000,
	K_AUTO_UPDATE_SIZE_LIMITS			= 0x00400000,
	K_CLOSE_ON_ESCAPE					= 0x00800000,
	K_NO_SERVER_SIDE_WINDOW_MODIFIERS	= 0x00000200
};


#define K_CURRENT_WORKSPACE				0
*/
class KWindow : public BLooper {
public:
								KWindow(/*Int_Rect*/BRect frame, const char* title,
									window_type type, uint32 flags,
									uint32 workspace = B_CURRENT_WORKSPACE);

							KWindow(BRect frame, const char* title,
									window_look look, window_feel feel,
									uint32 flags, uint32 workspace
										= B_CURRENT_WORKSPACE);

	//virtual						~KWindow();

								KWindow(BMessage* archive);

void				AddChild(KView* child, KView* before = NULL);
			void				AddChild(KLayoutItem* child);
			bool				RemoveChild(KView* child);
			int32				CountChildren() const;
			KView*				ChildAt(int32 index) const;

	virtual	void				DispatchMessage(BMessage* message,
									BHandler* handler);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				FrameMoved(BPoint newPosition);

	virtual	void				FrameResized(float newWidth, float newHeight);
	virtual void				WorkspacesChanged(uint32 oldWorkspaces,
									uint32 newWorkspaces);
	virtual void				WorkspaceActivated(int32 workspace,
									bool state);
	virtual void				Minimize(bool minimize);
virtual	void				Zoom(BPoint origin, float width, float height);
			void				Zoom();
			void				SetZoomLimits(float maxWidth, float maxHeight);
virtual void				ScreenChanged(BRect screenSize,
									color_space depth);

			void				SetPulseRate(bigtime_t rate);
			bigtime_t			PulseRate() const;

			void				AddShortcut(uint32 key, uint32 modifiers,
									BMessage* message);
			void				AddShortcut(uint32 key, uint32 modifiers,
									BMessage* message, BHandler* target);
			bool				HasShortcut(uint32 key, uint32 modifiers);
			void				RemoveShortcut(uint32 key, uint32 modifiers);

			void				SetDefaultButton(KButton* button);
			KButton*			DefaultButton() const;

	virtual	void				MenusBeginning();
	virtual	void				MenusEnded();

			bool				NeedsUpdate() const;
			void				UpdateIfNeeded();

			KView*				FindView(const char* viewName) const;
			KView*				FindView(BPoint) const;
			KView*				CurrentFocus() const;

			void				Activate(bool = true);

	virtual	void				WindowActivated(bool focus);

			void				ConvertToScreen(BPoint* point) const;
			BPoint				ConvertToScreen(BPoint point) const;
			void				ConvertFromScreen(BPoint* point) const;
			BPoint				ConvertFromScreen(BPoint point) const;
			void				ConvertToScreen(BRect* rect) const;
			BRect				ConvertToScreen(BRect rect) const;
			void				ConvertFromScreen(BRect* rect) const;
			BRect				ConvertFromScreen(BRect rect) const;

			void				MoveBy(float dx, float dy);
			void				MoveTo(BPoint);
			void				MoveTo(float x, float y);
			void				ResizeBy(float dx, float dy);
			void				ResizeTo(float width, float height);
			void				ResizeToPreferred();


			void				CenterIn(const BRect& rect);
			void				CenterOnScreen();
			void				CenterOnScreen(screen_id id);
			void				MoveOnScreen(uint32 flags = 0);


	virtual	void				Show();
	virtual	void				Hide();


			bool				IsHidden() const;
			bool				IsMinimized() const;

			void				Flush() const;
			void				Sync() const;

			status_t			SendBehind(const KWindow* window);

			void				DisableUpdates();
			void				EnableUpdates();

			void				BeginViewTransaction();
									// referred as OpenViewTransaction()
									// in BeBook
			void				EndViewTransaction();
									// referred as CommitViewTransaction()
									// in BeBook
			bool				InViewTransaction() const;

			BRect				Bounds() const;
			BRect				Frame() const;
			BRect				DecoratorFrame() const;
			BSize				Size() const;
			const char*			Title() const;
			void				SetTitle(const char* title);
			bool				IsFront() const;
			bool				IsActive() const;

			void				SetKeyMenuBar(KMenuBar* bar);
			KMenuBar*			KeyMenuBar() const;

			void				SetSizeLimits(float minWidth, float maxWidth,
									float minHeight, float maxHeight);
			void				GetSizeLimits(float* minWidth, float* maxWidth,
									float* minHeight, float* maxHeight);
			void				UpdateSizeLimits();

			status_t			SetDecoratorSettings(const BMessage& settings);
			status_t			GetDecoratorSettings(BMessage* settings) const;

			uint32				Workspaces() const;
			void				SetWorkspaces(uint32);

			KView*				LastMouseMovedView() const;

	virtual BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual status_t			GetSupportedSuites(BMessage* data);

			status_t			AddToSubset(KWindow* window);
			status_t			RemoveFromSubset(KWindow* window);

	virtual status_t			Perform(perform_code code, void* data);

			status_t			SetType(window_type type);
			window_type			Type() const;

			status_t			SetLook(window_look look);
			window_look			Look() const;

			status_t			SetFeel(window_feel feel);
			window_feel			Feel() const;

			status_t			SetFlags(uint32);
			uint32				Flags() const;

			bool				IsModal() const;
			bool				IsFloating() const;

			status_t			SetWindowAlignment(window_alignment mode,
									int32 h, int32 hOffset = 0,
									int32 width = 0, int32 widthOffset = 0,
									int32 v = 0, int32 vOffset = 0,
									int32 height = 0, int32 heightOffset = 0);
			status_t			GetWindowAlignment(
									window_alignment* mode = NULL,
									int32* h = NULL, int32* hOffset = NULL,
									int32* width = NULL,
									int32* widthOffset = NULL,
									int32* v = NULL, int32* vOffset = NULL,
									int32* height = NULL,
									int32* heightOffset = NULL) const;


	virtual	bool				QuitRequested();
	virtual thread_id			Run();

	virtual	void				SetLayout(KLayout* layout);

			KLayout*			GetLayout() const;

			void				InvalidateLayout(bool descendants = false);
			void				Layout(bool force);
			bool				IsOffscreenWindow() const;


private:
	// FBC padding and forbidden methods
	virtual	void				_ReservedWindow2();
	virtual	void				_ReservedWindow3();
	virtual	void				_ReservedWindow4();
	virtual	void				_ReservedWindow5();
	virtual	void				_ReservedWindow6();
	virtual	void				_ReservedWindow7();
	virtual	void				_ReservedWindow8();

private:
	struct unpack_cookie;

	class Shortcut;

	friend class KAlert;
	friend class KBitmap;
	friend class KView;
	friend class KMenuItem;

	friend class KWindowStack;

	friend void k_set_menu_sem_(KWindow* w, sem_id sem);
	friend status_t k_safe_get_server_token_(const BLooper*, int32*);

								KWindow(BRect frame, int32 bitmapToken);

void				_InitData(/*Int_Rect*/BRect frame, const char* title,
									window_look look, window_feel feel,
									uint32 flags, uint32 workspace,
									int32 bitmapToken = -1);

	virtual	void				task_looper();
			BPoint				AlertPosition(const BRect& frame);

	virtual BMessage*			ConvertToMessage(void* raw, int32 code);

void				AddShortcut(uint32 key, uint32 modifiers,
									KMenuItem* item);

BHandler*			_DetermineTarget(BMessage* message,
									BHandler* target);
			bool				_IsFocusMessage(BMessage* message);
			bool				_UnpackMessage(unpack_cookie& state,
									BMessage** _message, BHandler** _target,
									bool* _usePreferred);
			void				_SanitizeMessage(BMessage* message,
									BHandler* target, bool usePreferred);
			bool				_StealMouseMessage(BMessage* message,
									bool& deleteMessage);
			uint32				_TransitForMouseMoved(KView* view,
									KView* viewUnderMouse) const;

			bool				InUpdate();
			void				_DequeueAll();
			window_type			_ComposeType(window_look look,
									window_feel feel) const;
void				_DecomposeType(window_type type,
									window_look* look,
									window_feel* feel) const;

			void				SetIsFilePanel(bool yes);
			bool				IsFilePanel() const;

			void				_CreateTopView();
			void				_AdoptResize();

			void				_SetFocus(KView* focusView,
									bool notifyIputServer = false);
			void				_SetName(const char* title);

			Shortcut*			_FindShortcut(uint32 key, uint32 modifiers);
			KView*				_FindView(KView* view, BPoint point) const;
			KView*				_FindView(int32 token);
			KView*				_LastViewChild(KView* parent);

			KView*				_FindNextNavigable(KView* focus, uint32 flags);
			KView*				_FindPreviousNavigable(KView* focus,
									uint32 flags);
			void				_Switcher(int32 rawKey, uint32 modifiers,
									bool repeat);
			bool				_HandleKeyDown(BMessage* event);
			bool				_HandleUnmappedKeyDown(BMessage* event);
			void				_KeyboardNavigation();


			void				_GetDecoratorSize(float* _borderWidth,
									float* _tabHeight) const;
			void				_SendShowOrHideMessage();


private:

			/*bool				fOffscreen;
			char*				fTitle;
			bool				fInTransaction;
			bool				fActive;
			uint32				fFlags;
			KView*				fTopView;
			KView*				fFocus;
			KView*				fLastMouseMovedView;
			short				fShowLevel;


			bool				fUpdateRequested;
			bool				fIsFilePanel;

			sem_id				fMenuSem;
			bigtime_t			fPulseRate;
			bool				fMinimized;
			bool				fNoQuitShortcut;
			bool				_unused6;
			sem_id				fMenuSem;
			float				fMaxZoomHeight;
			float				fMaxZoomWidth;
			float				fMinHeight;
			float				fMinWidth;
			float				fMaxHeight;
			float				fMaxWidth;
			*//*Int_Rect*/
			/*BRect		fFrame;
			window_look			fLook;
			window_feel			fFeel;
			int32				fLastViewToken;

			::BPrivate::PortLink* fLink;

			BMessageRunner*		fPulseRunner;
			BRect				fPreviousFrame;
*/

			char*				fTitle;
			int32				_unused0;
			bool				fInTransaction;
			bool				fActive;
			short				fShowLevel;
			uint32				fFlags;

			KView*				fTopView;
			KView*				fFocus;
			KView*				fLastMouseMovedView;
			uint32				_unused1;
			KMenuBar*			fKeyMenuBar;
			KButton*			fDefaultButton;
			BList				fShortcuts;
			int32				fTopViewToken;
			bool				fUpdateRequested;
			bool				fOffscreen;
			bool				fIsFilePanel;
			bool				_unused4;
			bigtime_t			fPulseRate;
			bool				_unused5;
			bool				fMinimized;
			bool				fNoQuitShortcut;
			bool				_unused6;
			sem_id				fMenuSem;
			float				fMaxZoomHeight;
			float				fMaxZoomWidth;
			float				fMinHeight;
			float				fMinWidth;
			float				fMaxHeight;
			float				fMaxWidth;
			BRect				fFrame;
			window_look			fLook;
			window_feel			fFeel;
			int32				fLastViewToken;
			::BPrivate::PortLink* fLink;
			BMessageRunner*		fPulseRunner;
			BRect				fPreviousFrame;

			uint32				_reserved[9];
};
#endif
