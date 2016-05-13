/*  HBTitle.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class HBChapter;

/**
 * HBTitles is an interface to the low-level hb_title_t.
 * the properties are lazy-loaded.
 */
@interface HBTitle : NSObject

@property (nonatomic, readonly) NSString *name;
@property (nonatomic, readonly, getter=isFeatured) BOOL featured;
@property (nonatomic, readonly, getter=isStream) BOOL stream;

@property (nonatomic, readonly) NSURL *url;

@property (nonatomic, readonly) int index;
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

@property (nonatomic, readonly) NSArray<NSDictionary<NSString *, id> *> *audioTracks;
@property (nonatomic, readonly) NSArray<NSDictionary<NSString *, id> *> *subtitlesTracks;
@property (nonatomic, readonly) NSArray<HBChapter *> *chapters;

@end

NS_ASSUME_NONNULL_END
