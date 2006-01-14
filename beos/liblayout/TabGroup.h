
#ifndef _TABGROUP_H
#define _TABGROUP_H

#include "MGroup.h"
#include <Control.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT TabGroup :  public MGroup, public BControl
{
	public:		TabGroup(minimax mpm,char *arg=0, ...);
			 	TabGroup(char *arg=0, ...);
				TabGroup(BMessage*);
			 	virtual ~TabGroup();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);
				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	AttachedToWindow();
				virtual void	MouseDown(BPoint point);
				virtual void	Draw(BRect clip);
				virtual void	KeyDown(const char *bytes, int32 numbytes);
				virtual void	ActivateTab(int);
						int32	ActiveTab();
				virtual void	DetachedFromWindow();
				virtual void	SetEnabled(bool enabled);
						void	SetExtraSpacing(float spacing);
						float	ExtraSpacing();

	private:	virtual void	_expansiontabgroup1();
				virtual void	_expansiontabgroup2();
	
				int numkids;
				int activekid;
				float tabheight;
				float biggesttabmin;
				float fontdescent;
				float extraspacing;

				uint32 _expansiondata[1];
};

#endif
