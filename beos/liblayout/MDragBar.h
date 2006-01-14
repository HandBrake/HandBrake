
#ifndef _MDRAGBAR_H
#define _MDRAGBAR_H
#include "layout.h"
#include <Control.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MDragBar :  public MView, public BControl
{
	public:		MDragBar(minimax size=minimax(1,1,1E6,1E6));
				MDragBar(BMessage*);
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);
				virtual ~MDragBar();

				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	AttachedToWindow();
				virtual void	Draw(BRect rect);
				virtual void	KeyDown(const char *bytes, int32 numbytes);
				virtual void	DetachedFromWindow();
				virtual void	MouseDown(BPoint);

	private:	virtual void _expansionmdragbar1(); 

				thread_id mousethread;
				BPoint dragpoint;

				static	long	_mousetracker(void *arg);
						void	_MouseTracker();

				uint32  _expansiondata[2];
};

#endif
