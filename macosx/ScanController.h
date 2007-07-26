/*   $Id: ScanController.h,v 1.4 2005/03/21 12:37:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <Cocoa/Cocoa.h>

#include "hb.h"
@class DriveDetector;
@interface ScanController : NSObject
{
    hb_handle_t                  * fHandle;
	hb_list_t                    * fList;
    
    IBOutlet NSWindow            * fWindow;
    IBOutlet NSPanel             * fPanel;
    
    IBOutlet NSTextField         * fStatusField;
    IBOutlet NSProgressIndicator * fIndicator;

	
    DriveDetector                * fDriveDetector;
    NSDictionary                 * fDrives;	
}

- (void)     SetHandle:     (hb_handle_t *) handle;
- (void)     Show;
- (IBAction) Browse:        (id) sender;
- (IBAction) Cancel:        (id) sender;
- (void) BrowseDone: (NSOpenPanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo;


@end
