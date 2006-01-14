#ifndef _MPLAYFW_H
#define _MPLAYFW_H

#include "MPictureButton.h"

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MPlayFW : public MPictureButton
{
	public:		MPlayFW(BHandler*);
				MPlayFW(BHandler *id, BMessage*);
				MPlayFW(BMessage*);
				virtual ~MPlayFW();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);
				virtual void MakePictures();
};
extern const IMPEXPLIBLAYOUT char M_BUTTON_POINTER[];

#endif
