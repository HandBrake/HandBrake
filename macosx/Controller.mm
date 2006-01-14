/* $Id: Controller.mm,v 1.9 2003/11/09 22:06:15 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <paths.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>

#include "Controller.h"

@implementation HBController

- (void) applicationDidFinishLaunching: (NSNotification *) notification
{
    /* Init libhb */
    fHandle = HBInit( 1, 0 );
    [fPictureGLView SetHandle: fHandle];

    /* Update the GUI every 1/10 sec */
    fDie = false;
    [NSTimer scheduledTimerWithTimeInterval: 0.1
        target: self selector: @selector( UpdateIntf: )
        userInfo: nil repeats: YES];

    /* Detect drives mounted after the app is started */
    [[[NSWorkspace sharedWorkspace] notificationCenter]
        addObserver: self selector: @selector( DetectDrives: )
        name: NSWorkspaceDidMountNotification object: nil];
}

- (NSApplicationTerminateReply) applicationShouldTerminate:
    (NSApplication *) app
{
    /* Clean up */
    fDie = true;
    HBClose( &fHandle );

    return NSTerminateNow;
}

- (void) awakeFromNib
{
    [fDVDPopUp removeAllItems];
    [fScanProgress setStyle: NSProgressIndicatorSpinningStyle];
    [fScanProgress setDisplayedWhenStopped: NO];
    [fVideoCodecPopUp removeAllItems];
    [fVideoCodecPopUp addItemWithTitle: @"MPEG-4 (Ffmpeg)"];
    [fVideoCodecPopUp addItemWithTitle: @"MPEG-4 (XviD)"];
    [fVideoCodecPopUp selectItemWithTitle: @"MPEG-4 (Ffmpeg)"];
    [fAudioBitratePopUp removeAllItems];
    [fAudioBitratePopUp addItemWithTitle: @"32"];
    [fAudioBitratePopUp addItemWithTitle: @"64"];
    [fAudioBitratePopUp addItemWithTitle: @"96"];
    [fAudioBitratePopUp addItemWithTitle: @"128"];
    [fAudioBitratePopUp addItemWithTitle: @"160"];
    [fAudioBitratePopUp addItemWithTitle: @"192"];
    [fAudioBitratePopUp addItemWithTitle: @"224"];
    [fAudioBitratePopUp addItemWithTitle: @"256"];
    [fAudioBitratePopUp addItemWithTitle: @"288"];
    [fAudioBitratePopUp addItemWithTitle: @"320"];
    [fAudioBitratePopUp selectItemWithTitle: @"128"];

    char string[1024]; memset( string, 0, 1024 );
    snprintf( string, 1024, "%s/Desktop/Movie.avi", getenv( "HOME" ) );
    [fFileField setStringValue: [NSString stringWithCString: string]];

    /* Show the scan view */
    [fWindow setContentSize: [fScanView frame].size];
    [fWindow setContentView: fScanView];
    [fWindow center];

    /* Detect DVD drives */
    [self DetectDrives: nil];
    [self ScanMatrixChanged: self];
}

- (BOOL) windowShouldClose: (id) sender
{
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
        [fDVDFolderField setStringValue:
            [[sheet filenames] objectAtIndex: 0]];
    }
}

- (IBAction) VideoMatrixChanged: (id) sender;
{
    if( ![fVideoMatrix selectedRow] )
    {
        [fCustomBitrateField setEnabled: YES];
        [fTargetSizeField setEnabled: NO];
    }
    else
    {
        [fCustomBitrateField setEnabled: NO];
        [fTargetSizeField setEnabled: YES];
        [fTargetSizeField UpdateBitrate];
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
        [fFileField setStringValue: [sheet filename]];
    }
}

- (IBAction) Scan: (id) sender
{
    /* Ask libhb to start scanning the specified volume */
    
    if( ![fScanMatrix selectedRow] )
    {
        /* DVD drive */
        HBScanDevice( fHandle,
                      (char*) [[fDVDPopUp titleOfSelectedItem] cString],
                      0 );
    }
    else
    {
        /* DVD folder */
        HBScanDevice( fHandle,
                      (char*) [[fDVDFolderField stringValue] cString],
                      0 );
    }
}

- (IBAction) ShowPicturePanel: (id) sender
{
    HBTitle * title = (HBTitle*)
        HBListItemAt( fTitleList, [fTitlePopUp indexOfSelectedItem] );
    
    [fPictureGLView SetTitle: title];

    fPicture = 0;
    [fPictureGLView ShowPicture: fPicture animate: HB_ANIMATE_NONE];

    [fWidthStepper  setValueWraps: NO];
    [fWidthStepper  setIncrement: 16];
    [fWidthStepper  setMinValue: 16];
    [fWidthStepper  setMaxValue: title->outWidthMax];
    [fWidthStepper  setIntValue: title->outWidth];
    [fWidthField    setIntValue: title->outWidth];
    [fDeinterlaceCheck setState:
        title->deinterlace ? NSOnState : NSOffState];
    [fTopStepper    setValueWraps: NO];
    [fTopStepper    setIncrement: 2];
    [fTopStepper    setMinValue: 0];
    [fTopStepper    setMaxValue: title->inHeight / 4];
    [fTopStepper    setIntValue: title->topCrop];
    [fTopField      setIntValue: title->topCrop];
    [fBottomStepper setValueWraps: NO];
    [fBottomStepper setIncrement: 2];
    [fBottomStepper setMinValue: 0];
    [fBottomStepper setMaxValue: title->inHeight / 4];
    [fBottomStepper setIntValue: title->bottomCrop];
    [fBottomField   setIntValue: title->bottomCrop];
    [fLeftStepper   setValueWraps: NO];
    [fLeftStepper   setIncrement: 2];
    [fLeftStepper   setMinValue: 0];
    [fLeftStepper   setMaxValue: title->inWidth / 4];
    [fLeftStepper   setIntValue: title->leftCrop];
    [fLeftField     setIntValue: title->leftCrop];
    [fRightStepper  setValueWraps: NO];
    [fRightStepper  setIncrement: 2];
    [fRightStepper  setMinValue: 0];
    [fRightStepper  setMaxValue: title->inWidth / 4];
    [fRightStepper  setIntValue: title->rightCrop];
    [fRightField    setIntValue: title->rightCrop];

    [fPreviousButton setEnabled: NO];
    [fNextButton     setEnabled: YES];

    char string[1024]; memset( string, 0, 1024 );
    sprintf( string, "Final size: %dx%d",
             title->outWidth, title->outHeight );
    [fInfoField setStringValue: [NSString stringWithCString: string]];

    /* Resize the panel */
    NSSize newSize;
    /* XXX */
    newSize.width  = 762 /*fPicturePanelSize.width*/ +
        title->outWidthMax - 720;
    newSize.height = 740 /*fPicturePanelSize.height*/ +
        title->outHeightMax - 576;
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
    if( [[fRipButton title] compare: @"Cancel" ] == NSOrderedSame )
    {
        [self Cancel: self];
        return;
    }
    
    if( [fCustomBitrateField intValue] < 256 )
    {
        NSBeginCriticalAlertSheet( @"Invalid video bitrate", @"Ooops",
            nil, nil, fWindow, self, nil, nil, nil,
            @"Video bitrate is too low !" );
        return;
    }
    if( [fCustomBitrateField intValue] > 8192 )
    {
        NSBeginCriticalAlertSheet( @"Invalid video bitrate", @"Ooops",
            nil, nil, fWindow, self, nil, nil, nil,
            @"Video bitrate is too high !" );
        return;
    }
    if( [fLanguagePopUp indexOfSelectedItem] ==
            [fSecondaryLanguagePopUp indexOfSelectedItem] )
    {
        NSBeginCriticalAlertSheet( @"Invalid secondary language",
            @"Ooops", nil, nil, fWindow, self, nil, nil, nil,
            @"Do you _really_ want to encode the same audio track twice?" );
        return;
    }

    FILE * file;
    if( ( file = fopen( [[fFileField stringValue] cString], "r" ) ) )
    {
        fclose( file );
        NSBeginCriticalAlertSheet( @"File already exists",
            @"Nooo!", @"Yes, go ahead", nil, fWindow, self,
            @selector( OverwriteAlertDone:returnCode:contextInfo: ),
            nil, nil,
            [NSString stringWithFormat: @"Do you want to overwrite %s?",
                [[fFileField stringValue] cString]] );
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
    HBTitle * title = (HBTitle*)
        HBListItemAt( fTitleList, [fTitlePopUp indexOfSelectedItem] );
    HBAudio * audio1 = (HBAudio*)
        HBListItemAt( title->audioList,
                      [fLanguagePopUp indexOfSelectedItem] );
    HBAudio * audio2 = (HBAudio*)
        HBListItemAt( title->audioList,
                      [fSecondaryLanguagePopUp indexOfSelectedItem] );

    /* Use user settings */
    title->file    = strdup( [[fFileField stringValue] cString] );
    title->bitrate = [fCustomBitrateField intValue];
    title->twoPass = ( [fTwoPassCheck state] == NSOnState );
    title->codec   = ( [[fVideoCodecPopUp titleOfSelectedItem] compare:
                         @"MPEG-4 (Ffmpeg)"] == NSOrderedSame ) ?
                         HB_CODEC_FFMPEG : HB_CODEC_XVID;
    audio1->outBitrate = [[fAudioBitratePopUp titleOfSelectedItem]
                              intValue];
    if( audio2 )
    {
        audio2->outBitrate =
            [[fAudioBitratePopUp titleOfSelectedItem] intValue];
    }

    /* Let libhb do the job */
    HBStartRip( fHandle, title, audio1, audio2 );
}

- (IBAction) Cancel: (id) sender
{
    HBStopRip( fHandle );
}

- (IBAction) Suspend: (id) sender
{
    if( [[fSuspendButton title] compare: @"Resume" ] == NSOrderedSame )
    {
        [self Resume: self];
        return;
    }

    HBPauseRip( fHandle );
}

- (IBAction) Resume: (id) sender
{
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
        HBListItemAt( fTitleList, [fTitlePopUp indexOfSelectedItem] );
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
    [fWidthField    setIntValue: [fWidthStepper intValue]];
    [fTopField      setIntValue: [fTopStepper intValue]];
    [fBottomField   setIntValue: [fBottomStepper intValue]];
    [fLeftField     setIntValue: [fLeftStepper intValue]];
    [fRightField    setIntValue: [fRightStepper intValue]];

    char string[1024]; memset( string, 0, 1024 );
    sprintf( string, "Final size: %dx%d",
             title->outWidth, title->outHeight );
    [fInfoField setStringValue: [NSString stringWithCString: string]];
}

- (void) UpdateIntf: (NSTimer *) timer
{
    if( fDie )
    {
        [timer invalidate];
        return;
    }

    int      modeChanged;
    HBStatus status;

    modeChanged = HBGetStatus( fHandle, &status );

    switch( status.mode )
    {
        case HB_MODE_NEED_DEVICE:
            break;

        case HB_MODE_SCANNING:
        {
            if( modeChanged )
            {
                [fScanMatrix setEnabled: NO];
                [fDVDPopUp setEnabled: NO];
                [fDVDFolderField setEnabled: NO];
                [fScanBrowseButton setEnabled: NO];
                [fScanProgress startAnimation: self];
                [fScanButton setEnabled: NO];
            }

            char string[1024]; memset( string, 0, 1024 );
            if( status.scannedTitle )
            {
                sprintf( string, "Scanning title %d...",
                         status.scannedTitle );
            }
            else
            {
                sprintf( string, "Opening device..." );
            }
            [fScanStatusField setStringValue:
                [NSString stringWithCString: string]];

            break;
        }

        case HB_MODE_INVALID_DEVICE:
        {
            if( !modeChanged )
                break;
            
            [fScanMatrix setEnabled: YES];
            [self ScanMatrixChanged: self];
            [fScanProgress stopAnimation: self];
            [fScanButton setEnabled: YES];

            [fScanStatusField setStringValue:
                @"Invalid volume, try again" ];
            break;
        }

        case HB_MODE_READY_TO_RIP:
        {
            if( !modeChanged )
                break;
            
            fTitleList = status.titleList;
            
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
            [fSuspendButton setEnabled: NO];
            
            [fTitlePopUp removeAllItems];
            HBTitle * title;
            for( int i = 0; i < HBListCountItems( fTitleList ); i++ )
            {
                title = (HBTitle*) HBListItemAt( fTitleList, i );
                char string[1024]; memset( string, 0, 1024 );
                sprintf( string, "%d - %02dh%02dm%02ds",
                         title->index, title->length / 3600,
                         ( title->length % 3600 ) / 60,
                         title->length % 60 );
                [[fTitlePopUp menu] addItemWithTitle:
                    [NSString stringWithCString: string]
                    action: nil keyEquivalent: @""];
            }
            [self TitlePopUpChanged: self];
            
            break;
        }

        case HB_MODE_ENCODING:
        {
            if( modeChanged )
            {
                [fTitlePopUp             setEnabled: NO];
                [fVideoCodecPopUp        setEnabled: NO];
                [fVideoMatrix            setEnabled: NO];
                [fCustomBitrateField     setEnabled: NO];
                [fTargetSizeField        setEnabled: NO];
                [fTwoPassCheck           setEnabled: NO];
                [fCropButton             setEnabled: NO];
                [fLanguagePopUp          setEnabled: NO];
                [fSecondaryLanguagePopUp setEnabled: NO];
                [fAudioCodecPopUp        setEnabled: NO];
                [fAudioBitratePopUp      setEnabled: NO];
                [fFileFormatPopUp        setEnabled: NO];
                [fFileField              setEnabled: NO];
                [fFileBrowseButton       setEnabled: NO];
                [fSuspendButton          setEnabled: YES];
                [fSuspendButton          setTitle: @"Suspend"];
                [fRipButton              setTitle: @"Cancel"];
            }
        
            if( !status.position )
            {
                [fRipStatusField setStringValue: @"Starting..."];
                [fRipProgress setIndeterminate: YES];
                [fRipProgress startAnimation: self];;
            }
            else
            {
                char string[1024];
                memset( string, 0, 1024 );
                sprintf( string, "Encoding: %.2f %% (pass %d of %d)",
                         100 * status.position, status.pass,
                         status.passCount );
                [fRipStatusField setStringValue:
                    [NSString stringWithCString: string]];
                memset( string, 0, 1024 );
                sprintf( string, "Speed: %.2f fps (avg %.2f fps, "
                         "%02dh%02dm%02ds remaining)",
                         status.frameRate, status.avFrameRate,
                         status.remainingTime / 3600,
                         ( status.remainingTime / 60 ) % 60,
                         status.remainingTime % 60 );
                [fRipInfoField setStringValue:
                    [NSString stringWithCString: string]];

                [fRipProgress setIndeterminate: NO];
                [fRipProgress setDoubleValue: 100 * status.position];
            }
            
            break;
        }

        case HB_MODE_PAUSED:
        {
            if( !modeChanged )
                break;
            
            char string[1024]; memset( string, 0, 1024 );
            sprintf( string, "Encoding: %.2f %% (PAUSED)",
                     100 * status.position ) ;
            [fRipStatusField setStringValue:
                [NSString stringWithCString: string]];
            [fRipInfoField setStringValue: @""];
            
            [fRipProgress setDoubleValue: 100 * status.position];

            [fSuspendButton setTitle: @"Resume"];
            break;
        }

        case HB_MODE_STOPPING:
            if( !modeChanged )
                break;

            [fRipStatusField setStringValue: @"Stopping..."];
            [fRipInfoField setStringValue: @""];
            [fRipProgress setIndeterminate: YES];
            [fRipProgress startAnimation: self];;
            break;

        case HB_MODE_DONE:
        case HB_MODE_CANCELED:
        case HB_MODE_ERROR:
            if( !modeChanged )
                break;

            /* Warn the finder to update itself */
            [[NSWorkspace sharedWorkspace] noteFileSystemChanged:
                [fFileField stringValue]];
            
            [fRipProgress setIndeterminate: NO];
            [fRipInfoField setStringValue: @""];

            if( status.mode == HB_MODE_DONE )
            {
                [fRipProgress setDoubleValue: 100];
                [fRipStatusField setStringValue: @"Done." ];
                NSBeep();
                [NSApp requestUserAttention: NSInformationalRequest];
                [NSApp beginSheet: fDonePanel
                    modalForWindow: fWindow modalDelegate: nil
                    didEndSelector: nil contextInfo: nil];
                [NSApp runModalForWindow: fDonePanel];
                [NSApp endSheet: fDonePanel];
                [fDonePanel orderOut: self];
            }
            else if( status.mode == HB_MODE_CANCELED )
            {
                [fRipProgress setDoubleValue: 0];
                [fRipStatusField setStringValue: @"Canceled." ];
            }
            else
            {
                [fRipProgress setDoubleValue: 0];
                switch( status.error )
                {
                    case HB_ERROR_A52_SYNC:
                        [fRipStatusField setStringValue:
                        @"An error occured (corrupted AC3 data)." ];
                        break;
                    case HB_ERROR_AVI_WRITE:
                        [fRipStatusField setStringValue:
                        @"An error occured (could not write to file)." ];
                        break;
                    case HB_ERROR_DVD_OPEN:
                        [fRipStatusField setStringValue:
                        @"An error occured (could not open device)." ];
                        break;
                    case HB_ERROR_DVD_READ:
                        [fRipStatusField setStringValue:
                        @"An error occured (DVD read failed)." ];
                        break;
                    case HB_ERROR_MP3_INIT:
                        [fRipStatusField setStringValue:
                        @"An error occured (could not init MP3 encoder)." ];
                        break;
                    case HB_ERROR_MP3_ENCODE:
                        [fRipStatusField setStringValue:
                        @"An error occured (MP3 encoder failed)." ];
                        break;
                    case HB_ERROR_MPEG4_INIT:
                        [fRipStatusField setStringValue:
                        @"An error occured (could not init MPEG4 encoder)." ];
                        break;
                }
            }

            [fTitlePopUp             setEnabled: YES];
            [fVideoCodecPopUp        setEnabled: YES];
            [fVideoMatrix            setEnabled: YES];
            [fTwoPassCheck           setEnabled: YES];
            [fCropButton             setEnabled: YES];
            [fLanguagePopUp          setEnabled: YES];
            [fSecondaryLanguagePopUp setEnabled: YES];
            [fAudioCodecPopUp        setEnabled: YES];
            [fAudioBitratePopUp      setEnabled: YES];
            [fFileFormatPopUp        setEnabled: YES];
            [fFileField              setEnabled: YES];
            [fFileBrowseButton       setEnabled: YES];
            [fSuspendButton          setEnabled: NO];
            [fSuspendButton          setTitle: @"Suspend"];
            [fRipButton              setTitle: @"Rip"];

            [self VideoMatrixChanged: self];

            break;

        default:
            break;
    }
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

    [fDVDPopUp removeAllItems];
    for( unsigned i = 0; i < [drivesList count]; i++ )
    {
        [[fDVDPopUp menu] addItemWithTitle:
            [drivesList objectAtIndex: i] action: nil
            keyEquivalent: @""];
    }
    [self ScanMatrixChanged: self];
}

- (IBAction) ScanMatrixChanged: (id) sender
{
    if( ![fScanMatrix selectedRow] )
    {
        [fDVDPopUp setEnabled: YES];
        [fDVDFolderField setEnabled: NO];
        [fScanBrowseButton setEnabled: NO];
        [fScanButton setEnabled: ( [fDVDPopUp selectedItem] != nil )];
    }
    else
    {
        [fDVDPopUp setEnabled: NO];
        [fDVDFolderField setEnabled: YES];
        [fScanBrowseButton setEnabled: YES];
        [fScanButton setEnabled: YES];
    }
}

- (IBAction) TitlePopUpChanged: (id) sender
{
    HBTitle * title = (HBTitle*)
        HBListItemAt( fTitleList, [fTitlePopUp indexOfSelectedItem] );

    [fLanguagePopUp removeAllItems];
    [fSecondaryLanguagePopUp removeAllItems];
    
    HBAudio * audio;
    for( int i = 0; i < HBListCountItems( title->audioList ); i++ )
    {
        audio = (HBAudio*) HBListItemAt( title->audioList, i );

        /* We cannot use NSPopUpButton's addItemWithTitle because
           it checks for duplicate entries */
        [[fLanguagePopUp menu] addItemWithTitle:
            [NSString stringWithCString: audio->language]
            action: nil keyEquivalent: @""];
        [[fSecondaryLanguagePopUp menu] addItemWithTitle:
            [NSString stringWithCString: audio->language]
            action: nil keyEquivalent: @""];
    }
    [fSecondaryLanguagePopUp addItemWithTitle: @"None"];
    [fSecondaryLanguagePopUp selectItemWithTitle: @"None"];
    [fSecondaryLanguagePopUp setEnabled:
        ( HBListCountItems( title->audioList ) > 1 )];

    [fTargetSizeField SetHBTitle: title];
    if( [fVideoMatrix selectedRow] )
    {
        [fTargetSizeField UpdateBitrate];
    }
}

- (IBAction) AudioPopUpChanged: (id) sender
{
    if( [fVideoMatrix selectedRow] )
    {
        [fTargetSizeField UpdateBitrate];
    }
}

@end
