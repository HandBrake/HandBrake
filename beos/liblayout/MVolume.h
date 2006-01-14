#ifndef _MVOLUME_H
#define _MVOLUME_H

#include "layout.h"
#include <Control.h>

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MVolume : public MView, public BControl
{
	public:		MVolume(BHandler*);
				MVolume(BMessage*);
				virtual ~MVolume();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				virtual	minimax	layoutprefs();
				BRect layout(BRect);
				virtual void	AttachedToWindow();
				virtual void DetachedFromWindow();
				virtual void Draw(BRect);
				virtual void DrawVolume(void);
						float Volume();
						void SetVolume(float vol);
				virtual void MouseDown(BPoint);
				virtual void KeyDown(const char *bytes, int32 numbytes);

	private:	float volume;
				BHandler *target;
				BPoint lastvoldot;
				BPoint clickpoint;
				thread_id mousethread;
				bool ispressed;
				// moved into private area 21-6-98
				static	long _mousetracker(void *arg);
						void _MouseTracker();
				// added 21-6-98
						void _PUMouseTracker();
};

inline float MVolume::Volume()
{
	return volume;
}

#endif
