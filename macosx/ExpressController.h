/* ExpressController */

#import <Cocoa/Cocoa.h>
#import "hb.h"

@class DriveDetector;

@interface ExpressController : NSObject

{
    hb_handle_t                  * fHandle;
    hb_list_t                    * fList;

    IBOutlet NSWindow            * fWindow;
    IBOutlet NSView              * fEmptyView;

    IBOutlet NSView              * fOpenView;
    IBOutlet NSMatrix            * fOpenMatrix;
    IBOutlet NSPopUpButton       * fOpenPopUp;
    IBOutlet NSTextField         * fOpenFolderField;
    IBOutlet NSButton            * fOpenBrowseButton;
    IBOutlet NSTextField         * fOpenProgressField;
    IBOutlet NSProgressIndicator * fOpenIndicator;
    IBOutlet NSButton            * fOpenGoButton;
    NSString                     * fOpenFolderString;

    IBOutlet NSView              * fConvertView;
    IBOutlet NSTableView         * fConvertTableView;
    IBOutlet NSPopUpButton       * fConvertFolderPopUp;
    IBOutlet NSProgressIndicator * fConvertIndicator;
    NSMutableArray               * fConvertCheckArray;
    NSString                     * fConvertFolderString;

    DriveDetector                * fDriveDetector;
}

- (void) openShow: (id) sender;
- (void) openMatrixChanged: (id) sender;
- (void) openBrowse: (id) sender;
- (void) openGo: (id) sender;

- (void) convertGo: (id) sender;

@end

