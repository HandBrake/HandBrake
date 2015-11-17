/*  HBSubtitlesTrack.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class HBSubtitlesTrack;

/**
 *  HBTrackDataSource
 */
@protocol HBTrackDataSource <NSObject>
- (NSDictionary<NSString *, id> *)sourceTrackAtIndex:(NSUInteger)idx;
- (NSArray<NSString *> *)sourceTracksArray;
@end

/**
 * HBTrackDelegate
 */
@protocol HBTrackDelegate <NSObject>
- (void)track:(HBSubtitlesTrack *)track didChangeSourceFrom:(NSUInteger)oldSourceIdx;

- (BOOL)canSetBurnedInOption:(HBSubtitlesTrack *)track;
- (void)didSetBurnedInOption:(HBSubtitlesTrack *)track;

- (void)didSetDefaultOption:(HBSubtitlesTrack *)track;
@end

@interface HBSubtitlesTrack : NSObject <NSSecureCoding, NSCopying>

- (instancetype)initWithTrackIdx:(NSUInteger)index
                       container:(int)container
                      dataSource:(id<HBTrackDataSource>)dataSource
                        delegate:(id<HBTrackDelegate>)delegate;

/// The index of the source in the data source tracks array.
@property (nonatomic, readonly) NSUInteger sourceTrackIdx;
/// Format.
@property (nonatomic, readonly) int type;
@property (nonatomic, readwrite) int container;

/// Whether to use only the forced subtitles of the track.
@property (nonatomic, readwrite) BOOL forcedOnly;
/// Whether the track should be burned.
@property (nonatomic, readwrite) BOOL burnedIn;
/// Whether is the default track.
@property (nonatomic, readwrite) BOOL def;

/// The URL of the external subtitles file.
@property (nonatomic, readwrite, copy, nullable) NSURL *fileURL;
/// The ISO 639/2 language code of the external subtitles file.
@property (nonatomic, readwrite, copy, nullable) NSString *isoLanguage;
/// The character encoding of the external subtitles file.
@property (nonatomic, readwrite, copy, nullable) NSString *charCode;
/// The offset in milliseconds  of the external subtitles file.
@property (nonatomic, readwrite) int offset;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@property (nonatomic, readwrite, weak) id<HBTrackDataSource> dataSource;
@property (nonatomic, readwrite, weak) id<HBTrackDelegate> delegate;

/// A complete list of the possible languages.
- (NSArray<NSString *> *)languages;
/// A complete list of the possible encodings.
- (NSArray<NSString *> *)encodings;

@property (nonatomic, readonly) BOOL isSrt;
@property (nonatomic, readonly) BOOL isEnabled;
@property (nonatomic, readonly) BOOL canPassthru;

@end

/**
 * HBIsoLanguageTrasformer is a trasformer to transform
 * a ISO 639/2 code to a human readable language name.
 */
@interface HBIsoLanguageTrasformer : NSValueTransformer
@end

NS_ASSUME_NONNULL_END
