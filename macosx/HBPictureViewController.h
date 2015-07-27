//
//  HBPictureViewController.h
//  HandBrake
//
//  Created by Damiano Galassi on 24/07/15.
//
//

#import <Cocoa/Cocoa.h>

@class HBPicture;
@class HBFilters;

@interface HBPictureViewController : NSViewController

@property (nonatomic, readwrite, weak) HBPicture *picture;
@property (nonatomic, readwrite, weak) HBFilters *filters;

@end
