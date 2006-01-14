
#ifndef _SPACE_H
#define _SPACE_H
#include "layout.h"
#include "View.h"

#if __POWERPC__
#pragma simple_class_byval off
#endif
class IMPEXPLIBLAYOUT Space :  public MView, public BView
{
	public:		Space();
				Space(minimax);
				Space(BMessage*);
		virtual ~Space();
		virtual long Archive(BMessage *archive, bool deep=true) const;
		static BArchivable *Instantiate(BMessage *archive);
		virtual	minimax	layoutprefs();
		virtual BRect	layout(BRect rect);
};

#endif
