/*  HBAudioTrack.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBAudio;
@protocol HBAudioTrackDataSource;
@protocol HBAudioTrackDelegate;

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

@interface HBAudioTrack : NSObject <NSSecureCoding, NSCopying>

@property (nonatomic, strong) NSDictionary *track;
@property (nonatomic, strong) NSDictionary *codec;
@property (nonatomic, strong) NSDictionary *mixdown;
@property (nonatomic, strong) NSDictionary *sampleRate;
@property (nonatomic, strong) NSDictionary *bitRate;
@property (nonatomic, strong) NSNumber *drc;
@property (nonatomic, strong) NSNumber *gain;
@property (nonatomic, strong) NSNumber *videoContainerTag;
@property (nonatomic, unsafe_unretained) id<HBAudioTrackDataSource> dataSource;
@property (nonatomic, unsafe_unretained) id<HBAudioTrackDelegate> delegate;

@property (nonatomic, strong) NSMutableArray *codecs;
@property (nonatomic, strong) NSMutableArray *mixdowns;
@property (nonatomic, readonly) NSArray *sampleRates;
@property (nonatomic, strong) NSArray *bitRates;
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
@end

@protocol HBAudioTrackDelegate <NSObject>
- (void)settingTrackToNone:(HBAudioTrack *)newNoneTrack;
- (void)switchingTrackFromNone:(HBAudioTrack *)noLongerNoneTrack;
- (void)mixdownChanged;
@end
