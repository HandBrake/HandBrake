
#ifndef _MTEXTVIEW_H
#define _MTEXTVIEW_H
#include "layout.h"
#include <TextView.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MTextView :  public MView, public BTextView
{
	public:		MTextView(minimax size=0);
				MTextView(BMessage*);
				virtual ~MTextView();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	AttachedToWindow();
				virtual void DetachedFromWindow();
				virtual void MessageReceived(BMessage *mes);

	private:	void initobject();
				static long AsyncSetTextRect(void *arg);
				thread_id resizer;
				uint32  _expansiondata[2];
};

#endif
