#include <interface/Region.h>
#include <interface/TextControl.h>
#include <interface/Window.h>

#include "Stepper.h"

#include <stdio.h>

HBStepper::HBStepper( BRect rect, int step, int min, int max, int val,
                      BMessage * message )
    : BView( rect, NULL, B_FOLLOW_NONE, B_WILL_DRAW )
{
    fStep    = step;
    fMin     = min;
    fMax     = max;
    fMessage = message;

    BRect b = Bounds();

    fEnabled = true;

    fControl = new BTextControl( BRect( 0,1,b.Width()-14,b.Height()-1 ),
        NULL, NULL, "", new BMessage() );
    fControl->SetDivider( 0.0 );
    fControl->TextView()->MakeEditable( false );
    AddChild( fControl );

    SetValue( val );
}

void HBStepper::Draw( BRect rect )
{
    /* Why do we have to do this here!? */
    fControl->TextView()->MakeEditable( false );
    fControl->TextView()->MakeSelectable( false );

    BRect b = Bounds();
    BRegion region;

    SetHighColor( 128,128,128 ); /* Dark gray */
    region.MakeEmpty();
    region.Include( BRect(  3, 0,10, 0 ) );
    region.Include( BRect(  2, 1, 3, 1 ) );
    region.Include( BRect( 10, 1,11, 1 ) );
    region.Include( BRect(  1, 2, 2, 2 ) );
    region.Include( BRect( 11, 2,12, 2 ) );
    region.Include( BRect(  1, 2, 1,18 ) );
    region.Include( BRect(  1,10,12,10 ) );
    region.Include( BRect( 12, 2,12,18 ) );
    region.Include( BRect(  1,18, 2,18 ) );
    region.Include( BRect( 11,18,12,18 ) );
    region.Include( BRect(  2,19, 3,19 ) );
    region.Include( BRect( 10,19,11,19 ) );
    region.Include( BRect(  3,20,10,20 ) );
    region.OffsetBy( b.Width()-12,0 );
    FillRegion( &region );

    SetHighColor( 0,0,0 ); /* Black */
    region.MakeEmpty();
    region.Include( BRect(  6, 4, 7, 4 ) );
    region.Include( BRect(  5, 5, 8, 6 ) );
    region.Include( BRect(  5,14, 8,15 ) );
    region.Include( BRect(  6,16, 7,16 ) );
    region.OffsetBy( b.Width()-12,0 );
    FillRegion( &region );

    BView::Draw( rect );
}

void HBStepper::AttachedToWindow()
{
    if( Parent() )
    {
        SetViewColor( Parent()->ViewColor() );
    }
}

void HBStepper::MouseDown( BPoint point )
{
    BRect r, b = Bounds();

    if( !fEnabled )
    {
        return;
    }

    BMessenger messenger( Window() );

    r = BRect( 2,1,11,9 );
    r.OffsetBy( b.Width()-12,0 );
    if( r.Contains( point ) )
    {
        SetValue( fValue + fStep );
        messenger.SendMessage( fMessage );
    }
    r.OffsetBy( 0,10 );
    if( r.Contains( point ) )
    {
        SetValue( fValue - fStep );
        messenger.SendMessage( fMessage );
    }
}

void HBStepper::SetValue( int val )
{
    fValue = val;
    if( fValue < fMin )
    {
        fValue = fMin;
    }
    if( fValue > fMax )
    {
        fValue = fMax;
    }

    char text[16];
    snprintf( text, 16, "%d", fValue );
    fControl->SetText( text );
}

int HBStepper::Value()
{
    return fValue;
}

void HBStepper::SetEnabled( bool e )
{
    fEnabled = e;
    fControl->SetEnabled( fEnabled );
}
