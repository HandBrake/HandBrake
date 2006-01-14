
#ifndef _MSPLITTER_H
#define _MSPLITTER_H

#include "layout.h"

#if __POWERPC__
#pragma simple_class_byval off
#endif

class IMPEXPLIBLAYOUT MSplitter : public MView, public BView
{
	public:					MSplitter();
							MSplitter(bool cosmetic);
							MSplitter(BMessage*);
			virtual			~MSplitter();
			virtual long Archive(BMessage *archive, bool deep=true) const;
			static BArchivable *Instantiate(BMessage *archive);

			virtual	minimax	layoutprefs();
			virtual BRect	layout(BRect rect);
			virtual void	MouseDown(BPoint);
			virtual void	Draw(BRect);
			virtual void	MouseMoved(BPoint, ulong, const BMessage*);
			virtual void	AttachedToWindow();
			virtual void	DetachedFromWindow();

	private:
			float		siblingweight;
			MView		*previoussibling;
			MView		*nextsibling;
			thread_id 	mousethread;
			static long	_mousetracker(void *);
			void		_MouseTracker(void);
			uint32		_expansiondata[2];
};

#endif
