
#ifndef _MMENUBAR_H
#define _MMENUBAR_H

#include "layout.h"
#include <MenuBar.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MMenuBar: public MView, public BMenuBar
{
	public:
		MMenuBar(menu_layout layout=B_ITEMS_IN_ROW);
		MMenuBar(menu_layout layout, bool resizetofit);
		MMenuBar(BMessage*);
		virtual ~MMenuBar();
		virtual long Archive(BMessage *archive, bool deep=true) const;
		static BArchivable *Instantiate(BMessage *archive);
		
		virtual minimax layoutprefs();
		virtual BRect layout(BRect);
		virtual void	reloadfont(BFont *font[]);
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
	private:
		virtual void _expansionmmenubar1();
		virtual void _expansionmmenubar2();

		uint32  _expansiondata[2];
};
#endif
