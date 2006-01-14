
#ifndef _MAPPLICATION_H
#define _MAPPLICATION_H

#include "layout.h"
#include <Application.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MApplication : public BApplication
{
	public:	MApplication(char *);
			MApplication(BMessage*);
			virtual ~MApplication();
			virtual long Archive(BMessage *archive, bool deep=true) const;
			static BArchivable *Instantiate(BMessage *archive);
			virtual void MessageReceived(BMessage*);

			virtual status_t GetSupportedSuites(BMessage *message);
			virtual BHandler *ResolveSpecifier(BMessage *message, int32 index, BMessage *specifier, int32 command, const char *property);

	private:
			virtual void _expansionmapplication1();
			virtual void _expansionmapplication2();
			uint32 _expansiondata[4];
};
#endif
