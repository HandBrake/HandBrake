//
//  HBAudioController.h
//  HandBrake
//
//  Created on 2010-08-24.
//

#import <Cocoa/Cocoa.h>
#import "HBViewValidation.h"

extern NSString *keyAudioTrackIndex;
extern NSString *keyAudioTrackName;
extern NSString *keyAudioInputBitrate;
extern NSString *keyAudioInputSampleRate;
extern NSString *keyAudioInputCodec;
extern NSString *keyAudioInputCodecParam;
extern NSString *keyAudioInputChannelLayout;

extern NSString *HBMixdownChangedNotification;

@class HBAudio;
@class HBAudioDefaults;
/**
 *  HBAudioController
 *
 *  Responds to HBContainerChangedNotification and HBTitleChangedNotification notifications.
 */
@interface HBAudioController : NSViewController <HBViewValidation>

@property (nonatomic, readonly, retain) NSArray *masterTrackArray;
@property (nonatomic, readonly) NSDictionary *noneTrack;

@property(nonatomic, readonly) HBAudioDefaults *settings;

// Get the list of audio tracks
@property (readonly, nonatomic, copy) NSArray *audioTracks;

- (void) applySettingsFromPreset:(NSDictionary *)preset;
- (void) addTracksFromQueue: (NSArray *) queueArray;

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
