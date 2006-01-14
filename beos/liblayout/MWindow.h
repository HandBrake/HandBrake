
#ifndef _MWINDOW_H
#define _MWINDOW_H

#include "layout.h"
#include <Window.h>

class BPopUpMenu;

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MWindow : public BWindow
{
	public:
		ulong	flags;

		MWindow(BRect r,const char *name,window_type type,uint32 flags, uint32 workspaces=B_CURRENT_WORKSPACE);
		MWindow(BRect r,const char *name,window_look look, window_feel feel,uint32 flags, uint32 workspaces=B_CURRENT_WORKSPACE);
		MWindow(BMessage*);
		virtual ~MWindow();
		virtual long Archive(BMessage *archive, bool deep=true) const;
		static BArchivable *Instantiate(BMessage *archive);

		virtual const BFont *getfont(fontspec font);

		virtual void MessageReceived(BMessage *message);
		virtual void Show();
		virtual void RecalcSize();
		virtual void FrameResized(float width, float height);

		virtual status_t GetSupportedSuites(BMessage *message);
		virtual BHandler *ResolveSpecifier(BMessage *message, int32 index, BMessage *specifier, int32 command, const char *property);
		virtual void ScreenChanged(BRect frame, color_space mode);
		virtual void WorkspaceActivated(int32 workspace, bool active);

		void StartDragging();
		static long _mousetracker(void *arg);
		void _MouseTracker();

	private:
		virtual void _expansionmwindow1();
		virtual void _expansionmwindow2();
		virtual void _expansionmwindow3();

		void		initobject();
		BFont		**fontlist;
		BRect		lastrect;
		BPopUpMenu 	*pop;
		thread_id	mousethread;
		BPoint		dragpoint;
		
		uint32		_expansiondata[1];
};

enum {
 M_WIN_AUTORESIZE	=0x00000100,
 M_WIN_ESCAPETOCLOSE=0x00000200
};

#endif // MWINDOW_H
