//
//  HBPresetsMenuBuilder.h
//  HandBrake
//
//  Created by Damiano Galassi on 24/11/2017.
//

#import <Cocoa/Cocoa.h>

@import HandBrakeKit;

@interface HBPresetsMenuBuilder : NSObject

@property (nonatomic, readonly) NSMenu *menu;

- (instancetype)initWithMenu:(NSMenu *)menu action:(SEL)action size:(CGFloat)size presetsManager:(HBPresetsManager *)manager;

/**
 *  Adds the presets list to the menu.
 */
- (void)build;

@end
