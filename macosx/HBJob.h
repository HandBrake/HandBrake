/*  HBJob.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBPreset;
@class HBMutablePreset;
@class HBTitle;

#import "HBRange.h"
#import "HBVideo.h"
#import "HBPicture.h"
#import "HBFilters.h"

#import "HBAudio.h"
#import "HBSubtitles.h"
#import "HBChapter.h"

NS_ASSUME_NONNULL_BEGIN

extern NSString *HBContainerChangedNotification;
extern NSString *HBChaptersChangedNotification;

/**
 * HBJob
 */
@interface HBJob : NSObject <NSSecureCoding, NSCopying, HBPresetCoding>

- (instancetype)initWithTitle:(HBTitle *)title andPreset:(HBPreset *)preset;

- (void)applyPreset:(HBPreset *)preset;

@property (nonatomic, readwrite, weak, nullable) HBTitle *title;
@property (nonatomic, readonly) int titleIdx;

@property (nonatomic, readwrite, copy) NSString *presetName;

/// The file URL of the source.
@property (nonatomic, readonly) NSURL *fileURL;

/// The file URL at which the new file will be created.
@property (nonatomic, readwrite, copy, nullable) NSURL *outputURL;

/// The name of the new file that will be created.
@property (nonatomic, readwrite, copy, nullable) NSString *outputFileName;

/// The URL at which the new file will be created.
@property (nonatomic, readonly, nullable) NSURL *completeOutputURL;

// Job settings
@property (nonatomic, readwrite) int container;
@property (nonatomic, readwrite) int angle;

// Container options
@property (nonatomic, readwrite) BOOL mp4HttpOptimize;
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

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@end

NS_ASSUME_NONNULL_END
