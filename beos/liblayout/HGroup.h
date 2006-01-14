
#ifndef _HGROUP_H
#define _HGROUP_H
#include "MGroup.h"
#include <View.h>
#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT HGroup :  public MGroup, public BView
{
	public:		HGroup(minimax mpm,MView *kid=0, ...);
			 	HGroup(MView *kid=0, ...);
				HGroup(BMessage*);
			 	virtual ~HGroup();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);
				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void AttachedToWindow();
				virtual void DetachedFromWindow();
				virtual void MouseDown(BPoint);

	private:	virtual void _expansionhgroup1();
				virtual void _expansionhgroup2();
				virtual void _expansionhgroup3();

				static int cmpkids(const void* v1,const void *v2);
				int *size;
				float totalweight;
				int numkids;
				sortstruct	*childorder;
				MView **mkid;
				float totalminx,totalmaxx;
				BRect *lastrect;

				uint32	_expansiondata[2];
};

#endif
