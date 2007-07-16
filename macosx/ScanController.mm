/*  $Id: ScanController.mm,v 1.10 2005/04/27 21:05:24 titer Exp $

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
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DisableDvdAutoDetect"] == 0)
	{
	/* We manually show these, in case they were hidden during a previous scan
	with the Auto Detect turned off */
	[fMatrix setHidden: NO];
	[fDetectedPopUp setHidden: NO];
	[fFolderField setHidden: NO];
	[fOpenButton setHidden: NO];
	[fCancelButton setHidden: NO];
	[fBrowseButton setHidden: NO];
	
	[fSelectString setStringValue:@"Select a DVD Source:"];
		fDriveDetector = [[DriveDetector alloc] initWithCallback:self selector:@selector(openUpdateDrives:)];
		[fDriveDetector run];
	/* We show the scan choice sheet */
	[NSApp beginSheet:fPanel modalForWindow:fWindow modalDelegate:nil didEndSelector:NULL contextInfo:NULL];
	}
	else // If dvd auto detect is turned off
	{
	[fSelectString setStringValue:@""];

	[fDetectedCell  setEnabled: 0];
	[fDetectedPopUp setEnabled: 0];
    [fFolderField   setEnabled: 1];
    [fBrowseButton  setEnabled: 1];
	[fOpenButton    setEnabled: 0];
	[fBrowseButton  setEnabled: 1];
	[fMatrix selectCell: fFolderCell];
	[fMatrix setHidden: YES];
	[fDetectedPopUp setHidden: YES];
	[fFolderField setHidden: YES];
	[fOpenButton setHidden: YES];
	[fCancelButton setHidden: NO];
	[fBrowseButton setHidden: YES];
	/* We go straight to the Browse Sheet */
	[self Browse2: NULL];
	}

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
	[fDetectedPopUp setEnabled: 0];
    [fFolderField   setEnabled: 1];
    [fBrowseButton  setEnabled: 1];
	[fOpenButton    setEnabled: 0];
	[fBrowseButton  setEnabled: 1];
	[fMatrix selectCell: fFolderCell];


    }
	else
	{
	[fDetectedPopUp setEnabled: 1];
    [fFolderField   setEnabled: 0];
    [fBrowseButton  setEnabled: 0];
	[fOpenButton    setEnabled: 1];
	[fBrowseButton  setEnabled: 0];
	}

	[fDetectedPopUp selectItemAtIndex: 0];

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
			[fSelectString setStringValue:@"HandBrake is Scanning Your Source..."];
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
            /*
			 if (hb_list_count(hb_get_titles(fHandle)))
             {
				 [fStatusField setStringValue:@""];
				 [NSApp endSheet:fPanel];
				 [fPanel orderOut:self];
				 
			 }
			 else
			 {
				 [fStatusField setStringValue:_( @"No valid title found.")];
				 // If DVD Auto Detect is disabled 
				 if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DisableDvdAutoDetect"] == 1)
				 {
					 [NSApp endSheet:fPanel];
					 [fPanel orderOut:self];
				 }
			 }
			 */
			[fStatusField setStringValue:@""];
			[NSApp endSheet:fPanel];
			[fPanel orderOut:self];
            break;
        /* garbage collection here just in case we get caught in a HB_STATE_WORKING
		   phase if scanning while encoding */
        case HB_STATE_WORKING:

		/* Update slider */
		/* Use "barber pole" as we currently have no way to measure
		   progress of scan while encoding */
		   [fStatusField setStringValue:@"Performing background scan ..."];
            [fIndicator setIndeterminate: YES];
            [fIndicator startAnimation: nil];
			break;


    }
}

- (IBAction) MatrixChanged: (id) sender
{
    /* Do we have detected drives  and is "Disable DVD Drive Auto Scan" off in prefs*/

    if( [fDetectedPopUp numberOfItems] > 0  && [[NSUserDefaults standardUserDefaults] boolForKey:@"DisableDvdAutoDetect"] == 0)
    {
        [fDetectedCell setEnabled: YES];
		//[fMatrix selectCellAtRow:0 column:0];
		
    }
    else
    {
       //[fMatrix selectCell: fFolderCell];
        [fDetectedCell setEnabled: NO];
		
    }

    /* Enable controls related to the current choice */
	/* If Detected Volume is selected */
    if ( [fMatrix selectedRow] == 0 )
	{
	[fDetectedPopUp setEnabled: YES];
    [fFolderField   setEnabled: NO];
    [fBrowseButton  setEnabled: NO];
	}
	else
	{
	[fDetectedPopUp setEnabled: NO];
    [fFolderField   setEnabled: YES];
    [fBrowseButton  setEnabled: YES];
	}


}



/* Browse:
   Remove the current sheet (the scan panel) and show an OpenPanel.
   Because we can't open the new sheet before the other one is
   completely gone, we use performSelectorOnMainThread so it will be
   done right afterwards */
- (IBAction) Browse: (id) sender
{
    [NSApp endSheet:fPanel];
	[fPanel orderOut:self];
	[self performSelectorOnMainThread:@selector(Browse2:) withObject:nil waitUntilDone:NO];
}
- (void) Browse2: (id) sender
{
    NSOpenPanel * panel;
	
    panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: YES ];
    NSString * sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastSourceDirectory"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastSourceDirectory"];
	}
	else
	{
		sourceDirectory = @"~/Desktop";
		sourceDirectory = [sourceDirectory stringByExpandingTildeInPath];
	}
    /*
	 Old open browse window, lets keep it her for a couple of revs for reference
	 if problems might crop up */
	/*
	 [panel beginSheetForDirectory: sourceDirectory file: nil types: nil
					modalForWindow: [NSApp mainWindow] modalDelegate: self
					didEndSelector: @selector( BrowseDone:returnCode:contextInfo: )
					   contextInfo: nil];
	 */
	[panel beginSheetForDirectory: sourceDirectory file: nil types: nil
				   modalForWindow: fWindow modalDelegate: self
				   didEndSelector: @selector( BrowseDone:returnCode:contextInfo: )
					  contextInfo: nil];
}

/* Get the folder and switch sheets again. Use the same trick as
   above */
- (void) BrowseDone: (NSOpenPanel *) sheet
		 returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    /* User selected a file to open */
	if( returnCode == NSOKButton )
    {
        [fFolderField setStringValue:
            [[sheet filenames] objectAtIndex: 0]];
        [self Open: nil];
		[self performSelectorOnMainThread: @selector( BrowseDone2: )
							   withObject: nil waitUntilDone: NO];
    }
	else // User clicked Cancel in browse window
	{
		/* If DVD Auto Detect is disabled */
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DisableDvdAutoDetect"] == 1)
		{
			/* We close the scan panel altogether */
			    [self Cancel: nil];
		}
		else /* Dvd auto detect is on */
		{
			/* We return back to the scan choice sheet */
			[self performSelectorOnMainThread: @selector( BrowseDone2: )
								   withObject: nil waitUntilDone: NO];
								   
	
		}
	}
    
    
	
}
- (void) BrowseDone2: (id) sender
{
    [NSApp beginSheet:fPanel modalForWindow:fWindow modalDelegate:nil didEndSelector:NULL contextInfo:NULL];

}

- (IBAction) Open: (id) sender
{

		[fStatusField setStringValue: _( @"Opening..." )];
		[fIndicator setIndeterminate: YES];
        [fIndicator startAnimation: nil];
		[fDriveDetector stop];
		
		if( [fMatrix selectedRow] )
		{
			/* we set the last source directory in the prefs here */
			NSString *sourceDirectory = [[fFolderField stringValue] stringByDeletingLastPathComponent];
			[[NSUserDefaults standardUserDefaults] setObject:sourceDirectory forKey:@"LastSourceDirectory"];
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
    [NSApp endSheet:fPanel];
	[fPanel orderOut:self];
}

@end
