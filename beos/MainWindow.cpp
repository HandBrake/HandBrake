/* $Id: MainWindow.cpp,v 1.14 2003/10/05 14:56:38 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include <fs_info.h>
#include <sys/ioctl.h>
#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Directory.h>
#include <Drivers.h>
#include <MenuField.h>
#include <Path.h>
#include <Query.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <Slider.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TextControl.h>
#include <VolumeRoster.h>

#include "MainWindow.h"
#include "Manager.h"
#include "PictureWin.h"

#define DEFAULT_FILE          "/boot/home/Desktop/Movie.avi"
#define WINDOW_RECT           BRect( 0,0,400,405 )

#define BUTTON_ADVANCED 'badv'
#define BUTTON_FILE     'bfil'
#define BUTTON_PICTURE  'bpic'
#define BUTTON_START    'bsta'
#define BUTTON_CANCEL   'bcan'
#define BUTTON_SUSPEND  'bsus'
#define BUTTON_RESUME   'bres'
#define POPUP_AUDIO     'paud'
#define POPUP_TITLE     'ptit'
#define POPUP_VOLUME    'pvol'
#define SLIDER_AUDIO    'saud'
#define SLIDER_VIDEO    'svid'

/* HBBox : almost a simple BBox, unless we draw a horizontal line
   before the "Picture" and "Advanced" buttons. There must be a
   cleaner way to do this, but I'm not a expert GUI programmer. */

/* Constructor */
HBBox::HBBox( BRect rect )
    : BBox( rect, NULL )
{
}

/* Draw */
void HBBox::Draw( BRect rect )
{
    /* Inherited method */
	BBox::Draw( rect );

    /* Draw the line */
    SetHighColor( 120, 120, 120 );
	SetLowColor( 255, 255, 255 );
	StrokeLine( BPoint( 10, 265 ),
                BPoint( Bounds().Width() - 10, 265 ),
                B_SOLID_HIGH );
	StrokeLine( BPoint( 11, 266 ),
	            BPoint( Bounds().Width() - 10, 266 ),
	            B_SOLID_LOW );
}

HBVolumeItem::HBVolumeItem( HBVolume * volume )
    : BMenuItem( "", new BMessage( POPUP_VOLUME ) )
{
    fVolume = volume;

    SetLabel( fVolume->fName );
}

HBTitleItem::HBTitleItem( HBTitle * title )
    : BMenuItem( "", new BMessage( POPUP_TITLE) )
{
    fTitle = title;

    char label[1024]; memset( label, 0, 1024 );
    sprintf( label, "%d (%02lld:%02lld:%02lld)", fTitle->fIndex,
             fTitle->fLength / 3600, ( fTitle->fLength % 3600 ) / 60,
             fTitle->fLength % 60 );
    SetLabel( label );
}

HBAudioItem::HBAudioItem( HBAudio * audio )
    : BMenuItem( "", new BMessage( POPUP_AUDIO ) )
{
    fAudio = audio;

    SetLabel( fAudio ? fAudio->fDescription : "None" );
}

HBWindow::HBWindow( bool debug )
    : BWindow( WINDOW_RECT, "HandBrake " VERSION, B_TITLED_WINDOW,
               B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
    /* Center the window */
    BScreen screen;
    MoveTo( ( screen.Frame().Width() - Frame().Width() ) / 2,
            ( screen.Frame().Height() - Frame().Height() ) / 2 );

    /* -- GUI starts here -- */

    BRect r;

    /* Add a background view */
    BView * view;
    view = new BView( Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW );
    view->SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );

    /* Add the settings box */
    r = BRect( 10, 10, view->Bounds().Width() - 10,
               view->Bounds().Height() - 85 );
    fBox = new HBBox( r );
    fBox->SetLabel( "Settings" );

    /* Volume */
    r = BRect( 10, 15, fBox->Bounds().Width() - 10, 35 );
    fVolumePopUp = new BPopUpMenu( "" );
    fVolumeField = new BMenuField( r, NULL, "Volume :",
                                   fVolumePopUp, true );
    fBox->AddChild( fVolumeField );

    /* Title */
    r = BRect( 10, 45, fBox->Bounds().Width() - 10, 65 );
    fTitlePopUp = new BPopUpMenu( "" );
    fTitleField = new BMenuField( r, NULL, "Title :",
                                  fTitlePopUp, true );
    fBox->AddChild( fTitleField );

    /* Audio 1 */
    r = BRect( 10, 75, fBox->Bounds().Width() - 10, 95 );
    fAudio1PopUp = new BPopUpMenu( "" );
    fAudio1Field = new BMenuField( r, NULL, "Audio 1 :",
                                     fAudio1PopUp, true );
    fBox->AddChild( fAudio1Field );

    /* Audio 2 */
    r = BRect( 10, 105, fBox->Bounds().Width() - 10, 125 );
    fAudio2PopUp = new BPopUpMenu( "" );
    fAudio2Field = new BMenuField( r, NULL, "Audio 2 :",
                                     fAudio2PopUp, true );
    fBox->AddChild( fAudio2Field );

    /* Video bitrate */
    r = BRect( 10, 135, fBox->Bounds().Width() - 10, 165 );
    fVideoSlider = new BSlider( r, NULL, "Video bitrate : 1024 kbps",
                                new BMessage( SLIDER_VIDEO ),
                                128, 4096, B_TRIANGLE_THUMB );
    fVideoSlider->SetValue( 1024 );
    fBox->AddChild( fVideoSlider );

    /* Audio bitrate */
    r = BRect( 10, 175, fBox->Bounds().Width() - 10, 205 );
    fAudioSlider = new BSlider( r, NULL, "Audio bitrate : 128 kbps",
                                new BMessage( SLIDER_AUDIO ),
                                64, 384, B_TRIANGLE_THUMB );
    fAudioSlider->SetValue( 128 );
    fBox->AddChild( fAudioSlider );

    /* Destination file */
    r = BRect( 10, 215, fBox->Bounds().Width() - 10, 230 );
    fFileString = new BStringView( r, NULL, "Destination file :" );
    fBox->AddChild( fFileString );
    r = BRect( 10, 235, fBox->Bounds().Width() - 90, 255 );
    fFileControl = new BTextControl( r, NULL, "", DEFAULT_FILE,
                                     new BMessage() );
    fFileControl->SetDivider( 0 );
    fBox->AddChild( fFileControl );
    r = BRect( fBox->Bounds().Width() - 80, 230,
               fBox->Bounds().Width() - 10, 255 );
    fFileButton = new BButton( r, NULL, "Browse...",
                               new BMessage( BUTTON_FILE ) );
    fBox->AddChild( fFileButton );

    /* Settings buttons */
    r = BRect( fBox->Bounds().Width() - 200, 275,
               fBox->Bounds().Width() - 100, 300 );
    fPictureButton = new BButton( r, NULL, "Picture settings...",
                                  new BMessage( BUTTON_PICTURE ) );
    fBox->AddChild( fPictureButton );

    r = BRect( fBox->Bounds().Width() - 90, 275,
               fBox->Bounds().Width() - 10, 300 );
    fAdvancedButton = new BButton( r, NULL, "Advanced...",
                                   new BMessage( BUTTON_ADVANCED ) );
    fBox->AddChild( fAdvancedButton );

    view->AddChild( fBox );

    /* Status bar */
    r = BRect( 10, view->Bounds().Height() - 75,
               view->Bounds().Width() - 10, view->Bounds().Height() - 45 );
    fStatusBar = new BStatusBar( r, NULL, NULL );
    fStatusBar->SetMaxValue( 1.0 );
    view->AddChild( fStatusBar );

    /* Buttons */
    r = BRect( view->Bounds().Width() - 240, view->Bounds().Height() - 35,
               view->Bounds().Width() - 170, view->Bounds().Height() - 10 );
    BButton * aboutButton;
    aboutButton = new BButton( r, NULL, "About...",
                               new BMessage( B_ABOUT_REQUESTED ) );
    view->AddChild( aboutButton );

    r = BRect( view->Bounds().Width() - 160, view->Bounds().Height() - 35,
               view->Bounds().Width() - 90, view->Bounds().Height() - 10 );
    fSuspendButton = new BButton( r, NULL, "Suspend",
                                  new BMessage( BUTTON_SUSPEND ) );
    view->AddChild( fSuspendButton );

    r = BRect( view->Bounds().Width() - 80, view->Bounds().Height() - 35,
               view->Bounds().Width() - 10, view->Bounds().Height() - 10 );
    fStartButton = new BButton( r, NULL, "Start !",
                                new BMessage( BUTTON_START ) );
    view->AddChild( fStartButton );

    AddChild( view );

    /* -- GUI ends here -- */

    /* Init libhb & launch the manager thread */
    fManager = new HBManager( debug );

    /* Detects DVD drives & VOB folders, then tell libhb to scan it */
    ScanVolumes();

    /* Update the interface */
    fUpdateThread = spawn_thread( (int32 (*)(void *)) UpdateInterface,
                                  "interface", B_DISPLAY_PRIORITY, this );
    resume_thread( fUpdateThread );
}

bool HBWindow::QuitRequested()
{
    /* Clean up */
    kill_thread( fUpdateThread );
    delete fManager;

    /* Stop the application */
    be_app->PostMessage( B_QUIT_REQUESTED );
    return true;
}

void HBWindow::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case B_ABOUT_REQUESTED:
        {
            BAlert * alert;
            alert = new BAlert( "About HandBrake",
                "HandBrake " VERSION "\n\n"
                "by Eric Petit <titer@videolan.org>\n"
                "Homepage : <http://beos.titer.org/handbrake/>\n\n"
                "No, you don't want to know where this stupid app "
                "name comes from.\n\n"
                "Thanks to BGA for pointing out very cool bugs ;)",
                "Woot !" );
            alert->Go( NULL );
            break;
        }

        case BUTTON_ADVANCED:
            break;

        case BUTTON_FILE:
            break;

        case BUTTON_PICTURE:
        {
            HBTitle * title =
                ((HBTitleItem*) fTitlePopUp->FindMarked())->fTitle;
            HBPictureWin * pictureWin = new HBPictureWin( fManager, title );
            pictureWin->Show();
            break;
        }

        case BUTTON_START:
        {
            HBTitle * title =
                ((HBTitleItem*) fTitlePopUp->FindMarked())->fTitle;
            HBAudio * audio1 =
                ((HBAudioItem*) fAudio1PopUp->FindMarked())->fAudio;
            HBAudio * audio2 =
                ((HBAudioItem*) fAudio2PopUp->FindMarked())->fAudio;

            title->fBitrate = fVideoSlider->Value();
            if( audio1 )
            {
                audio1->fOutBitrate = fAudioSlider->Value();
            }
            if( audio2 )
            {
                audio2->fOutBitrate = fAudioSlider->Value();
            }

            fManager->StartRip( title, audio1, audio2,
                                (char*) fFileControl->Text() );
            break;
        }

        case BUTTON_CANCEL:
            fManager->StopRip();
            break;

        case BUTTON_SUSPEND:
            fManager->SuspendRip();
            break;

        case BUTTON_RESUME:
            fManager->ResumeRip();
            break;

        case POPUP_AUDIO:
            break;

        case POPUP_TITLE:
        {
            HBTitle * title =
                ((HBTitleItem*) fTitlePopUp->FindMarked())->fTitle;

            /* Empty audio popups */
            HBAudioItem * audioItem;
            while( ( audioItem = (HBAudioItem*) fAudio1PopUp->ItemAt( 0 ) ) )
            {
                fAudio1PopUp->RemoveItem( audioItem );
                delete audioItem;
            }
            while( ( audioItem = (HBAudioItem*) fAudio2PopUp->ItemAt( 0 ) ) )
            {
                fAudio2PopUp->RemoveItem( audioItem );
                delete audioItem;
            }

            HBAudio * audio;
            for( uint32_t i = 0;
                 i < title->fAudioList->CountItems();
                 i++ )
            {
                audio = (HBAudio*) title->fAudioList->ItemAt( i );
                fAudio1PopUp->AddItem( new HBAudioItem( audio ) );
                fAudio2PopUp->AddItem( new HBAudioItem( audio ) );
            }
            fAudio1PopUp->AddItem( new HBAudioItem( NULL ) );
            fAudio2PopUp->AddItem( new HBAudioItem( NULL ) );
            ((HBAudioItem*) fAudio1PopUp->ItemAt( 0 ))->SetMarked( true );
            ((HBAudioItem*) fAudio2PopUp->ItemAt(
                fAudio2PopUp->CountItems() - 1 ))->SetMarked( true );

            break;
        }

        case POPUP_VOLUME:
            break;

        case SLIDER_AUDIO:
        {
            char label[64]; memset( label, 0, 64 );
            snprintf( label, 128, "Audio bitrate : %ld kbps",
                      fAudioSlider->Value() );
            fAudioSlider->SetLabel( label );
            break;
        }

        case SLIDER_VIDEO:
        {
            char label[64]; memset( label, 0, 64 );
            snprintf( label, 128, "Video bitrate : %ld kbps",
                      fVideoSlider->Value() );
            fVideoSlider->SetLabel( label );
            break;
        }

        default:
            BWindow::MessageReceived( message );
            break;
    }
}


void HBWindow::ScanVolumes()
{
    BVolumeRoster   * roster  = new BVolumeRoster();
    BVolume         * bVolume = new BVolume();
    fs_info           info;
    int               device;
    device_geometry   geometry;

    HBVolume * volume;
    HBList   * volumeList = new HBList();

    /* Parse mounted volumes */
    while( roster->GetNextVolume( bVolume ) == B_NO_ERROR )
    {
        /* open() and ioctl() for more informations */
        fs_stat_dev( bVolume->Device(), &info );
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
        bVolume->GetName( volumeName );

        if( bVolume->IsReadOnly() && geometry.device_type == B_CD )
        {
            /* May be a DVD */
            volume = new HBVolume( info.device_name, volumeName );
            volumeList->AddItem( volume );
        }
        else if( geometry.device_type == B_DISK )
        {
            /* May be a hard drive. Look for VIDEO_TS folders on it */
            BQuery * query = new BQuery();

            if( query->SetVolume( bVolume ) != B_OK )
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

                volume = new HBVolume( (char*) path.Path() );
                volumeList->AddItem( volume );
            }

            delete query;
        }
    }

    fManager->ScanVolumes( volumeList );
}

void HBWindow::UpdateInterface( HBWindow * _this )
{
    for( ;; )
    {
        _this->_UpdateInterface();
        snooze( 10000 );
    }
}

void HBWindow::_UpdateInterface()
{
    if( !fManager->NeedUpdate() )
    {
        return;
    }

    HBStatus status = fManager->GetStatus();

    if( !Lock() )
    {
        fprintf( stderr, "Lock() failed\n" );
        return;
    }

    EnableInterface( status.fMode );

    switch( status.fMode )
    {
        case HB_MODE_UNDEF:
            break;

        case HB_MODE_SCANNING:
        {
            char text[1024]; memset( text, 0, 1024 );
            sprintf( text, "Scanning %s, title %d...",
                     status.fScannedVolume, status.fScannedTitle );
            fStatusBar->Update( - fStatusBar->CurrentValue(), text );
            break;
        }

        case HB_MODE_SCANDONE:
        {
            HBVolume * volume;
            for( uint32_t i = 0;
                 i < status.fVolumeList->CountItems();
                 i++ )
            {
                volume = (HBVolume*) status.fVolumeList->ItemAt( i );
                fVolumePopUp->AddItem( new HBVolumeItem( volume ) );
            }
            ((HBVolumeItem*) fVolumePopUp->ItemAt( 0 ))->SetMarked( true );

            HBTitle * title;
            volume = (HBVolume*) status.fVolumeList->ItemAt( 0 );
            for( uint32_t i = 0;
                 i < volume->fTitleList->CountItems();
                 i++ )
            {
                title = (HBTitle*) volume->fTitleList->ItemAt( i );
                fTitlePopUp->AddItem( new HBTitleItem( title ) );
            }
            ((HBTitleItem*) fTitlePopUp->ItemAt( 0 ))->SetMarked( true );

            HBAudio * audio;
            title = (HBTitle*) volume->fTitleList->ItemAt( 0 );
            for( uint32_t i = 0;
                 i < title->fAudioList->CountItems();
                 i++ )
            {
                audio = (HBAudio*) title->fAudioList->ItemAt( i );
                fAudio1PopUp->AddItem( new HBAudioItem( audio ) );
                fAudio2PopUp->AddItem( new HBAudioItem( audio ) );
            }
            fAudio1PopUp->AddItem( new HBAudioItem( NULL ) );
            fAudio2PopUp->AddItem( new HBAudioItem( NULL ) );
            ((HBAudioItem*) fAudio1PopUp->ItemAt( 0 ))->SetMarked( true );
            ((HBAudioItem*) fAudio2PopUp->ItemAt(
                fAudio2PopUp->CountItems() - 1 ))->SetMarked( true );

            fStatusBar->Update( - fStatusBar->CurrentValue(),
                                "Ready. Press 'Start' to rip." );
            break;
        }

        case HB_MODE_ENCODING:
        {
            char text[1024]; memset( text, 0, 1024 );
            sprintf( text,
                     "Encoding : %.2f %%, %.2f fps (%02d:%02d:%02d remaining)",
                     100 * status.fPosition, status.fFrameRate,
                     status.fRemainingTime / 3600,
                     ( status.fRemainingTime % 3600 ) / 60,
                     status.fRemainingTime % 60 );
            fStatusBar->Update( status.fPosition -
                                fStatusBar->CurrentValue(), text );
            break;
        }

        case HB_MODE_SUSPENDED:
        {
            char text[1024]; memset( text, 0, 1024 );
            sprintf( text, "Encoding : %.2f %%, %.2f fps (Paused)",
                     100 * status.fPosition, status.fFrameRate );
            fStatusBar->Update( status.fPosition -
                                fStatusBar->CurrentValue(), text );
            break;
        }

        case HB_MODE_DONE:
            break;

        case HB_MODE_CANCELED:
            break;

        case HB_MODE_ERROR:
            break;

        default:
            break;
    }

    Unlock();
}

void HBWindow::EnableInterface( HBMode mode )
{
    if( mode == fOldMode && mode != HB_MODE_UNDEF )
    {
        return;
    }

    switch( mode )
    {
        case HB_MODE_UNDEF:
        {
            fAdvancedButton->SetEnabled( false );
            fFileButton->SetEnabled( false );
            fPictureButton->SetEnabled( false );
            fStartButton->SetEnabled( false );
            fSuspendButton->SetEnabled( false );
            fAudio1Field->SetEnabled( false );
            fAudio2Field->SetEnabled( false );
            fTitleField->SetEnabled( false );
            fVolumeField->SetEnabled( false );
            fAudioSlider->SetEnabled( false );
            fVideoSlider->SetEnabled( false );
            fFileString->SetHighColor( 156, 156, 156 );
            fFileString->Invalidate();
            fFileControl->SetEnabled( false );
            break;
        }

        case HB_MODE_SCANNING:
        {
            fAdvancedButton->SetEnabled( true );
            fFileButton->SetEnabled( true );
            fPictureButton->SetEnabled( false );
            fStartButton->SetEnabled( false );
            fSuspendButton->SetEnabled( false );
            fAudio1Field->SetEnabled( false );
            fAudio2Field->SetEnabled( false );
            fTitleField->SetEnabled( false );
            fVolumeField->SetEnabled( false );
            fAudioSlider->SetEnabled( true );
            fVideoSlider->SetEnabled( true );
            fFileString->SetHighColor( 0, 0, 0 );
            fFileString->Invalidate();
            fFileControl->SetEnabled( true );
            break;
        }

        case HB_MODE_SCANDONE:
        {
            fAdvancedButton->SetEnabled( true );
            fFileButton->SetEnabled( true );
            fPictureButton->SetEnabled( true );
            fStartButton->SetLabel( "Start" );
            fStartButton->SetMessage( new BMessage( BUTTON_START ) );
            fStartButton->SetEnabled( true );
            fSuspendButton->SetEnabled( false );
            fAudio1Field->SetEnabled( true );
            fAudio2Field->SetEnabled( true );
            fTitleField->SetEnabled( true );
            fVolumeField->SetEnabled( true );
            fAudioSlider->SetEnabled( true );
            fVideoSlider->SetEnabled( true );
            fFileString->SetHighColor( 0, 0, 0 );
            fFileString->Invalidate();
            fFileControl->SetEnabled( true );
            break;
        }

        case HB_MODE_ENCODING:
        {
            fFileButton->SetEnabled( false );
            fPictureButton->SetEnabled( false );
            fStartButton->SetLabel( "Cancel" );
            fStartButton->SetMessage( new BMessage( BUTTON_CANCEL ) );
            fStartButton->SetEnabled( true );
            fSuspendButton->SetLabel( "Suspend" );
            fSuspendButton->SetMessage( new BMessage( BUTTON_SUSPEND ) );
            fSuspendButton->SetEnabled( true );
            fAudio1Field->SetEnabled( false );
            fAudio2Field->SetEnabled( false );
            fTitleField->SetEnabled( false );
            fVolumeField->SetEnabled( false );
            fAudioSlider->SetEnabled( false );
            fVideoSlider->SetEnabled( false );
            fFileString->SetHighColor( 156, 156, 156 );
            fFileString->Invalidate();
            fFileControl->SetEnabled( false );
            break;
        }

        case HB_MODE_SUSPENDED:
        {
            fFileButton->SetEnabled( false );
            fPictureButton->SetEnabled( false );
            fStartButton->SetLabel( "Cancel" );
            fStartButton->SetMessage( new BMessage( BUTTON_CANCEL ) );
            fStartButton->SetEnabled( true );
            fSuspendButton->SetLabel( "Resume" );
            fSuspendButton->SetMessage( new BMessage( BUTTON_RESUME ) );
            fSuspendButton->SetEnabled( true );
            fAudio1Field->SetEnabled( false );
            fAudio2Field->SetEnabled( false );
            fTitleField->SetEnabled( false );
            fVolumeField->SetEnabled( false );
            fAudioSlider->SetEnabled( false );
            fVideoSlider->SetEnabled( false );
            fFileString->SetHighColor( 156, 156, 156 );
            fFileString->Invalidate();
            fFileControl->SetEnabled( false );
            break;
        }

        case HB_MODE_DONE:
        {
            fFileButton->SetEnabled( false );
            fPictureButton->SetEnabled( false );
            fStartButton->SetEnabled( true );
            fSuspendButton->SetEnabled( true );
            fAudio1Field->SetEnabled( false );
            fAudio2Field->SetEnabled( false );
            fTitleField->SetEnabled( false );
            fVolumeField->SetEnabled( false );
            fAudioSlider->SetEnabled( false );
            fVideoSlider->SetEnabled( false );
            fFileString->SetHighColor( 156, 156, 156 );
            fFileString->Invalidate();
            fFileControl->SetEnabled( false );
            break;
        }

        case HB_MODE_CANCELED:
        {
            fFileButton->SetEnabled( false );
            fPictureButton->SetEnabled( false );
            fStartButton->SetEnabled( true );
            fSuspendButton->SetEnabled( true );
            fAudio1Field->SetEnabled( false );
            fAudio2Field->SetEnabled( false );
            fTitleField->SetEnabled( false );
            fVolumeField->SetEnabled( false );
            fAudioSlider->SetEnabled( false );
            fVideoSlider->SetEnabled( false );
            fFileString->SetHighColor( 156, 156, 156 );
            fFileString->Invalidate();
            fFileControl->SetEnabled( false );
            break;
        }

        case HB_MODE_ERROR:
        {
            fFileButton->SetEnabled( false );
            fPictureButton->SetEnabled( false );
            fStartButton->SetEnabled( true );
            fSuspendButton->SetEnabled( true );
            fAudio1Field->SetEnabled( false );
            fAudio2Field->SetEnabled( false );
            fTitleField->SetEnabled( false );
            fVolumeField->SetEnabled( false );
            fAudioSlider->SetEnabled( false );
            fVideoSlider->SetEnabled( false );
            fFileString->SetHighColor( 156, 156, 156 );
            fFileString->Invalidate();
            fFileControl->SetEnabled( false );
            break;
        }

        default:
            break;
    }
}

