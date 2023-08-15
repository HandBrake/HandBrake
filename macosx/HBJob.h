/*  HBJob.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBPreset, HBMutablePreset;
@class HBTitle;
@class HBVideo;
@class HBRange;
@class HBPicture;
@class HBFilters;
@class HBAudio;
@class HBSubtitles;
@class HBChapter;

#import <HandBrakeKit/HBPresetCoding.h>
#import <HandBrakeKit/HBSecurityAccessToken.h>

NS_ASSUME_NONNULL_BEGIN

extern NSString *HBContainerChangedNotification;
extern NSString *HBChaptersChangedNotification;

typedef NS_ENUM(NSUInteger, HBJobHardwareDecoderUsage) {
    HBJobHardwareDecoderUsageNone,
    HBJobHardwareDecoderUsageAlways,
    HBJobHardwareDecoderUsageFullPathOnly
};

/**
 * HBJob
 */
@interface HBJob : NSObject <NSSecureCoding, NSCopying, HBPresetCoding, HBSecurityScope>

- (nullable instancetype)initWithTitle:(HBTitle *)title preset:(HBPreset *)preset;

@property (nonatomic, readwrite, weak, nullable) HBTitle *title;
@property (nonatomic, readonly) int titleIdx;

// Whether the source is a single file or a DVD-Video/Blu-ray
@property (nonatomic, readonly, getter=isStream) BOOL stream;

@property (nonatomic, readwrite, copy) NSString *presetName;

/// The file URL of the source.
@property (nonatomic, readonly) NSURL *fileURL;

/// The file URL at which the new file will be created.
@property (nonatomic, readwrite, copy, nullable) NSURL *destinationFolderURL;

/// The name of the new file that will be created.
@property (nonatomic, readwrite, copy, nullable) NSString *destinationFileName;

/// The URL at which the new file will be created.
@property (nonatomic, readonly, nullable) NSURL *destinationURL;

// Job settings
@property (nonatomic, readwrite) int container;
@property (nonatomic, readwrite) int angle;

// Container options
@property (nonatomic, readwrite) BOOL optimize;
@property (nonatomic, readwrite) BOOL mp4iPodCompatible;
@property (nonatomic, readwrite) BOOL alignAVStart;

@property (nonatomic, readonly) HBRange *range;
@property (nonatomic, readonly) HBVideo *video;
@property (nonatomic, readonly) HBPicture *picture;
@property (nonatomic, readonly) HBFilters *filters;

@property (nonatomic, readonly) HBAudio *audio;
@property (nonatomic, readonly) HBSubtitles *subtitles;

@property (nonatomic, readwrite) BOOL chaptersEnabled;
@property (nonatomic, readonly) NSArray<HBChapter *> *chapterTitles;

@property (nonatomic, readwrite) BOOL metadataPassthru;
@property (nonatomic, readwrite) HBJobHardwareDecoderUsage hwDecodeUsage;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
