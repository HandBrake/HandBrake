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



- (void) SetHandle: (hb_handle_t *) handle
{
    fHandle    = handle;
}

- (void) Show
{

	[self Browse: NULL];
}

- (void) Browse: (id) sender
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
   [panel beginSheetForDirectory: sourceDirectory file: nil types: nil
				   modalForWindow: fWindow modalDelegate: self
				   didEndSelector: @selector( BrowseDone:returnCode:contextInfo: )
					  contextInfo: nil];
}

- (void) BrowseDone: (NSOpenPanel *) sheet
		 returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    /* User selected a file to open */
	if( returnCode == NSOKButton )
    {
        [fStatusField setStringValue: _( @"Opening a new source ..." )];
		[fIndicator setHidden: NO];
	    [fIndicator setIndeterminate: YES];
        [fIndicator startAnimation: nil];
		[fDriveDetector stop];
		
		/* we set the last source directory in the prefs here */
		NSString *sourceDirectory = [[[sheet filenames] objectAtIndex: 0] stringByDeletingLastPathComponent];
		[[NSUserDefaults standardUserDefaults] setObject:sourceDirectory forKey:@"LastSourceDirectory"];
		
		hb_scan( fHandle, [[[sheet filenames] objectAtIndex: 0] UTF8String], 0 );
		
		[self Cancel: nil];
    }
	else // User clicked Cancel in browse window
	{
		
		[self Cancel: nil];
		
	}
    
    
	
}


- (IBAction) Cancel: (id) sender
{
   [NSApp endSheet:fPanel];
	[fPanel orderOut:self];
}

@end
