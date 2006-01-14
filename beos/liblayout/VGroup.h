
#ifndef _VGROUP_H
#define _VGROUP_H
#include "MGroup.h"
#include <View.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT VGroup :  public MGroup, public BView
{
	public:		VGroup(minimax mpm,MView *kid=0, ...);
			 	VGroup(MView *kid=0, ...);
				VGroup(BMessage*);
			 	virtual ~VGroup();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);
				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void AttachedToWindow();
				virtual void DetachedFromWindow();
				virtual void MouseDown(BPoint);

	private:	virtual void _expansionvgroup1();
				virtual void _expansionvgroup2();
				virtual void _expansionvgroup3();

				static int cmpkids(const void* v1,const void *v2);
				int *size;
				float totalweight;
				int numkids;
				sortstruct	*childorder;
				MView **mkid;
				float totalminy,totalmaxy;
				BRect *lastrect;

				uint32	_expansiondata[2];
};

#endif
