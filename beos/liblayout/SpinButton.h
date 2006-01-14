
#ifndef _SPINBUTTON_H
#define _SPINBUTTON_H

#include <View.h>
#include "layout.h"
#include "HGroup.h"
#include "MTextControl.h"

enum spinmode
{
	SPIN_FLOAT,
	SPIN_INTEGER
};

class NumberTextView;
class TinyButton;
#if __POWERPC__
#pragma warn_hidevirtual off
#endif
class IMPEXPLIBLAYOUT SpinButton: public MView, public MDividable, public BControl
{
	public:
		SpinButton(const char *label,spinmode mode, BHandler *target=NULL);
		virtual ~SpinButton();

		virtual void reloadfont(BFont *font[]);
		virtual minimax layoutprefs();
		virtual BRect layout(BRect);
		
		virtual float LabelWidth();

		virtual void AllAttached();
		virtual void DetachedFromWindow();
		virtual void Draw(BRect);
		virtual void MessageReceived(BMessage *mes);
		virtual void SetEnabled(bool);

		double Increment();
		double Decrement();
		double StepSize();
		void SetStepSize(double step);

		virtual void SetValue(int32 v);
		virtual void SetValue(double v);
		double Value();
		double Maximum();
		void SetMaximum(double max);
		double Minimum();
		void SetMinimum(double min);
		const char * Format() const;
		void SetFormat(const char *f);

	private:
		spinmode mode;
		ulong height;
		TinyButton *tb1,*tb2;
		NumberTextView *tv;		
		long lx,ly;
		void NotifyWorld(BMessage *mes);
		BHandler *target;
		uint32 _expansiondata[4];
};

enum 
{
	M_SPIN_UP='!!up',
	M_SPIN_DOWN='!!dn',
	M_SPIN_TICK='!spn'
};

extern const IMPEXPLIBLAYOUT char M_RELEASE[];

#if __POWERPC__
#pragma warn_hidevirtual on
#endif

#endif
