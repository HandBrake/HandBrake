/* $Id: RipView.cpp,v 1.3 2003/11/07 21:52:56 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
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

#define DEFAULT_FILE "/boot/home/Desktop/Movie.avi"

RipView::RipView( HBHandle * handle )
    : BView( BRect( 0,0,400,480 ), NULL, B_FOLLOW_ALL, B_WILL_DRAW )
{
    fHandle = handle;
    
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
    fCropButton = new BButton( r, NULL, "Crop & Scale...",
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
    fVideoCodecPopUp->AddItem( new BMenuItem( "MPEG-4 (Ffmpeg)",
        new BMessage( RIP_VIDEO_CODEC_POPUP ) ) );
    fVideoCodecPopUp->AddItem( new BMenuItem( "MPEG-4 (XviD)",
        new BMessage( RIP_VIDEO_CODEC_POPUP ) ) );
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
            HBTitle * title = (HBTitle*) HBListItemAt( fTitleList, index );

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
            for( int i = 0; i < HBListCountItems( title->audioList ); i++ )
            {
                audio = (HBAudio*) HBListItemAt( title->audioList, i );
                fLanguagePopUp->AddItem(
                    new BMenuItem( audio->language, NULL ) );
                fSecondaryLanguagePopUp->AddItem(
                    new BMenuItem( audio->language,
                                   new BMessage( RIP_TARGET_CONTROL ) ) );
            }
            fLanguagePopUp->ItemAt( 0 )->SetMarked( true );
            fSecondaryLanguagePopUp->AddItem( new BMenuItem( "None",
                new BMessage( RIP_TARGET_CONTROL ) ) );
            fSecondaryLanguagePopUp->ItemAt(
                fSecondaryLanguagePopUp->CountItems() - 1 )->SetMarked( true );
            
            fSecondaryLanguageField->SetEnabled(
                ( HBListCountItems( title->audioList ) > 1 ) );
            
            break;
        }

        case RIP_VIDEO_CODEC_POPUP:
        {
            if( fVideoCodecPopUp->IndexOf( fVideoCodecPopUp->FindMarked() ) )
            {
                fTwoPassCheck->SetValue( 0 );
                fTwoPassCheck->SetEnabled( false );
            }
            else
            {
                fTwoPassCheck->SetEnabled( true );
            }
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
            HBTitle * title = (HBTitle*) HBListItemAt( fTitleList, index );
            
            available  = (int64_t) 1024 * 1024 *
                             atoi( fTargetSizeControl->Text() );
            
            /* AVI headers */
            available -= 2048;

            /* Video chunk headers (8 bytes / frame) and
               and index (16 bytes / frame) */
            available -= 24 * title->length * title->rate /
                             title->rateBase;
            
            /* Audio tracks */
            available -=
                ( strcmp( fSecondaryLanguagePopUp->FindMarked()->Label(),
                          "None" ) ? 2 : 1 ) *
                ( title->length *
                  atoi( fAudioBitratePopUp->FindMarked()->Label() ) * 128 +
                  24 * title->length * 44100 / 1152 );
            
            char string[1024]; memset( string, 0, 1024 );
            if( available < 0 )
            {
                sprintf( string, "0" );
            }
            else
            {
                sprintf( string, "%lld", available /
                         ( 128 * title->length ) );
            }
            fCustomBitrateControl->SetText( string );
            break;
        }

        case RIP_CROP_BUTTON:
        {
            int index = fTitlePopUp->IndexOf( fTitlePopUp->FindMarked() );
            HBTitle * title = (HBTitle*) HBListItemAt( fTitleList, index );

            HBPictureWin * win;
            win = new HBPictureWin( fHandle, title );
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
                HBResumeRip( fHandle );
            }
            else
            {
                HBPauseRip( fHandle );
            }
            
            break;
        }

        case RIP_RIP_BUTTON:
        {
            if( strcmp( fStartButton->Label(), "Rip !" ) )
            {
                HBStopRip( fHandle );
            }
            else
            {
                int index;
                
                /* Get asked title & languages */
                index = fTitlePopUp->IndexOf( fTitlePopUp->FindMarked() );
                HBTitle * title = (HBTitle*) HBListItemAt( fTitleList, index );
                index = fLanguagePopUp->IndexOf( fLanguagePopUp->FindMarked() );
                HBAudio * audio1 =
                    (HBAudio*) HBListItemAt( title->audioList, index );
                index = fSecondaryLanguagePopUp->IndexOf(
                    fSecondaryLanguagePopUp->FindMarked() );
                HBAudio * audio2 =
                    (HBAudio*) HBListItemAt( title->audioList, index );

                /* Use user settings */
                title->file = strdup( fFileControl->Text() );
                title->bitrate = atoi( fCustomBitrateControl->Text() );
                title->twoPass = ( fTwoPassCheck->Value() != 0 );
                title->codec = fVideoCodecPopUp->IndexOf(
                    fVideoCodecPopUp->FindMarked() ) ? HB_CODEC_XVID :
                    HB_CODEC_FFMPEG;
                audio1->outBitrate =
                    atoi( fAudioBitratePopUp->FindMarked()->Label() );
                if( audio2 )
                {
                    audio2->outBitrate =
                        atoi( fAudioBitratePopUp->FindMarked()->Label() );
                }

                /* Let libhb do the job */
                HBStartRip( fHandle, title, audio1, audio2 );
            }
            break;
        }

        default:
            BView::MessageReceived( message );
            break;
    }
}

void RipView::UpdateIntf( HBStatus status, int modeChanged )
{
    switch( status.mode )
    {
        case HB_MODE_READY_TO_RIP:
        {
            if( !modeChanged )
                break;
            
            fTitleList = status.titleList;
            
            HBTitle * title;
            for( int i = 0; i < HBListCountItems( fTitleList ); i++ )
            {
                title = (HBTitle*) HBListItemAt( fTitleList, i );
                char string[1024]; memset( string, 0, 1024 );
                sprintf( string, "%d (%02d:%02d:%02d)",
                         title->index, title->length / 3600,
                         ( title->length % 3600 ) / 60,
                         title->length % 60 );
                fTitlePopUp->AddItem(
                    new BMenuItem( string, new BMessage( RIP_TITLE_POPUP ) ) );
            }
            fTitlePopUp->ItemAt( 0 )->SetMarked( true );
            Window()->PostMessage( RIP_TITLE_POPUP );
            break;
        }

        case HB_MODE_ENCODING:
        {
            if( modeChanged )
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
                fSuspendButton->SetLabel( "Suspend" );
                fSuspendButton->SetEnabled( true );
                fStartButton->SetLabel( "Cancel" );
                fStartButton->SetEnabled( true );
           }

            if( !status.position )
            {
                fStatusBar->Update( - fStatusBar->CurrentValue(),
                                    "Starting..." );
            }
            else
            {
                char string[1024]; memset( string, 0, 1024 );
                sprintf( string, "Encoding: %.2f %% (%.2f fps, "
                         "%02d:%02d:%02d remaining)",
                         100 * status.position,
                         status.frameRate,
                         status.remainingTime / 3600,
                         ( status.remainingTime % 3600 ) / 60,
                         status.remainingTime % 60 );
                fStatusBar->Update( 100 * status.position -
                                    fStatusBar->CurrentValue(),
                                    string );
            }
            break;
        }

        case HB_MODE_PAUSED:
        {
            if( modeChanged )
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
                fSuspendButton->SetLabel( "Resume" );
                fSuspendButton->SetEnabled( true );
                fStartButton->SetLabel( "Cancel" );
                fStartButton->SetEnabled( true );
            }
            
            fStatusBar->Update( 100 * status.position -
                                fStatusBar->CurrentValue(), "Suspended" );
            break;
        }

        case HB_MODE_STOPPING:
        {
            if( modeChanged )
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
                fSuspendButton->SetLabel( "Suspend" );
                fSuspendButton->SetEnabled( false );
                fStartButton->SetLabel( "Cancel" );
                fStartButton->SetEnabled( false );
            }

            fStatusBar->Update( - fStatusBar->CurrentValue(),
                                "Stopping..." );
            break;
        }

        case HB_MODE_DONE:
        {
            if( modeChanged )
            {
                fTitleField->SetEnabled( true );
                fVideoCodecField->SetEnabled( true );
                fCustomBitrateRadio->SetEnabled( true );
                fCustomBitrateControl->SetEnabled(
                        fCustomBitrateRadio->Value() );
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
                
                fSuspendButton->SetLabel( "Suspend" );
                fSuspendButton->SetEnabled( false );
                fStartButton->SetLabel( "Rip !" );
                fStartButton->SetEnabled( true );
                MessageReceived( new BMessage( RIP_VIDEO_CODEC_POPUP ) );
            }

            fStatusBar->Update( 100.0 - fStatusBar->CurrentValue(),
                                "Done." );
            break;
        }

        case HB_MODE_CANCELED:
        {
            if( modeChanged )
            {
                fTitleField->SetEnabled( true );
                fVideoCodecField->SetEnabled( true );
                fCustomBitrateRadio->SetEnabled( true );
                fCustomBitrateControl->SetEnabled(
                        fCustomBitrateRadio->Value() );
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
                fSuspendButton->SetLabel( "Suspend" );
                fSuspendButton->SetEnabled( false );
                fStartButton->SetLabel( "Rip !" );
                fStartButton->SetEnabled( true );
                MessageReceived( new BMessage( RIP_VIDEO_CODEC_POPUP ) );
            }

            fStatusBar->Update( - fStatusBar->CurrentValue(),
                                "Canceled." );
            break;
        }

        case HB_MODE_ERROR:
        {
            if( modeChanged )
            {
                fTitleField->SetEnabled( true );
                fVideoCodecField->SetEnabled( true );
                fCustomBitrateRadio->SetEnabled( true );
                fCustomBitrateControl->SetEnabled(
                        fCustomBitrateRadio->Value() );
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
                fSuspendButton->SetLabel( "Suspend" );
                fSuspendButton->SetEnabled( false );
                fStartButton->SetLabel( "Rip !" );
                fStartButton->SetEnabled( true );
                MessageReceived( new BMessage( RIP_VIDEO_CODEC_POPUP ) );
            }

            fStatusBar->Update( - fStatusBar->CurrentValue(),
                                "Error." );
            break;
        }

        default:
            break;
    }
}

