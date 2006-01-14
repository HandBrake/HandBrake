
#ifndef _MSTRINGVIEW_H
#define _MSTRINGVIEW_H
#include "layout.h"
#include <StringView.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MStringView :  public MView, public BStringView
{
	public:		MStringView(const char *label,alignment a=B_ALIGN_LEFT,minimax size=minimax(10,10,65536,65536,1));
				MStringView(BMessage*);
				virtual ~MStringView();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	AttachedToWindow();
				virtual void DetachedFromWindow();
};

#endif
