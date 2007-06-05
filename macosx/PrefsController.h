/**
 * @file
 * Interface of class PrefsController.
 */

#include <Cocoa/Cocoa.h>

@interface PrefsController : NSObject
{
    IBOutlet NSPanel  * fPanel;
    IBOutlet NSButton * fUpdateCheck;
	IBOutlet NSComboBox * fdefaultlanguage;
	IBOutlet NSButton * fFileExtItunes;
	IBOutlet NSButton * fDefCrf;
	IBOutlet NSButton * fDefDeinterlace;
	IBOutlet NSButton * fDefPicSizeAutoSetipod;
	IBOutlet NSButton * fDefPixelRatio;
	IBOutlet NSButton * fDefPresetDrawerShow;
	IBOutlet NSButton * fDefAutoNaming;
	IBOutlet NSButton * fDefChapterMarkers;

	IBOutlet NSTextField * fDefAdvancedx264FlagsView;
	IBOutlet NSButton * fDefAdvancedx264FlagsShow;

}

+ (void)registerUserDefaults;

- (IBAction) OpenPanel:    (id) sender;
- (IBAction) ClosePanel:   (id) sender;
- (IBAction) CheckChanged: (id) sender;

@end
