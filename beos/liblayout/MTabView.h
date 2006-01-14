
#ifndef _MTABVIEW_H
#define _MTABVIEW_H

#include <TabView.h>
#include "MGroup.h"

class IMPEXPLIBLAYOUT MTab: public BTab
{
	public:
		MTab(MView *view, const char *name=NULL);
		MTab(BMessage *archive);
		virtual ~MTab();
};


class IMPEXPLIBLAYOUT MTabView: public MGroup, public BTabView
{
	public:
		MTabView();
		MTabView(BMessage *archive);
		virtual void Add(MTab *tab);
		virtual void Select(int32 tab);

		virtual void	reloadfont(BFont *font[]);
		virtual minimax layoutprefs();
		virtual BRect layout(BRect);
		
	private:
		void LayoutCurrentTab();
};

#endif
