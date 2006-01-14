
#ifndef _MSCROLLVIEW_H
#define _MSCROLLVIEW_H

#include "layout.h"
#include <ScrollView.h>

// An MScrollView accepts another MView as its target.
// The MScrollView will display scrollbars as requested
// so that the target MView may be scrolled left/right or
// up/down.

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MScrollView: public MView, public BScrollView
{
	public:
		MScrollView(MView *target, bool horizontal=false, bool vertical=false,
			border_style border=B_FANCY_BORDER, minimax size=0);
		MScrollView(BMessage*);
		virtual ~MScrollView();
		virtual long Archive(BMessage *archive, bool deep=true) const;
		static BArchivable *Instantiate(BMessage *archive);
		virtual minimax layoutprefs();
		virtual BRect layout(BRect);
		virtual void	AttachedToWindow();
		virtual void DetachedFromWindow();

	private:
		MView *kid;
		float leftinset;
		float rightinset;
		float topinset;
		float bottominset;
};

#endif
