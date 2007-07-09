/* QueueController */

#include <Cocoa/Cocoa.h>

#include "hb.h"

@interface QueueController : NSObject
{
    hb_handle_t            * fHandle;
    IBOutlet NSScrollView  * fScrollView;
    IBOutlet NSView        * fTaskView;
	
	/*Display variables for each job in view*/
	NSString               * jobFormat;
	NSString               * jobPictureDetail;
	NSString               * jobVideoDetail;
	NSString               * jobVideoCodec;
	NSString               * jobVideoQuality;
	
	NSString               * jobAudioDetail;
	NSString               * jobAudioCodec;
}

- (void)     SetHandle: (hb_handle_t *) handle;
- (IBAction) Update: (id) sender;
- (IBAction) ClosePanel: (id) sender;

@end
