
#ifndef _MOUTLINELISTVIEW_H
#define _MOUTLINELISTVIEW_H
#include "layout.h"
#include <OutlineListView.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MOutlineListView :  public MView, public BOutlineListView
{
	public:		MOutlineListView(list_view_type type=B_SINGLE_SELECTION_LIST,
									 minimax size=minimax(50,50));
				MOutlineListView(BMessage*);
				virtual ~MOutlineListView();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);
				virtual void reloadfont(BFont *font[]);
				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	MessageReceived(BMessage*);
				virtual void	AttachedToWindow();
				virtual void DetachedFromWindow();

	private:	virtual void _expansionmoutlinelistview1(); 
				virtual void _expansionmoutlinelistview2(); 

				uint32  _expansiondata[2];
};

#endif
