/*  HBRange.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

typedef NS_ENUM(NSUInteger, HBRangeType) {
    HBRangeTypeChapters,
    HBRangeTypeFrames,
    HBRangeTypeSeconds,
};

@interface HBRange : NSObject <NSCoding>

@property (nonatomic, readwrite) HBRangeType type;

@property (nonatomic, readwrite) NSInteger chapterStart;
@property (nonatomic, readwrite) NSInteger chapterStop;

@property (nonatomic, readwrite) NSInteger frameStart;
@property (nonatomic, readwrite) NSInteger frameStop;

@property (nonatomic, readwrite) NSInteger secondsStart;
@property (nonatomic, readwrite) NSInteger secondsStop;

@end
