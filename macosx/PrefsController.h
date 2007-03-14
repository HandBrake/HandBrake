/* PrefsController */

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
	IBOutlet NSButton * fDefAudioSurround;
	IBOutlet NSButton * fDefPresetDrawerShow;
	IBOutlet NSButton * fDefAutoNaming;

	IBOutlet NSTextView * fDefAdvancedx264FlagsView;
}

- (IBAction) OpenPanel:    (id) sender;
- (IBAction) ClosePanel:   (id) sender;
- (IBAction) CheckChanged: (id) sender;

@end
