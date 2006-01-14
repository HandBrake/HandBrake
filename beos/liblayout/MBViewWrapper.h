
#ifndef _MBVIEWWRAPPER_H
#define _MBVIEWWRAPPER_H
#include "layout.h"
#include <View.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MBViewWrapper :  public MView, public BView
{
	public:		MBViewWrapper(BView *view, bool usepreferred=true, bool x_fixed=true, bool y_fixed=true);
				MBViewWrapper(BMessage*);
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);
				virtual ~MBViewWrapper();
				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void AttachedToWindow();
				virtual void DetachedFromWindow();

	private:	virtual void _expansionmbviewwrapper1(); 

				BView *childview;

				uint32  _expansiondata[2];
};

#endif
