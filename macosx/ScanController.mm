/* $Id: ScanController.mm,v 1.10 2005/04/27 21:05:24 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */
/* These are now called in DriveDetector.h
#include <paths.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>
*/

#include "ScanController.h"
#include "DriveDetector.h"

#define _(a) NSLocalizedString(a,nil)
#define INSERT_STRING @"Insert a DVD"

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

    [self TranslateStrings];

    [fStatusField setStringValue: @""];
}

- (void) Show
{
 
	/* rev63 Drive Detector
    DriveDetector * driveDetector;
    driveDetector = [[DriveDetector alloc] initWithCallback: self
        selector: @selector( UpdatePopup: )];
    [driveDetector run];
    */
	// Taken From IHB for Post rev 63 drive detector
	fDriveDetector = [[DriveDetector alloc] initWithCallback: self
        selector: @selector( openUpdateDrives: )];
    [fDriveDetector run];
	
	// Here down continue with existing HB
    [NSApp beginSheet: fPanel modalForWindow: fWindow
        modalDelegate: nil didEndSelector: nil contextInfo: nil];
    [NSApp runModalForWindow: fPanel];
    [NSApp endSheet: fPanel];
    [fPanel orderOut: self];

    //driveDetector stop/release not needed
	//[driveDetector stop];
    //[driveDetector release];
}

- (void) openUpdateDrives: (NSDictionary *) drives
{
    if( fDrives )
    {
        [fDrives release];
    }
    fDrives = [[NSDictionary alloc] initWithDictionary: drives];

    NSString * device;
    NSEnumerator * enumerator = [fDrives keyEnumerator];
    [fDetectedPopUp removeAllItems];
    while( ( device = [enumerator nextObject] ) )
    {
        [fDetectedPopUp addItemWithTitle: device];
    }

    if( ![fDetectedPopUp numberOfItems] )
    {
        [fDetectedPopUp addItemWithTitle: INSERT_STRING];
    }
    [fDetectedPopUp selectItemAtIndex: 0];
    /* May not have any bearing on anything
	here as this is from IHB. Need to test
	
	if( [fMatrix isEnabled] )
    {
        [self openEnable: YES];
    */
}


// UpdatePopup Not used for post rev 63 dd
/*
- (void) UpdatePopup: (NSArray *) drives
{
    [fDetectedPopUp removeAllItems];
    [fDetectedPopUp addItemsWithTitles: drives];
    [self MatrixChanged: self];
}
*/
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
  //  NSString * path;
    
    [self         EnableUI: NO];
    [fStatusField setStringValue: _( @"Opening..." )];

	// From IHB
	[fDriveDetector stop];

    if( [fMatrix selectedRow] )
    {
    	hb_scan( fHandle, [[fFolderField stringValue] UTF8String], 0 );
    }
    else
    {
	    hb_scan( fHandle, [[fDrives objectForKey: [fDetectedPopUp
                 titleOfSelectedItem]] UTF8String], 0 );
    }
	
	
	
}

- (IBAction) Cancel: (id) sender
{
    [NSApp stopModal];
}

@end
