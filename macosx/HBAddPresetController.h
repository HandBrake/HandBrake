//
//  HBAddPresetController.h
//  HandBrake
//
//  Created by Damiano Galassi on 23/11/14.
//
//

#import <Cocoa/Cocoa.h>

@class HBPreset;

@interface HBAddPresetController : NSWindowController

- (instancetype)initWithPreset:(HBPreset *)preset videoSize:(NSSize)size;

@property (nonatomic, readonly) HBPreset *preset;

@end
