/* HBQueueController

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */


#include <Cocoa/Cocoa.h>

#include "hb.h"

@interface HBQueueController : NSObject
{
    hb_handle_t                  *fHandle;
    NSViewAnimation              *fAnimation;
    BOOL                         fCurrentJobHidden;  // YES when fCurrentJobPane has been shifted out of view (see showCurrentJobPane)
    
    //  +---------------fQueueWindow----------------+
    //  |+-------------fCurrentJobPane-------------+|
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  |+-----------------------------------------+|
    //  |+---------------fQueuePane----------------+|
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  ||                                         ||
    //  |+-----------------------------------------+|
    //  +-------------------------------------------+
    
    IBOutlet NSWindow            *fQueueWindow;

    // fCurrentJobPane - visible only when processing a job
    IBOutlet NSView              *fCurrentJobPane;
    IBOutlet NSTextField         *fJobDescTextField;
    IBOutlet NSProgressIndicator *fProgressBar;
    IBOutlet NSTextField         *fProgressStatus;
    IBOutlet NSTextField         *fProgressTimeRemaining;
    
    // fQueuePane - always visible; fills entire window when fCurrentJobPane is hidden
    IBOutlet NSView              *fQueuePane;
    IBOutlet NSTableView         *fTaskView;
    IBOutlet NSButton            *fDetailCheckbox;
    IBOutlet NSTextField         *fQueueCountField;
    IBOutlet NSButton            *fStartPauseButton;
    
}

- (void)setHandle: (hb_handle_t *)handle;
- (void)updateQueueUI;
- (void)updateCurrentJobUI;
- (IBAction)removeSelectedJob: (id)sender;
- (IBAction)cancelCurrentJob: (id)sender;
- (IBAction)detailChanged: (id)sender;
- (IBAction)showQueueWindow: (id)sender;
- (IBAction)toggleStartPause: (id)sender;

@end
