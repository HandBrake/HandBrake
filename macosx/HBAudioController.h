//
//  HBAudioController.h
//  HandBrake
//
//  Created on 2010-08-24.
//

#import <Cocoa/Cocoa.h>

@class HBJob;
@class HBAudio;

/**
 *  HBAudioController
 *
 *  Responds to HBContainerChangedNotification.
 */
@interface HBAudioController : NSViewController

@property (nonatomic, readonly, retain) NSArray *masterTrackArray;
@property (nonatomic, readonly) NSDictionary *noneTrack;

@property (nonatomic, readwrite, assign) HBJob *job;

- (void) applySettingsFromPreset:(NSDictionary *)preset;

- (BOOL) anyCodecMatches: (int) aCodecValue;
- (void) settingTrackToNone: (HBAudio *) newNoneTrack;
- (void) switchingTrackFromNone: (HBAudio *) noLongerNoneTrack;

@end

@interface HBAudioController (KVC)

- (NSUInteger) countOfAudioArray;
- (HBAudio *) objectInAudioArrayAtIndex: (NSUInteger) index;
- (void) insertObject: (HBAudio *) audioObject inAudioArrayAtIndex: (NSUInteger) index;
- (void) removeObjectFromAudioArrayAtIndex: (NSUInteger) index;

@end
