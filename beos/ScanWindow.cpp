#include <drivers/Drivers.h>
#include <interface/Box.h>
#include <interface/Button.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/RadioButton.h>
#include <interface/Screen.h>
#include <interface/StatusBar.h>
#include <interface/StringView.h>
#include <interface/TextControl.h>
#include <kernel/fs_info.h>
#include <storage/FilePanel.h>
#include <storage/Path.h>
#include <storage/Query.h>
#include <storage/VolumeRoster.h>
#include <sys/ioctl.h>

#include "ScanWindow.h"

#define MSG_RADIO  'radi'
#define MSG_BROWSE 'brow'
#define MSG_CANCEL 'canc'
#define MSG_OPEN   'open'

ScanView::ScanView( hb_handle_t * handle )
    : BView( BRect( 0,0,400,215 ), NULL, B_FOLLOW_NONE, B_WILL_DRAW )
{
    fHandle = handle;

    BRect r, b;
    BBox * box;
    BStringView * stringView;
    
    SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );

    b = Bounds();

    r = BRect( 10,10,b.right-10,130 );
    box = new BBox( r );
    AddChild( box );

    b = box->Bounds();

    r = BRect( 10,10,b.right-10,30 );
    stringView = new BStringView( r, NULL, "Select a DVD:" );
    box->AddChild( stringView );

    r = BRect( 10,35,b.right/2,55 );
    fDetectedRadio = new BRadioButton( r, NULL, "Detected volume",
                                       new BMessage( MSG_RADIO ) );
    box->AddChild( fDetectedRadio );

    r = BRect( b.right/2+1,35,b.right-10,55 );
    fPopUp = new BPopUpMenu( "No volume detected" );
    fMenu = new BMenuField( r, NULL, "", fPopUp, true );
    fMenu->SetDivider( 0 );
    box->AddChild( fMenu );

    r = BRect( 10,60,b.right/2,80 );
    fFolderRadio = new BRadioButton( r, NULL, "DVD Folder / Image",
                                     new BMessage( MSG_RADIO ) );
    box->AddChild( fFolderRadio );

    r = BRect( b.right/2+1,60,b.right-10,80 );
    fControl = new BTextControl( r, NULL, "", "", new BMessage() );
    fControl->SetDivider( 0 );
    box->AddChild( fControl );

    r = BRect( b.right-90,85,b.right-10,110 );
    fBrowseButton = new BButton( r, NULL, "Browse",
        new BMessage( MSG_BROWSE ) );
    box->AddChild( fBrowseButton );

    b = Bounds();

    r = BRect( 10,b.bottom-75,b.right-10,b.bottom-45 );
    fBar = new BStatusBar( r, NULL, NULL, NULL );
    AddChild( fBar );

    r = BRect( b.right-180,b.bottom-35,b.right-100,b.bottom-10 );
    fCancelButton = new BButton( r, NULL, "Cancel",
        new BMessage( MSG_CANCEL ) );
    AddChild( fCancelButton );

    r = BRect( b.right-90,b.bottom-35,b.right-10,b.bottom-10 );
    fOpenButton = new BButton( r, NULL, "Open", new BMessage( MSG_OPEN ) );
    AddChild( fOpenButton );

    DetectVolumes();

    if( fPopUp->CountItems() > 0 )
    {
        fDetectedRadio->SetValue( true );
    }
    else
    {
        fFolderRadio->SetValue( true );
    }
    RadioChanged();

    fFilePanel = NULL;
}

void ScanView::HandleMessage( BMessage * msg )
{
    switch( msg->what )
    {
        case MSG_RADIO:
            RadioChanged();
            break;

        case MSG_BROWSE:
            if( !fFilePanel )
            {
                fFilePanel = new BFilePanel( B_OPEN_PANEL,
                    new BMessenger( Window() ), NULL, 
                    B_FILE_NODE | B_DIRECTORY_NODE, false, NULL, NULL,
                    true );
            }
            fFilePanel->Show();
            break;

        case B_REFS_RECEIVED:
        {
            entry_ref ref;
            if( msg->FindRef( "refs", 0, &ref ) == B_OK )
            {
                BPath * path = new BPath( &ref );
                fControl->SetText( path->Path());
            }
            break;
        }

        case MSG_CANCEL:
            Window()->Hide();
            break;

        case MSG_OPEN:
            SetEnabled( false );
            fBar->Update( - fBar->CurrentValue(), "Opening..." );
            if( fDetectedRadio->Value() && fPopUp->CountItems() > 0 )
            {
                hb_scan( fHandle, fPopUp->FindMarked()->Label(), 0 );
            }
            else if( fFolderRadio->Value() )
            {
                hb_scan( fHandle, fControl->Text(), 0 );
            }
            break;
    }
}

void ScanView::Update( hb_state_t * s )
{
    if( !LockLooper() )
    {
        return;
    }

    switch( s->state )
    {
#define p s->param.scanning
        case HB_STATE_SCANNING:
        {
            char text[1024];
            snprintf( text, 1024, "Scanning title %d of %d...", p.title_cur,
                  p.title_count );
            fBar->Update( fBar->MaxValue() * ( - 0.5 + p.title_cur ) /
                  p.title_count - fBar->CurrentValue(), text );
            break;
        }
#undef p

        case HB_STATE_SCANDONE:
            /* If we are still here, then no title was found */
            fBar->Update( - fBar->CurrentValue(),
                          "No valid title found." );
            SetEnabled( true );
            break;
    }

    UnlockLooper();
}

void ScanView::RadioChanged()
{
    bool b = fDetectedRadio->Value();
    fMenu->SetEnabled( b );
    fControl->SetEnabled( !b );
    fBrowseButton->SetEnabled( !b );
    fOpenButton->SetEnabled( !b || ( fPopUp->CountItems() > 0 ) );
}

void ScanView::SetEnabled( bool b )
{
    fDetectedRadio->SetEnabled( b );
    fMenu->SetEnabled( b );
    fFolderRadio->SetEnabled( b );
    fControl->SetEnabled( b );
    fBrowseButton->SetEnabled( b );
    fOpenButton->SetEnabled( b );

    if( b )
    {
        RadioChanged();
    }
}

void ScanView::DetectVolumes()
{
    BVolumeRoster   * roster  = new BVolumeRoster();
    BVolume         * volume = new BVolume();
    fs_info           info;
    int               device;
    device_geometry   geometry;

    while( roster->GetNextVolume( volume ) == B_NO_ERROR )
    {
        /* open() and ioctl() for more informations */
        fs_stat_dev( volume->Device(), &info );
        if( ( device = open( info.device_name, O_RDONLY ) ) < 0 )
        {
            continue;
        }

        if( ioctl( device, B_GET_GEOMETRY, &geometry,
                   sizeof( geometry ) ) < 0 )
        {
            continue;
        }

        /* Get the volume name */
        char volumeName[B_FILE_NAME_LENGTH];
        volume->GetName( volumeName );

        if( volume->IsReadOnly() && geometry.device_type == B_CD )
        {
            /* May be a DVD */
            fPopUp->AddItem( new BMenuItem( info.device_name, NULL ) );
        }
        else if( geometry.device_type == B_DISK )
        {
            /* May be a hard drive. Look for VIDEO_TS folders on it */
            BQuery * query = new BQuery();

            if( query->SetVolume( volume ) != B_OK )
            {
                delete query;
                continue;
            }

            if( query->SetPredicate( "name = VIDEO_TS.IFO" ) != B_OK )
            {
                delete query;
                continue;
            }

            query->Fetch();

            BEntry entry, parentEntry;
            BPath  path;
            while( query->GetNextEntry( &entry ) == B_OK )
            {
                entry.GetParent( &parentEntry );
                parentEntry.GetPath( &path );

                fPopUp->AddItem( new BMenuItem( path.Path(), NULL ) );
            }

            delete query;
        }
    }

    if( fPopUp->CountItems() > 0 )
    {
        fPopUp->ItemAt( 0 )->SetMarked( true );
    }
}

ScanWindow::ScanWindow( hb_handle_t * handle )
    : BWindow( BRect( 0,0,10,10 ), "Scan", B_FLOATING_WINDOW_LOOK,
               B_MODAL_SUBSET_WINDOW_FEEL, B_NOT_CLOSABLE |
               B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
    /* Add the scan view */
    fView = new ScanView( handle );
    AddChild( fView );
    
    /* Resize to fit */
    ResizeTo( fView->Bounds().Width(), fView->Bounds().Height() );
    
    /* Center */
    BScreen screen;
    MoveTo( ( screen.Frame().Width() - fView->Bounds().Width() ) / 2,
            ( screen.Frame().Height() - fView->Bounds().Height() ) / 2 );
}

void ScanWindow::MessageReceived( BMessage * msg )
{
    switch( msg->what )
    {
        case MSG_RADIO:
        case MSG_BROWSE:
        case MSG_CANCEL:
        case MSG_OPEN:
        case B_REFS_RECEIVED:
            fView->HandleMessage( msg );
            break;

        default:
            BWindow::MessageReceived( msg );
    }
}

void ScanWindow::Update( hb_state_t * s )
{
    fView->Update( s );
}

