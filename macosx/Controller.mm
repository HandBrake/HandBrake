/* $Id: Controller.mm,v 1.30 2004/03/12 14:22:19 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <paths.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>

#include "Controller.h"

#define _(a) NSLocalizedString(a,nil)

static void _Scanning( void * data, int title, int titleCount );
static void _ScanDone( void * data, HBList * titleList );
static void _Encoding( void * data, float position, int pass,
                      int passCount, float curFrameRate,
                      float avgFrameRate, int remainingTime );
static void _RipDone( void * data, int result );

/*******************************
 * HBController implementation *
 *******************************/
@implementation HBController

- (void) applicationDidFinishLaunching: (NSNotification *) notification
{
    /* Init libhb */
    HBCallbacks callbacks;
    callbacks.data     = self;
    callbacks.scanning = _Scanning;
    callbacks.scanDone = _ScanDone;
    callbacks.encoding = _Encoding;
    callbacks.ripDone  = _RipDone;

    fHandle = HBInit( 1, 0 );
    HBSetCallbacks( fHandle, callbacks );

    [fPictureGLView SetHandle: fHandle];

    /* Detect drives mounted after the app is started */
    [[[NSWorkspace sharedWorkspace] notificationCenter]
        addObserver: self selector: @selector( DetectDrives: )
        name: NSWorkspaceDidMountNotification object: nil];
}

- (NSApplicationTerminateReply) applicationShouldTerminate:
    (NSApplication *) app
{
    if( [[fRipRipButton title] compare: _( @"Cancel" ) ]
            == NSOrderedSame )
    {
        [self Cancel: self];
        return NSTerminateCancel;
    }
    
    /* Clean up */
    HBClose( &fHandle );

    return NSTerminateNow;
}

- (void) awakeFromNib
{
    /* Strings for the Scan view */
    [fScWelcomeField  setStringValue: _( @"Welcome to HandBrake" )];
    [fScSelectField   setStringValue: _( @"Select a DVD:" )];
    [fScDetectedCell  setTitle: _( @"Detected volume" )];
    [fScDetectedPopUp removeAllItems];
    [fScFolderCell    setTitle: _( @"DVD Folder" )];
    [fScBrowseButton  setTitle: _( @"Browse" )];
    [fScStatusField   setStringValue: @""];
    [fScOpenButton    setTitle: _( @"Open" )];

    /* Strings for the Rip view */
    /* General box */
    [fRipGeneralField setStringValue: _( @"General" )];
    [fRipTitleField   setStringValue: _( @"DVD title" )];
    [fRipTitlePopUp   removeAllItems];
    [fRipFormatField  setStringValue: _( @"Output format" )];
    [fRipFormatPopUp  removeAllItems];
    [fRipFormatPopUp  addItemWithTitle:
        _( @"MP4 file / MPEG-4 video / AAC audio" )];
    [fRipFormatPopUp  addItemWithTitle:
        _( @"AVI file / MPEG-4 video / MP3 audio" )];
    [fRipFormatPopUp  addItemWithTitle:
        _( @"AVI file / H264 video / MP3 audio" )];
    [fRipFormatPopUp  addItemWithTitle:
        _( @"OGM file / MPEG-4 video / Vorbis audio" )];
    [fRipFileField1   setStringValue: _( @"File" )];
    [fRipFileField2   setStringValue: [NSString stringWithFormat:
        @"%@/Desktop/Movie.mp4", NSHomeDirectory()]];
    [fRipBrowseButton setTitle: _( @"Browse" )];

    /* Video box */
    [fRipVideoField   setStringValue: _( @"Video" )];
    [fRipEncoderField setStringValue: _( @"MPEG-4 encoder" )];
    [fRipEncoderPopUp removeAllItems];
    [fRipEncoderPopUp addItemWithTitle: @"FFmpeg"];
    [fRipEncoderPopUp addItemWithTitle: @"XviD"];
    [fRipBitrateField setStringValue: _( @"Bitrate" )];
    [fRipCustomCell   setTitle: _( @"Custom (kbps)" )];
    [fRipCustomField  setIntValue: 1024];
    [fRipTargetCell   setTitle: _( @"Target size (MB)" )];
    [fRipTargetField  setIntValue: 700];
    [fRipTwoPassCheck setTitle: _( @"2-pass encoding" )];
    [fRipCropButton   setTitle: _( @"Crop & Scale..." )];

    /* Audio box */
    [fRipAudioField  setStringValue: _( @"Audio" )];
    [fRipLang1Field  setStringValue: _( @"Language 1" )];
    [fRipLang1PopUp  removeAllItems];
    [fRipLang2Field  setStringValue: _( @"Language 2 (optional)" )];
    [fRipLang2PopUp  removeAllItems];
    [fRipAudBitField setStringValue: _( @"Bitrate (kbps)" )];
    [fRipAudBitPopUp removeAllItems];
    [fRipAudBitPopUp addItemWithTitle: @"32"];
    [fRipAudBitPopUp addItemWithTitle: @"40"];
    [fRipAudBitPopUp addItemWithTitle: @"48"];
    [fRipAudBitPopUp addItemWithTitle: @"56"];
    [fRipAudBitPopUp addItemWithTitle: @"64"];
    [fRipAudBitPopUp addItemWithTitle: @"80"];
    [fRipAudBitPopUp addItemWithTitle: @"96"];
    [fRipAudBitPopUp addItemWithTitle: @"112"];
    [fRipAudBitPopUp addItemWithTitle: @"128"];
    [fRipAudBitPopUp addItemWithTitle: @"160"];
    [fRipAudBitPopUp addItemWithTitle: @"192"];
    [fRipAudBitPopUp addItemWithTitle: @"224"];
    [fRipAudBitPopUp addItemWithTitle: @"256"];
    [fRipAudBitPopUp addItemWithTitle: @"320"];
    [fRipAudBitPopUp selectItemWithTitle: @"128"];

    /* Bottom */
    [fRipStatusField setStringValue: @""];
    [fRipInfoField   setStringValue: @""];
    [fRipPauseButton setTitle: _( @"Pause" )];
    [fRipRipButton   setTitle: _( @"Rip" )];

    /* Strings for the crop panel */
    [fWidthField1      setStringValue: _( @"Picture width" )];
    [fDeinterlaceCheck setTitle: _( @"Deinterlace picture" )];
    [fTopField1        setStringValue: _( @"Top cropping" )];
    [fBottomField1     setStringValue: _( @"Bottom cropping" )];
    [fLeftField1       setStringValue: _( @"Left cropping" )];
    [fRightField1      setStringValue: _( @"Right cropping" )];
    [fPreviousButton   setTitle: _( @"Previous" )];
    [fNextButton       setTitle: _( @"Next" )];
    [fAutocropButton   setTitle: _( @"Autocrop" )];
    [fOpenGLCheck      setTitle: _( @"Useless OpenGL effects" )];
    [fInfoField        setStringValue: @""];
    [fCloseButton      setTitle: _( @"Close" )];

    [self VideoMatrixChanged: self];

    /* Show the scan view */
    [fWindow setContentSize: [fScView frame].size];
    [fWindow setContentView: fScView];
    [fWindow center];

    /* Detect DVD drives */
    [self DetectDrives: nil];
    [self ScanMatrixChanged: self];
}

- (BOOL) windowShouldClose: (id) sender
{
    if( [[fRipRipButton title] compare: _( @"Cancel" ) ]
            == NSOrderedSame )
    {
        [self Cancel: self];
        return NO;
    }

    /* Stop the application when the user closes the window */
    [NSApp terminate: self];
    return YES;
}

- (IBAction) BrowseDVD: (id) sender
{
    /* Open a panel to let the user choose and update the text field */
    NSOpenPanel * panel = [NSOpenPanel openPanel];

    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: NO];
    [panel setCanChooseDirectories: YES ];

    [panel beginSheetForDirectory: nil file: nil types: nil
        modalForWindow: fWindow modalDelegate: self
        didEndSelector: @selector( BrowseDVDDone:returnCode:contextInfo: )
        contextInfo: nil];
}

- (void) BrowseDVDDone: (NSOpenPanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        [fScFolderField setStringValue:
            [[sheet filenames] objectAtIndex: 0]];
    }
}

- (IBAction) VideoMatrixChanged: (id) sender;
{
    if( ![fRipVideoMatrix isEnabled] )
    {
        [fRipCustomField setEnabled: NO];
        [fRipTargetField setEnabled: NO];
        return;
    }
    
    if( ![fRipVideoMatrix selectedRow] )
    {
        [fRipCustomField setEnabled: YES];
        [fRipTargetField setEnabled: NO];
    }
    else
    {
        [fRipCustomField setEnabled: NO];
        [fRipTargetField setEnabled: YES];
        [fRipTargetField UpdateBitrate];
    }
}

- (IBAction) BrowseFile: (id) sender
{
    /* Open a panel to let the user choose and update the text field */
    NSSavePanel * panel = [NSSavePanel savePanel];

    [panel beginSheetForDirectory: nil file: nil
        modalForWindow: fWindow modalDelegate: self
        didEndSelector: @selector( BrowseFileDone:returnCode:contextInfo: )
        contextInfo: nil];
}

- (void) BrowseFileDone: (NSSavePanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        [fRipFileField2 setStringValue: [sheet filename]];
        [self FormatPopUpChanged: self];
    }
}

- (IBAction) Scan: (id) sender
{
    [fScMatrix       setEnabled: NO];
    [fScDetectedPopUp         setEnabled: NO];
    [fScFolderField   setEnabled: NO];
    [fScBrowseButton setEnabled: NO];
    [fScProgress     setIndeterminate: YES];
    [fScProgress     startAnimation: self];
    [fScOpenButton       setEnabled: NO];
    [fScStatusField setStringValue: _( @"Opening device..." )];

    /* Ask libhb to start scanning the specified volume */
    if( ![fScMatrix selectedRow] )
    {
        /* DVD drive */
        HBScanDVD( fHandle,
                   [[fScDetectedPopUp titleOfSelectedItem] cString], 0 );
    }
    else
    {
        /* DVD folder */
        HBScanDVD( fHandle,
                   [[fScFolderField stringValue] cString], 0 );
    }
}

- (IBAction) ShowPicturePanel: (id) sender
{
    HBTitle * title = (HBTitle*)
        HBListItemAt( fTitleList, [fRipTitlePopUp indexOfSelectedItem] );

    [fPictureGLView SetTitle: title];

    fPicture = 0;
    [fPictureGLView ShowPicture: fPicture animate: HB_ANIMATE_NONE];

    [fWidthStepper  setValueWraps: NO];
    [fWidthStepper  setIncrement: 16];
    [fWidthStepper  setMinValue: 16];
    [fWidthStepper  setMaxValue: title->outWidthMax];
    [fWidthStepper  setIntValue: title->outWidth];
    [fWidthField2   setIntValue: title->outWidth];
    [fDeinterlaceCheck setState:
        title->deinterlace ? NSOnState : NSOffState];
    [fTopStepper    setValueWraps: NO];
    [fTopStepper    setIncrement: 2];
    [fTopStepper    setMinValue: 0];
    [fTopStepper    setMaxValue: title->inHeight / 4];
    [fTopStepper    setIntValue: title->topCrop];
    [fTopField2     setIntValue: title->topCrop];
    [fBottomStepper setValueWraps: NO];
    [fBottomStepper setIncrement: 2];
    [fBottomStepper setMinValue: 0];
    [fBottomStepper setMaxValue: title->inHeight / 4];
    [fBottomStepper setIntValue: title->bottomCrop];
    [fBottomField2  setIntValue: title->bottomCrop];
    [fLeftStepper   setValueWraps: NO];
    [fLeftStepper   setIncrement: 2];
    [fLeftStepper   setMinValue: 0];
    [fLeftStepper   setMaxValue: title->inWidth / 4];
    [fLeftStepper   setIntValue: title->leftCrop];
    [fLeftField2    setIntValue: title->leftCrop];
    [fRightStepper  setValueWraps: NO];
    [fRightStepper  setIncrement: 2];
    [fRightStepper  setMinValue: 0];
    [fRightStepper  setMaxValue: title->inWidth / 4];
    [fRightStepper  setIntValue: title->rightCrop];
    [fRightField2   setIntValue: title->rightCrop];

    [fPreviousButton setEnabled: NO];
    [fNextButton     setEnabled: YES];

    [fInfoField setStringValue: [NSString stringWithFormat:
        _( @"Final size: %dx%d" ), title->outWidth, title->outHeight] ];

    /* Resize the panel */
    NSSize newSize;
    newSize.width  = 42 +  MAX( 720, title->outWidthMax );
    newSize.height = 165 + title->outHeightMax;
    [fPicturePanel setContentSize: newSize];

    [NSApp beginSheet: fPicturePanel modalForWindow: fWindow
        modalDelegate: nil didEndSelector: nil contextInfo: nil];
    [NSApp runModalForWindow: fPicturePanel];
    [NSApp endSheet: fPicturePanel];
    [fPicturePanel orderOut: self];
}

- (IBAction) ClosePanel: (id) sender
{
    [NSApp stopModal];
}

- (IBAction) Rip: (id) sender
{
    /* Rip or Cancel ? */
    if( [[fRipRipButton title] compare: _( @"Cancel" ) ]
            == NSOrderedSame )
    {
        [self Cancel: self];
        return;
    }

    if( [fRipCustomField intValue] < 64 )
    {
        NSBeginCriticalAlertSheet( _( @"Invalid video bitrate" ),
            _( @"Ooops" ), nil, nil, fWindow, self, nil, nil, nil,
            _( @"Video bitrate is too low." ) );
        return;
    }
    if( [fRipCustomField intValue] > 8192 )
    {
        NSBeginCriticalAlertSheet( _( @"Invalid video bitrate" ),
            _( @"Ooops" ), nil, nil, fWindow, self, nil, nil, nil,
            _( @"Video bitrate is too high." ) );
        return;
    }
    if( [fRipLang1PopUp indexOfSelectedItem] ==
            [fRipLang2PopUp indexOfSelectedItem] )
    {
        NSBeginCriticalAlertSheet( _( @"Invalid secondary language" ),
            _( @"Ooops" ), nil, nil, fWindow, self, nil, nil, nil,
            _( @"You can't encode the same audio track twice." ) );
        return;
    }

    if( [[NSFileManager defaultManager] fileExistsAtPath:
            [fRipFileField2 stringValue]] )
    {
        NSBeginCriticalAlertSheet( _( @"File already exists" ),
            _( @"No" ), _( @"Yes" ), nil, fWindow, self,
            @selector( OverwriteAlertDone:returnCode:contextInfo: ),
            nil, nil, [NSString stringWithFormat:
            _( @"Do you want to overwrite %@?" ),
            [fRipFileField2 stringValue]] );
        return;
    }

    [self _Rip];
}

- (void) OverwriteAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSAlertAlternateReturn )
    {
        [self _Rip];
    }
}

- (void) _Rip
{
    /* Get the specified title & audio track(s) */
    HBTitle * title = (HBTitle*) HBListItemAt( fTitleList,
            [fRipTitlePopUp indexOfSelectedItem] );
    HBAudio * audio1 = (HBAudio*) HBListItemAt( title->audioList,
            [fRipLang1PopUp indexOfSelectedItem] );
    HBAudio * audio2 = (HBAudio*) HBListItemAt( title->audioList,
            [fRipLang2PopUp indexOfSelectedItem] );

    /* Use user settings */
    title->file    = strdup( [[fRipFileField2 stringValue] cString] );
    title->bitrate = [fRipCustomField intValue];
    title->twoPass = ( [fRipTwoPassCheck state] == NSOnState );

    int format = [fRipFormatPopUp indexOfSelectedItem];
    int codec  = [fRipEncoderPopUp indexOfSelectedItem];
    title->mux = ( !format ) ? HB_MUX_MP4 : ( ( format == 3 ) ?
            HB_MUX_OGM : HB_MUX_AVI );
    title->codec = ( format == 2 ) ? HB_CODEC_X264 : ( ( !codec ) ?
            HB_CODEC_FFMPEG : HB_CODEC_XVID );

    audio1->outBitrate = [[fRipAudBitPopUp titleOfSelectedItem]
                              intValue];
    audio1->outCodec = ( !format ) ? HB_CODEC_AAC : ( ( format == 3 ) ?
            HB_CODEC_VORBIS : HB_CODEC_MP3 );;
    HBListAdd( title->ripAudioList, audio1 );
    if( audio2 )
    {
        audio2->outBitrate = [[fRipAudBitPopUp
            titleOfSelectedItem] intValue];
        audio2->outCodec = ( !format ) ? HB_CODEC_AAC : ( ( format == 3 ) ?
                HB_CODEC_VORBIS : HB_CODEC_MP3 );
        HBListAdd( title->ripAudioList, audio2 );
    }

    /* Disable interface */
    [fRipTitlePopUp   setEnabled: NO];
    [fRipFormatPopUp  setEnabled: NO];
    [fRipVideoMatrix  setEnabled: NO];
    [fRipCustomField  setEnabled: NO];
    [fRipTargetField  setEnabled: NO];
    [fRipTwoPassCheck setEnabled: NO];
    [fRipCropButton   setEnabled: NO];
    [fRipLang1PopUp   setEnabled: NO];
    [fRipLang2PopUp   setEnabled: NO];
    [fRipAudBitPopUp  setEnabled: NO];
    [fRipFileField2   setEnabled: NO];
    [fRipEncoderPopUp setEnabled: NO];
    [fRipBrowseButton setEnabled: NO];
    [fRipPauseButton  setEnabled: YES];
    [fRipRipButton    setTitle: _( @"Cancel" )];
    [fRipProgress     setIndeterminate: YES];
    [fRipProgress     startAnimation: self];;

    /* Let libhb do the job */
    HBStartRip( fHandle, title );
}

- (IBAction) Cancel: (id) sender
{
    NSBeginCriticalAlertSheet( _( @"Cancel - Are you sure?" ),
        _( @"No" ), _( @"Yes" ), nil, fWindow, self,
        @selector( _Cancel:returnCode:contextInfo: ), nil, nil,
        _( @"Encoding won't be recoverable." ) );
}

- (void) _Cancel: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSAlertAlternateReturn )
    {
        if( [[fRipPauseButton title] compare: _( @"Resume" ) ]
                == NSOrderedSame )
        {
            HBResumeRip( fHandle );
        }
        HBStopRip( fHandle );
    }
}

- (IBAction) Pause: (id) sender
{
    if( [[fRipPauseButton title] compare: _( @"Resume" ) ]
            == NSOrderedSame )
    {
        [self Resume: self];
        return;
    }

    [fRipPauseButton setTitle: _( @"Resume" )];
    HBPauseRip( fHandle );
}

- (IBAction) Resume: (id) sender
{
    [fRipPauseButton setTitle: _( @"Pause" )];
    HBResumeRip( fHandle );
}

- (IBAction) PreviousPicture: (id) sender
{
    fPicture--;
    if( [fOpenGLCheck state] == NSOnState )
    {
        [fPictureGLView ShowPicture: fPicture
            animate: HB_ANIMATE_LEFT];
    }
    else
    {
        [fPictureGLView ShowPicture: fPicture
            animate: HB_ANIMATE_NONE];
    }

    [fPreviousButton setEnabled: ( fPicture > 0 )];
    [fNextButton     setEnabled: YES];
}

- (IBAction) NextPicture: (id) sender
{
    fPicture++;
    if( [fOpenGLCheck state] == NSOnState )
    {
        [fPictureGLView ShowPicture: fPicture
            animate: HB_ANIMATE_RIGHT];
    }
    else
    {
        [fPictureGLView ShowPicture: fPicture
            animate: HB_ANIMATE_NONE];
    }

    [fPreviousButton setEnabled: YES];
    [fNextButton     setEnabled: ( fPicture < 9 )];
}

- (IBAction) UpdatePicture: (id) sender
{
    HBTitle * title = (HBTitle*)
        HBListItemAt( fTitleList, [fRipTitlePopUp indexOfSelectedItem] );
    title->outWidth    = [fWidthStepper intValue];
    title->deinterlace = ( [fDeinterlaceCheck state] == NSOnState );
    title->topCrop     = [fTopStepper intValue];
    title->bottomCrop  = [fBottomStepper intValue];
    title->leftCrop    = [fLeftStepper intValue];
    title->rightCrop   = [fRightStepper intValue];

    [fPictureGLView ShowPicture: fPicture animate: HB_ANIMATE_NONE];

    [fWidthStepper  setIntValue: title->outWidth];
    [fTopStepper    setIntValue: title->topCrop];
    [fBottomStepper setIntValue: title->bottomCrop];
    [fLeftStepper   setIntValue: title->leftCrop];
    [fRightStepper  setIntValue: title->rightCrop];
    [fWidthField2   setIntValue: [fWidthStepper intValue]];
    [fTopField2     setIntValue: [fTopStepper intValue]];
    [fBottomField2  setIntValue: [fBottomStepper intValue]];
    [fLeftField2    setIntValue: [fLeftStepper intValue]];
    [fRightField2   setIntValue: [fRightStepper intValue]];

    [fInfoField setStringValue: [NSString stringWithFormat:
        _( @"Final size: %dx%d" ), title->outWidth, title->outHeight]];
}

- (IBAction) AutoCrop: (id) sender
{
    HBTitle * title = (HBTitle*)
        HBListItemAt( fTitleList, [fRipTitlePopUp indexOfSelectedItem] );
    title->topCrop     = title->autoTopCrop;
    title->bottomCrop  = title->autoBottomCrop;
    title->leftCrop    = title->autoLeftCrop;
    title->rightCrop   = title->autoRightCrop;

    [fPictureGLView ShowPicture: fPicture animate: HB_ANIMATE_NONE];

    [fWidthStepper  setIntValue: title->outWidth];
    [fTopStepper    setIntValue: title->topCrop];
    [fBottomStepper setIntValue: title->bottomCrop];
    [fLeftStepper   setIntValue: title->leftCrop];
    [fRightStepper  setIntValue: title->rightCrop];
    [fWidthField2   setIntValue: [fWidthStepper intValue]];
    [fTopField2     setIntValue: [fTopStepper intValue]];
    [fBottomField2  setIntValue: [fBottomStepper intValue]];
    [fLeftField2    setIntValue: [fLeftStepper intValue]];
    [fRightField2   setIntValue: [fRightStepper intValue]];

    [fInfoField setStringValue: [NSString stringWithFormat:
        _( @"Final size: %dx%d" ), title->outWidth, title->outHeight]];
}

- (void) DetectDrives: (NSNotification *) notification
{
    /* Scan DVD drives (stolen from VLC) */
    io_object_t next_media;
    mach_port_t master_port;
    kern_return_t kern_result;
    io_iterator_t media_iterator;
    CFMutableDictionaryRef classes_to_match;

    kern_result = IOMasterPort( MACH_PORT_NULL, &master_port );
    if( kern_result != KERN_SUCCESS )
    {
        return;
    }

    classes_to_match = IOServiceMatching( kIODVDMediaClass );
    if( classes_to_match == NULL )
    {
        return;
    }

    CFDictionarySetValue( classes_to_match, CFSTR( kIOMediaEjectableKey ),
                          kCFBooleanTrue );

    kern_result =
        IOServiceGetMatchingServices( master_port, classes_to_match,
                                      &media_iterator );
    if( kern_result != KERN_SUCCESS )
    {
        return;
    }

    NSMutableArray * drivesList;
    drivesList = [NSMutableArray arrayWithCapacity: 1];

    next_media = IOIteratorNext( media_iterator );
    if( next_media != NULL )
    {
        char psz_buf[0x32];
        size_t dev_path_length;
        CFTypeRef str_bsd_path;
        do
        {
            str_bsd_path =
                IORegistryEntryCreateCFProperty( next_media,
                                                 CFSTR( kIOBSDNameKey ),
                                                 kCFAllocatorDefault,
                                                 0 );
            if( str_bsd_path == NULL )
            {
                IOObjectRelease( next_media );
                continue;
            }

            snprintf( psz_buf, sizeof(psz_buf), "%s%c", _PATH_DEV, 'r' );
            dev_path_length = strlen( psz_buf );

            if( CFStringGetCString( (CFStringRef) str_bsd_path,
                                    (char*)&psz_buf + dev_path_length,
                                    sizeof(psz_buf) - dev_path_length,
                                    kCFStringEncodingASCII ) )
            {
                [drivesList addObject:
                    [NSString stringWithCString: psz_buf]];
            }

            CFRelease( str_bsd_path );

            IOObjectRelease( next_media );

        } while( ( next_media = IOIteratorNext( media_iterator ) ) != NULL );
    }

    IOObjectRelease( media_iterator );

    [fScDetectedPopUp removeAllItems];
    for( unsigned i = 0; i < [drivesList count]; i++ )
    {
        [[fScDetectedPopUp menu] addItemWithTitle:
            [drivesList objectAtIndex: i] action: nil
            keyEquivalent: @""];
    }
    [self ScanMatrixChanged: self];
}

- (IBAction) ScanMatrixChanged: (id) sender
{
    if( ![fScMatrix selectedRow] )
    {
        [fScDetectedPopUp setEnabled: YES];
        [fScFolderField setEnabled: NO];
        [fScBrowseButton setEnabled: NO];
        [fScOpenButton setEnabled: ( [fScDetectedPopUp selectedItem] != nil )];
    }
    else
    {
        [fScDetectedPopUp setEnabled: NO];
        [fScFolderField setEnabled: YES];
        [fScBrowseButton setEnabled: YES];
        [fScOpenButton setEnabled: YES];
    }
}

- (IBAction) TitlePopUpChanged: (id) sender
{
    HBTitle * title = (HBTitle*)
        HBListItemAt( fTitleList, [fRipTitlePopUp indexOfSelectedItem] );

    [fRipLang1PopUp removeAllItems];
    [fRipLang2PopUp removeAllItems];

    HBAudio * audio;
    for( int i = 0; i < HBListCount( title->audioList ); i++ )
    {
        audio = (HBAudio*) HBListItemAt( title->audioList, i );

        /* We cannot use NSPopUpButton's addItemWithTitle because
           it checks for duplicate entries */
        [[fRipLang1PopUp menu] addItemWithTitle:
            [NSString stringWithCString: audio->language]
            action: nil keyEquivalent: @""];
        [[fRipLang2PopUp menu] addItemWithTitle:
            [NSString stringWithCString: audio->language]
            action: nil keyEquivalent: @""];
    }
    [fRipLang2PopUp addItemWithTitle: _( @"None" )];
    [fRipLang2PopUp selectItemWithTitle: _( @"None" )];
    [fRipLang2PopUp setEnabled:
        ( HBListCount( title->audioList ) > 1 )];

    [fRipTargetField SetHBTitle: title];
    if( [fRipVideoMatrix selectedRow] )
    {
        [fRipTargetField UpdateBitrate];
    }
}

- (IBAction) FormatPopUpChanged: (id) sender
{
    /* Headers size changes depending on the format, so let's
       recalculate the bitrate if necessary */
    if( [fRipVideoMatrix selectedRow] )
    {
        [fRipTargetField UpdateBitrate];
    }

    /* Add/replace to the correct extension */
    NSString * string = [fRipFileField2 stringValue];
    int format = [fRipFormatPopUp indexOfSelectedItem];
    if( [string characterAtIndex: [string length] - 4] == '.' )
    {
        [fRipFileField2 setStringValue: [NSString stringWithFormat:
            @"%@.%s", [string substringToIndex: [string length] - 4],
            ( !format ) ? "mp4" : ( ( format == 3 ) ?
            "ogm" : "avi" )]];
    }
    else
    {
        [fRipFileField2 setStringValue: [NSString stringWithFormat:
            @"%@.%s", string, ( !format ) ? "mp4" :
            ( ( format == 3 ) ? "ogm" : "avi" )]];
    }

    if( format == 2 )
    {
        /* Can't set X264 bitrate */
        [fRipEncoderPopUp setEnabled: NO];
        [fRipVideoMatrix  setEnabled: NO];
        [fRipTwoPassCheck setEnabled: NO];
    }
    else if( format == 3 )
    {
        [fRipEncoderPopUp setEnabled: YES];
        [fRipVideoMatrix  setEnabled: YES];
        [fRipTwoPassCheck setEnabled: YES];
    }
    else
    {
        [fRipEncoderPopUp setEnabled: YES];
        [fRipVideoMatrix  setEnabled: YES];
        [fRipTwoPassCheck setEnabled: YES];
    }
    [self VideoMatrixChanged: self];
}

- (IBAction) AudioPopUpChanged: (id) sender
{
    /* Recalculate the bitrate */
    if( [fRipVideoMatrix selectedRow] )
    {
        [fRipTargetField UpdateBitrate];
    }
}

/*******************
 * libhb callbacks *
 *******************/
static void _Scanning( void * data, int title, int titleCount )
{
    HBController * controller = (HBController*) data;
    controller->fTitle        = title;
    controller->fTitleCount   = titleCount;
    [controller performSelectorOnMainThread: @selector(Scanning:)
        withObject: nil waitUntilDone: YES];
}
- (void) Scanning: (id) sender
{
    [fScProgress stopAnimation: self];
    [fScProgress setIndeterminate: NO];
    [fScProgress setDoubleValue: 100.0 * fTitle / fTitleCount];

    [fScStatusField setStringValue: [NSString stringWithFormat:
        _( @"Scanning title %d of %d..." ), fTitle, fTitleCount]];
}

static void _ScanDone( void * data, HBList * titleList )
{
    HBController * controller = (HBController*) data;
    controller->fTitleList    = titleList;
    [controller performSelectorOnMainThread: @selector(ScanDone:)
        withObject: nil waitUntilDone: YES];
}
- (void) ScanDone: (id) sender
{
    if( !fTitleList )
    {
        [fScMatrix setEnabled: YES];
        [self ScanMatrixChanged: self];
        [fScProgress stopAnimation: self];
        [fScProgress setIndeterminate: NO];
        [fScOpenButton setEnabled: YES];
        [fScStatusField setStringValue:
            _( @"Invalid volume, try again" ) ];
        return;
    }

    /* Show a temporary empty view while the window
       resizing animation */
    [fWindow setContentView: fTempView ];

    /* Actually resize it */
    NSRect newFrame;
    newFrame = [NSWindow contentRectForFrameRect: [fWindow frame]
                 styleMask: [fWindow styleMask]];
    newFrame.origin.y    += newFrame.size.height -
                                [fRipView frame].size.height;
    newFrame.size.height  = [fRipView frame].size.height;
    newFrame.size.width   = [fRipView frame].size.width;
    newFrame = [NSWindow frameRectForContentRect: newFrame
                 styleMask: [fWindow styleMask]];
    [fWindow setFrame: newFrame display: YES animate: YES];

    /* Show the new GUI */
    [fWindow setContentView: fRipView ];
    [fRipPauseButton setEnabled: NO];

    [fRipTitlePopUp removeAllItems];
    HBTitle * title;
    for( int i = 0; i < HBListCount( fTitleList ); i++ )
    {
        title = (HBTitle*) HBListItemAt( fTitleList, i );
        [[fRipTitlePopUp menu] addItemWithTitle:
            [NSString stringWithFormat: @"%d - %02dh%02dm%02ds",
            title->index, title->length / 3600, ( title->length % 3600 )
            / 60, title->length % 60] action: nil keyEquivalent: @""];
    }
    [self TitlePopUpChanged: self];
}

static void _Encoding( void * data, float position, int pass,
                      int passCount, float curFrameRate,
                      float avgFrameRate, int remainingTime )
{
    HBController * controller  = (HBController*) data;
    controller->fPosition      = position;
    controller->fPass          = pass;
    controller->fPassCount     = passCount;
    controller->fCurFrameRate  = curFrameRate;
    controller->fAvgFrameRate  = avgFrameRate;
    controller->fRemainingTime = remainingTime;
    [controller performSelectorOnMainThread: @selector(Encoding:)
        withObject: nil waitUntilDone: YES];
}
- (void) Encoding: (id) sender
{
    [fRipStatusField setStringValue: [NSString stringWithFormat:
        _( @"Encoding: %.2f %% (pass %d of %d)" ),
        100.0 * fPosition, fPass, fPassCount]];
    [fRipInfoField setStringValue: [NSString stringWithFormat:
        _( @"Speed: %.2f fps (avg %.2f fps), %02dh%02dm%02ds remaining" ),
        fCurFrameRate, fAvgFrameRate, fRemainingTime / 3600,
        ( fRemainingTime / 60 ) % 60, fRemainingTime % 60]];

    [fRipProgress setIndeterminate: NO];
    [fRipProgress setDoubleValue: 100.0 * fPosition];
}

static void _RipDone( void * data, int result )
{
    HBController * controller = (HBController*) data;
    controller->fResult       = result;
    [controller performSelectorOnMainThread: @selector(RipDone:)
        withObject: nil waitUntilDone: YES];
}
- (void) RipDone: (id) sender
{
    [fRipTitlePopUp   setEnabled: YES];
    [fRipFormatPopUp  setEnabled: YES];
    [fRipVideoMatrix  setEnabled: YES];
    [fRipTwoPassCheck setEnabled: YES];
    [fRipCropButton   setEnabled: YES];
    [fRipLang1PopUp   setEnabled: YES];
    [fRipLang2PopUp   setEnabled: YES];
    [fRipAudBitPopUp  setEnabled: YES];
    [fRipFileField2   setEnabled: YES];
    [fRipBrowseButton setEnabled: YES];
    [fRipEncoderPopUp setEnabled: YES];
    [fRipPauseButton  setEnabled: NO];
    [fRipPauseButton  setTitle: _( @"Pause" )];
    [fRipRipButton    setTitle: _( @"Rip" )];
    [fRipProgress     setIndeterminate: NO];
    [fRipProgress     setDoubleValue: 0.0];
    [self VideoMatrixChanged: self];

    switch( fResult )
    {
        case HB_SUCCESS:
            [fRipStatusField setStringValue: _( @"Rip completed." )];
            [fRipInfoField   setStringValue: @""];
            NSBeep();
            [NSApp requestUserAttention: NSInformationalRequest];
            [NSApp beginSheet: fDonePanel
                modalForWindow: fWindow modalDelegate: nil
                didEndSelector: nil contextInfo: nil];
            [NSApp runModalForWindow: fDonePanel];
            [NSApp endSheet: fDonePanel];
            [fDonePanel orderOut: self];
            break;
        case HB_CANCELED:
            [fRipStatusField setStringValue: _( @"Canceled." )];
            [fRipInfoField   setStringValue: @""];
            break;
        case HB_ERROR_A52_SYNC:
            [fRipStatusField setStringValue: @"Error."];
            [fRipInfoField   setStringValue: @"Corrupted AC3 data"];
            break;
        case HB_ERROR_AVI_WRITE:
            [fRipStatusField setStringValue: @"Error."];
            [fRipInfoField   setStringValue: @"Write error"];
            break;
        case HB_ERROR_DVD_OPEN:
            [fRipStatusField setStringValue: @"Error."];
            [fRipInfoField   setStringValue: @"Could not open the DVD"];
            break;
        case HB_ERROR_DVD_READ:
            [fRipStatusField setStringValue: @"Error."];
            [fRipInfoField   setStringValue: @"DVD read error"];
            break;
        case HB_ERROR_MP3_INIT:
            [fRipStatusField setStringValue: @"Error."];
            [fRipInfoField   setStringValue:
                @"MP3 encoder initialization failed"];
            break;
        case HB_ERROR_MP3_ENCODE:
            [fRipStatusField setStringValue: @"Error."];
            [fRipInfoField   setStringValue: @"MP3 encoder failed"];
            break;
        case HB_ERROR_MPEG4_INIT:
            [fRipStatusField setStringValue: @"Error."];
            [fRipInfoField   setStringValue:
                @"MPEG4 encoder initialization failed"];
            break;
        default:
            [fRipStatusField setStringValue: @"Error."];
            [fRipInfoField   setStringValue: @"Unknown error"];
    }
}

@end
