#ifndef _MPICTUREBUTTON_H
#define _MPICTUREBUTTON_H

#include "layout.h"
#include <PictureButton.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MPictureButton : public MView, public BPictureButton
{
	public:
		MPictureButton(minimax size, BPicture *off, BPicture *on,
				BMessage *message=NULL, BHandler *target=NULL,
				uint32 behavior=B_ONE_STATE_BUTTON);
		MPictureButton(BMessage *archive);
		virtual ~MPictureButton();
				void	SetRepeat(ulong initial_delay, ulong repeat_delay);

		virtual long Archive(BMessage *archive, bool deep=true) const;
		static BArchivable *Instantiate(BMessage *archive);

		virtual	minimax	layoutprefs();
		virtual BRect layout(BRect);
		virtual void setcolor(rgb_color col, bool deep=false);
		virtual void MakePictures();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual	void MouseDown(BPoint);

	private:
		virtual void _expansionmpicturebutton1(); 
		virtual void _expansionmpicturebutton2(); 

		thread_id	mousethread;
		int64		lastwhen;
		ulong	initialdelay;
		ulong	repeatdelay;
		ulong		buttonmask;
		BHandler	*target;

		static	long _mousetracker(void *arg);
				void _MouseTracker();

		uint32  _expansiondata[2];
};

extern const IMPEXPLIBLAYOUT char M_DOUBLECLICK[];
extern const IMPEXPLIBLAYOUT char M_BUTTON_MASK[];
extern const IMPEXPLIBLAYOUT char M_REPEAT[];
extern const IMPEXPLIBLAYOUT char M_RELEASE[];

#endif
