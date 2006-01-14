

#ifndef _MSLIDER_H
#define _MSLIDER_H

#include "layout.h"
#include "Slider.h"

class IMPEXPLIBLAYOUT MSlider: public MView, public BSlider
{
	public:
		MSlider(const char *label, int32 minval, int32 maxval,int32 granularity=1, BMessage *message=NULL, BHandler *target=NULL, thumb_style ts=B_BLOCK_THUMB);
		virtual ~MSlider();
		virtual void AllAttached();
		virtual void DetachedFromWindow();
		virtual void SetValue(int32 value);
		void SetGranularity(int32 granul);

		virtual minimax layoutprefs();
		virtual BRect layout(BRect);
		
	private:
		virtual void _expansionmslider1();
		virtual void _expansionmslider2();
		BHandler *target;
		int32 granularity;
		uint32 _expansiondata[4];
};

#endif
