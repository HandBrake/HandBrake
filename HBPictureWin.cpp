/* $Id: HBPictureWin.cpp,v 1.15 2003/08/25 21:50:48 titer Exp $ */

#include "HBCommon.h"
#include "HBPictureWin.h"
#include "HBManager.h"

#include <Bitmap.h>
#include <Box.h>
#include <CheckBox.h>
#include <Slider.h>

#include <ffmpeg/avcodec.h>

#define UPDATE_BITMAP 'upbi'

/* Handy way to access HBTitleInfo members */
#define fInWidth     fTitleInfo->fInWidth
#define fInHeight    fTitleInfo->fInHeight
#define fPixelWidth  fTitleInfo->fPixelWidth
#define fPixelHeight fTitleInfo->fPixelHeight
#define fDeinterlace fTitleInfo->fDeinterlace
#define fOutWidth    fTitleInfo->fOutWidth
#define fOutHeight   fTitleInfo->fOutHeight
#define fTopCrop     fTitleInfo->fTopCrop
#define fBottomCrop  fTitleInfo->fBottomCrop
#define fLeftCrop    fTitleInfo->fLeftCrop
#define fRightCrop   fTitleInfo->fRightCrop
#define fPictures    fTitleInfo->fPictures

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
        Log( "HBPictureView::Draw() : LockLooper() failed" );
    }
    
    BView::Draw( rect );
}


/* Constructor */
HBPictureWin::HBPictureWin( HBTitleInfo * titleInfo )
    : BWindow( BRect( 50, 50, 60, 60 ), "Picture settings",
               B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
    fTitleInfo    = titleInfo;
    
    fMaxOutWidth  = MULTIPLE_16( fInWidth );
    fMaxOutHeight = MULTIPLE_16( fInHeight * fPixelHeight / fPixelWidth );
    
    /* Leave a one-pixel margin to draw the white line around the picture */
    fBitmap       = new BBitmap( BRect( 0, 0, fMaxOutWidth + 1, fMaxOutHeight + 1 ),
                                 0, B_RGB32 );

    ResizeTo( fMaxOutWidth + 40, fMaxOutHeight + 331 );

    BRect r;

    /* Add a background view */
    BView * view;
    view = new BView( Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW );
    view->SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );
    AddChild( view );
    
    /* First box : picture + slider */
    r = BRect( 10, 10, fMaxOutWidth + 31, fMaxOutHeight + 68 );
    BBox * pictureBox;
    pictureBox = new BBox( r, NULL );
    pictureBox->SetLabel( "Preview" );
    
    /* Picture view */
    r = BRect( 10, 15, fMaxOutWidth + 11, fMaxOutHeight + 16 );
    fPictureView = new HBPictureView( r, fBitmap );
    pictureBox->AddChild( fPictureView );
    
    /* Slider */
    r = BRect( 10, fMaxOutHeight + 26, fMaxOutWidth + 10, fMaxOutHeight + 56 );
    fPictureSlider = new BSlider( r, NULL, NULL, new BMessage( UPDATE_BITMAP ), 0, 9 );
    pictureBox->AddChild( fPictureSlider );
    
    view->AddChild( pictureBox );
    
    /* Second box : resize & crop settings */
    r = BRect( 10, fMaxOutHeight + 76, fMaxOutWidth + 30, fMaxOutHeight + 321 );
    BBox * settingsBox;
    settingsBox = new BBox( r, NULL );
    settingsBox->SetLabel( "Settings" );

    r = BRect( 10, 15, fMaxOutWidth + 10, 45 );
    fWidthSlider = new BSlider( r, NULL, "Picture size",
                                new BMessage( UPDATE_BITMAP ),
                                1, fMaxOutWidth / 16,
                                B_TRIANGLE_THUMB );
    fWidthSlider->SetValue( fMaxOutWidth / 16 );
    settingsBox->AddChild( fWidthSlider );
    
    r = BRect( 10, 55, fMaxOutWidth + 10, 85 );
    fTopCropSlider = new BSlider( r, NULL, "Top cropping",
                                  new BMessage( UPDATE_BITMAP ),
                                  0, fInHeight / 4,
                                  B_TRIANGLE_THUMB );
    settingsBox->AddChild( fTopCropSlider );

    r = BRect( 10, 95, fMaxOutWidth + 10, 125 );
    fBottomCropSlider = new BSlider( r, NULL, "Bottom cropping",
                                     new BMessage( UPDATE_BITMAP ),
                                     0, fInHeight / 4,
                                     B_TRIANGLE_THUMB );
    settingsBox->AddChild( fBottomCropSlider );

    r = BRect( 10, 135, fMaxOutWidth + 10, 165 );
    fLeftCropSlider = new BSlider( r, NULL, "Left cropping",
                                   new BMessage( UPDATE_BITMAP ),
                                   0, fInWidth / 4,
                                   B_TRIANGLE_THUMB );
    settingsBox->AddChild( fLeftCropSlider );

    r = BRect( 10, 175, fMaxOutWidth + 10, 205 );
    fRightCropSlider = new BSlider( r, NULL, "Right cropping",
                                    new BMessage( UPDATE_BITMAP ),
                                    0, fInWidth / 4,
                                    B_TRIANGLE_THUMB );
    settingsBox->AddChild( fRightCropSlider );
    
    r = BRect( 10, 215, fMaxOutWidth + 10, 235 );
    fDeinterlaceCheck = new BCheckBox( r, NULL, "Deinterlace",
                                       new BMessage( UPDATE_BITMAP ) );
    settingsBox->AddChild( fDeinterlaceCheck );
    
    view->AddChild( settingsBox );
    
    /* Buttons */
    
    
    Hide();
    Show();
    
    UpdateBitmap( 0 );
}

bool HBPictureWin::QuitRequested()
{
    if( Lock() )
    {
        Hide();
        Unlock();
    }
    else
    {
        Log( "HBPictureWin::QuitRequested : cannot Lock()" );
    }
    return false;
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

void HBPictureWin::UpdateBitmap( int which )
{
    fTopCrop     = 2 * fTopCropSlider->Value();
    fBottomCrop  = 2 * fBottomCropSlider->Value();
    fLeftCrop    = 2 * fLeftCropSlider->Value();
    fRightCrop   = 2 * fRightCropSlider->Value();
    fDeinterlace = ( fDeinterlaceCheck->Value() != 0 );

    fOutWidth    = MULTIPLE_16( 16 * fWidthSlider->Value() - fLeftCrop - fRightCrop );
    fOutHeight   = MULTIPLE_16( ( fInHeight - fTopCrop - fBottomCrop ) *
                                ( 16 * fWidthSlider->Value() * fPixelHeight ) /
                                ( fInWidth * fPixelWidth ) );
    
    AVPicture pic1; /* original YUV picture */
    avpicture_fill( &pic1, fPictures[which],
                    PIX_FMT_YUV420P, fInWidth, fInHeight );

    AVPicture pic2; /* deinterlaced YUV picture */
    uint8_t * buf2 = (uint8_t*) malloc( 3 * fInWidth * fInHeight / 2 );
    avpicture_fill( &pic2, buf2, PIX_FMT_YUV420P, fInWidth, fInHeight );

    AVPicture pic3; /* resized YUV picture */
    uint8_t * buf3 = (uint8_t*) malloc( 3 * fOutWidth * fOutHeight / 2 );
    avpicture_fill( &pic3, buf3, PIX_FMT_YUV420P, fOutWidth, fOutHeight );

    AVPicture pic4; /* resized RGB picture */
    uint8_t * buf4 = (uint8_t*) malloc( 4 * fOutWidth * fOutHeight );
    avpicture_fill( &pic4, buf4, PIX_FMT_RGBA32, fOutWidth, fOutHeight );

    ImgReSampleContext * resampleContext;
    resampleContext = img_resample_full_init( fOutWidth, fOutHeight,
                                              fInWidth, fInHeight,
                                              fTopCrop, fBottomCrop,
                                              fLeftCrop, fRightCrop );
    
    if( fDeinterlace )
    {
        avpicture_deinterlace( &pic2, &pic1, PIX_FMT_YUV420P, fInWidth, fInHeight );
        img_resample( resampleContext, &pic3, &pic2 );
    }
    else
    {
        img_resample( resampleContext, &pic3, &pic1 );
    }
    img_convert( &pic4, PIX_FMT_RGBA32, &pic3, PIX_FMT_YUV420P, fOutWidth, fOutHeight );

    /* Blank the bitmap */
    for( uint32_t i = 0; i < fMaxOutHeight + 2; i++ )
    {
        memset( (uint8_t*) fBitmap->Bits() + i * fBitmap->BytesPerRow(),
                0, ( fMaxOutWidth + 2 ) * 4 );
    }
        
    /* Draw the picture (centered) */
    uint32_t leftOffset = 1 + ( fMaxOutWidth - fOutWidth ) / 2;
    uint32_t topOffset  = 1 + ( fMaxOutHeight - fOutHeight ) / 2;
    for( uint32_t i = 0; i < fOutHeight; i++ )
    {
        memcpy( (uint8_t*) fBitmap->Bits() +
                    ( i + topOffset ) * fBitmap->BytesPerRow() +
                    leftOffset * 4,
                buf4 + i * fOutWidth * 4,
                fOutWidth * 4 );
    }

    /* Draw the cropping zone */
    memset( (uint8_t*) fBitmap->Bits() +
                ( topOffset - 1 ) * fBitmap->BytesPerRow() +
                ( leftOffset - 1 ) * 4,
            0xFF,
            ( fOutWidth + 2 ) * 4 );
    
    for( uint32_t i = 0; i < fOutHeight + 2; i++ )
    {
        memset( (uint8_t*) fBitmap->Bits() +
                    ( i + ( topOffset - 1 ) ) * fBitmap->BytesPerRow() +
                    ( leftOffset - 1 ) * 4,
                0xFF,
                4 );
        memset( (uint8_t*) fBitmap->Bits() +
                   ( i + ( topOffset - 1 ) ) * fBitmap->BytesPerRow() +
                   ( leftOffset + fOutWidth ) * 4,
                0xFF,
                4 );
    }

    memset( (uint8_t*) fBitmap->Bits() +
                ( topOffset + fOutHeight ) * fBitmap->BytesPerRow() +
                ( leftOffset - 1 ) * 4,
            0xFF,
            ( fOutWidth + 2 ) * 4 );

    /* Clean up */
    free( buf2 );
    free( buf3 );
    free( buf4 );
    
    /* Show the output size */
    if( !Lock() )
    {
        Log( "HBPictureWin::UpdateBitmap() : cannot Lock()" );
        return;
    }
    
    char label[128]; memset( label, 0, 128 );
    snprintf( label, 128, "Picture size : %d x %d",
              fOutWidth, fOutHeight );
    fWidthSlider->SetLabel( label );
    
    Unlock();
}
