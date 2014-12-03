/*  HBJob.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBPreset;

@class HBTitle;

@class HBVideo;
@class HBPicture;
@class HBFilters;

@class HBAudioDefaults;
@class HBSubtitlesDefaults;

typedef NS_ENUM(NSUInteger, HBJobStatus) {
    HBJobStatusNone,
    HBJobStatusWorking,
    HBJobStatusCompleted,
    HBJobStatusCanceled
};

/**
 * HBJob
 */
@interface HBJob : NSObject <NSCoding, NSCopying>

- (instancetype)initWithTitle:(HBTitle *)title url:(NSURL *)fileURL andPreset:(HBPreset *)preset;

@property (nonatomic, readonly) HBJobStatus status;

// libhb
@property (nonatomic, readonly) HBTitle *title;
@property (nonatomic, readonly) NSURL *fileURL;

// Old job format
@property (nonatomic, readwrite, retain) NSDictionary *jobDict;
@property (nonatomic, readonly) NSAttributedString *jobDescription;

// Job settings
@property (nonatomic, readwrite) int fileFormat;

@property (nonatomic, readwrite) BOOL mp4LargeFile;
@property (nonatomic, readwrite) BOOL mp4HttpOptimize;
@property (nonatomic, readwrite) BOOL mp4iPodCompatible;

@property (nonatomic, readonly) HBVideo *video;
@property (nonatomic, readonly) HBPicture *picture;
@property (nonatomic, readonly) HBFilters *filters;

// Defaults settings
@property (nonatomic, readonly) HBAudioDefaults *audioDefaults;
@property (nonatomic, readonly) HBSubtitlesDefaults *subtitlesDefaults;

@end
