//
//  HBPicture.h
//  HandBrake
//
//  Created by Damiano Galassi on 12/08/14.
//
//

#import <Foundation/Foundation.h>

@interface HBPicture : NSObject

- (void)applySettingsFromPreset:(NSDictionary *)preset;

@property (nonatomic, readwrite) int width;
@property (nonatomic, readwrite) int height;

@property (nonatomic, readwrite) BOOL autocrop;
@property (nonatomic, readwrite) int *crop;

@property (nonatomic, readwrite) int modulus;

/*
 anamorphic {
 mode
 keepDisplayAspect
 par_width
 par_height
 dar_width
 dar_height
 }
 modulus
 */

@end
