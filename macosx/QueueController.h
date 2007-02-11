/* QueueController */

#include <Cocoa/Cocoa.h>

#include "mediafork.h"

@interface QueueController : NSObject
{
    hb_handle_t            * fHandle;
    IBOutlet NSScrollView  * fScrollView;
    IBOutlet NSView        * fTaskView;
}

- (void)     SetHandle: (hb_handle_t *) handle;
- (IBAction) Update: (id) sender;
- (IBAction) ClosePanel: (id) sender;

@end
