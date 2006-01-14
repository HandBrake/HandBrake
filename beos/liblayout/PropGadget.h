
#ifndef _PROPGADGET
#define _PROPGADGET
#include "layout.h"
#include <Control.h>
#include <Bitmap.h>

class IMPEXPLIBLAYOUT PropGadget;

typedef void (*propgadget_hook)(PropGadget*, void*, double, double);

#if __POWERPC__
#pragma simple_class_byval off
#endif

class PropGadget : public MView, public BControl
{
	public:		PropGadget(double xprop, double xval, double yprop, double yval,
					BBitmap *knobimage=NULL,
					 propgadget_hook=NULL,
					 void *callbackarg=NULL,
					 long extraspacing=0);
				PropGadget(BMessage*);
				virtual ~PropGadget();
				virtual long Archive(BMessage *archive, bool deep=true) const;
				static BArchivable *Instantiate(BMessage *archive);

				BBitmap*	Pknob;

				virtual	void	Draw(BRect);
				virtual void	AttachedToWindow();
				virtual void	DetachedFromWindow();
				virtual void	MouseDown(BPoint);

				void			SetProportion(double, double);
				void			SetProportionNoDraw(double,double);
				virtual void	SetValues(double,double);
				void			SetValuesNoDraw(double,double);
				virtual void	FrameResized(float,float);
				void			ReDraw();
				inline double	Hval() {return hval;}
				inline double	Vval() {return 1.0-vval;}

				virtual	minimax	layoutprefs();
				virtual BRect	layout(BRect rect);
				virtual void	setcolor(rgb_color col, bool deep=false);
				virtual void	DrawContainer(BRect);
				virtual void	DrawKnob(BRect);
				virtual void	KeyDown(const char *bytes, int32 numbytes);
						bool	IsBusy();

	private:	virtual void _expansionpropgadget1(); 
				virtual void _expansionpropgadget2(); 

				thread_id mousethread;
				BPoint	clickpoint;
				BRect	lastknobrect;
				bool	_isbusy;
				bool	vertical;
				bool	horizontal;

				double	hprop;
				double	hval;
				double	vprop;
				double	vval;
				void	(*callback)(PropGadget*,void*,double,double);
				void	*callbackarg;
				long	borderspacing;

				static	long	_mousetracker(void *arg);
						void	_MouseTracker();

				uint32  _expansiondata[2];
};
#endif
