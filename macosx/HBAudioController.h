//
//  HBAudioController.h
//  HandBrake
//
//  Created on 2010-08-24.
//

#import <Cocoa/Cocoa.h>

extern NSString *keyAudioTrackIndex;
extern NSString *keyAudioTrackName;
extern NSString *keyAudioInputBitrate;
extern NSString *keyAudioInputSampleRate;
extern NSString *keyAudioInputCodec;
extern NSString *keyAudioInputCodecParam;
extern NSString *keyAudioInputChannelLayout;

extern NSString *HBMixdownChangedNotification;

@class HBAudio;
@class HBAudioSettings;
/**
 *  HBAudioController
 *
 *  Responds to HBContainerChangedNotification and HBTitleChangedNotification notifications.
 */
@interface HBAudioController : NSViewController

@property (nonatomic, readonly, retain) NSArray *masterTrackArray;
@property (nonatomic, readonly) NSDictionary *noneTrack;

@property(nonatomic, readonly) HBAudioSettings *settings;

// Get the list of audio tracks
@property (readonly, nonatomic, copy) NSArray *audioTracks;

- (void) enableUI: (BOOL) b;

- (void) applySettingsFromPreset:(NSDictionary *)preset;
- (void) addTracksFromQueue: (NSMutableDictionary *) aQueue;

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
