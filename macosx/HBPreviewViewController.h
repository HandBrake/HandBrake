//
//  HBPreviewViewController.h
//  HandBrake
//
//  Created by Damiano Galassi on 14/12/2017.
//

#import <Cocoa/Cocoa.h>

@class HBPreviewGenerator;
@class HBPreviewController;

@interface HBPreviewViewController : NSViewController

@property (nonatomic, readwrite, weak, nullable) HBPreviewGenerator *generator;

- (void)update;

@end
