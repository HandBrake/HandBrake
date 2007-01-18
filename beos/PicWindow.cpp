#include <app/Application.h>
#include <interface/Bitmap.h>
#include <interface/Box.h>
#include <interface/Button.h>
#include <interface/CheckBox.h>
#include <interface/RadioButton.h>
#include <interface/Screen.h>
#include <interface/StringView.h>

#include "PicWindow.h"
#include "Stepper.h"

#define MSG_PREV   'prev'
#define MSG_NEXT   'next'
#define MSG_CLOSE  'clos'
#define MSG_WIDTH  'widt'
#define MSG_HEIGHT 'heig'
#define MSG_RADIO  'radi'

PicView::PicView( hb_handle_t * handle, int index )
    : BView( BRect( 0,0,10,10 ), NULL, B_FOLLOW_NONE, B_WILL_DRAW )
{
    fHandle = handle;

    /* Get the title and the job */
    hb_list_t * list;
    list   = hb_get_titles( fHandle );
    fTitle = (hb_title_t *) hb_list_item( list, index );
    fJob   = fTitle->job;

    /* We'll start with the first picture */
    fIndex = 0;

    /* Allocate a buffer large enough to call hb_get_preview() later */
    fRawPic = (uint8_t *) malloc( ( fTitle->width + 2 ) *
                ( fTitle->height + 2 ) * 4 );

    /* Create the RGB BBitmap we'll use to display */
    fBitmap = new BBitmap( BRect( 0, 0, fTitle->width + 1,
                                  fTitle->height + 1 ), 0, B_RGB32 );

    /* Now build the interface */
    BRect r, b;
    BBox * box;
    BButton * button;
    BStringView * stringView;

    /* Resize ourselves so the picture fits just fine */
    b = fBitmap->Bounds();
    ResizeTo( b.Width()+170, b.Height()+65 );

    /* Now build the UI around the BBitmap */
    SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );

    /* "Size" box */
    b = Bounds();
    r = BRect( b.right-150,10,b.right-10,105 );
    box = new BBox( r );
    box->SetLabel( "Size" );
    AddChild( box );

    b = box->Bounds();

    /* Width */
    r = BRect( 10,15,b.right/2,35 );
    stringView = new BStringView( r, NULL, "Width:" );
    box->AddChild( stringView );
    r = BRect( b.right/2+1,15,b.right-10,35 );
    fWidthStepper = new HBStepper( r, 16, 16, fTitle->width,
        fJob->width, new BMessage( MSG_WIDTH ) );
    box->AddChild( fWidthStepper );

    /* Height */
    r = BRect( 10,40,b.right/2,60 );
    stringView = new BStringView( r, NULL, "Height:" );
    box->AddChild( stringView );
    r = BRect( b.right/2+1,40,b.right-10,60 );
    fHeightStepper = new HBStepper( r, 16, 16, fTitle->height,
        fJob->height, new BMessage( MSG_HEIGHT ) );
    box->AddChild( fHeightStepper );

    /* Aspect ratio */
    r = BRect( 10,65,b.right-10,85 );
    fRatioCheck = new BCheckBox( r, NULL, "Keep aspect ratio",
                                 new BMessage( MSG_WIDTH ) );
    fRatioCheck->SetValue( fJob->keep_ratio );
    box->AddChild( fRatioCheck );

    /* "Crop" box */
    b = Bounds();
    r = BRect( b.right-150,115,b.right-10,260 );
    box = new BBox( r );
    box->SetLabel( "Crop" );
    AddChild( box );

    b = box->Bounds();

    /* Automatic */
    r = BRect( 10,15,b.right-10,35 );
    fAutoRadio = new BRadioButton( r, NULL, "Automatic",
                                   new BMessage( MSG_RADIO ) );
    box->AddChild( fAutoRadio );

    /* Custom */
    r = BRect( 10,40,b.right-10,60 );
    fCustomRadio = new BRadioButton( r, NULL, "Custom:",
                                     new BMessage( MSG_RADIO ) );
    box->AddChild( fCustomRadio );
    float width = ( b.Width() - 30 ) / 2;
    r = BRect( (b.right-width)/2,65,(b.right+width)/2,85 );
    fCropSteppers[0] = new HBStepper( r, 2, 0, fTitle->height/2-2,
        fJob->crop[0], new BMessage( MSG_WIDTH ) );
    box->AddChild( fCropSteppers[0] );
    r = BRect( (b.right-width)/2,115,(b.right+width)/2,135 );
    fCropSteppers[1] = new HBStepper( r, 2, 0, fTitle->height/2-2,
        fJob->crop[1], new BMessage( MSG_WIDTH ) );
    box->AddChild( fCropSteppers[1] );
    r = BRect( 10,90,10+width,110 );
    fCropSteppers[2] = new HBStepper( r, 2, 0, fTitle->width/2-2,
        fJob->crop[2], new BMessage( MSG_HEIGHT ) );
    box->AddChild( fCropSteppers[2] );
    r = BRect( width+20,90,b.right-10,110 );
    fCropSteppers[3] = new HBStepper( r, 2, 0, fTitle->width/2-2,
        fJob->crop[3], new BMessage( MSG_HEIGHT ) );
    box->AddChild( fCropSteppers[3] );

    if( memcmp( fTitle->crop, fJob->crop, 4 * sizeof( int ) ) )
    {
        fCustomRadio->SetValue( 1 );
    }
    else
    {
        fAutoRadio->SetValue( 1 );
    }

    /* "Misc" box */
    b = Bounds();
    r = BRect( b.right-150,270,b.right-10,315 );
    box = new BBox( r );
    box->SetLabel( "Misc" );
    AddChild( box );

    b = box->Bounds();

    /* Deinterlace */
    r = BRect( 10,15,b.right-10,35 );
    fDeintCheck = new BCheckBox( r, NULL, "Deinterlace picture",
                                 new BMessage( MSG_WIDTH ) );
    fDeintCheck->SetValue( fJob->deinterlace );
    box->AddChild( fDeintCheck );

    b = Bounds();

    /* Next/Prev buttons */
    r = BRect( b.right-90,325,b.right-10,350 );
    fPrevButton = new BButton( r, NULL, "Previous",
                               new BMessage( MSG_PREV ) );
    AddChild( fPrevButton );
    r = BRect( b.right-90,355,b.right-10,380 );
    fNextButton = new BButton( r, NULL, "Next",
                               new BMessage( MSG_NEXT ) );
    AddChild( fNextButton );

    /* Info string and OK button */
    r = BRect( 10,b.bottom-30,b.right-100,b.bottom-10 );
    fInfoString = new BStringView( r, NULL, "" );
    AddChild( fInfoString );
    r = BRect( b.right-90,b.bottom-35,b.right-10,b.bottom-10 );
    button = new BButton( r, NULL, "OK", new BMessage( MSG_CLOSE ) );
    AddChild( button );

    /* Process and draw a first picture */
    RadioChanged();
    UpdateBitmap();
}

PicView::~PicView()
{
    free( fRawPic );
    delete fBitmap;
}

/************************************************************************
 * PicView::Draw
 ************************************************************************
 * Calls the inherited BView::Draw, plus draws the BBitmap preview
 * and the horizontal line above the info string and OK button
 ***********************************************************************/
void PicView::Draw( BRect rect )
{
    BRect b;

    BView::Draw( rect );

    if( LockLooper() )
    {
        b = fBitmap->Bounds();
        DrawBitmap( fBitmap, BRect( 10,10,b.Width()+10,b.Height()+10 ) );
        UnlockLooper();
    }

    b = Bounds();
    SetHighColor( 128,128,128 );
    StrokeLine( BPoint( 10,b.bottom-45 ), BPoint( b.right-10,b.bottom-45 ) );
    SetHighColor( 255,255,255 );
    StrokeLine( BPoint( 10,b.bottom-44 ), BPoint( b.right-10,b.bottom-44 ) );
}

void PicView::HandleMessage( BMessage * msg )
{
    switch( msg->what )
    {
        case MSG_PREV:
            fIndex--;
            UpdateBitmap();
            break;

        case MSG_NEXT:
            fIndex++;
            UpdateBitmap();
            break;

        case MSG_WIDTH:
        case MSG_HEIGHT:
            UpdateSettings( msg->what );
            UpdateBitmap();
            break;

        case MSG_RADIO:
            RadioChanged();
            break;

    }
}

void PicView::RadioChanged()
{
    int cus = fCustomRadio->Value();
    for( int i = 0; i < 4; i++ )
    {
        fCropSteppers[i]->SetEnabled( cus );
    }
    if( !cus )
    {
        memcpy( fJob->crop, fTitle->crop, 4 * sizeof( int ) );
        for( int i = 0; i < 4; i++ )
        {
            fCropSteppers[i]->SetValue( fJob->crop[i] );
        }
        UpdateSettings( MSG_WIDTH );
        UpdateBitmap();
    }
}

void PicView::UpdateSettings( uint32 what )
{
    fJob->width       = fWidthStepper->Value();
    fJob->height      = fHeightStepper->Value();
    fJob->keep_ratio  = fRatioCheck->Value();
    fJob->deinterlace = fDeintCheck->Value();
    for( int i = 0; i < 4; i++ )
    {
        fJob->crop[i] = fCropSteppers[i]->Value();
    }

    if( fJob->keep_ratio )
    {
        if( what == MSG_WIDTH )
        {
            hb_fix_aspect( fJob, HB_KEEP_WIDTH );
            if( fJob->height > fTitle->height )
            {
                fJob->height = fTitle->height;
                hb_fix_aspect( fJob, HB_KEEP_HEIGHT );
            }
        }
        else
        {
            hb_fix_aspect( fJob, HB_KEEP_HEIGHT );
            if( fJob->width > fTitle->width )
            {
                fJob->width = fTitle->width;
                hb_fix_aspect( fJob, HB_KEEP_WIDTH );
            }
        }
    }

    fWidthStepper->SetValue( fJob->width );
    fHeightStepper->SetValue( fJob->height );
}

void PicView::UpdateBitmap()
{
    /* Sanity checks */
    if( fIndex < 0 )
    {
        fIndex = 0;
        return;
    }
    if( fIndex > 9 )
    {
        fIndex = 9;
        return;
    }

    /* Enable/disable buttons */
    fPrevButton->SetEnabled( fIndex > 0 );
    fNextButton->SetEnabled( fIndex < 9 );

    /* Get new preview and copy it in our BBitmap */
    hb_get_preview( fHandle, fTitle, fIndex, fRawPic );
    for( int i = 0; i < fTitle->height + 2; i++ )
    {
        memcpy( ( (uint8_t *) fBitmap->Bits() ) +
                    i * fBitmap->BytesPerRow(),
                fRawPic + 4 * ( fTitle->width + 2 ) * i,
                4 * ( fTitle->width + 2 ) );
    }

    /* Update size info */
    char text[1024];
    snprintf( text, 1024, "Source: %dx%d, output: %dx%d",
              fTitle->width, fTitle->height,
              fJob->width, fJob->height );
    fInfoString->SetText( text );

    /* Force redraw */
    Draw( Bounds() );
}

PicWindow::PicWindow( hb_handle_t * handle, int index )
    : BWindow( BRect( 0,0,10,10 ), "Picture settings",
               B_FLOATING_WINDOW_LOOK, B_MODAL_APP_WINDOW_FEEL,
               B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
    /* Add the PicView */
    fView = new PicView( handle, index );
    AddChild( fView );
    
    /* Resize to fit */
    ResizeTo( fView->Bounds().Width(), fView->Bounds().Height() );
    
    /* Center */
    BScreen screen;
    MoveTo( ( screen.Frame().Width() - fView->Bounds().Width() ) / 2,
            ( screen.Frame().Height() - fView->Bounds().Height() ) / 2 );
}

void PicWindow::MessageReceived( BMessage * msg )
{
    switch( msg->what )
    {
        case MSG_PREV:
        case MSG_NEXT:
        case MSG_WIDTH:
        case MSG_HEIGHT:
        case MSG_RADIO:
            fView->HandleMessage( msg );
            break;

        case MSG_CLOSE:
            Lock();
            Quit();
            break;

        default:
            BWindow::MessageReceived( msg );
    }
}
