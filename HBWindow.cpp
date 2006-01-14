/* $Id: HBWindow.cpp,v 1.21 2003/08/25 22:04:22 titer Exp $ */

#include "HBCommon.h"
#include "HBWindow.h"
#include "HBManager.h"
#include "HBPictureWin.h"

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <StatusBar.h>
#include <StringView.h>
#include <TextControl.h>

#define WINDOW_RECT           BRect( 100,100,500,505 )

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

/* HBWindow : the real interface */

/* Constructor */
HBWindow::HBWindow()
    : BWindow( WINDOW_RECT, "HandBrake " VERSION, B_TITLED_WINDOW,
               B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
    BRect r;
    
    /* Add a background view */
    BView * view;
    view = new BView( Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW );
    view->SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );
    AddChild( view );
    
    /* Add the settings box */
    r = BRect( 10, 10, view->Bounds().Width() - 10,
               view->Bounds().Height() - 85 );
    fBox = new HBBox( r );
    fBox->SetLabel( "Settings" );

    /* Volume */
    r = BRect( 10, 15, fBox->Bounds().Width() - 10, 35 );
    fVolumePopUp = new BPopUpMenu( "No DVD found" );
    fVolumeField = new BMenuField( r, NULL, "Volume :",
                                   fVolumePopUp, true );
    fBox->AddChild( fVolumeField );

    /* Title */
    r = BRect( 10, 45, fBox->Bounds().Width() - 10, 65 );
    fTitlePopUp = new BPopUpMenu( "No title found" );
    fTitleField = new BMenuField( r, NULL, "Title :",
                                  fTitlePopUp, true );
    fBox->AddChild( fTitleField );

    /* Audio 1 */
    r = BRect( 10, 75, fBox->Bounds().Width() - 10, 95 );
    fAudio1PopUp = new BPopUpMenu( "No audio found" );
    fAudio1Field = new BMenuField( r, NULL, "Audio 1 :",
                                     fAudio1PopUp, true );
    fBox->AddChild( fAudio1Field );
    
    /* Audio 2 */
    r = BRect( 10, 105, fBox->Bounds().Width() - 10, 125 );
    fAudio2PopUp = new BPopUpMenu( "No audio found" );
    fAudio2Field = new BMenuField( r, NULL, "Audio 2 :",
                                     fAudio2PopUp, true );
    fBox->AddChild( fAudio2Field );

    /* Video bitrate */
    r = BRect( 10, 135, fBox->Bounds().Width() - 10, 165 );
    fVideoSlider = new BSlider( r, NULL, "Video bitrate : 1024 kbps",
                                new BMessage( VIDEO_SLIDER ),
                                128, 4096, B_TRIANGLE_THUMB );
    fVideoSlider->SetValue( 1024 );
    fBox->AddChild( fVideoSlider );
    
    /* Audio bitrate */
    r = BRect( 10, 175, fBox->Bounds().Width() - 10, 205 );
    fAudioSlider = new BSlider( r, NULL, "Audio bitrate : 128 kbps",
                                new BMessage( AUDIO_SLIDER ),
                                64, 384, B_TRIANGLE_THUMB );
    fAudioSlider->SetValue( 128 );
    fBox->AddChild( fAudioSlider );
    
    /* Destination file */
    r = BRect( 10, 215, fBox->Bounds().Width() - 10, 230 );
    fFileString = new BStringView( r, NULL, "Destination file :" );
    fBox->AddChild( fFileString );
    r = BRect( 10, 235, fBox->Bounds().Width() - 90, 255 );
    fFileControl = new BTextControl( r, NULL, "", "/boot/home/Desktop/Movie.avi",
                                     new BMessage() );
    fFileControl->SetDivider( 0 );
    fBox->AddChild( fFileControl );
    r = BRect( fBox->Bounds().Width() - 80, 230,
               fBox->Bounds().Width() - 10, 255 );
    fFileButton = new BButton( r, NULL, "Browse...",
                               new BMessage( NOT_IMPLEMENTED ) );
    fBox->AddChild( fFileButton );

    view->AddChild( fBox );
    
    /* Settings buttons */
    r = BRect( fBox->Bounds().Width() - 200, 275,
               fBox->Bounds().Width() - 100, 300 );
    fPictureButton = new BButton( r, NULL, "Picture settings...",
                                  new BMessage( PICTURE_WIN ) );
    fBox->AddChild( fPictureButton );

    r = BRect( fBox->Bounds().Width() - 90, 275,
               fBox->Bounds().Width() - 10, 300 );
    fAdvancedButton = new BButton( r, NULL, "Advanced...",
                                   new BMessage( NOT_IMPLEMENTED ) );
    fBox->AddChild( fAdvancedButton );
    
    /* Status bar */
    r = BRect( 10, view->Bounds().Height() - 75,
               view->Bounds().Width() - 10, view->Bounds().Height() - 45 );
    fStatusBar = new BStatusBar( r, NULL, NULL );
    fStatusBar->SetMaxValue( 1.0 );
    view->AddChild( fStatusBar );
    
    /* Buttons */
    r = BRect( view->Bounds().Width() - 320, view->Bounds().Height() - 35,
               view->Bounds().Width() - 250, view->Bounds().Height() - 10 );
    BButton * aboutButton;
    aboutButton = new BButton( r, NULL, "About...",
                               new BMessage( B_ABOUT_REQUESTED ) );
    view->AddChild( aboutButton );

    r = BRect( view->Bounds().Width() - 240, view->Bounds().Height() - 35,
               view->Bounds().Width() - 170, view->Bounds().Height() - 10 );
    fRefreshButton = new BButton( r, NULL, "Refresh",
                                  new BMessage( REFRESH_VOLUMES ) );
    view->AddChild( fRefreshButton );

    r = BRect( view->Bounds().Width() - 160, view->Bounds().Height() - 35,
               view->Bounds().Width() - 90, view->Bounds().Height() - 10 );
    fSuspendButton = new BButton( r, NULL, "Suspend", new BMessage( SUSPEND_CONVERT ) );
    view->AddChild( fSuspendButton );
    
    r = BRect( view->Bounds().Width() - 80, view->Bounds().Height() - 35,
               view->Bounds().Width() - 10, view->Bounds().Height() - 10 );
    fStartButton = new BButton( r, NULL, "Start !", new BMessage( START_CONVERT ) );
    view->AddChild( fStartButton );
}

bool HBWindow::QuitRequested()
{
    /* Empty the PopUps - the BMenuItems do not belong to us */
    HBVolumeInfo * volumeInfo;
    while( ( volumeInfo = (HBVolumeInfo*) fVolumePopUp->ItemAt( 0 ) ) )
    {
        fVolumePopUp->RemoveItem( volumeInfo );
    }

    HBTitleInfo * titleInfo;
    while( ( titleInfo = (HBTitleInfo*) fTitlePopUp->ItemAt( 0 ) ) )
    {
        fTitlePopUp->RemoveItem( titleInfo );
    }
    
    HBAudioInfo * audioInfo;
    while( ( audioInfo = (HBAudioInfo*) fAudio1PopUp->ItemAt( 0 ) ) )
    {
        fAudio1PopUp->RemoveItem( audioInfo );
    }
    while( ( audioInfo = (HBAudioInfo*) fAudio2PopUp->ItemAt( 0 ) ) )
    {
        fAudio2PopUp->RemoveItem( audioInfo );
    }

    /* Stop the application */
    be_app->PostMessage( B_QUIT_REQUESTED );
    return true;
}

void HBWindow::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case NOT_IMPLEMENTED:
        {
            /* Warn the user with a BAlert */
            BAlert * alert;
            alert = new BAlert( "Not implemented",
                                "This feature has not yet been implemented.",
                                "Come back later !" );
            alert->Go( NULL );
            break;
        }
        
        case VOLUME_SELECTED:
        case TITLE_SELECTED:
        case LANGUAGE_SELECTED:
            SelectionChanged();
            break;
        
        case VIDEO_SLIDER:
        {
            /* Update the slider label */
            char label[128]; memset( label, 0, 128 );
            snprintf( label, 128,
                      "Video bitrate : %ld kbps",
                      fVideoSlider->Value() );
            fVideoSlider->SetLabel( label );
            break;
        }
            
        case AUDIO_SLIDER:
        {
            /* Update the slider label */
            char label[128]; memset( label, 0, 128 );
            snprintf( label, 128,
                      "Audio bitrate : %ld kbps",
                      fAudioSlider->Value() );
            fAudioSlider->SetLabel( label );
            break;
        }
        
        case PICTURE_WIN:
        {
            HBTitleInfo * titleInfo;
            titleInfo = (HBTitleInfo*) fTitlePopUp->FindMarked();
            
            if( titleInfo->fPictureWin->Lock() )
            {
                titleInfo->fPictureWin->Show();
                titleInfo->fPictureWin->Unlock();
                break;
            }
            else
            {
               Log( "Couldn't lock fPictureWin" );
            }
            
            break;
        }
            
        case SUSPEND_CONVERT:
            /* Suspend all threads */
            fManager->Suspend();
            
            /* Update the button label */
            if( Lock() )
            {
                fSuspendButton->SetLabel( "Resume" );
                fSuspendButton->SetMessage( new BMessage( RESUME_CONVERT ) );
                Unlock();
            }
            break;
            
        case RESUME_CONVERT:
            /* Resume all threads */
            fManager->Resume();
            
            /* Update the button label */
            if( Lock() )
            {
                fSuspendButton->SetLabel( "Suspend" );
                fSuspendButton->SetMessage( new BMessage( SUSPEND_CONVERT ) );
                Unlock();
            }
            break;
            
        case START_CONVERT:
        {
            /* Shouldn't happen */
            if( !fVolumePopUp->FindMarked() ||
                !fTitlePopUp->FindMarked() ||
                !fAudio1PopUp->FindMarked() ||
                !fAudio2PopUp->FindMarked() )
                break;

            /* Disable the interface */
            Status( "Starting...", 0.0, ENABLE_ENCODING );
            
            /* Start the job */
            HBVolumeInfo * volumeInfo = (HBVolumeInfo*) fVolumePopUp->FindMarked();
            HBTitleInfo  * titleInfo  = (HBTitleInfo*) fTitlePopUp->FindMarked();
            HBAudioInfo  * audio1Info = (HBAudioInfo*) fAudio1PopUp->FindMarked();
            HBAudioInfo  * audio2Info = (HBAudioInfo*) fAudio2PopUp->FindMarked();
            
            titleInfo->fBitrate     = fVideoSlider->Value();
            audio1Info->fOutBitrate = fAudioSlider->Value();
            audio2Info->fOutBitrate = fAudioSlider->Value();
            
            fManager->Start( volumeInfo, titleInfo,
                             audio1Info, audio2Info,
                             (char*) fFileControl->Text() );
            
            /* Update the button label */
            if( Lock() )
            {
                fStartButton->SetLabel( "Cancel" );
                fStartButton->SetMessage( new BMessage( STOP_CONVERT ) );
                Unlock();
            }
            break;
        }
        
        case STOP_CONVERT:
            /* Stop the job */
            fManager->Cancel();
            
            /* Update the button label */
            if( Lock() )
            {
                fStartButton->SetLabel( "Start !" );
                fStartButton->SetMessage( new BMessage( START_CONVERT ) );
                Unlock();
            }
            
            /* Enable the interface */
            Status( "Cancelled.", 0.0, ENABLE_READY );
            break;
        
        case REFRESH_VOLUMES:
            /* Disable the interface */
            Status( "Checking DVD volumes...", 0.0, ENABLE_DETECTING );

            /* Ask the manager to start the detection */
            fManager->PostMessage( DETECT_VOLUMES );
            break;
        
        case VOLUMES_DETECTED:
        {
            /* Update the popup */
            BList * volumeList;
            message->FindPointer( "list", (void**)&volumeList );
            RefreshVolumes( volumeList );
            
            /* Enable the interface */
            Status( "Ready.", 0.0, ENABLE_READY );
            break;
        }
        
        case B_ABOUT_REQUESTED:
        {
            BAlert * alert;
            alert = new BAlert( "title",
                                "HandBrake " VERSION "\n\n"
                                "by Eric Petit <titer@videolan.org>\n"
                                "Homepage : <http://beos.titer.org/handbrake/>\n\n"
                                "No, you don't want to know where this stupid app "
                                "name comes from.",
                                "Woot !" );
            alert->Go( NULL );
            break;
        }
        
        case MANAGER_CREATED:
        {
            message->FindPointer( "manager", (void**)&fManager );
            break;
        }
        
        case CHANGE_STATUS:
        {
            char * text;
            float  pos;
            int    mode;
            message->FindPointer( "text", (void**) &text );
            message->FindFloat( "pos", &pos );
            message->FindInt32( "mode", (int32*) &mode );
            
            if( !Lock() )
            {
                Log( "HBWindow::MessageReceived() : Lock() failed" );
                break;
            }
            fStatusBar->Update( pos - fStatusBar->CurrentValue(), text );
            Enable( mode );
            Unlock();
            
            free( text );
            
            break;
        }
    
        default:
        {
            BWindow::MessageReceived( message );
        }
    }
}

void HBWindow::Enable( int mode )
{
    switch( mode )
    {
        case ENABLE_DETECTING:
            fVolumeField->SetEnabled( false );
            fTitleField->SetEnabled( false );
            fAudio1Field->SetEnabled( false );
            fAudio2Field->SetEnabled( false );
            fVideoSlider->SetEnabled( true );
            fAudioSlider->SetEnabled( true );
            fFileControl->SetEnabled( true );
            fAdvancedButton->SetEnabled( true );
            fFileString->SetHighColor( 0, 0, 0 );
            fFileString->Invalidate();
            fPictureButton->SetEnabled( false );
            fRefreshButton->SetEnabled( false );
            fSuspendButton->SetEnabled( false );
            fStartButton->SetEnabled( false );
            break;
        
        case ENABLE_READY:
            fVolumeField->SetEnabled( true );
            fTitleField->SetEnabled( true );
            fAudio1Field->SetEnabled( true );
            fAudio2Field->SetEnabled( true );
            fVideoSlider->SetEnabled( true );
            fAudioSlider->SetEnabled( true );
            fFileControl->SetEnabled( true );
            fAdvancedButton->SetEnabled( true );
            fFileString->SetHighColor( 0, 0, 0 );
            fFileString->Invalidate();
            fPictureButton->SetEnabled( true );
            fRefreshButton->SetEnabled( true );
            fSuspendButton->SetEnabled( false );
            fStartButton->SetEnabled( true );
            break;
        
        case ENABLE_ENCODING:
            fVolumeField->SetEnabled( false );
            fTitleField->SetEnabled( false );
            fAudio1Field->SetEnabled( false );
            fAudio2Field->SetEnabled( false );
            fVideoSlider->SetEnabled( false );
            fAudioSlider->SetEnabled( false );
            fFileControl->SetEnabled( false );
            fAdvancedButton->SetEnabled( false );
            fFileString->SetHighColor( 156, 156, 156 );
            fFileString->Invalidate();
            fPictureButton->SetEnabled( false );
            fRefreshButton->SetEnabled( false );
            fSuspendButton->SetEnabled( true );
            fStartButton->SetEnabled( true );
            break;
    }
}

void HBWindow::RefreshVolumes( BList * volumeList )
{
    if( !( Lock() ) )
    {
        Log( "HBWindow::RefreshVolumes : Lock() failed" );
        return;
    }

    /* Empty the PopUps */
    HBVolumeInfo * volumeInfo;
    while( ( volumeInfo = (HBVolumeInfo*) fVolumePopUp->ItemAt( 0 ) ) )
    {
        fVolumePopUp->RemoveItem( volumeInfo );
    }

    HBTitleInfo * titleInfo;
    while( ( titleInfo = (HBTitleInfo*) fTitlePopUp->ItemAt( 0 ) ) )
    {
        fTitlePopUp->RemoveItem( titleInfo );
    }
    
    HBAudioInfo * audioInfo;
    while( ( audioInfo = (HBAudioInfo*) fAudio1PopUp->ItemAt( 0 ) ) )
    {
        fAudio1PopUp->RemoveItem( audioInfo );
    }
    while( ( audioInfo = (HBAudioInfo*) fAudio2PopUp->ItemAt( 0 ) ) )
    {
        fAudio2PopUp->RemoveItem( audioInfo );
    }
    
    /* Fill the Volumes PopUp */
    for( int i = 0; i < volumeList->CountItems(); i++ )
    {
        fVolumePopUp->AddItem( (HBVolumeInfo*) volumeList->ItemAt( i ) );
    }

    /* Select the first volume */
    if( !( volumeInfo = (HBVolumeInfo*) volumeList->ItemAt( 0 ) ) )
    {
        Log( "HBWindow::RefreshVolumes : no volume found" );
        Unlock();
        return;
    }
    volumeInfo->SetMarked( true );
    
    /* Fill the Titles PopUp */
    BList * titleList = volumeInfo->fTitleList;
    for( int i = 0; i < titleList->CountItems(); i++ )
    {
        fTitlePopUp->AddItem( (HBTitleInfo*) titleList->ItemAt( i ) );
    }

    /* Select the first title */
    if( !( titleInfo = (HBTitleInfo*) titleList->ItemAt( 0 ) ) )
    {
        Log( "HBWindow::RefreshVolumes : no title found" );
        Unlock();
        return;
    }
    titleInfo->SetMarked( true );
    
    /* Fill the Audios PopUp */
    BList * audioList1 = titleInfo->fAudioInfoList1;
    BList * audioList2 = titleInfo->fAudioInfoList2;
    for( int i = 0; i < audioList1->CountItems(); i++ )
    {
        fAudio1PopUp->AddItem( (HBAudioInfo*) audioList1->ItemAt( i ) );
        fAudio2PopUp->AddItem( (HBAudioInfo*) audioList2->ItemAt( i ) );
    }
        
    audioInfo = (HBAudioInfo*) fAudio1PopUp->ItemAt( 0 );
    audioInfo->SetMarked( true );
    audioInfo = (HBAudioInfo*) fAudio2PopUp->ItemAt( fAudio2PopUp->CountItems() - 1 );
    audioInfo->SetMarked( true );

    Unlock();
}

void HBWindow::SelectionChanged()
{
    HBVolumeInfo * volumeInfo;
    HBTitleInfo  * titleInfo;
    HBAudioInfo  * audioInfo;

    /* Update the Title popup if needed */
    bool updateTitlePopUp = true;
    volumeInfo            = (HBVolumeInfo*) fVolumePopUp->FindMarked();
    titleInfo             = (HBTitleInfo*) fTitlePopUp->FindMarked();
    
    for( int i = 0; i < volumeInfo->fTitleList->CountItems(); i++ )
    {
        if( titleInfo == volumeInfo->fTitleList->ItemAt( i ) )
        {
            /* No need to update titles, we already are on the right
               volume */
            updateTitlePopUp = false;
            break;
        }
    }
    
    if( updateTitlePopUp )
    {
        /* Empty the popup */
        while( ( titleInfo = (HBTitleInfo*) fTitlePopUp->ItemAt( 0 ) ) )
        {
            fTitlePopUp->RemoveItem( titleInfo );
        }
        
        /* Fill it */
        for( int i = 0; i < volumeInfo->fTitleList->CountItems(); i++ )
        {
            fTitlePopUp->AddItem( (HBTitleInfo*) volumeInfo->fTitleList->ItemAt( i ) );
        }
        
        /* Select the first title */
        ((HBTitleInfo*) fTitlePopUp->ItemAt( 0 ))->SetMarked( true );
    }
    
    /* Update the Audio popups if needed */
    bool updateAudioPopUp = true;
    titleInfo             = (HBTitleInfo*) fTitlePopUp->FindMarked();
    audioInfo             = (HBAudioInfo*) fAudio1PopUp->FindMarked();
    
    for( int i = 0; i < titleInfo->fAudioInfoList1->CountItems(); i++ )
    {
        if( audioInfo == titleInfo->fAudioInfoList1->ItemAt( i ) )
        {
            /* No need to update audio, we already are on the right
               title */
            updateAudioPopUp = false;
            break;
        }
    }
    
    if( updateAudioPopUp )
    {
        /* Empty the popups */
        while( ( audioInfo = (HBAudioInfo*) fAudio1PopUp->ItemAt( 0 ) ) )
        {
            fAudio1PopUp->RemoveItem( audioInfo );
        }
        while( ( audioInfo = (HBAudioInfo*) fAudio2PopUp->ItemAt( 0 ) ) )
        {
            fAudio2PopUp->RemoveItem( audioInfo );
        }
        
        /* Fill it */
        for( int i = 0; i < titleInfo->fAudioInfoList1->CountItems(); i++ )
        {
            fAudio1PopUp->AddItem( (HBAudioInfo*) titleInfo->fAudioInfoList1->ItemAt( i ) );
            fAudio2PopUp->AddItem( (HBAudioInfo*) titleInfo->fAudioInfoList2->ItemAt( i ) );
        }
        
        /* Select the first track */
        ((HBAudioInfo*) fAudio1PopUp->ItemAt( 0 ))->SetMarked( true );
        
        /* Select "None" */
        ((HBAudioInfo*) fAudio2PopUp->ItemAt( fAudio2PopUp->CountItems() - 1 ))->SetMarked( true );
    }
   
}
