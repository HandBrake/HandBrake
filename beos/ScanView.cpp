/* $Id: ScanView.cpp,v 1.4 2003/10/13 23:42:03 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include <fs_info.h>
#include <sys/ioctl.h>
#include <Box.h>
#include <Button.h>
#include <Directory.h>
#include <Drivers.h>
#include <FilePanel.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <RadioButton.h>
#include <StringView.h>
#include <TextControl.h>
#include <VolumeRoster.h>

#include "Manager.h"
#include "ScanView.h"

ScanView::ScanView( HBManager * manager )
    : BView( BRect( 0,0,400,190 ), NULL, B_FOLLOW_ALL, B_WILL_DRAW )
{
    fManager = manager;
    
    BRect r;
    SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );

    r = BRect( 10, 10, Bounds().Width() - 10, 115 );
    BBox * box = new BBox( r );
    box->SetLabel( "Select source:" );
    AddChild( box );
   
    r = BRect( 10, 15, box->Bounds().Width() / 2, 35 );
    fRadioDetected = new BRadioButton( r, NULL, "Detected volume:",
                                       new BMessage( SCAN_RADIO ) );
    box->AddChild( fRadioDetected );
    
    r = BRect( box->Bounds().Width() / 2, 15,
               box->Bounds().Width() - 10, 35 );
    fPopUp = new BPopUpMenu( "" );
    fField = new BMenuField( r, NULL, NULL, fPopUp, true );
    fField->SetDivider( 0 );
    box->AddChild( fField );

    r = BRect( 10, 45, box->Bounds().Width() / 3, 65 );
    fRadioFolder = new BRadioButton( r, NULL, "DVD folder:",
                                     new BMessage( SCAN_RADIO ) );
    box->AddChild( fRadioFolder );

    r = BRect( box->Bounds().Width() / 3, 45,
               box->Bounds().Width() - 10, 65 );
    fFolderControl = new BTextControl( r, NULL, NULL, NULL, NULL );
    box->AddChild( fFolderControl );
    
    r = BRect( box->Bounds().Width() - 80, 70,
               box->Bounds().Width() - 10, 95 );
    fBrowseButton = new BButton( r, NULL, "Browse...",
                                 new BMessage( SCAN_BROWSE_BUTTON ) );
    box->AddChild( fBrowseButton );

    fFilePanel = new BFilePanel( B_OPEN_PANEL, NULL,
                                 NULL, B_DIRECTORY_NODE );

    r = BRect( 10, 125, Bounds().Width() - 10, 145 );
    fStatusString = new BStringView( r, NULL, NULL );
    AddChild( fStatusString );
    
    r = BRect( Bounds().Width() - 70, 155,
               Bounds().Width() - 10, 175 );
    fOpenButton = new BButton( r, NULL, "Open", new BMessage( SCAN_OPEN ) );
    fOpenButton->MakeDefault( true );
    AddChild( fOpenButton );

    DetectVolumes();
}

void ScanView::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case SCAN_RADIO:
        {
            if( fRadioDetected->Value() )
            {
                fField->SetEnabled( true );
                fFolderControl->SetEnabled( false );
                fBrowseButton->SetEnabled( false );
                fOpenButton->SetEnabled( fPopUp->CountItems() > 0 );
            }
            else
            {
                fField->SetEnabled( false );
                fFolderControl->SetEnabled( true );
                fBrowseButton->SetEnabled( true );
                fOpenButton->SetEnabled( true );
            }
            break;
        }

        case SCAN_BROWSE_BUTTON:
        {
            fFilePanel->Show();
            break;
        }

        case B_REFS_RECEIVED:
        {
            entry_ref ref;
            if( message->FindRef( "refs", 0, &ref ) == B_OK )
            {
                BPath * path = new BPath( &ref );
                fFolderControl->SetText( path->Path() );
            }
            break;
        }
        
        case SCAN_OPEN:
        {
            if( fRadioDetected->Value() )
            {
                fManager->ScanVolumes( (char*)
                                       fPopUp->FindMarked()->Label() );
            }
            else
            {
                fManager->ScanVolumes( (char*) fFolderControl->Text() );
            }
            break;
        }
        
        default:
            BView::MessageReceived( message );
    }
}

void ScanView::UpdateIntf( HBStatus status )
{
    switch( status.fMode )
    {
        case HB_MODE_SCANNING:
        {
            fRadioDetected->SetEnabled( false );
            fRadioFolder->SetEnabled( false );
            fField->SetEnabled( false );
            fFolderControl->SetEnabled( false );
            fBrowseButton->SetEnabled( false );
            fOpenButton->SetEnabled( false );

            char string[1024]; memset( string, 0, 1024 );
            if( !status.fScannedTitle )
            {
                sprintf( string, "Opening %s...",
                         status.fScannedVolume );
            }
            else
            {
                sprintf( string, "Scanning %s, title %d...",
                         status.fScannedVolume, status.fScannedTitle );
            }
            fStatusString->SetText( string );
            break;
        }

        case HB_MODE_INVALID_VOLUME:
        {
            fRadioDetected->SetEnabled( true );
            fRadioFolder->SetEnabled( true );

            if( fRadioDetected->Value() )
            {
                fField->SetEnabled( true );
                fFolderControl->SetEnabled( false );
                fBrowseButton->SetEnabled( false );
                fOpenButton->SetEnabled( fPopUp->CountItems() > 0 );
            }
            else
            {
                fField->SetEnabled( false );
                fFolderControl->SetEnabled( true );
                fBrowseButton->SetEnabled( true );
                fOpenButton->SetEnabled( true );
            }
            
            fStatusString->SetText( "Invalid volume." );
            break;
        }

        default:
            break;
    }
}

void ScanView::DetectVolumes()
{
    BVolumeRoster   * roster  = new BVolumeRoster();
    BVolume         * volume = new BVolume();
    fs_info           info;
    int               device;
    device_geometry   geometry;

    /* Parse mounted volumes */
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

            if( query->SetPredicate( "name = VIDEO_TS.BUP" ) != B_OK )
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
        fRadioDetected->SetValue( true );
        fFolderControl->SetEnabled( false );
        fBrowseButton->SetEnabled( false );
    }
    else
    {
        fRadioFolder->SetValue( true );
        fField->SetEnabled( false );
    }
}

