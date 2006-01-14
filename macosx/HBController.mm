/* $Id: HBController.mm,v 1.24 2003/10/06 21:13:45 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://beos.titer.org/handbrake/>.
   It may be used under the terms of the GNU General Public License. */

#include <paths.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>

#include "HBController.h"
#include "Manager.h"

@implementation HBController

- (void) applicationDidFinishLaunching: (NSNotification *) notification
{
    /* Init libhb */
    fManager = new HBManager( true );

    /* Update the GUI every 1/10 sec */
    [NSTimer scheduledTimerWithTimeInterval: 0.1
        target: self selector: @selector( UpdateIntf: )
        userInfo: nil repeats: FALSE];
}

- (NSApplicationTerminateReply) applicationShouldTerminate:
    (NSApplication *) app
{
    /* Clean up */
    delete fManager;

    return NSTerminateNow;
}

- (void) awakeFromNib
{
    [[fScanMatrix cellAtRow: 0 column: 0]
        setAction: @selector( ScanEnableIntf: )];
    [[fScanMatrix cellAtRow: 0 column: 0] setTarget: self];
    [[fScanMatrix cellAtRow: 1 column: 0]
        setAction: @selector( ScanEnableIntf: )];
    [[fScanMatrix cellAtRow: 1 column: 0] setTarget: self];
    [fScanProgress setStyle: NSProgressIndicatorSpinningStyle];
    [fScanProgress setDisplayedWhenStopped: NO];
    [fRipProgress setIndeterminate: NO];
    [fTitlePopUp removeAllItems];
    [fAudioPopUp removeAllItems];

    char string[1024]; memset( string, 0, 1024 );
    snprintf( string, 1024, "%s/Desktop/Movie.avi", getenv( "HOME" ) );
    [fFileField setStringValue: [NSString stringWithCString: string]];

    /* Show the scan view */
    [fWindow setContentSize: [fScanView frame].size];
    [fWindow setContentView: fScanView];
    [fWindow center];

    /* Detect DVD drives */
    [self DetectDrives];
    [self ScanEnableIntf: self];

    /* Init a blank view, used in window resizing animation */
    fBlankView = [[NSView alloc] init];
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
    /* Ask the manager to start scanning the specified volume */
    
    if( ![fScanMatrix selectedRow] )
    {
        /* DVD drive */
        fManager->ScanVolumes( (char*) [[fDVDPopUp titleOfSelectedItem]
                                           cString] );
    }
    else
    {
        /* DVD folder */
        fManager->ScanVolumes( (char*) [[fDVDFolderField stringValue]
                                           cString] );
    }
}

- (IBAction) ShowPicturePanel: (id) sender
{
    HBTitle * title = (HBTitle*)
        fTitleList->ItemAt( [fTitlePopUp indexOfSelectedItem] );
    
    [fPictureGLView SetManager: fManager];
    [fPictureGLView SetTitle: title];

    fPicture = 0;
    [fPictureGLView ShowPicture: fPicture];

    [fWidthStepper  setValueWraps: NO];
    [fWidthStepper  setIncrement: 16];
    [fWidthStepper  setMinValue: 16];
    [fWidthStepper  setMaxValue: title->fOutWidthMax];
    [fWidthStepper  setIntValue: title->fOutWidth];
    [fWidthField    setIntValue: title->fOutWidth];
    [fTopStepper  setValueWraps: NO];
    [fTopStepper    setIncrement: 2];
    [fTopStepper    setMinValue: 0];
    [fTopStepper    setMaxValue: title->fInHeight / 4];
    [fTopStepper    setIntValue: title->fTopCrop];
    [fTopField      setIntValue: title->fTopCrop];
    [fBottomStepper setValueWraps: NO];
    [fBottomStepper setIncrement: 2];
    [fBottomStepper setMinValue: 0];
    [fBottomStepper setMaxValue: title->fInHeight / 4];
    [fBottomStepper setIntValue: title->fBottomCrop];
    [fBottomField   setIntValue: title->fBottomCrop];
    [fLeftStepper   setValueWraps: NO];
    [fLeftStepper   setIncrement: 2];
    [fLeftStepper   setMinValue: 0];
    [fLeftStepper   setMaxValue: title->fInWidth / 4];
    [fLeftStepper   setIntValue: title->fLeftCrop];
    [fLeftField     setIntValue: title->fLeftCrop];
    [fRightStepper  setValueWraps: NO];
    [fRightStepper  setIncrement: 2];
    [fRightStepper  setMinValue: 0];
    [fRightStepper  setMaxValue: title->fInWidth / 4];
    [fRightStepper  setIntValue: title->fRightCrop];
    [fRightField    setIntValue: title->fRightCrop];

    char string[1024]; memset( string, 0, 1024 );
    sprintf( string, "Final size: %dx%d",
             title->fOutWidth, title->fOutHeight );
    [fInfoField setStringValue: [NSString stringWithCString: string]];

    /* Resize the panel */
    NSSize newSize;
    /* XXX */
    newSize.width  = 762 /*fPicturePanelSize.width*/ +
        title->fOutWidthMax - 720;
    newSize.height = 754 /*fPicturePanelSize.height*/ +
        title->fOutHeightMax - 576;
    [fPicturePanel setContentSize: newSize];

    [NSApp beginSheet: fPicturePanel
        modalForWindow: fWindow
        modalDelegate: nil
        didEndSelector: nil
        contextInfo: nil];
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
    
    /* Get the specified title & audio track(s) */
    HBTitle * title = (HBTitle*)
        fTitleList->ItemAt( [fTitlePopUp indexOfSelectedItem] );
    HBAudio * audio = (HBAudio*)
        title->fAudioList->ItemAt( [fAudioPopUp indexOfSelectedItem] );

    /* Use user settings */
    title->fBitrate    = [fVideoStepper intValue];
    audio->fOutBitrate = [fAudioStepper intValue];
    title->fTwoPass    = ( [fTwoPassCheck state] == NSOnState );

    /* Let libhb do the job */
    fManager->StartRip( title, audio, NULL,
                        (char*) [[fFileField stringValue] cString] );
}

- (IBAction) Cancel: (id) sender
{
    fManager->StopRip();
}

- (IBAction) Suspend: (id) sender
{
    if( [[fSuspendButton title] compare: @"Resume" ] == NSOrderedSame )
    {
        [self Resume: self];
        return;
    }

    fManager->SuspendRip();
}

- (IBAction) Resume: (id) sender
{
    fManager->ResumeRip();
}

- (IBAction) PreviousPicture: (id) sender
{
    if( fPicture > 0 )
    {
        fPicture--;
        [fPictureGLView ShowPicture: fPicture];
    }
}

- (IBAction) NextPicture: (id) sender
{
    if( fPicture < 9 )
    {
        fPicture++;
        [fPictureGLView ShowPicture: fPicture];
    }
}

- (IBAction) UpdatePicture: (id) sender
{
    HBTitle * title = (HBTitle*)
        fTitleList->ItemAt( [fTitlePopUp indexOfSelectedItem] );
    title->fOutWidth    = [fWidthStepper intValue];
    title->fDeinterlace = ( [fDeinterlaceCheck state] == NSOnState );
    title->fTopCrop     = [fTopStepper intValue];
    title->fBottomCrop  = [fBottomStepper intValue];
    title->fLeftCrop    = [fLeftStepper intValue];
    title->fRightCrop   = [fRightStepper intValue];

    [fPictureGLView ShowPicture: fPicture];

    [fWidthStepper  setIntValue: title->fOutWidth];
    [fTopStepper    setIntValue: title->fTopCrop];
    [fBottomStepper setIntValue: title->fBottomCrop];
    [fLeftStepper   setIntValue: title->fLeftCrop];
    [fRightStepper  setIntValue: title->fRightCrop];
    [fWidthField    setIntValue: [fWidthStepper intValue]];
    [fTopField      setIntValue: [fTopStepper intValue]];
    [fBottomField   setIntValue: [fBottomStepper intValue]];
    [fLeftField     setIntValue: [fLeftStepper intValue]];
    [fRightField    setIntValue: [fRightStepper intValue]];

    char string[1024]; memset( string, 0, 1024 );
    sprintf( string, "Final size: %dx%d",
             title->fOutWidth, title->fOutHeight );
    [fInfoField setStringValue: [NSString stringWithCString: string]];
}

- (void) UpdateIntf: (NSTimer *) timer
{
    /* Ask libhb about what's happening now */
    if( fManager->NeedUpdate() )
    {
        HBStatus status = fManager->GetStatus();

        switch( status.fMode )
        {
            case HB_MODE_NEED_VOLUME:
                break;

            case HB_MODE_SCANNING:
            {
                [fScanMatrix setEnabled: NO];
                [fDVDPopUp setEnabled: NO];
                [fDVDFolderField setEnabled: NO];
                [fScanBrowseButton setEnabled: NO];
                [fScanProgress startAnimation: self];
                [fScanButton setEnabled: NO];

                char string[1024]; memset( string, 0, 1024 );
                if( status.fScannedTitle )
                {
                    sprintf( string, "Scanning %s, title %d...",
                             status.fScannedVolume,
                             status.fScannedTitle );
                }
                else
                {
                    sprintf( string, "Opening %s...",
                             status.fScannedVolume );
                }
                [fScanStatusField setStringValue:
                    [NSString stringWithCString: string]];

                break;
            }

            case HB_MODE_INVALID_VOLUME:
            {
                [fScanMatrix setEnabled: YES];
                [self ScanEnableIntf: self];
                [fScanProgress stopAnimation: self];
                [fScanButton setEnabled: YES];

                [fScanStatusField setStringValue:
                    @"Invalid volume, try again" ];
                break;
            }

            case HB_MODE_READY_TO_RIP:
            {
                fTitleList = status.fTitleList;
                
                /* Show a temporary empty view while the window
                   resizing animation */
                [fWindow setContentView: fBlankView ];

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
                
                HBTitle * title;
                for( uint32_t i = 0; i < fTitleList->CountItems(); i++ )
                {
                    title = (HBTitle*) fTitleList->ItemAt( i );
                    char string[1024]; memset( string, 0, 1024 );
                    sprintf( string, "%d (%02lld:%02lld:%02lld)",
                             title->fIndex, title->fLength / 3600,
                             ( title->fLength % 3600 ) / 60,
                             title->fLength % 60 );
                    [[fTitlePopUp menu] addItemWithTitle:
                        [NSString stringWithCString: string]
                        action: @selector( UpdatePopUp: )
                        keyEquivalent: @""];
                }
                [self UpdatePopUp: self];
                
                break;
            }

            case HB_MODE_ENCODING:
            {
                [fTitlePopUp setEnabled: NO];
                [fAudioPopUp setEnabled: NO];
                [fVideoField setEnabled: NO];
                [fVideoStepper setEnabled: NO];
                [fAudioField setEnabled: NO];
                [fAudioStepper setEnabled: NO];
                [fTwoPassCheck setEnabled: NO];
                [fCropButton setEnabled: NO];
                [fFileField setEnabled: NO];
                [fRipBrowseButton setEnabled: NO];
                [fRipButton setTitle: @"Cancel"];
                [fSuspendButton setEnabled: YES];
                [fSuspendButton setTitle: @"Suspend"];
            
                if( !status.fPosition )
                {
                    [fRipStatusField setStringValue: @"Starting..."];
                    [fRipProgress setIndeterminate: YES];
                    [fRipProgress startAnimation: self];;
                }
                else
                {
                    char string[1024]; memset( string, 0, 1024 );
                    sprintf( string, "Encoding: %.2f %%, %.2f fps "
                             "(%02d:%02d:%02d remaining)",
                             100 * status.fPosition, status.fFrameRate,
                             status.fRemainingTime / 3600,
                             ( status.fRemainingTime % 3600 ) / 60,
                             status.fRemainingTime % 60 );
                    [fRipStatusField setStringValue:
                        [NSString stringWithCString: string]];
                    [fRipProgress setIndeterminate: NO];
                    [fRipProgress setDoubleValue: 100 * status.fPosition];
                }
                
                break;
            }

            case HB_MODE_SUSPENDED:
            {
                char string[1024]; memset( string, 0, 1024 );
                sprintf( string, "Encoding: %.2f %%, %.2f fps (PAUSED)",
                         100 * status.fPosition, status.fFrameRate) ;
                [fRipStatusField setStringValue:
                    [NSString stringWithCString: string]];
                
                [fRipProgress setDoubleValue: 100 * status.fPosition];

                [fSuspendButton setTitle: @"Resume"];
                break;
            }

            case HB_MODE_DONE:
            case HB_MODE_CANCELED:
            case HB_MODE_ERROR:
                [fTitlePopUp setEnabled: YES];
                [fAudioPopUp setEnabled: YES];
                [fVideoField setEnabled: YES];
                [fVideoStepper setEnabled: YES];
                [fAudioField setEnabled: YES];
                [fAudioStepper setEnabled: YES];
                [fTwoPassCheck setEnabled: YES];
                [fCropButton setEnabled: YES];
                [fFileField setEnabled: YES];
                [fRipBrowseButton setEnabled: YES];
                [fRipButton setEnabled: YES];
                [fRipButton setTitle: @"Rip"];
                [fSuspendButton setEnabled: NO];
                [fSuspendButton setTitle: @"Suspend"];

                if( status.fMode == HB_MODE_DONE )
                {
                    [fRipStatusField setStringValue: @"Done." ];
                    [fRipProgress setDoubleValue: 100];
                    NSBeep();
                    [NSApp requestUserAttention: NSInformationalRequest];
                    [NSApp beginSheet: fDonePanel
                        modalForWindow: fWindow
                        modalDelegate: nil
                        didEndSelector: nil
                        contextInfo: nil];
                    [NSApp runModalForWindow: fDonePanel];
                    [NSApp endSheet: fDonePanel];
                    [fDonePanel orderOut: self];
                }
                else if( status.fMode == HB_MODE_CANCELED )
                {
                    [fRipStatusField setStringValue: @"Canceled." ];
                    [fRipProgress setDoubleValue: 0];
                }
                else
                {
                    [fRipStatusField setStringValue: @"An error occured." ];
                    [fRipProgress setDoubleValue: 0];
                }

                /* Warn the finder to update itself */
                [[NSWorkspace sharedWorkspace] noteFileSystemChanged:
                    [fFileField stringValue]];
                break;

            default:
                break;
        }
    }

    /* Do it again 1/10 second later */
    [NSTimer scheduledTimerWithTimeInterval: 0.1
        target: self selector: @selector( UpdateIntf: )
        userInfo: nil repeats: FALSE];
}

- (void) DetectDrives
{
    /* Empty the current popup */
    [fDVDPopUp removeAllItems];
    
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

    CFDictionarySetValue( classes_to_match, CFSTR( kIOMediaEjectable ),
                          kCFBooleanTrue );

    kern_result =
        IOServiceGetMatchingServices( master_port, classes_to_match,
                                      &media_iterator );
    if( kern_result != KERN_SUCCESS )
    {
        return;
    }

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
                                                 CFSTR( kIOBSDName ),
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
                [[fDVDPopUp menu] addItemWithTitle:
                    [NSString stringWithCString: psz_buf]
                    action: nil keyEquivalent: @""];
            }

            CFRelease( str_bsd_path );

            IOObjectRelease( next_media );

        } while( ( next_media = IOIteratorNext( media_iterator ) ) != NULL );
    }

    IOObjectRelease( media_iterator );
}

- (void) ScanEnableIntf: (id) sender
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

- (void) UpdatePopUp: (id) sender
{
    HBTitle * title = (HBTitle*)
        fTitleList->ItemAt( [fTitlePopUp indexOfSelectedItem] );

    [fAudioPopUp removeAllItems];
    
    HBAudio * audio;
    for( uint32_t i = 0; i < title->fAudioList->CountItems(); i++ )
    {
        audio = (HBAudio*) title->fAudioList->ItemAt( i );

        /* We cannot use NSPopUpButton's addItemWithTitle because
           it checks for duplicate entries */
        [[fAudioPopUp menu] addItemWithTitle:
            [NSString stringWithCString: audio->fDescription]
            action: nil keyEquivalent: @""];
    }
}

@end
