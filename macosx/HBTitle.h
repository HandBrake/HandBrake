/*  HBTitle.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBSecurityAccessToken.h>

NS_ASSUME_NONNULL_BEGIN

@class HBChapter;

@interface HBMetadata : NSObject

@property (nonatomic, readonly, nullable) NSString *name;
@property (nonatomic, readonly, nullable) NSString *artist;
@property (nonatomic, readonly, nullable) NSString *composer;
@property (nonatomic, readonly, nullable) NSString *releaseDate;
@property (nonatomic, readonly, nullable) NSString *comment;
@property (nonatomic, readonly, nullable) NSString *album;
@property (nonatomic, readonly, nullable) NSString *albumArtist;
@property (nonatomic, readonly, nullable) NSString *genre;
@property (nonatomic, readonly, nullable) NSString *shortDescription;
@property (nonatomic, readonly, nullable) NSString *longDescription;

@end

@interface HBTitleAudioTrack : NSObject<NSSecureCoding>

- (instancetype)initWithDisplayName:(NSString *)displayName;

@property (nonatomic, readonly) int index;
@property (nonatomic, readonly) NSString *displayName;
@property (nonatomic, readonly, nullable) NSString *title;

@property (nonatomic, readonly) int bitRate;
@property (nonatomic, readonly) int sampleRate;
@property (nonatomic, readonly) int codec;
@property (nonatomic, readonly) int codecParam;
@property (nonatomic, readonly) uint64_t channelLayout;

@property (nonatomic, readonly) NSString *isoLanguageCode;

@end

@interface HBTitleSubtitlesTrack : NSObject<NSSecureCoding, HBSecurityScope>

- (instancetype)initWithDisplayName:(NSString *)displayName type:(int)type fileURL:(nullable NSURL *)fileURL;

@property (nonatomic, readonly) int index;
@property (nonatomic, readonly) NSString *displayName;
@property (nonatomic, readonly, nullable) NSString *title;

@property (nonatomic, readonly) int type;
@property (nonatomic, readonly) NSString *isoLanguageCode;

@property (nonatomic, readonly, nullable) NSURL *fileURL;

@end

/**
 * HBTitles is an interface to the low-level hb_title_t.
 * the properties are lazy-loaded.
 */
@interface HBTitle : NSObject

@property (nonatomic, readonly) NSString *name;
@property (nonatomic, readonly) NSString *shortFormatDescription;
@property (nonatomic, readonly, getter=isFeatured) BOOL featured;
@property (nonatomic, readonly, getter=isStream) BOOL stream;

@property (nonatomic, readonly) NSURL *url;

@property (nonatomic, readonly) int index;
@property (nonatomic, readonly) BOOL keepDuplicateTitles;
@property (nonatomic, readonly) int angles;
@property (nonatomic, readonly) int duration;
@property (nonatomic, readonly) int frames;

@property (nonatomic, readonly) NSString *timeCode;

@property (nonatomic, readonly) int width;
@property (nonatomic, readonly) int height;

@property (nonatomic, readonly) int parWidth;
@property (nonatomic, readonly) int parHeight;

@property (nonatomic, readonly) int autoCropTop;
@property (nonatomic, readonly) int autoCropBottom;
@property (nonatomic, readonly) int autoCropLeft;
@property (nonatomic, readonly) int autoCropRight;

@property (nonatomic, readonly) int looseAutoCropTop;
@property (nonatomic, readonly) int looseAutoCropBottom;
@property (nonatomic, readonly) int looseAutoCropLeft;
@property (nonatomic, readonly) int looseAutoCropRight;

@property (nonatomic, readonly) NSArray<HBTitleAudioTrack *> *audioTracks;
@property (nonatomic, readonly) NSArray<HBTitleSubtitlesTrack *> *subtitlesTracks;
@property (nonatomic, readonly) NSArray<HBChapter *> *chapters;

@property (nonatomic, readonly) HBMetadata *metadata;

@end

NS_ASSUME_NONNULL_END
