
#ifndef _LAYEREDGROUP_H
#define _LAYEREDGROUP_H

#include "MGroup.h"
#include <Control.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT LayeredGroup :  public MGroup, public BControl
{
	public:		LayeredGroup(minimax mpm,MView *arg=0, ...);
			 	LayeredGroup(MView *arg=0, ...);
				LayeredGroup(BMessage*);
				virtual ~LayeredGroup();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);
				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				void			ActivateLayer(int);
				virtual void	MessageReceived(BMessage *mes);
				virtual void AttachedToWindow();
				virtual void DetachedFromWindow();

	private:	virtual void _expansionlayeredgroup1();
				virtual void _expansionlayeredgroup2();

				int numkids;
				int activekid;

				uint32 _expansiondata[3];
};


#endif
