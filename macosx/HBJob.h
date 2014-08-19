/*  HBJob.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

#include "hb.h"

@class HBPreset;

@class HBVideo;
@class HBPicture;
@class HBFilters;

@class HBAudioSettings;
@class HBSubtitlesSettings;

/**
 * HBJob
 */
@interface HBJob : NSObject <NSCoding, NSCopying>

- (instancetype)initWithTitle:(hb_title_t *)title url:(NSURL *)fileURL andPreset:(HBPreset *)preset;

// libhb
@property (nonatomic, readonly) hb_title_t *title;
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
@property (nonatomic, readonly) HBAudioSettings *audioSettings;
@property (nonatomic, readonly) HBSubtitlesSettings *subtitlesSettings;

// File resources
@property (nonatomic, readonly) NSMutableArray *audioTracks;
@property (nonatomic, readonly) NSMutableArray *subtitlesTracks;
@property (nonatomic, readonly) NSMutableArray *chapters;

@end
