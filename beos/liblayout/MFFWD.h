#ifndef _MFFWD_H
#define _MFFWD_H

#include "MPictureButton.h"

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MFFWD : public MPictureButton
{
	public:		MFFWD(BHandler *id=NULL);
				MFFWD(BHandler *id, BMessage*);
				MFFWD(BMessage*);
				virtual ~MFFWD();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);
				
				virtual void MakePictures();
};

extern const IMPEXPLIBLAYOUT char M_BUTTON_POINTER[];

#endif
