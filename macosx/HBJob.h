/*  HBJob.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

#import "HBTitle.h"

#import "HBRange.h"
#import "HBVideo.h"
#import "HBPicture.h"
#import "HBFilters.h"

#import "HBAudioDefaults.h"
#import "HBSubtitlesDefaults.h"

#include "hb.h"

@class HBPreset;

typedef NS_ENUM(NSUInteger, HBJobState) {
    HBJobStateReady,
    HBJobStateWorking,
    HBJobStateCompleted,
    HBJobStateCanceled
};

/**
 * HBJob
 */
@interface HBJob : NSObject <NSCoding, NSCopying>

- (instancetype)initWithTitle:(HBTitle *)title andPreset:(HBPreset *)preset;
- (void)applyPreset:(HBPreset *)preset;

/**
 *  Current state of the job.
 */
@property (nonatomic, readonly) HBJobState state;

@property (nonatomic, readonly) HBTitle *title;

// urls
@property (nonatomic, readonly) NSURL *fileURL;
@property (nonatomic, readonly) NSURL *destURL;

// Libhb job
@property (nonatomic, readonly) hb_job_t *hb_job;
@property (nonatomic, readonly) NSAttributedString *jobDescription;

// Job settings
@property (nonatomic, readwrite) int container;

@property (nonatomic, readwrite) BOOL mp4HttpOptimize;
@property (nonatomic, readwrite) BOOL mp4iPodCompatible;

@property (nonatomic, readonly) HBRange *range;
@property (nonatomic, readonly) HBVideo *video;
@property (nonatomic, readonly) HBPicture *picture;
@property (nonatomic, readonly) HBFilters *filters;

@property (nonatomic, readonly) NSMutableArray *audioTracks;
@property (nonatomic, readonly) NSMutableArray *subtitlesTracks;

@property (nonatomic, readwrite) BOOL chaptersEnabled;
@property (nonatomic, readonly) NSMutableArray *chapterTitles;

// Defaults settings
@property (nonatomic, readonly) HBAudioDefaults *audioDefaults;
@property (nonatomic, readonly) HBSubtitlesDefaults *subtitlesDefaults;

@end
