
#ifndef _MLISTVIEW_H
#define _MLISTVIEW_H
#include "layout.h"
#include <ListView.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MListView :  public MView, public BListView
{
	public:		MListView(list_view_type type=B_SINGLE_SELECTION_LIST,
									 minimax size=minimax(50,50));
				MListView(BMessage*);
				virtual ~MListView();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);
				virtual void reloadfont(BFont *font[]);
				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	MessageReceived(BMessage*);
				virtual void	AttachedToWindow();
				virtual void DetachedFromWindow();

	private:	virtual void _expansionmlistview1(); 
				uint32  _expansiondata[2];
};

#endif
