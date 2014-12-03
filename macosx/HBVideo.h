//
//  HBVideo.h
//  HandBrake
//
//  Created by Damiano Galassi on 12/08/14.
//
//

#import <Foundation/Foundation.h>

@interface HBVideo : NSObject

- (void)applySettingsFromPreset:(NSDictionary *)preset;

@property (nonatomic, readwrite) int videoEncoder;

@property (nonatomic, readwrite) int qualityType;
@property (nonatomic, readwrite) int avgBitrate;
@property (nonatomic, readwrite) float quality;

@property (nonatomic, readwrite) int frameRate;
@property (nonatomic, readwrite) int frameRateMode;


@property (nonatomic, readwrite) BOOL fastFirstPass;
@property (nonatomic, readwrite) BOOL twoPass;
@property (nonatomic, readwrite) BOOL turboTwoPass;

@property (nonatomic, readwrite, copy) NSString *videoOptionExtra;

@end
