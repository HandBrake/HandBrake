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

@interface HBAudioController : NSObject

{
    id                             myController;
    NSMutableArray               * audioArray;        // the configured audio information
    NSArray                      * masterTrackArray;  // the master list of audio tracks from the title
    NSDictionary                 * noneTrack;         // this represents no audio track selection
    NSNumber                     * videoContainerTag; // initially is the default HB_MUX_MP4
}

@property (nonatomic, readonly, retain) NSArray *masterTrackArray;
@property (nonatomic, readonly) NSDictionary *noneTrack;
@property (nonatomic, retain) NSNumber *videoContainerTag;

- (void) setHBController: (id) aController;
- (void) prepareAudioForQueueFileJob: (NSMutableDictionary *) aDict;
- (void) prepareAudioForJob: (hb_job_t *) aJob;
- (void) prepareAudioForPreset: (NSMutableArray *) anArray;
- (void) addTracksFromQueue: (NSMutableDictionary *) aQueue;
- (void) addTracksFromPreset: (NSMutableDictionary *) aPreset;
- (void) addAllTracksFromPreset: (NSMutableDictionary *) aPreset;
- (BOOL) anyCodecMatches: (int) aCodecValue;
- (void) addNewAudioTrack;
- (void) settingTrackToNone: (HBAudio *) newNoneTrack;
- (void) switchingTrackFromNone: (HBAudio *) noLongerNoneTrack;

@end

@interface HBAudioController (KVC)

- (NSUInteger) countOfAudioArray;
- (HBAudio *) objectInAudioArrayAtIndex: (NSUInteger) index;
- (void) insertObject: (HBAudio *) audioObject inAudioArrayAtIndex: (NSUInteger) index;
- (void) removeObjectFromAudioArrayAtIndex: (NSUInteger) index;

@end
