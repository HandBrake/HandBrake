#ifndef _MREW_H
#define _MREW_H

#include "MPictureButton.h"

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MRew : public MPictureButton
{
	public:		MRew(BHandler*);
				MRew(BHandler *id, BMessage*);
				MRew(BMessage*);
				virtual ~MRew();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);
				virtual void MakePictures();
};
extern const IMPEXPLIBLAYOUT char M_BUTTON_POINTER[];

#endif
