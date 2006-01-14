
#ifndef _MSTATUSBAR_H
#define _MSTATUSBAR_H
#include "layout.h"
#include <View.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MProgressBar :  public MView, public BView
{
	public:		float	value;
				float currentwidth;

	public:		MProgressBar(BHandler *, bool pulsed_updates=false);
				MProgressBar(BMessage*);
				virtual ~MProgressBar();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	setcolor(rgb_color,bool);
						void	setbarcolor(rgb_color);
					rgb_color	getbarcolor() {return bar_fill;};
				virtual void	Draw(BRect);
				virtual void	Pulse();
						void	Refresh();
						void	SetValue(float value);
				virtual void	MouseDown(BPoint);
				virtual void	WindowActivated(bool);
				virtual void	FrameResized(float,float);
				virtual void	MessageReceived(BMessage *mes);
				virtual void	AttachedToWindow();
				virtual void	DetachedFromWindow();

	private:	virtual void _expansionmprogressbar1(); 
				virtual void _expansionmprogressbar2(); 

				thread_id		mousethread;
				static	long	_mousetracker(void *arg);
						long	mousetracker();

				BRect	lastbounds;
				BHandler *target;
				rgb_color	bar_hi;
				rgb_color	bar_fill;
				rgb_color	bar_low;
				float barwidth;

				uint32  _expansiondata[2];
};

extern const IMPEXPLIBLAYOUT char M_PROGRESSBAR_FRACTION[];

#endif

