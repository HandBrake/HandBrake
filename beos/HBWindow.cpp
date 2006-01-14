#include "HBWindow.h"

#include <MenuItem.h>
#include <Screen.h>

#define SCAN_OPEN 'scop'
#define RIP_RIP   'riri'

static void _Scanning( void * data, int title, int titleCount )
{
    ((HBWindow*)data)->Scanning( title, titleCount );
}
static void _ScanDone( void * data, HBList * titleList )
{
    ((HBWindow*)data)->ScanDone( titleList );
}
static void _Encoding( void * data, float position, int pass,
                      int passCount, float frameRate,
                      float avgFrameRate, int remainingTime )
{
    ((HBWindow*)data)->Encoding( position, pass, passCount, frameRate,
                                 avgFrameRate, remainingTime );
}
static void _RipDone( void * data, int result )
{
    ((HBWindow*)data)->RipDone( result );
}

HBWindow::HBWindow()
    : MWindow( BRect( 0,0,10,10 ), "HandBrake " HB_VERSION,
               B_TITLED_WINDOW, 0 )
{
    /* Init libhb */
    HBCallbacks callbacks;
    callbacks.data     = this;
    callbacks.scanning = _Scanning;
    callbacks.scanDone = _ScanDone;
    callbacks.encoding = _Encoding;
    callbacks.ripDone  = _RipDone;

    fHandle = HBInit( 1, 0 );
    HBSetCallbacks( fHandle, callbacks );

    fScanView = new HGroup(
        new Space( minimax( 10,-1,10,-1 ) ),
        new VGroup(
          new Space( minimax( -1,10,-1,10 ) ),
          new MStringView( "Welcome to HandBrake. Select a DVD to open:" ),
          new Space( minimax( -1,5,-1,5 ) ),
          new HGroup(
            fScanRadio = new MRadioGroup( "Detected volume:",
                "DVD Folder:", 0 ),
            new VGroup(
              fScanDetectedPopup = new MPopup( NULL,
                  "/Bulk/ANGEL_SEASON2_DISC6", 0 ),
              fScanFolderControl = new MTextControl( NULL, NULL ),
              0 ),
            0 ),
          fScanBrowseButton = new MButton( "Browse", 12 ),
          fScanStatusString = new MStringView( "" ),
          fScanProgress = new MProgressBar( this ),
          fScanOpenButton = new MButton( "Open",
              new BMessage( SCAN_OPEN ) ),
          new Space( minimax( -1,10,-1,10000 ) ),
          0 ),
        new Space( minimax( 10,-1,10,-1 ) ),
        0 );

    fScanDetectedPopup->Menu()->ItemAt(0)->SetMarked(true);
    fScanProgress->ct_mpm = minimax( 0,12,10000,12 );

    fRipView = new HGroup(
        new Space( minimax( 10,-1,10,-1 ) ),
        new VGroup(
          new Space( minimax( -1,10,-1,10 ) ),
          new MBorder(
            M_LABELED_BORDER, 15, "General",
            new VGroup(
              fRipTitlePopup = new MPopup( "DVD title:", "dummy", 0 ),
              new MPopup( "Output format:",
                  "MP4 file / MPEG-4 video / AAC audio",
                  "OGM file / MPEG-4 video / Vorbis audio",
                  "AVI file / MPEG-4 video / MP3 audio",
                  "AVI file / H264 video / MP3 audio", 0 ),
              new MTextControl( "File:", "/boot/home/Desktop/Movie.mp4" ),
              new MButton( "Browse" ),
              0 )
            ),
          new MBorder(
            M_LABELED_BORDER, 15, "Video",
            new VGroup(
              new MPopup( "MPEG-4 encoder:", "FFmpeg", "XviD", 0 ),
              new HGroup(
                new MStringView( "Bitrate:" ),
                new MRadioGroup( "Custom (kbps)", "Target size (MB)", 0 ),
                new VGroup(
                  new MTextControl( NULL, NULL ),
                  new MTextControl( NULL, NULL ),
                  0 ),
                0 ),
              new MSplitter(),
              new HGroup(
                new MCheckBox( "2-pass encoding" ),
                new MButton( "Crop & Scale..." ),
                0 ),
              0 )
            ),
          new MBorder(
            M_LABELED_BORDER, 15, "Audio",
            new VGroup(
              fRipLanguage1Popup = new MPopup( "Language 1:",
                  "dummy", 0 ),
              fRipLanguage2Popup = new MPopup( "Language 2 (optional):",
                  "dummy", 0 ),
              fRipBitratePopup = new MPopup( "Bitrate (kbps):", "32",
                  "64", "96", "128", "160", "192", "224", "256", "288",
                  "320", 0 ),
              0 )
            ),
          new MProgressBar( this ),
          new HGroup(
            new MButton( "Pause" ),
            new MButton( "Rip!", new BMessage( RIP_RIP ) ),
            0 ),
          new Space( minimax( -1,10,-1,10 ) ),
          0 ),
        new Space( minimax( 10,-1,10,-1 ) ),
        0 );

    fRipBitratePopup->Menu()->ItemAt(3)->SetMarked( true );

    fLayers = new LayeredGroup( fScanView, fRipView, 0 );
    AddChild( dynamic_cast<BView*>(fLayers) );

    /* Center the window */
    BScreen screen;
    MoveTo( ( screen.Frame().Width() ) / 2,
            ( screen.Frame().Height() ) / 2 );
    
    Show();
}

bool HBWindow::QuitRequested()
{
    HBClose( &fHandle );
    be_app->PostMessage( B_QUIT_REQUESTED );
    return true;
}

void HBWindow::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case SCAN_OPEN:
            Lock();
            /* That's ugly, but there doesn't seem to be another way */
            ((BRadioButton*)fScanRadio->ChildAt(0))->SetEnabled( false );
            ((BRadioButton*)fScanRadio->ChildAt(1))->SetEnabled( false );
            fScanDetectedPopup->SetEnabled( false );
            fScanFolderControl->SetEnabled( false );
            fScanBrowseButton->SetEnabled( false );
            fScanOpenButton->SetEnabled( false );
            fScanStatusString->SetText( "Opening device..." );
            Unlock();
            HBScanDVD( fHandle,
                fScanDetectedPopup->Menu()->FindMarked()->Label(), 0 );
            break;

        case RIP_RIP:
        {
            HBTitle * title = (HBTitle*) HBListItemAt( fTitleList,
                    fRipTitlePopup->Menu()->IndexOf(
                        fRipTitlePopup->Menu()->FindMarked() ) );
            title->file = strdup( "/boot/home/Desktop/Movie.mp4" );
            title->twoPass = 0;
            title->deinterlace = 0;
            title->topCrop    = title->autoTopCrop;
            title->bottomCrop = title->autoBottomCrop;
            title->leftCrop   = title->autoLeftCrop;
            title->rightCrop  = title->autoRightCrop;
            title->bitrate = 1024;
            title->codec = HB_CODEC_FFMPEG;
            title->mux = HB_MUX_MP4;
            HBAudio * audio = (HBAudio*) HBListItemAt(
                    title->audioList, 0 );
            audio->codec = HB_CODEC_AAC;
            audio->outBitrate = 128;
            HBListAdd( title->ripAudioList, audio );
            HBStartRip( fHandle, title );
            break;
        }

        default:
            MWindow::MessageReceived( message );
            break;
    }
}

void HBWindow::Scanning( int title, int titleCount )
{
    Lock();
    char string[1024]; memset( string, 0, 1024 );
    snprintf( string, 1023, "Scanning title %d of %d...",
              title, titleCount );
    fScanStatusString->SetText( string );
    fScanProgress->SetValue( (float) title / titleCount );
    Unlock();
}

void HBWindow::ScanDone( HBList * titleList )
{
#define menu fRipTitlePopup->Menu()
    Lock();
    BMenuItem * item;
    while( ( item = menu->ItemAt(0) ) )
    {
        menu->RemoveItem( item );
        delete item;
    }
    HBTitle * title;
    char      label[1024];
    for( int i = 0; i < HBListCount( titleList ); i++ )
    {
        memset( label, 0, 1024 );
        title = (HBTitle*) HBListItemAt( titleList, i );
        snprintf( label, 1023, "%d - %02dh%02dm%02ds", title->index,
                  title->length / 3600, ( title->length % 3600 ) / 60,
                  title->length % 60 );
        menu->AddItem( new BMenuItem( label, NULL ) );
    }
    menu->ItemAt(0)->SetMarked( true );
    fTitleList = titleList;
    UpdateLanguages();
    fLayers->ActivateLayer( 1 );
    Unlock();
#undef menu
}

void HBWindow::Encoding( float position, int pass, int passCount,
               float frameRate, float avgFrameRate,
               int remainingTime )
{
}

void HBWindow::RipDone( int result )
{
}

void HBWindow::UpdateLanguages()
{
#define menu fRipTitlePopup->Menu()
    HBTitle * title = (HBTitle*) HBListItemAt( fTitleList,
            menu->IndexOf( menu->FindMarked() ) );
#undef menu

#define menu1 fRipLanguage1Popup->Menu()
#define menu2 fRipLanguage2Popup->Menu()
    BMenuItem * item;
    while( ( item = menu1->ItemAt(0) ) )
    {
        menu1->RemoveItem( item );
        delete item;
    }
    while( ( item = menu2->ItemAt(0) ) )
    {
        menu2->RemoveItem( item );
        delete item;
    }
   
    HBAudio * audio;
    for( int i = 0; i < HBListCount( title->audioList ); i++ )
    {
        audio = (HBAudio*) HBListItemAt( title->audioList, i );
        menu1->AddItem( new BMenuItem( audio->language, NULL ) );
        menu2->AddItem( new BMenuItem( audio->language, NULL ) );
    }
    menu1->ItemAt(0)->SetMarked( true );
    menu2->AddItem( new BMenuItem( "None", NULL ) );
    menu2->ItemAt( menu2->CountItems() - 1 )->SetMarked( true );
#undef menu1
#undef menu2
}

