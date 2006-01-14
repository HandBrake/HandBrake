/* $Id: RipView.cpp,v 1.6 2003/10/13 23:42:03 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <RadioButton.h>
#include <StatusBar.h>
#include <String.h>
#include <TextControl.h>

#include "RipView.h"
#include "PictureWin.h"
#include "Manager.h"

#define DEFAULT_FILE "/boot/home/Desktop/Movie.avi"

RipView::RipView( HBManager * manager )
    : BView( BRect( 0,0,400,480 ), NULL, B_FOLLOW_ALL, B_WILL_DRAW )
{
    fManager = manager;
    
    BRect r;
    SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );
    
    /* Video box */
    r = BRect( 10, 10, Bounds().Width() - 10, 160 );
    fVideoBox = new BBox( r );
    fVideoBox->SetLabel( "Video" );
    AddChild( fVideoBox );

    /* Title */
    r = BRect( 10, 15, fVideoBox->Bounds().Width() - 10, 35 );
    fTitlePopUp = new BPopUpMenu( "" );
    fTitleField = new BMenuField( r, NULL, "Title:",
                                  fTitlePopUp, true );
    fVideoBox->AddChild( fTitleField );

    /* Video codec */
    r = BRect( 10, 40, fVideoBox->Bounds().Width() - 10, 60 );
    fVideoCodecPopUp = new BPopUpMenu( "" );
    fVideoCodecField = new BMenuField( r, NULL, "Codec:",
                                       fVideoCodecPopUp, true );
    fVideoBox->AddChild( fVideoCodecField );

    /* Video bitrate */
    r = BRect( 10, 65, fVideoBox->Bounds().Width() / 2, 85 );
    fCustomBitrateRadio =
        new BRadioButton( r, NULL, "Custom bitrate (kbps)",
                          new BMessage( RIP_BITRATE_RADIO ) );
    fCustomBitrateRadio->SetValue( 1 );
    fVideoBox->AddChild( fCustomBitrateRadio );
    r = BRect( fVideoBox->Bounds().Width() - 80, 65,
               fVideoBox->Bounds().Width() - 10, 85 );
    fCustomBitrateControl = new BTextControl( r, NULL, NULL, "1024", NULL );
    fCustomBitrateControl->SetDivider( 0 );
    fVideoBox->AddChild( fCustomBitrateControl );
    r = BRect( 10, 90, fVideoBox->Bounds().Width() / 2, 110 );
    fTargetSizeRadio =
        new BRadioButton( r, NULL, "Target size (MB)",
                          new BMessage( RIP_BITRATE_RADIO ) );
    fVideoBox->AddChild( fTargetSizeRadio );
    r = BRect( fVideoBox->Bounds().Width() - 80, 90,
               fVideoBox->Bounds().Width() - 10, 110 );
    fTargetSizeControl = new BTextControl( r, NULL, NULL, "700", NULL );
    fTargetSizeControl->SetDivider( 0 );
    fTargetSizeControl->SetEnabled( false );
    fTargetSizeControl->SetModificationMessage(
        new BMessage( RIP_TARGET_CONTROL ) );
    fVideoBox->AddChild( fTargetSizeControl );

    /* 2-pass */
    r = BRect( 10, 125, fVideoBox->Bounds().Width() / 2, 140 );
    fTwoPassCheck = new BCheckBox( r, NULL, "2-pass encoding", NULL );
    fVideoBox->AddChild( fTwoPassCheck );

    /* Crop */
    r = BRect( fVideoBox->Bounds().Width() - 120, 120,
               fVideoBox->Bounds().Width() - 10, 140 );
    fCropButton = new BButton( r, NULL, "Crop & Resize...",
                               new BMessage( RIP_CROP_BUTTON ) );
    fVideoBox->AddChild( fCropButton );

    /* Audio box */
    r = BRect( 10, 170, Bounds().Width() - 10, 290 );
    fAudioBox = new BBox( r );
    fAudioBox->SetLabel( "Audio" );
    AddChild( fAudioBox );

    /* Language */
    r = BRect( 10, 15, fAudioBox->Bounds().Width() - 10, 35 );
    fLanguagePopUp = new BPopUpMenu( "" );
    fLanguageField = new BMenuField( r, NULL, "Language:",
                                     fLanguagePopUp, true );
    fAudioBox->AddChild( fLanguageField );

    /* Secondary language */
    r = BRect( 10, 40, fAudioBox->Bounds().Width() - 10, 60 );
    fSecondaryLanguagePopUp = new BPopUpMenu( "" );
    fSecondaryLanguageField = new BMenuField( r, NULL, "Secondary language:",
                                              fSecondaryLanguagePopUp, true );
    fAudioBox->AddChild( fSecondaryLanguageField );

    /* Audio codec */
    r = BRect( 10, 65, fAudioBox->Bounds().Width() - 10, 85 );
    fAudioCodecPopUp = new BPopUpMenu( "" );
    fAudioCodecField = new BMenuField( r, NULL, "Codec:",
                                       fAudioCodecPopUp, true );
    fAudioBox->AddChild( fAudioCodecField );

    /* Audio bitrate */
    r = BRect( 10, 90, fAudioBox->Bounds().Width() - 10, 110 );
    fAudioBitratePopUp = new BPopUpMenu( "" );
    fAudioBitrateField = new BMenuField( r, NULL, "Bitrate:",
                                         fAudioBitratePopUp, true );
    fAudioBox->AddChild( fAudioBitrateField );

    /* Destination box */
    r = BRect( 10, 300, Bounds().Width() - 10, 395 );
    fDestinationBox = new BBox( r );
    fDestinationBox->SetLabel( "Destination" );
    AddChild( fDestinationBox );

    /* File format */
    r = BRect( 10, 15, fDestinationBox->Bounds().Width() - 10, 35 );
    fFileFormatPopUp = new BPopUpMenu( "" );
    fFileFormatField = new BMenuField( r, NULL, "File format:",
                                       fFileFormatPopUp, true );
    fDestinationBox->AddChild( fFileFormatField );

    /* File location */
    r = BRect( 10, 40, fDestinationBox->Bounds().Width() - 10, 60 );
    fFileControl = new BTextControl( r, NULL, "Location:",
                                     DEFAULT_FILE, NULL );
    fFileControl->SetDivider( 100 );
    fDestinationBox->AddChild( fFileControl );

    /* Browse button */
    r = BRect( fDestinationBox->Bounds().Width() - 80, 65,
               fDestinationBox->Bounds().Width() - 10, 85 );
    fFileButton = new BButton( r, NULL, "Browse...",
                               new BMessage( RIP_BROWSE_BUTTON ) );
    fDestinationBox->AddChild( fFileButton );

    fFilePanel = new BFilePanel( B_SAVE_PANEL, NULL, NULL, 0, false );

    /* Status bar */
    r = BRect( 10, 405, Bounds().Width() - 10, 435 );
    fStatusBar = new BStatusBar( r, NULL );
    AddChild( fStatusBar );

    /* Suspend/Rip buttons */
    r = BRect( Bounds().Width() - 180, 445,
               Bounds().Width() - 100, 465 );
    fSuspendButton = new BButton( r, NULL, "Suspend",
                                  new BMessage( RIP_SUSPEND_BUTTON ) );
    fSuspendButton->SetEnabled( false );
    AddChild( fSuspendButton );
    r = BRect( Bounds().Width() - 90, 445,
               Bounds().Width() - 10, 465 );
    fStartButton = new BButton( r, NULL, "Rip !",
                                new BMessage( RIP_RIP_BUTTON ) );
    fStartButton->MakeDefault( true );
    AddChild( fStartButton );

    /* Fill popups */
    fVideoCodecPopUp->AddItem( new BMenuItem( "MPEG-4", NULL ) );
    fVideoCodecPopUp->ItemAt( 0 )->SetMarked( true );
    fAudioCodecPopUp->AddItem( new BMenuItem( "MP3", NULL ) );
    fAudioCodecPopUp->ItemAt( 0 )->SetMarked( true );
    fAudioBitratePopUp->AddItem(
        new BMenuItem( "32", new BMessage( RIP_TARGET_CONTROL ) ) );
    fAudioBitratePopUp->AddItem(
        new BMenuItem( "64", new BMessage( RIP_TARGET_CONTROL ) ) );
    fAudioBitratePopUp->AddItem(
        new BMenuItem( "96", new BMessage( RIP_TARGET_CONTROL ) ) );
    fAudioBitratePopUp->AddItem(
        new BMenuItem( "128", new BMessage( RIP_TARGET_CONTROL ) ) );
    fAudioBitratePopUp->AddItem(
        new BMenuItem( "160", new BMessage( RIP_TARGET_CONTROL ) ) );
    fAudioBitratePopUp->AddItem(
        new BMenuItem( "192", new BMessage( RIP_TARGET_CONTROL ) ) );
    fAudioBitratePopUp->AddItem(
        new BMenuItem( "224", new BMessage( RIP_TARGET_CONTROL ) ) );
    fAudioBitratePopUp->AddItem(
        new BMenuItem( "256", new BMessage( RIP_TARGET_CONTROL ) ) );
    fAudioBitratePopUp->AddItem(
        new BMenuItem( "288", new BMessage( RIP_TARGET_CONTROL ) ) );
    fAudioBitratePopUp->AddItem(
        new BMenuItem( "320", new BMessage( RIP_TARGET_CONTROL ) ) );
    fAudioBitratePopUp->ItemAt( 3 )->SetMarked( true );
    fFileFormatPopUp->AddItem( new BMenuItem( "AVI", NULL ) );
    fFileFormatPopUp->ItemAt( 0 )->SetMarked( true );
}

void RipView::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case RIP_TITLE_POPUP:
        {
            int index = fTitlePopUp->IndexOf( fTitlePopUp->FindMarked() );
            HBTitle * title = (HBTitle*) fTitleList->ItemAt( index );

            /* Empty current popups */
            BMenuItem * item;
            while( ( item = fLanguagePopUp->ItemAt( 0 ) ) )
            {
                fLanguagePopUp->RemoveItem( item );
                delete item;
            }
            while( ( item = fSecondaryLanguagePopUp->ItemAt( 0 ) ) )
            {
                fSecondaryLanguagePopUp->RemoveItem( item );
                delete item;
            }

            /* Show new languages */
            HBAudio * audio;
            for( uint32_t i = 0; i < title->fAudioList->CountItems(); i++ )
            {
                audio = (HBAudio*) title->fAudioList->ItemAt( i );
                fLanguagePopUp->AddItem(
                    new BMenuItem( audio->fDescription, NULL ) );
                fSecondaryLanguagePopUp->AddItem(
                    new BMenuItem( audio->fDescription,
                                   new BMessage( RIP_TARGET_CONTROL ) ) );
            }
            fLanguagePopUp->ItemAt( 0 )->SetMarked( true );
            fSecondaryLanguagePopUp->AddItem( new BMenuItem( "None",
                new BMessage( RIP_TARGET_CONTROL ) ) );
            fSecondaryLanguagePopUp->ItemAt(
                fSecondaryLanguagePopUp->CountItems() - 1 )->SetMarked( true );
            
            fSecondaryLanguageField->SetEnabled(
                ( title->fAudioList->CountItems() > 1 ) );
            
            break;
        }

        case RIP_BITRATE_RADIO:
        {
            if( fCustomBitrateRadio->Value() )
            {
                fCustomBitrateControl->SetEnabled( true );
                fTargetSizeControl->SetEnabled( false );
            }
            else
            {
                fCustomBitrateControl->SetEnabled( false );
                fTargetSizeControl->SetEnabled( true );
                Window()->PostMessage( RIP_TARGET_CONTROL );
            }
            break;
        }

        case RIP_TARGET_CONTROL:
        {
            if( !fTargetSizeRadio->Value() )
            {
                break;
            }
            
            int64_t available;
            int index = fTitlePopUp->IndexOf( fTitlePopUp->FindMarked() );
            HBTitle * title = (HBTitle*) fTitleList->ItemAt( index );
            
            available  = (int64_t) 1024 * 1024 *
                             atoi( fTargetSizeControl->Text() );
            
            /* AVI headers */
            available -= 2048;

            /* Video chunk headers (8 bytes / frame) and
               and index (16 bytes / frame) */
            available -= 24 * title->fLength * title->fRate /
                             title->fScale;
            
            /* Audio tracks */
            available -=
                ( strcmp( fSecondaryLanguagePopUp->FindMarked()->Label(),
                          "None" ) ? 2 : 1 ) *
                ( title->fLength *
                  atoi( fAudioBitratePopUp->FindMarked()->Label() ) * 128 +
                  24 * title->fLength * 44100 / 1152 );
            
            char string[1024]; memset( string, 0, 1024 );
            if( available < 0 )
            {
                sprintf( string, "0" );
            }
            else
            {
                sprintf( string, "%lld", available /
                         ( 128 * title->fLength ) );
            }
            fCustomBitrateControl->SetText( string );
            break;
        }

        case RIP_CROP_BUTTON:
        {
            int index = fTitlePopUp->IndexOf( fTitlePopUp->FindMarked() );
            HBTitle * title = (HBTitle*) fTitleList->ItemAt( index );

            HBPictureWin * win;
            win = new HBPictureWin( fManager, title );
            win->Show();
            break;
        }

        case RIP_BROWSE_BUTTON:
        {
            fFilePanel->Show();
        }

        case B_SAVE_REQUESTED:
        {
            entry_ref ref;
            BString string;
            if( message->FindRef( "directory", 0, &ref ) == B_OK &&
                message->FindString( "name", &string ) == B_OK )
            {
                BPath * path = new BPath( &ref );
                string.Prepend( "/" );
                string.Prepend( path->Path() );
                fFileControl->SetText( string.String() );
            }
            break;
        }

        case RIP_SUSPEND_BUTTON:
        {
            if( strcmp( fSuspendButton->Label(), "Suspend" ) )
            {
                fManager->ResumeRip();
            }
            else
            {
                fManager->SuspendRip();
            }
            
            break;
        }

        case RIP_RIP_BUTTON:
        {
            if( strcmp( fStartButton->Label(), "Rip !" ) )
            {
                fManager->StopRip();
            }
            else
            {
                int index;
                
                /* Get asked title & languages */
                index = fTitlePopUp->IndexOf( fTitlePopUp->FindMarked() );
                HBTitle * title = (HBTitle*) fTitleList->ItemAt( index );
                index = fLanguagePopUp->IndexOf( fLanguagePopUp->FindMarked() );
                HBAudio * audio1 =
                    (HBAudio*) title->fAudioList->ItemAt( index );
                index = fSecondaryLanguagePopUp->IndexOf(
                    fSecondaryLanguagePopUp->FindMarked() );
                HBAudio * audio2 =
                    (HBAudio*) title->fAudioList->ItemAt( index );

                /* Use user settings */
                title->fBitrate = atoi( fCustomBitrateControl->Text() );
                title->fTwoPass = ( fTwoPassCheck->Value() != 0 );
                audio1->fOutBitrate =
                    atoi( fAudioBitratePopUp->FindMarked()->Label() );
                if( audio2 )
                {
                    audio2->fOutBitrate =
                        atoi( fAudioBitratePopUp->FindMarked()->Label() );
                }

                /* Let libhb do the job */
                fManager->StartRip( title, audio1, audio2,
                                    (char*) fFileControl->Text() );
            }
            break;
        }

        default:
            BView::MessageReceived( message );
            break;
    }
}

void RipView::UpdateIntf( HBStatus status )
{
    switch( status.fMode )
    {
        case HB_MODE_READY_TO_RIP:
        {
            fTitleList = status.fTitleList;
            
            HBTitle * title;
            for( uint32_t i = 0; i < fTitleList->CountItems(); i++ )
            {
                title = (HBTitle*) fTitleList->ItemAt( i );
                char string[1024]; memset( string, 0, 1024 );
                sprintf( string, "%d (%02lld:%02lld:%02lld)",
                         title->fIndex, title->fLength / 3600,
                         ( title->fLength % 3600 ) / 60,
                         title->fLength % 60 );
                fTitlePopUp->AddItem(
                    new BMenuItem( string, new BMessage( RIP_TITLE_POPUP ) ) );
            }
            fTitlePopUp->ItemAt( 0 )->SetMarked( true );
            Window()->PostMessage( RIP_TITLE_POPUP );
            break;
        }

        case HB_MODE_ENCODING:
        {
            fTitleField->SetEnabled( false );
            fVideoCodecField->SetEnabled( false );
            fCustomBitrateRadio->SetEnabled( false );
            fCustomBitrateControl->SetEnabled( false );
            fTargetSizeRadio->SetEnabled( false );
            fTargetSizeControl->SetEnabled( false );
            fTwoPassCheck->SetEnabled( false );
            fCropButton->SetEnabled( false );
            fLanguageField->SetEnabled( false );
            fSecondaryLanguageField->SetEnabled( false );
            fAudioCodecField->SetEnabled( false );
            fAudioBitrateField->SetEnabled( false );
            fFileFormatField->SetEnabled( false );
            fFileControl->SetEnabled( false );
            fFileButton->SetEnabled( false );

            if( !status.fPosition )
            {
                fStatusBar->Update( - fStatusBar->CurrentValue(),
                                    "Starting..." );
            }
            else
            {
                char string[1024]; memset( string, 0, 1024 );
                sprintf( string, "Encoding: %.2f %% (%.2f fps, "
                         "%02d:%02d:%02d remaining)",
                         100 * status.fPosition,
                         status.fFrameRate,
                         status.fRemainingTime / 3600,
                         ( status.fRemainingTime % 3600 ) / 60,
                         status.fRemainingTime % 60 );
                fStatusBar->Update( 100 * status.fPosition -
                                    fStatusBar->CurrentValue(),
                                    string );
            }
            
            fSuspendButton->SetLabel( "Suspend" );
            fSuspendButton->SetEnabled( true );
            fStartButton->SetLabel( "Cancel" );
            fStartButton->SetEnabled( true );
            break;
        }

        case HB_MODE_SUSPENDED:
        {
            fTitleField->SetEnabled( false );
            fVideoCodecField->SetEnabled( false );
            fCustomBitrateRadio->SetEnabled( false );
            fCustomBitrateControl->SetEnabled( false );
            fTargetSizeRadio->SetEnabled( false );
            fTargetSizeControl->SetEnabled( false );
            fTwoPassCheck->SetEnabled( false );
            fCropButton->SetEnabled( false );
            fLanguageField->SetEnabled( false );
            fSecondaryLanguageField->SetEnabled( false );
            fAudioCodecField->SetEnabled( false );
            fAudioBitrateField->SetEnabled( false );
            fFileFormatField->SetEnabled( false );
            fFileControl->SetEnabled( false );
            fFileButton->SetEnabled( false );

            fStatusBar->Update( 100 * status.fPosition -
                                fStatusBar->CurrentValue(), "Suspended" );
            
            fSuspendButton->SetLabel( "Resume" );
            fSuspendButton->SetEnabled( true );
            fStartButton->SetLabel( "Cancel" );
            fStartButton->SetEnabled( true );
            break;
        }

        case HB_MODE_STOPPING:
        {
            fTitleField->SetEnabled( false );
            fVideoCodecField->SetEnabled( false );
            fCustomBitrateRadio->SetEnabled( false );
            fCustomBitrateControl->SetEnabled( false );
            fTargetSizeRadio->SetEnabled( false );
            fTargetSizeControl->SetEnabled( false );
            fTwoPassCheck->SetEnabled( false );
            fCropButton->SetEnabled( false );
            fLanguageField->SetEnabled( false );
            fSecondaryLanguageField->SetEnabled( false );
            fAudioCodecField->SetEnabled( false );
            fAudioBitrateField->SetEnabled( false );
            fFileFormatField->SetEnabled( false );
            fFileControl->SetEnabled( false );
            fFileButton->SetEnabled( false );

            fStatusBar->Update( - fStatusBar->CurrentValue(),
                                "Stopping..." );
            
            fSuspendButton->SetLabel( "Suspend" );
            fSuspendButton->SetEnabled( false );
            fStartButton->SetLabel( "Cancel" );
            fStartButton->SetEnabled( false );
            break;
        }

        case HB_MODE_DONE:
        {
            fTitleField->SetEnabled( true );
            fVideoCodecField->SetEnabled( true );
            fCustomBitrateRadio->SetEnabled( true );
            fCustomBitrateControl->SetEnabled( fCustomBitrateRadio->Value() );
            fTargetSizeRadio->SetEnabled( true );
            fTargetSizeControl->SetEnabled( fTargetSizeRadio->Value() );
            fTwoPassCheck->SetEnabled( true );
            fCropButton->SetEnabled( true );
            fLanguageField->SetEnabled( true );
            fSecondaryLanguageField->SetEnabled(
                ( fSecondaryLanguagePopUp->CountItems() > 2 ) );
            fAudioCodecField->SetEnabled( true );
            fAudioBitrateField->SetEnabled( true );
            fFileFormatField->SetEnabled( true );
            fFileControl->SetEnabled( true );
            fFileButton->SetEnabled( true );

            fStatusBar->Update( 100.0 - fStatusBar->CurrentValue(),
                                "Done." );
            
            fSuspendButton->SetLabel( "Suspend" );
            fSuspendButton->SetEnabled( false );
            fStartButton->SetLabel( "Rip !" );
            fStartButton->SetEnabled( true );
            break;
        }

        case HB_MODE_CANCELED:
        {
            fTitleField->SetEnabled( true );
            fVideoCodecField->SetEnabled( true );
            fCustomBitrateRadio->SetEnabled( true );
            fCustomBitrateControl->SetEnabled( fCustomBitrateRadio->Value() );
            fTargetSizeRadio->SetEnabled( true );
            fTargetSizeControl->SetEnabled( fTargetSizeRadio->Value() );
            fTwoPassCheck->SetEnabled( true );
            fCropButton->SetEnabled( true );
            fLanguageField->SetEnabled( true );
            fSecondaryLanguageField->SetEnabled(
                ( fSecondaryLanguagePopUp->CountItems() > 2 ) );
            fAudioCodecField->SetEnabled( true );
            fAudioBitrateField->SetEnabled( true );
            fFileFormatField->SetEnabled( true );
            fFileControl->SetEnabled( true );
            fFileButton->SetEnabled( true );

            fStatusBar->Update( - fStatusBar->CurrentValue(),
                                "Canceled." );
            
            fSuspendButton->SetLabel( "Suspend" );
            fSuspendButton->SetEnabled( false );
            fStartButton->SetLabel( "Rip !" );
            fStartButton->SetEnabled( true );
            break;
        }

        case HB_MODE_ERROR:
        {
            fTitleField->SetEnabled( true );
            fVideoCodecField->SetEnabled( true );
            fCustomBitrateRadio->SetEnabled( true );
            fCustomBitrateControl->SetEnabled( fCustomBitrateRadio->Value() );
            fTargetSizeRadio->SetEnabled( true );
            fTargetSizeControl->SetEnabled( fTargetSizeRadio->Value() );
            fTwoPassCheck->SetEnabled( true );
            fCropButton->SetEnabled( true );
            fLanguageField->SetEnabled( true );
            fSecondaryLanguageField->SetEnabled(
                ( fSecondaryLanguagePopUp->CountItems() > 2 ) );
            fAudioCodecField->SetEnabled( true );
            fAudioBitrateField->SetEnabled( true );
            fFileFormatField->SetEnabled( true );
            fFileControl->SetEnabled( true );
            fFileButton->SetEnabled( true );

            fStatusBar->Update( - fStatusBar->CurrentValue(),
                                "Error." );
            
            fSuspendButton->SetLabel( "Suspend" );
            fSuspendButton->SetEnabled( false );
            fStartButton->SetLabel( "Rip !" );
            fStartButton->SetEnabled( true );
            break;
        }

        default:
            break;
    }
}

