/*  HBJob.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

#import "HBPreset.h"
#import "HBTitle.h"

#import "HBRange.h"
#import "HBVideo.h"
#import "HBPicture.h"
#import "HBFilters.h"

#import "HBAudio.h"
#import "HBSubtitles.h"

extern NSString *HBMixdownChangedNotification;
extern NSString *HBContainerChangedNotification;

/**
 *  A flag to indicate the job's state
 */
typedef NS_ENUM(NSUInteger, HBJobState){
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
- (void)applyCurrentSettingsToPreset:(NSMutableDictionary *)dict;

/**
 *  Current state of the job.
 */
@property (nonatomic, readwrite) HBJobState state;

@property (nonatomic, readwrite, copy) NSString *presetName;

@property (nonatomic, readwrite, assign) HBTitle *title;
@property (nonatomic, readonly) int titleIdx;
@property (nonatomic, readonly) NSString *uuid;

/**
 *  The file URL of the source.
 */
@property (nonatomic, readonly) NSURL *fileURL;

/**
 *  The file URL at which the new file will be created.
 */
@property (nonatomic, readwrite, copy) NSURL *destURL;

// Job settings
@property (nonatomic, readwrite) int container;
@property (nonatomic, readwrite) int angle;

@property (nonatomic, readwrite) BOOL mp4HttpOptimize;
@property (nonatomic, readwrite) BOOL mp4iPodCompatible;

@property (nonatomic, readonly) HBRange *range;
@property (nonatomic, readonly) HBVideo *video;
@property (nonatomic, readonly) HBPicture *picture;
@property (nonatomic, readonly) HBFilters *filters;

@property (nonatomic, readonly) HBAudio *audio;
@property (nonatomic, readonly) HBSubtitles *subtitles;

@property (nonatomic, readwrite) BOOL chaptersEnabled;
@property (nonatomic, readonly) NSMutableArray *chapterTitles;

@end
