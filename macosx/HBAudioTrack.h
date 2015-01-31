/*  HBAudioTrack.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBAudio;
@protocol HBAudioTrackDataSource;

/**
 *  Audio track dicts keys.
 */
extern NSString *keyAudioTrackIndex;
extern NSString *keyAudioTrackName;
extern NSString *keyAudioInputBitrate;
extern NSString *keyAudioInputSampleRate;
extern NSString *keyAudioInputCodec;
extern NSString *keyAudioInputCodecParam;
extern NSString *keyAudioInputChannelLayout;
extern NSString *keyAudioTrackLanguageIsoCode;

extern NSString *keyAudioCodecName;
extern NSString *keyAudioSampleRateName;
extern NSString *keyAudioBitrateName;
extern NSString *keyAudioMixdownName;
extern NSString *keyAudioCodec;
extern NSString *keyAudioMixdown;
extern NSString *keyAudioSamplerate;
extern NSString *keyAudioBitrate;

@interface HBAudioTrack : NSObject <NSCoding, NSCopying>

@property (nonatomic, retain) NSDictionary *track;
@property (nonatomic, retain) NSDictionary *codec;
@property (nonatomic, retain) NSDictionary *mixdown;
@property (nonatomic, retain) NSDictionary *sampleRate;
@property (nonatomic, retain) NSDictionary *bitRate;
@property (nonatomic, retain) NSNumber *drc;
@property (nonatomic, retain) NSNumber *gain;
@property (nonatomic, retain) NSNumber *videoContainerTag;
@property (nonatomic, assign) id<HBAudioTrackDataSource> dataSource;

@property (nonatomic, retain) NSMutableArray *codecs;
@property (nonatomic, retain) NSMutableArray *mixdowns;
@property (nonatomic, readonly) NSArray *sampleRates;
@property (nonatomic, retain) NSArray *bitRates;
@property (nonatomic, readonly) BOOL enabled;

- (void) setTrackFromIndex: (int) aValue;
- (BOOL) setCodecFromName: (NSString *) aValue;
- (void) setMixdownFromName: (NSString *) aValue;
- (void) setSampleRateFromName: (NSString *) aValue;
- (void) setBitRateFromName: (NSString *) aValue;

@end

@protocol HBAudioTrackDataSource <NSObject>
- (NSDictionary *)noneTrack;
- (NSArray *)masterTrackArray;

- (void)settingTrackToNone:(HBAudioTrack *)newNoneTrack;
- (void)switchingTrackFromNone:(HBAudioTrack *)noLongerNoneTrack;
@end
