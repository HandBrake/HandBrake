#ifndef _MEJECT_H
#define _MEJECT_H

#include "MPictureButton.h"

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MEject : public MPictureButton
{
	public:		MEject(BHandler *id);
				MEject(BHandler *id, BMessage*);
				MEject(BMessage*);
				virtual ~MEject();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				virtual	void MakePictures();
};
extern const IMPEXPLIBLAYOUT char M_BUTTON_POINTER[];

#endif
