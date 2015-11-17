/*  HBAudioTrack.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBAudioTrack;

NS_ASSUME_NONNULL_BEGIN

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

@protocol HBAudioTrackDataSource <NSObject>
- (NSDictionary *)noneTrack;
- (NSArray *)masterTrackArray;
@end

@protocol HBAudioTrackDelegate <NSObject>
- (void)settingTrackToNone:(HBAudioTrack *)newNoneTrack;
- (void)switchingTrackFromNone:(HBAudioTrack *)noLongerNoneTrack;
- (void)mixdownChanged;
@end

@interface HBAudioTrack : NSObject <NSSecureCoding, NSCopying>

@property (nonatomic, strong) NSDictionary *track;
@property (nonatomic, strong, nullable) NSDictionary *codec;
@property (nonatomic, strong) NSDictionary *mixdown;
@property (nonatomic, strong) NSDictionary *sampleRate;
@property (nonatomic, strong) NSDictionary *bitRate;
@property (nonatomic) double drc;
@property (nonatomic) double gain;
@property (nonatomic) int container;

@property (nonatomic, weak, nullable) id<HBAudioTrackDataSource> dataSource;
@property (nonatomic, weak, nullable) id<HBAudioTrackDelegate> delegate;

@property (nonatomic, readonly) NSArray *codecs;
@property (nonatomic, readonly) NSArray *mixdowns;
@property (nonatomic, readonly) NSArray *sampleRates;
@property (nonatomic, readonly) NSArray *bitRates;
@property (nonatomic, readonly) BOOL enabled;

- (void) setTrackFromIndex: (int) aValue;
- (BOOL) setCodecFromName: (NSString *) aValue;
- (void) setMixdownFromName: (NSString *) aValue;
- (void) setSampleRateFromName: (NSString *) aValue;
- (void) setBitRateFromName: (NSString *) aValue;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
