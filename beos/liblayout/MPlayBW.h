#ifndef _MPLAYBW_H
#define _MPLAYBW_H

#include "MPictureButton.h"

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MPlayBW : public MPictureButton
{
	public:		MPlayBW(BHandler*);
				MPlayBW(BHandler *id, BMessage*);
				MPlayBW(BMessage*);
				virtual ~MPlayBW();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				virtual void MakePictures();
};
extern const IMPEXPLIBLAYOUT char M_BUTTON_POINTER[];

#endif
