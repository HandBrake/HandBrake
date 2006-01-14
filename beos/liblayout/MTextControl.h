
#ifndef _MTEXTCONTROL_H
#define _MTEXTCONTROL_H
#include "layout.h"
#include "MDividable.h"
#include <TextControl.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MTextControl:
	public MView, public MDividable, public BTextControl
{
	public:		MTextControl(char *label, char *text);
				MTextControl(char *label, char *text, BMessage *mes);
				MTextControl(BMessage*);
				virtual ~MTextControl();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				virtual void	SetLabel(const char *);
				virtual float	LabelWidth();

				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	reloadfont(BFont *font[]);
				virtual void	AttachedToWindow();
				virtual void DetachedFromWindow();

	private:	float lastheight;

				uint32  _expansiondata[2];
};

#endif
