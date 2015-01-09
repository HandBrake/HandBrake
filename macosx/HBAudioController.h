//
//  HBAudioController.h
//  HandBrake
//
//  Created on 2010-08-24.
//

#import <Cocoa/Cocoa.h>

@class HBJob;
@class HBAudioTrack;

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
- (void) settingTrackToNone: (HBAudioTrack *) newNoneTrack;
- (void) switchingTrackFromNone: (HBAudioTrack *) noLongerNoneTrack;

@end

@interface HBAudioController (KVC)

- (NSUInteger) countOfAudioArray;
- (HBAudioTrack *) objectInAudioArrayAtIndex: (NSUInteger) index;
- (void) insertObject: (HBAudioTrack *) audioObject inAudioArrayAtIndex: (NSUInteger) index;
- (void) removeObjectFromAudioArrayAtIndex: (NSUInteger) index;

@end
