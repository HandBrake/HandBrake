//
//  HBAudioController.h
//  HandBrake
//
//  Created on 2010-08-24.
//

#import <Cocoa/Cocoa.h>
#import "hb.h"

extern NSString *keyAudioTrackIndex;
extern NSString *keyAudioTrackName;
extern NSString *keyAudioInputBitrate;
extern NSString *keyAudioInputSampleRate;
extern NSString *keyAudioInputCodec;
extern NSString *keyAudioInputCodecParam;
extern NSString *keyAudioInputChannelLayout;
extern NSString *HBMixdownChangedNotification;

@class HBAudio;

/**
 *  HBAudioController
 *
 *  Responds to HBContainerChangedNotification and HBTitleChangedNotification notifications.
 */
@interface HBAudioController : NSViewController

@property (nonatomic, readonly, retain) NSArray *masterTrackArray;
@property (nonatomic, readonly) NSDictionary *noneTrack;

@property(nonatomic, readwrite) BOOL allowAACPassCheck;
@property(nonatomic, readwrite) BOOL allowAC3PassCheck;
@property(nonatomic, readwrite) BOOL allowDTSHDPassCheck;
@property(nonatomic, readwrite) BOOL allowDTSPassCheck;
@property(nonatomic, readwrite) BOOL allowMP3PassCheck;

@property(nonatomic, readwrite, assign) NSString *audioEncoderFallback;
@property(nonatomic, readwrite) NSInteger audioEncoderFallbackTag;

- (void) enableUI: (BOOL) b;
- (void) setHBController: (id) aController;

- (void) prepareAudioForQueueFileJob: (NSMutableDictionary *) aDict;
- (void) prepareAudioForJobPreview: (hb_job_t *) aJob;
- (void) prepareAudioForPreset: (NSMutableArray *) anArray;
- (void) addTracksFromQueue: (NSMutableDictionary *) aQueue;
- (void) addTracksFromPreset: (NSMutableDictionary *) aPreset;

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
