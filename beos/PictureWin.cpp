/* $Id: PictureWin.cpp,v 1.2 2003/11/06 14:36:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Screen.h>
#include <Slider.h>

#include "PictureWin.h"

#define UPDATE_BITMAP 'upbi'

/* Handy way to access HBTitle members */
#define fInWidth      fTitle->inWidth
#define fInHeight     fTitle->inHeight
#define fPixelWidth   fTitle->pixelWidth
#define fPixelHeight  fTitle->pixelHeight
#define fDeinterlace  fTitle->deinterlace
#define fOutWidth     fTitle->outWidth
#define fOutHeight    fTitle->outHeight
#define fOutWidthMax  fTitle->outWidthMax
#define fOutHeightMax fTitle->outHeightMax
#define fTopCrop      fTitle->topCrop
#define fBottomCrop   fTitle->bottomCrop
#define fLeftCrop     fTitle->leftCrop
#define fRightCrop    fTitle->rightCrop

HBPictureView::HBPictureView( BRect rect, BBitmap * bitmap )
    : BView( rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW )
{
    fBitmap = bitmap;
}

void HBPictureView::Draw( BRect rect )
{
    if( LockLooper() )
    {
        DrawBitmap( fBitmap, Bounds() );
        UnlockLooper();
    }
    else
    {
        fprintf( stderr, "LockLooper() failed\n" );
    }

    BView::Draw( rect );
}


/* Constructor */
HBPictureWin::HBPictureWin( HBHandle * handle, HBTitle * title )
    : BWindow( BRect( 0, 0, 0, 0 ), "Picture settings",
               B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
               B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_NOT_CLOSABLE )
{
    fHandle  = handle;
    fTitle   = title;

    /* Resize & center */
    ResizeTo( fOutWidthMax + 40, fOutHeightMax + 280 );
    BScreen screen;
    MoveTo( ( screen.Frame().Width() - Frame().Width() ) / 2,
            ( screen.Frame().Height() - Frame().Height() ) / 2 );

    /* Build the GUI */
    BRect r;

    /* Add a background view */
    BView * view;
    view = new BView( Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW );
    view->SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );
    AddChild( view );

    /* First box : picture + slider */
    r = BRect( 10, 10, fOutWidthMax + 31, fOutHeightMax + 60 );
    BBox * pictureBox;
    pictureBox = new BBox( r, NULL );
    pictureBox->SetLabel( "Preview" );

    /* Leave a one-pixel margin to draw the white line around the picture */
    fBitmap = new BBitmap( BRect( 0, 0, fOutWidthMax + 1,
                                  fOutHeightMax + 1 ), 0, B_RGB32 );

    /* Picture view */
    r = BRect( 10, 15, fOutWidthMax + 11, fOutHeightMax + 16 );
    fPictureView = new HBPictureView( r, fBitmap );
    pictureBox->AddChild( fPictureView );

    /* Slider */
    r = BRect( 10, fOutHeightMax + 25, fOutWidthMax + 11,
               fOutHeightMax + 55 );
    fPictureSlider = new BSlider( r, NULL, NULL,
                                  new BMessage( UPDATE_BITMAP ), 0, 9 );
    pictureBox->AddChild( fPictureSlider );

    view->AddChild( pictureBox );

    /* Second box : scale & crop settings */
    r = BRect( 10, fOutHeightMax + 75, fOutWidthMax + 31,
               fOutHeightMax + 235 );
    BBox * settingsBox;
    settingsBox = new BBox( r, NULL );
    settingsBox->SetLabel( "Settings" );

    r = BRect( 10, 15, fOutWidthMax + 11, 30 );
    fDeinterlaceCheck = new BCheckBox( r, NULL, "Deinterlace",
                                       new BMessage( UPDATE_BITMAP ) );
    fDeinterlaceCheck->SetValue( fDeinterlace ? 1 : 0 );
    settingsBox->AddChild( fDeinterlaceCheck );

    r = BRect( 10, 40, fOutWidthMax + 11, 70 );
    fWidthSlider = new BSlider( r, NULL, "Picture size",
                                new BMessage( UPDATE_BITMAP ),
                                1, fOutWidthMax / 16,
                                B_TRIANGLE_THUMB );
    fWidthSlider->SetValue( fOutWidth / 16 );
    settingsBox->AddChild( fWidthSlider );

    r = BRect( 10, 80, ( fOutWidthMax / 2 ) + 5, 110 );
    fTopCropSlider = new BSlider( r, NULL, "Top cropping",
                                  new BMessage( UPDATE_BITMAP ),
                                  0, fInHeight / 4,
                                  B_TRIANGLE_THUMB );
    fTopCropSlider->SetValue( fTopCrop / 2 );
    settingsBox->AddChild( fTopCropSlider );

    r = BRect( ( fOutWidthMax / 2 ) + 15, 80, fOutWidthMax + 11, 110 );
    fBottomCropSlider = new BSlider( r, NULL, "Bottom cropping",
                                     new BMessage( UPDATE_BITMAP ),
                                     0, fInHeight / 4,
                                     B_TRIANGLE_THUMB );
    fBottomCropSlider->SetValue( fBottomCrop / 2 );
    settingsBox->AddChild( fBottomCropSlider );

    r = BRect( 10, 120, ( fOutWidthMax / 2 ) + 5, 150 );
    fLeftCropSlider = new BSlider( r, NULL, "Left cropping",
                                   new BMessage( UPDATE_BITMAP ),
                                   0, fInWidth / 4,
                                   B_TRIANGLE_THUMB );
    fLeftCropSlider->SetValue( fLeftCrop / 2 );
    settingsBox->AddChild( fLeftCropSlider );

    r = BRect( ( fOutWidthMax / 2 ) + 15, 120, fOutWidthMax + 11, 150 );
    fRightCropSlider = new BSlider( r, NULL, "Right cropping",
                                    new BMessage( UPDATE_BITMAP ),
                                    0, fInWidth / 4,
                                    B_TRIANGLE_THUMB );
    fRightCropSlider->SetValue( fRightCrop / 2 );
    settingsBox->AddChild( fRightCropSlider );

    view->AddChild( settingsBox );

    /* "Close" button */
    r = BRect( fOutWidthMax - 49, fOutHeightMax + 245,
               fOutWidthMax + 31, fOutHeightMax + 270 );
    BButton * button = new BButton( r, NULL, "OK",
                                    new BMessage( B_QUIT_REQUESTED ) );
    view->AddChild( button );

    UpdateBitmap( 0 );
}

void HBPictureWin::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case UPDATE_BITMAP:
            UpdateBitmap( fPictureSlider->Value() );
            fPictureView->Draw( fPictureView->Bounds() );
            break;

        default:
            BWindow::MessageReceived( message );
    }
}

void HBPictureWin::UpdateBitmap( int image )
{
    fOutWidth    = 16 * fWidthSlider->Value();
    fTopCrop     = 2 * fTopCropSlider->Value();
    fBottomCrop  = 2 * fBottomCropSlider->Value();
    fLeftCrop    = 2 * fLeftCropSlider->Value();
    fRightCrop   = 2 * fRightCropSlider->Value();
    fDeinterlace = ( fDeinterlaceCheck->Value() != 0 );

    uint8_t * preview = HBGetPreview( fHandle, fTitle, image );
    for( int i = 0; i < fOutHeightMax + 2; i++ )
    {
        memcpy( ((uint8_t*) fBitmap->Bits()) +
                    i * fBitmap->BytesPerRow(),
                preview + 4 * ( fOutWidthMax + 2 ) * i,
                4 * ( fOutWidthMax + 2 ) );
    }
    free( preview );

    if( !Lock() )
    {
        fprintf( stderr, "Lock() failed\n" );
        return;
    }

    char label[128];

    memset( label, 0, 128 );
    snprintf( label, 128, "Picture size : %d x %d",
              fOutWidth, fOutHeight );
    fWidthSlider->SetValue( fOutWidth / 16 );
    fWidthSlider->SetLabel( label );

    memset( label, 0, 128 );
    snprintf( label, 128, "Top cropping : %d", fTopCrop );
    fTopCropSlider->SetLabel( label );

    memset( label, 0, 128 );
    snprintf( label, 128, "Bottom cropping : %d", fBottomCrop );
    fBottomCropSlider->SetLabel( label );

    memset( label, 0, 128 );
    snprintf( label, 128, "Left cropping : %d", fLeftCrop );
    fLeftCropSlider->SetLabel( label );

    memset( label, 0, 128 );
    snprintf( label, 128, "Right cropping : %d", fRightCrop );
    fRightCropSlider->SetLabel( label );

    Unlock();
}
