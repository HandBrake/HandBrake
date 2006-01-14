
#ifndef _MCHECKBOX
#define _MCHECKBOX
#include "layout.h"
#include <CheckBox.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MCheckBox :  public MView, public BCheckBox
{
	public:		MCheckBox(const char *label,ulong id=0, bool state=false);
				MCheckBox(const char *label, BMessage *message, BHandler *handler=NULL, bool state=false);
				MCheckBox(BMessage*);
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				virtual ~MCheckBox();
				virtual void	AttachedToWindow();
				virtual void DetachedFromWindow();

				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);

	private:	BHandler *target;
				uint32  _expansiondata[2];
};

extern const IMPEXPLIBLAYOUT char M_CHECKBOX_POINTER[];
extern const IMPEXPLIBLAYOUT char M_CHECKBOX_ID[];

#endif
