
#ifndef _MRADIOGROUP_H
#define _MRADIOGROUP_H
#include "MGroup.h"
#include <RadioButton.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MRadioGroup :  public MGroup, public BView
{
	public:		MRadioGroup(char *item ...);
				MRadioGroup(BMessage *model, char *item ...);
				MRadioGroup(BMessage *model, BHandler *target, char *item ...);
				MRadioGroup(BMessage*);
				virtual ~MRadioGroup();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	SetActive(ulong);
				virtual long	ActiveButton();
				virtual void	reloadfont(BFont *font[]);
				virtual void	setcolor(rgb_color col,bool deep);
				virtual void	AttachedToWindow();
				virtual void	DetachedFromWindow();

	private:	virtual void _expansionmradiogroup1(); 
				virtual void _expansionmradiogroup2(); 

				ulong		numradios;
				BHandler	*handler;

				uint32  _expansiondata[3];
};

extern const IMPEXPLIBLAYOUT char M_RADIO_POINTER_NAME[];
extern const IMPEXPLIBLAYOUT char M_RADIO_INDEX_NAME[];


#endif



