#ifndef STEPPER_H
#define STEPPER_H

#include <interface/View.h>

class BTextControl;

class HBStepper : public BView
{
    public:
        HBStepper( BRect rect, int step, int min, int max, int val,
                   BMessage * message );
        void Draw( BRect rect );
        void AttachedToWindow();
        void MouseDown( BPoint point );
        void SetValue( int val );
        int  Value();
        void SetEnabled( bool e );

    private:
        int            fStep;
        int            fMin;
        int            fMax;
        int            fValue;
        BMessage     * fMessage;

        bool           fEnabled;
        BTextControl * fControl;
};

#endif
