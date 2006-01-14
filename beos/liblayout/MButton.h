
#ifndef _MBUTTON
#define _MBUTTON
#include "layout.h"
#include <Button.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MButton :  public MView, public BButton
{
	public:		ulong ID;

				MButton(const char *label, ulong id=0,minimax size=minimax(-1,-1,1E6,1E6,1));
				MButton(const char *label, BMessage *message, BHandler *handler=NULL, minimax size=minimax(-1,-1,1E6,1E6,1));
				MButton(BMessage*);
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				virtual ~MButton();
						void	SetRepeat(ulong initial_delay, ulong repeat_delay);
				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	setcolor(rgb_color, bool);

				virtual void	Draw(BRect);
				virtual void	AttachedToWindow();
				virtual void	DetachedFromWindow();

				virtual	void MouseDown(BPoint);

	private: 	static	long _mousetracker(void *arg);
						void _MouseTracker();
				thread_id mousethread;
				BHandler *target;
				int64	lastwhen;
				ulong	initialdelay;
				ulong	repeatdelay;
				void	initobject();
				ulong buttonmask;
		
				uint32  _expansiondata[2];
};

extern const IMPEXPLIBLAYOUT char M_BUTTON_POINTER[];
extern const IMPEXPLIBLAYOUT char M_BUTTON_ID[];
extern const IMPEXPLIBLAYOUT char M_BUTTON_MASK[];
extern const IMPEXPLIBLAYOUT char M_DOUBLECLICK[];
extern const IMPEXPLIBLAYOUT char M_REPEAT[];
extern const IMPEXPLIBLAYOUT char M_RELEASE[];

#endif
