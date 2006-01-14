/* $Id: ScanController.mm,v 1.10 2005/04/27 21:05:24 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <paths.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>

#include "ScanController.h"

#define _(a) NSLocalizedString(a,nil)

@implementation ScanController

- (void) TranslateStrings
{
    [fSelectString setStringValue: _( @"Select a DVD:" )];
    [fDetectedCell setTitle:       _( @"Detected volume" )];
    [fFolderCell   setTitle:       _( @"DVD Folder / Image" )];
    [fBrowseButton setTitle:       _( @"Browse" )];
    [fCancelButton setTitle:       _( @"Cancel" )];
    [fOpenButton   setTitle:       _( @"Open" )];
}

- (void) SetHandle: (hb_handle_t *) handle
{
    fHandle    = handle;
    fLastCheck = 0;

    [self TranslateStrings];

    [fStatusField setStringValue: @""];
}

- (void) DetectDrives: (NSNotification *) notification
{
    if( [fMatrix isEnabled] == NO )
    {
        /* We're scanning */
        return;
    }
    if( hb_get_date() < fLastCheck + 1000 )
    {
        /* Don't check more than every second */
        return;
    }
    fLastCheck = hb_get_date();

    /* Scan DVD drives (stolen from VLC) */
    io_object_t            next_media;
    mach_port_t            master_port;
    kern_return_t          kern_result;
    io_iterator_t          media_iterator;
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

    kern_result = IOServiceGetMatchingServices( master_port,
            classes_to_match, &media_iterator );
    if( kern_result != KERN_SUCCESS )
    {
        return;
    }

    NSMutableArray * drivesList; 
    drivesList = [NSMutableArray arrayWithCapacity: 1];

    next_media = IOIteratorNext( media_iterator );
    if( next_media )
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

        } while( ( next_media = IOIteratorNext( media_iterator ) ) );
    }

    IOObjectRelease( media_iterator );

    [fDetectedPopUp removeAllItems];
    for( unsigned i = 0; i < [drivesList count]; i++ )
    {
        [[fDetectedPopUp menu] addItemWithTitle:
            [drivesList objectAtIndex: i] action: nil
            keyEquivalent: @""];
    }
    [self MatrixChanged: self];
}

- (void) EnableUI: (bool) b
{
    [fMatrix        setEnabled: b];
    [fDetectedCell  setEnabled: b];
    [fDetectedPopUp setEnabled: b];
    [fFolderCell    setEnabled: b];
    [fFolderField   setEnabled: b];
    [fBrowseButton  setEnabled: b];
    [fOpenButton    setEnabled: b];

    if( b )
    {
        [self MatrixChanged: nil];
    }
}

- (void) UpdateUI: (hb_state_t *) s
{
    switch( s->state )
    {
#define p s->param.scanning
        case HB_STATE_SCANNING:
            [fStatusField setStringValue: [NSString stringWithFormat:
                _( @"Scanning title %d of %d..." ),
                p.title_cur, p.title_count]];
            [fIndicator setDoubleValue: 100.0 * ( p.title_cur - 1 ) /
                p.title_count];
            break;
#undef p

        case HB_STATE_SCANDONE:
            [self       EnableUI: YES];
            [fIndicator setDoubleValue: 0.0];

            if( hb_list_count( hb_get_titles( fHandle ) ) )
            {
                /* Success */
                [fStatusField setStringValue: @""];
                [NSApp abortModal];
            }
            else
            {
                [fStatusField setStringValue:
                    _( @"No valid title found." )];
            }
            break;
    }
}

- (IBAction) MatrixChanged: (id) sender
{
    /* Do we have detected drives */
    if( [fDetectedPopUp numberOfItems] > 0 )
    {
        [fDetectedCell setEnabled: YES];
    }
    else
    {
        [fMatrix       selectCell: fFolderCell];
        [fDetectedCell setEnabled: NO];
    }
    
    /* Enable controls related to the current choice */
    bool foo;
    foo = ( [fMatrix selectedRow] == 0 );
    [fDetectedPopUp setEnabled:  foo];
    [fFolderField   setEnabled: !foo];
    [fBrowseButton  setEnabled: !foo];
}

/* Browse:
   Remove the current sheet (the scan panel) and show an OpenPanel.
   Because we can't open the new sheet before the other one is
   completely gone, we use performSelectorOnMainThread so it will be
   done right afterwards */
- (IBAction) Browse: (id) sender
{
    [NSApp stopModal];
    [self performSelectorOnMainThread: @selector( Browse2: )
        withObject: nil waitUntilDone: NO];
}
- (void) Browse2: (id) sender
{
    NSOpenPanel * panel;

    panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: YES ];

    [panel beginSheetForDirectory: nil file: nil types: nil
        modalForWindow: [NSApp mainWindow] modalDelegate: self
        didEndSelector: @selector( BrowseDone:returnCode:contextInfo: )
        contextInfo: nil];
}

/* Get the folder and switch sheets again. Use the same trick as
   above */
- (void) BrowseDone: (NSOpenPanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        [fFolderField setStringValue:
            [[sheet filenames] objectAtIndex: 0]];
        [self Open: nil];
    }
    
    [self performSelectorOnMainThread: @selector( BrowseDone2: )
        withObject: nil waitUntilDone: NO];
}
- (void) BrowseDone2: (id) sender
{
    [NSApp beginSheet: fPanel modalForWindow: fWindow
        modalDelegate: nil didEndSelector: nil contextInfo: nil];
    [NSApp runModalForWindow: fWindow];
    [NSApp endSheet: fPanel];
    [fPanel orderOut: self];
}

- (IBAction) Open: (id) sender
{
    NSString * path;
    
    [self         EnableUI: NO];
    [fStatusField setStringValue: _( @"Opening..." )];

    path = [fMatrix selectedRow] ? [fFolderField stringValue] :
               [fDetectedPopUp titleOfSelectedItem];
    hb_scan( fHandle, [path UTF8String], 0 );
}

- (IBAction) Cancel: (id) sender
{
    [NSApp stopModal];
}

@end
