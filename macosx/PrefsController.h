/* PrefsController */

#include <Cocoa/Cocoa.h>

@interface PrefsController : NSObject
{
    IBOutlet NSPanel  * fPanel;
    IBOutlet NSButton * fUpdateCheck;
    IBOutlet NSButton * fPixelRatio;
	IBOutlet NSComboBox * fdefaultlanguage;
}

- (IBAction) OpenPanel:    (id) sender;
- (IBAction) ClosePanel:   (id) sender;
- (IBAction) CheckChanged: (id) sender;

@end
