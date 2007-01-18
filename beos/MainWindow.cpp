#include <app/Application.h>
#include <interface/Box.h>
#include <interface/Button.h>
#include <interface/CheckBox.h>
#include <interface/MenuField.h>
#include <interface/MenuItem.h>
#include <interface/PopUpMenu.h>
#include <interface/RadioButton.h>
#include <interface/Screen.h>
#include <interface/Slider.h>
#include <interface/StatusBar.h>
#include <interface/StringView.h>
#include <interface/TextControl.h>
#include <storage/FilePanel.h>
#include <storage/Path.h>
#include <support/String.h>

#include "MainWindow.h"
#include "PicWindow.h"
#include "QueueWindow.h"

#define MSG_TITLEPOPUP   'titl'
#define MSG_CHAPTERPOPUP 'chap'
#define MSG_FORMATPOPUP  'form'
#define MSG_CODECSPOPUP  'code'
#define MSG_BROWSE       'brow'
#define MSG_QUALITYRADIO 'radi'
#define MSG_SLIDER       'slid'
#define MSG_PICSETTINGS  'pise'
#define MSG_QUEUE_ENABLE 'quen'
#define MSG_QUEUE_ADD    'quad'
#define MSG_QUEUE_SHOW   'qush'
#define MSG_PAUSE        'paus'
#define MSG_START        'star'

static int FormatSettings[3][4] =
  { { HB_MUX_MP4 | HB_VCODEC_FFMPEG | HB_ACODEC_FAAC,
      HB_MUX_MP4 | HB_VCODEC_X264   | HB_ACODEC_FAAC,
      0,
      0 },
    { HB_MUX_AVI | HB_VCODEC_FFMPEG | HB_ACODEC_LAME,
      HB_MUX_AVI | HB_VCODEC_FFMPEG | HB_ACODEC_AC3,
      HB_MUX_AVI | HB_VCODEC_X264   | HB_ACODEC_LAME,
      HB_MUX_AVI | HB_VCODEC_X264   | HB_ACODEC_AC3 },
    { HB_MUX_OGM | HB_VCODEC_FFMPEG | HB_ACODEC_VORBIS,
      HB_MUX_OGM | HB_VCODEC_FFMPEG | HB_ACODEC_LAME,
      0,
      0 } };

MainView::MainView( hb_handle_t * handle )
    : BView( BRect( 0,0,700,475 ), NULL, B_FOLLOW_NONE, B_WILL_DRAW )
{
    fHandle = handle;

    BRect r, b;
    BBox * box;
    
    SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );

    /* Source box */
    b = Bounds();
    r = BRect( 10,10,b.right/2-5,135 );
    box = new BBox( r );
    box->SetLabel( "Source" );
    AddChild( box );

    b = box->Bounds();

    r = BRect( 10,15,b.right/4,35 );
    fSrcDVD1String = new BStringView( r, NULL, "DVD:" );
    box->AddChild( fSrcDVD1String );

    r = BRect( b.right/4+1,15,b.right-10,35 );
    fSrcDVD2String = new BStringView( r, NULL, "" );
    fSrcDVD2String->SetAlignment( B_ALIGN_RIGHT );
    box->AddChild( fSrcDVD2String );

    r = BRect( 10,40,b.right-10,60 );
    fSrcTitlePopUp = new BPopUpMenu( "" );
    fSrcTitleMenu  = new BMenuField( r, NULL, "Title:", fSrcTitlePopUp,
                                     true );
    fSrcTitleMenu->SetDivider( b.right-130 );
    box->AddChild( fSrcTitleMenu );

    r = BRect( 10,65,b.right-120,85 );
    fSrcChapString = new BStringView( r, NULL, "Chapters:" );
    box->AddChild( fSrcChapString );
    
    r = BRect( b.right-119,65,b.right-80,85 );
    fSrcChapStartPopUp = new BPopUpMenu( "" );
    fSrcChapStartMenu  = new BMenuField( r, NULL, "",
                                         fSrcChapStartPopUp, true );
    fSrcChapStartMenu->SetDivider( 0.0 );
    box->AddChild( fSrcChapStartMenu );

    r = BRect( b.right-79,65,b.right-50,85 );
    fSrcChapToString = new BStringView( r, NULL, "to" );
    fSrcChapToString->SetAlignment( B_ALIGN_CENTER );
    box->AddChild( fSrcChapToString );
    
    r = BRect( b.right-49,65,b.right-10,85 );
    fSrcChapEndPopUp = new BPopUpMenu( "" );
    fSrcChapEndMenu  = new BMenuField( r, NULL, "", fSrcChapEndPopUp,
                                       true );
    fSrcChapEndMenu->SetDivider( 0.0 );
    box->AddChild( fSrcChapEndMenu );

    r = BRect( 10,90,b.right/2,110 );
    fSrcDur1String = new BStringView( r, NULL, "Duration:" );
    box->AddChild( fSrcDur1String );

    r = BRect( b.right/2+1,90,b.right-10,110 );
    fSrcDur2String = new BStringView( r, NULL, "00:00:00" );
    fSrcDur2String->SetAlignment( B_ALIGN_RIGHT );
    box->AddChild( fSrcDur2String );

    /* Destination box */
    b = Bounds();
    r = BRect( b.right/2+5,10,b.right-10,135 );
    box = new BBox( r );
    box->SetLabel( "Destination" );
    AddChild( box );

    b = box->Bounds();

    r = BRect( 10,15,b.right-10,35 );
    fDstFormatPopUp = new BPopUpMenu( "" );
#define ADDITEM(a) fDstFormatPopUp->AddItem( new BMenuItem( a, \
    new BMessage( MSG_FORMATPOPUP ) ) )
    ADDITEM( "MP4 file" );
    ADDITEM( "AVI file" );
    ADDITEM( "OGM file" );
#undef ADDITEM
    fDstFormatPopUp->ItemAt( 0 )->SetMarked( true );
    fDstFormat = -1;
    fDstFormatMenu = new BMenuField( r, NULL, "File format:",
                                     fDstFormatPopUp, true );
    fDstFormatMenu->SetDivider( b.right/3 );
    box->AddChild( fDstFormatMenu );

    r = BRect( 10,40,b.right-10,60 );
    fDstCodecsPopUp = new BPopUpMenu( "" );
    fDstCodecsMenu  = new BMenuField( r, NULL, "Codecs:",
                                      fDstCodecsPopUp, true );
    fDstCodecsMenu->SetDivider( b.right/3 );
    box->AddChild( fDstCodecsMenu );

    r = BRect( 10,65,b.right-10,85 );
    fDstFileControl = new BTextControl( r, NULL, "File:",
        "/boot/home/Desktop/Movie", new BMessage() );
    fDstFileControl->SetDivider( b.right/3 );
    box->AddChild( fDstFileControl );

    r = BRect( b.right-90,90,b.right-10,115 );
    fBrowseButton = new BButton( r, NULL, "Browse",
                                 new BMessage( MSG_BROWSE ) );
    box->AddChild( fBrowseButton );

    /* Video box */
    b = Bounds();
    r = BRect( 10,145,b.right/2-5,395 );
    box = new BBox( r );
    box->SetLabel( "Video" );
    AddChild( box );

    b = box->Bounds();

    r = BRect( 10,15,b.right-10,35 );
    fVidRatePopUp = new BPopUpMenu( "" );
    fVidRatePopUp->AddItem( new BMenuItem( "Same as source",
        new BMessage() ) );
    for( int i = 0; i < hb_video_rates_count; i++ )
    {
        fVidRatePopUp->AddItem( new BMenuItem( hb_video_rates[i].string,
            new BMessage() ) );
    }
    fVidRatePopUp->ItemAt( 0 )->SetMarked( true );
    fVidRateMenu = new BMenuField( r, NULL, "Framerate (fps):",
                                   fVidRatePopUp, true );
    box->AddChild( fVidRateMenu );

    r = BRect( 10,40,b.right-10,60 );
    fVidEncoderPopUp = new BPopUpMenu( "" );
    fVidEncoderMenu  = new BMenuField( r, NULL, "Encoder:",
                                       fVidEncoderPopUp, true );
    box->AddChild( fVidEncoderMenu );

    r = BRect( 10,65,b.right-10,85 );
    fVidQualityString = new BStringView( r, NULL, "Quality:" );
    box->AddChild( fVidQualityString );

    r = BRect( 10,90,b.right*2/3,110);
    fVidTargetRadio = new BRadioButton( r, NULL, "Target size (MB):",
        new BMessage( MSG_QUALITYRADIO ) );
    box->AddChild( fVidTargetRadio );

    r = BRect( b.right*2/3+1,90,b.right-10,110); 
    fVidTargetControl = new BTextControl( r, NULL, "", "700",
        new BMessage() );
    fVidTargetControl->SetDivider( 0 );
    box->AddChild( fVidTargetControl );

    r = BRect( 10,115,b.right/2,135);
    fVidAverageRadio = new BRadioButton( r, NULL, "Average bitrate (kbps):",
        new BMessage( MSG_QUALITYRADIO ) );
    fVidAverageRadio->SetValue( 1 );
    box->AddChild( fVidAverageRadio );

    r = BRect( b.right*2/3+1,115,b.right-10,135); 
    fVidAverageControl = new BTextControl( r, NULL, "", "1000",
        new BMessage() );
    fVidAverageControl->SetDivider( 0 );
    box->AddChild( fVidAverageControl );

    r = BRect( 10,140,b.right/2,160);
    fVidConstantRadio = new BRadioButton( r, NULL, "Constant quality:",
        new BMessage( MSG_QUALITYRADIO ) );
    box->AddChild( fVidConstantRadio );

    r = BRect( 20,165,b.right-10,195);
    fVidConstantSlider = new BSlider( r, NULL, NULL,
        new BMessage( MSG_SLIDER ), 0, 100 );
    fVidConstantSlider->SetValue( 50 );
    SliderChanged();
    box->AddChild( fVidConstantSlider );

    r = BRect( 10,200,b.right-10,220);
    fVidGrayCheck = new BCheckBox( r, NULL, "Grayscale encoding", new BMessage() );
    box->AddChild( fVidGrayCheck );
    r = BRect( 10,220,b.right-10,240);
    fVidTwoPassCheck = new BCheckBox( r, NULL, "2-pass encoding", new BMessage() );
    box->AddChild( fVidTwoPassCheck );

    /* Subtitles box */
    b = Bounds();
    r = BRect( b.right/2+5,145,b.right-10,190 );
    box = new BBox( r );
    box->SetLabel( "Subtitles" );
    AddChild( box );

    b = box->Bounds();

    r = BRect( 10,15,b.right-10,35 );
    fSubPopUp = new BPopUpMenu( "" );
    fSubMenu  = new BMenuField( r, NULL, "Language:",
                                fSubPopUp, true );
    box->AddChild( fSubMenu );

    /* Audio box */
    b = Bounds();
    r = BRect( b.right/2+5,200,b.right-10,320 );
    box = new BBox( r );
    box->SetLabel( "Audio" );
    AddChild( box );

    b = box->Bounds();

    r = BRect( 10,15,b.right-10,35 );
    fAudLang1PopUp = new BPopUpMenu( "" );
    fAudLang1Menu  = new BMenuField( r, NULL, "Language 1:",
                                     fAudLang1PopUp, true );
    box->AddChild( fAudLang1Menu );
    r = BRect( 10,40,b.right-10,60 );
    fAudLang2PopUp = new BPopUpMenu( "" );
    fAudLang2Menu = new BMenuField( r, NULL, "Language 2:",
                                    fAudLang2PopUp, true );
    box->AddChild( fAudLang2Menu );
    r = BRect( 10,65,b.right-10,85 );
    fAudRatePopUp = new BPopUpMenu( "" );
    for( int i = 0; i < hb_audio_rates_count; i++ )
    {
        fAudRatePopUp->AddItem( new BMenuItem( hb_audio_rates[i].string,
            new BMessage ) );
    }
    fAudRatePopUp->ItemAt( hb_audio_rates_default )->SetMarked( true );
    fAudRateMenu = new BMenuField( r, NULL, "Sample rate (Hz):",
                                   fAudRatePopUp, true );
    box->AddChild( fAudRateMenu );
    r = BRect( 10,90,b.right-10,110 );
    fAudBitratePopUp = new BPopUpMenu( "" );
    for( int i = 0; i < hb_audio_bitrates_count; i++ )
    {
        fAudBitratePopUp->AddItem( new BMenuItem(
            hb_audio_bitrates[i].string, new BMessage ) );
    }
    fAudBitratePopUp->ItemAt(
        hb_audio_bitrates_default )->SetMarked( true );
    fAudBitrateMenu = new BMenuField( r, NULL, "Bitrate (kbps):",
                                      fAudBitratePopUp, true );
    box->AddChild( fAudBitrateMenu );

    /* Picture settings */
    b = Bounds();
    r = BRect( b.right-110,370,b.right-10,395 );
    fPictureButton = new BButton( r, NULL, "Picture settings...",
                                  new BMessage( MSG_PICSETTINGS ) );
    AddChild( fPictureButton );

    /* Bottom */
    r = BRect( 10,405,b.right-10,435 );
    fProgressBar = new BStatusBar( r, NULL );
    AddChild( fProgressBar );

    r = BRect( 10,450,b.right-370,470);
    fQueueCheck = new BCheckBox( r, NULL, "Enable Queue",
                                 new BMessage( MSG_QUEUE_ENABLE ) );
    AddChild( fQueueCheck );

    r = BRect( b.right-360,445,b.right-280,470 );
    fAddButton = new BButton( r, NULL, "Add to Queue",
                              new BMessage( MSG_QUEUE_ADD ) );

    r = BRect( b.right-270,445,b.right-190,470 );
    fShowButton = new BButton( r, NULL, "Show queue",
                               new BMessage( MSG_QUEUE_SHOW ) );

    r = BRect( b.right-180,445,b.right-100,470 );
    fPauseButton = new BButton( r, NULL, "Pause",
                                new BMessage( MSG_PAUSE ) );
    AddChild( fPauseButton );

    r = BRect( b.right-90,445,b.right-10,470 );
    fRipButton = new BButton( r, NULL, "Rip",
                              new BMessage( MSG_START ) );
    AddChild( fRipButton );

    EnableUI( false );
    fPauseButton->SetEnabled( false );
    fRipButton->SetEnabled( false );

    FormatPopUpChanged();

    fFilePanel = NULL;
}

void MainView::HandleMessage( BMessage * msg )
{
    switch( msg->what )
    {
        case MSG_TITLEPOPUP:
            TitlePopUpChanged();
            break;

        case MSG_CHAPTERPOPUP:
            ChapterPopUpChanged();
            break;

        case MSG_FORMATPOPUP:
            FormatPopUpChanged();
            break;

        case MSG_CODECSPOPUP:
            CodecsPopUpChanged();
            break;

        case MSG_BROWSE:
            if( !fFilePanel )
            {
                fFilePanel = new BFilePanel( B_SAVE_PANEL,
                    new BMessenger( Window() ), NULL, 0, false );
            }
            fFilePanel->Show();
            break;

        case B_SAVE_REQUESTED:
        {
            entry_ref ref;
            BString string;
            if( msg->FindRef( "directory", 0, &ref ) == B_OK &&
                msg->FindString( "name", &string ) == B_OK )
            {
                BPath * path = new BPath( &ref );
                string.Prepend( "/" );
                string.Prepend( path->Path() );
                fDstFileControl->SetText( string.String() );
                CheckExtension();
            }
            break;
        }

        case MSG_QUALITYRADIO:
            QualityRadioChanged();
            break;

        case MSG_SLIDER:
            SliderChanged();
            break;

        case MSG_PICSETTINGS:
            fPicWin = new PicWindow( fHandle, fSrcTitlePopUp->IndexOf(
                fSrcTitlePopUp->FindMarked() ) );
            fPicWin->Show();
            break;

        case MSG_QUEUE_ENABLE:
            if( fQueueCheck->Value() )
            {
                AddChild( fAddButton );
                AddChild( fShowButton );
            }
            else
            {
                RemoveChild( fAddButton );
                RemoveChild( fShowButton );
            }
            break;

        case MSG_QUEUE_ADD:
            AddJob();
            break;

        case MSG_QUEUE_SHOW:
            fQueueWin = new QueueWindow( fHandle );
            fQueueWin->Show();
            break;

        case MSG_PAUSE:
            fPauseButton->SetEnabled( false );
            fRipButton->SetEnabled( false );
            if( !strcmp( fPauseButton->Label(), "Resume" ) )
            {
                hb_resume( fHandle );
            }
            else
            {
                hb_pause( fHandle );
            }
            break;

        case MSG_START:
        {
            if( !strcmp( fRipButton->Label(), "Cancel" ) )
            {
                fPauseButton->SetEnabled( false );
                fRipButton->SetEnabled( false );
                hb_stop( fHandle );
                break;
            }

            EnableUI( false );
            fPauseButton->SetEnabled( false );
            fRipButton->SetEnabled( false );

            if( !fQueueCheck->Value() )
            {
                AddJob();
            }

            hb_start( fHandle );
            break;
        }
    }
}

void MainView::Update( hb_state_t * s )
{
    if( !LockLooper() )
    {
        fprintf( stderr, "LockLooper failed\n" );
        return;
    }

    switch( s->state )
    {
#define p s->param.scandone
        case HB_STATE_SCANDONE:
        {
            hb_list_t  * list;
            hb_title_t * title;
            char         string[1024];

            list = hb_get_titles( fHandle );
            for( int i = 0; i < hb_list_count( list ); i++ )
            {
                title = (hb_title_t *) hb_list_item( list, i );
                fSrcDVD2String->SetText( title->dvd );
                snprintf( string, 1024, "%d - %02dh%02dm%02ds",
                          title->index, title->hours, title->minutes,
                          title->seconds );
                fSrcTitlePopUp->AddItem( new BMenuItem( string,
                    new BMessage( MSG_TITLEPOPUP ) ) );
            }
            fSrcTitlePopUp->ItemAt( 0 )->SetMarked( true );
            fSrcTitle = -1;
            TitlePopUpChanged();

            EnableUI( true );
            fRipButton->SetEnabled( true );
            fPauseButton->SetEnabled( false );
            break;
        }
#undef p

#define p s->param.working
        case HB_STATE_WORKING:
        {
            float progress_total;
            char  text[1024];
            progress_total = ( p.progress + p.job_cur - 1 ) / p.job_count;
            snprintf( text, 1024, "Encoding: task %d of %d, %.2f %%",
                      p.job_cur, p.job_count, 100.0 * p.progress );
            if( p.seconds > -1 )
            {
                snprintf( text + strlen( text ), 1024 - strlen( text ),
                          " (%.2f fps, avg %.2f fps, ETA %02dh%02dm%02ds)",
                          p.rate_cur, p.rate_avg, p.hours, p.minutes,
                          p.seconds );
            }
            fProgressBar->Update( fProgressBar->MaxValue() *
                progress_total - fProgressBar->CurrentValue(), text );

            fPauseButton->SetLabel( "Pause" );
            fPauseButton->SetEnabled( true );
            fRipButton->SetLabel( "Cancel" );
            fRipButton->SetEnabled( true );
            break;
        }
#undef p

        case HB_STATE_PAUSED:
            fProgressBar->Update( 0, "Paused" );
            fPauseButton->SetLabel( "Resume" );
            fPauseButton->SetEnabled( true );
            fRipButton->SetLabel( "Cancel" );
            fRipButton->SetEnabled( true );
            break;          

#define p s->param.workdone 
        case HB_STATE_WORKDONE:
            fProgressBar->Update( - fProgressBar->CurrentValue(),
                                  "Done." );
            EnableUI( true );
            fPauseButton->SetLabel( "Pause" );
            fPauseButton->SetEnabled( false );
            fRipButton->SetLabel( "Rip" );
            fRipButton->SetEnabled( true );

            /* FIXME */
            hb_job_t * job;
            while( ( job = hb_job( fHandle, 0 ) ) )
            {
                hb_rem( fHandle, job );
            }
            break;
#undef p
    }

    UnlockLooper();
}

void MainView::EnableUI( bool b )
{

    rgb_color mycolor;
    mycolor.red = mycolor.green = mycolor.blue = b ? 0 : 128;
    mycolor.alpha = 255;

    BStringView * strings[] =
      { fSrcDVD1String, fSrcDVD2String, fSrcChapString,
        fSrcChapToString, fSrcDur1String, fSrcDur2String,
        fVidQualityString };
    for( unsigned i = 0; i < sizeof( strings ) / sizeof( void * ); i++ )
    {
        strings[i]->SetHighColor( mycolor );
        strings[i]->Invalidate(); /* Force redraw */
    }

    BMenuField * fields[] =
      { fSrcTitleMenu, fSrcChapStartMenu, fSrcChapEndMenu,
        fDstFormatMenu, fDstCodecsMenu, fVidRateMenu, fVidEncoderMenu,
        fSubMenu, fAudLang1Menu, fAudLang2Menu, fAudRateMenu,
        fAudBitrateMenu };
    for( unsigned i = 0; i < sizeof( fields ) / sizeof( void * ); i++ )
    {
        fields[i]->SetEnabled( b );
    }

    BControl * controls[] =
      { fDstFileControl, fBrowseButton, fVidTargetRadio,
        fVidTargetControl, fVidAverageRadio, fVidAverageControl,
        fVidConstantRadio, fVidConstantSlider, fVidGrayCheck,
        fVidTwoPassCheck, fPictureButton };
    for( unsigned i = 0; i < sizeof( controls ) / sizeof( void * ); i++ )
    {
        controls[i]->SetEnabled( b );
    }

    if( b )
    {
        QualityRadioChanged();
    }
}

void MainView::TitlePopUpChanged()
{
    int index = fSrcTitlePopUp->IndexOf( fSrcTitlePopUp->FindMarked() );
    if( index == fSrcTitle )
    {
        /* No change actually */
        return;
    }
    fSrcTitle = index;

    /* Get a pointer to the title */
    hb_list_t    * list;
    hb_title_t   * title;
    list  = hb_get_titles( fHandle );
    title = (hb_title_t *) hb_list_item( list, index );

    char text[1024];
    BMenuItem * item;

    /* Update chapters popups */
    while( ( item = fSrcChapStartPopUp->RemoveItem( 0L ) ) )
    {
        delete item;
    }
    while( ( item = fSrcChapEndPopUp->RemoveItem( 0L ) ) )
    {
        delete item;
    }
    for( int i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        snprintf( text, 1024, "%d", i + 1 );
        fSrcChapStartPopUp->AddItem( new BMenuItem( text,
            new BMessage( MSG_CHAPTERPOPUP ) ) );
        fSrcChapEndPopUp->AddItem( new BMenuItem( text,
            new BMessage( MSG_CHAPTERPOPUP ) ) );
    }
    fSrcChapStartPopUp->ItemAt( 0 )->SetMarked( true );
    fSrcChapEndPopUp->ItemAt( hb_list_count( title->list_chapter )
                                - 1 )->SetMarked( true );
    ChapterPopUpChanged();

    /* Update subtitles popup */
    hb_subtitle_t * sub;
    while( ( item = fSubPopUp->RemoveItem( 0L ) ) )
    {
        delete item;
    }
    fSubPopUp->AddItem( new BMenuItem( "None", new BMessage() ) );
    for( int i = 0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        sub = (hb_subtitle_t *) hb_list_item( title->list_subtitle, i );
        fSubPopUp->AddItem( new BMenuItem( sub->lang, new BMessage() ) );
    }
    fSubPopUp->ItemAt( 0 )->SetMarked( true );

    /* Update audio popups */
    hb_audio_t * audio;
    while( ( item = fAudLang1PopUp->RemoveItem( 0L ) ) )
    {
        delete item;
    }
    while( ( item = fAudLang2PopUp->RemoveItem( 0L ) ) )
    {
        delete item;
    }
    fAudLang1PopUp->AddItem( new BMenuItem( "None", new BMessage() ) );
    fAudLang2PopUp->AddItem( new BMenuItem( "None", new BMessage() ) );
    for( int i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = (hb_audio_t *) hb_list_item( title->list_audio, i );
        fAudLang1PopUp->AddItem( new BMenuItem( audio->lang, new BMessage() ) );
        fAudLang2PopUp->AddItem( new BMenuItem( audio->lang, new BMessage() ) );
    }
    fAudLang1PopUp->ItemAt( 1 )->SetMarked( true );
    fAudLang2PopUp->ItemAt( 0 )->SetMarked( true );
}

void MainView::ChapterPopUpChanged()
{
    /* Get a pointer to the title */
    hb_list_t    * list;
    hb_title_t   * title;
    list  = hb_get_titles( fHandle );
    title = (hb_title_t *) hb_list_item( list,
        fSrcTitlePopUp->IndexOf( fSrcTitlePopUp->FindMarked() ) );

    hb_chapter_t * chapter;
    int64_t        duration = 0;
    for( int i = fSrcChapStartPopUp->IndexOf(
             fSrcChapStartPopUp->FindMarked() );
         i <= fSrcChapEndPopUp->IndexOf(
             fSrcChapEndPopUp->FindMarked() );
         i++ )
    {
        chapter = (hb_chapter_t *) hb_list_item( title->list_chapter, i );
        duration += chapter->duration;
    }
    duration /= 90000; /* pts -> seconds */

    char text[1024];
    snprintf( text, sizeof(text), "%02lld:%02lld:%02lld",
              duration / 3600, ( duration / 60 ) % 60, duration % 60 );
    fSrcDur2String->SetText( text );
}

void MainView::FormatPopUpChanged()
{
    int format;
    BMenuItem * item;

    format = fDstFormatPopUp->IndexOf( fDstFormatPopUp->FindMarked() );
    if( format == fDstFormat )
    {
        /* No change actually */
        CheckExtension();
        return;
    }
    fDstFormat = format;

    /* Empty codecs popup */
    while( ( item = fDstCodecsPopUp->RemoveItem( 0L ) ) )
    {
        delete item;
    }

    /* Add acceptable video codec / audio codec combinations */
#define ADDITEM(a) \
    fDstCodecsPopUp->AddItem( new BMenuItem( a, new BMessage( MSG_CODECSPOPUP ) ) )
    switch( format )
    {
        case 0:
            ADDITEM( "MPEG-4 Video / AAC Audio" );
            ADDITEM( "AVC/H.264 Video / AAC Audio" );
            break;
        case 1:
            ADDITEM( "MPEG-4 Video / MP3 Audio" );
            ADDITEM( "MPEG-4 Video / AC-3 Audio" );
            ADDITEM( "AVC/H.264 Video / MP3 Audio" );
            ADDITEM( "AVC/H.264 Video / AC-3 Audio" );
            break;
        case 2:
            ADDITEM( "MPEG-4 Video / Vorbis Audio" );
            ADDITEM( "MPEG-4 Video / MP3 Audio" );
            break;
    }
#undef ADDITEM

    fDstCodecsPopUp->ItemAt( 0 )->SetMarked( true );

    CheckExtension();
    CodecsPopUpChanged();
}

void MainView::CodecsPopUpChanged()
{
    int format = fDstFormatPopUp->IndexOf( fDstFormatPopUp->FindMarked() );
    int codecs = fDstCodecsPopUp->IndexOf( fDstCodecsPopUp->FindMarked() );

    BMenuItem * item;

    /* Update the encoder popup if necessary */
    if( ( FormatSettings[format][codecs] & HB_VCODEC_X264 ) &&
        fVidEncoderPopUp->CountItems() > 1 )
    {
        /* MPEG-4 -> H.264 */
        while( ( item = fVidEncoderPopUp->RemoveItem( 0L ) ) )
            delete item;
        fVidEncoderPopUp->AddItem( new BMenuItem( "x264", new BMessage() ) );
        fVidEncoderPopUp->ItemAt( 0 )->SetMarked( true );
    }
    else if( ( FormatSettings[format][codecs] & HB_VCODEC_FFMPEG ) &&
             fVidEncoderPopUp->CountItems() < 2 )
    {
        /* H.264 -> MPEG-4 */
        while( ( item = fVidEncoderPopUp->RemoveItem( 0L ) ) )
            delete item;
        fVidEncoderPopUp->AddItem( new BMenuItem( "FFmpeg", new BMessage() ) );
        fVidEncoderPopUp->AddItem( new BMenuItem( "XviD", new BMessage() ) );
        fVidEncoderPopUp->ItemAt( 0 )->SetMarked( true );
    }

    if( FormatSettings[format][codecs] & HB_ACODEC_AC3 )
    {
        /* AC-3 pass-through: disable samplerate and bitrate */
        fAudRatePopUp->SetEnabled( false );
        fAudBitratePopUp->SetEnabled( false );
    }
    else if( fVidEncoderPopUp->IsEnabled() )
    {
        fAudRatePopUp->SetEnabled( true );
        fAudBitratePopUp->SetEnabled( true );
    }
}

void MainView::CheckExtension()
{
    char * ext = NULL;
    switch( fDstFormat )
    {
        case 0:
            ext = ".mp4";
            break;
        case 1:
            ext = ".avi";
            break;
        case 2:
            ext = ".ogm";
            break;
    }

    char newname[1024];
    const char * oldname = fDstFileControl->Text();
    memcpy( newname, oldname, strlen( oldname ) );
    if( strlen( oldname ) > 4 &&
        oldname[strlen( oldname ) - 4] == '.' )
    {
        /* Replace extension */
        memcpy( &newname[strlen( oldname ) - 4], ext, 5 );
    }
    else
    {
        /* Add extension */
        memcpy( &newname[strlen( oldname )], ext, 5 );
    }
    fDstFileControl->SetText( newname );
}

void MainView::QualityRadioChanged()
{
    fVidTargetControl->SetEnabled( fVidTargetRadio->Value() );
    fVidAverageControl->SetEnabled( fVidAverageRadio->Value() );
    fVidConstantSlider->SetEnabled( fVidConstantRadio->Value() );
    fVidTwoPassCheck->SetEnabled( !fVidConstantRadio->Value() );
    if( fVidConstantRadio->Value() )
        fVidTwoPassCheck->SetValue( 0 );
}

void MainView::SliderChanged()
{
    char text[1024];
    snprintf( text, 1024, "Constant quality: %ld %%",
              fVidConstantSlider->Value() );
    fVidConstantRadio->SetLabel( text );
}

void MainView::AddJob()
{
    hb_list_t  * list;
    hb_title_t * title;
    hb_job_t   * job;
    list  = hb_get_titles( fHandle );
    title = (hb_title_t *) hb_list_item( list,
        fSrcTitlePopUp->IndexOf( fSrcTitlePopUp->FindMarked() ) );
    job = title->job;

    job->chapter_start = fSrcChapStartPopUp->IndexOf(
        fSrcChapStartPopUp->FindMarked() ) + 1;
    job->chapter_end = fSrcChapEndPopUp->IndexOf(
        fSrcChapEndPopUp->FindMarked() ) + 1;

    int format = fDstFormatPopUp->IndexOf(
        fDstFormatPopUp->FindMarked() );
    int codecs = fDstCodecsPopUp->IndexOf(
        fDstCodecsPopUp->FindMarked() );

    job->mux    = FormatSettings[format][codecs] & HB_MUX_MASK;
    job->vcodec = FormatSettings[format][codecs] & HB_VCODEC_MASK;
    job->acodec = FormatSettings[format][codecs] & HB_ACODEC_MASK;

    if( ( job->vcodec & HB_VCODEC_FFMPEG ) &&
        fVidEncoderPopUp->IndexOf(
            fVidEncoderPopUp->FindMarked() ) > 0 )
    {
        job->vcodec = HB_VCODEC_XVID;
    }

    int index;
    index = fVidRatePopUp->IndexOf(
        fVidRatePopUp->FindMarked() );
    if( index > 0 )
    {
        job->vrate_base = hb_video_rates[index-1].rate;
    }
    else
    {
        job->vrate_base = title->rate_base;
    }

    job->grayscale = fVidGrayCheck->Value();

    job->subtitle = fSubPopUp->IndexOf(
        fSubPopUp->FindMarked() ) - 1;

    job->audios[0] = fAudLang1PopUp->IndexOf(
        fAudLang1PopUp->FindMarked() ) - 1;
    job->audios[1] = fAudLang2PopUp->IndexOf(
        fAudLang2PopUp->FindMarked() ) - 1;
    job->audios[2] = -1;

    job->arate = hb_audio_rates[fAudRatePopUp->IndexOf(
        fAudRatePopUp->FindMarked() )].rate;
    job->abitrate = hb_audio_bitrates[fAudBitratePopUp->IndexOf(
        fAudBitratePopUp->FindMarked() )].rate;

    if( fVidConstantRadio->Value() )
    {
        job->vquality = 0.01 * fVidConstantSlider->Value();
        job->vbitrate = 0;
    }
    else if( fVidTargetRadio->Value() )
    {
        job->vquality = -1.0;
        job->vbitrate = hb_calc_bitrate( job,
            atoi( fVidTargetControl->Text() ) );
    }
    else
    {
        job->vquality = -1.0;
        job->vbitrate = atoi( fVidAverageControl->Text() );
    }

    job->file = strdup( fDstFileControl->Text() );

    if( fVidTwoPassCheck->Value() )
    {
        job->pass = 1;
        hb_add( fHandle, job );
        job->pass = 2;
        hb_add( fHandle, job );
    }
    else
    {
        job->pass = 0;
        hb_add( fHandle, job );
    }
}

MainWindow::MainWindow( hb_handle_t * handle )
    : BWindow( BRect( 0,0,10,10 ), "HandBrake " HB_VERSION,
               B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
    fHandle = handle;

    /* Add the main view */
    fView = new MainView( fHandle );
    AddChild( fView );

    /* Resize to fit */
    ResizeTo( fView->Bounds().Width(), fView->Bounds().Height() );

    /* Center */
    BScreen screen;
    MoveTo( ( screen.Frame().Width() - fView->Bounds().Width() ) / 2,
            ( screen.Frame().Height() - fView->Bounds().Height() ) / 2 );
}

void MainWindow::MessageReceived( BMessage * msg )
{
    switch( msg->what )
    {
        case MSG_TITLEPOPUP:
        case MSG_CHAPTERPOPUP:
        case MSG_FORMATPOPUP:
        case MSG_CODECSPOPUP:
        case MSG_BROWSE:
        case MSG_QUALITYRADIO:
        case MSG_SLIDER:
        case MSG_PICSETTINGS:
        case MSG_QUEUE_ENABLE:
        case MSG_QUEUE_ADD:
        case MSG_QUEUE_SHOW:
        case MSG_PAUSE:
        case MSG_START:
        case B_SAVE_REQUESTED:
            fView->HandleMessage( msg );
            break;

        default:
            BWindow::MessageReceived( msg );
            break;
    }
}

bool MainWindow::QuitRequested()
{
    be_app_messenger.SendMessage( B_QUIT_REQUESTED );
    return true;
}

void MainWindow::Update( hb_state_t * s )
{
    fView->Update( s );
}
