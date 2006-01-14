
#ifndef _MPOPUP_H
#define _MPOPUP_H
#include "layout.h"
#include "MDividable.h"
#include <MenuField.h>

class BPopUpMenu;

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MPopup:
	public MView, public MDividable, public BMenuField
{
	public:		MPopup(char *label, char *item ...);
				MPopup(char *label, BMessage*, BHandler *, char *item ...);
				MPopup(BMessage*);
				virtual ~MPopup();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				virtual float	LabelWidth();

				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	reloadfont(BFont *font[]);
				void			SetActive(ulong, bool send=true);
				void			EnableItem(ulong index, bool enabled);
				virtual void	AttachedToWindow();
				virtual void	DetachedFromWindow();

	private:	virtual void _expansionmpopup1();

				char		*poplabel;
				BHandler	*target;
				BPopUpMenu	*popup;

				uint32  _expansiondata[2];
};

extern const IMPEXPLIBLAYOUT char M_POPUP_POINTER_NAME[];

#endif
