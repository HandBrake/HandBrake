/*  HBRange.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBTitle;

typedef NS_ENUM(NSUInteger, HBRangeType) {
    HBRangeTypeChapters,
    HBRangeTypeFrames,
    HBRangeTypeSeconds,
};

@interface HBRange : NSObject <NSCoding>

- (instancetype)initWithTitle:(HBTitle *)title;

@property (nonatomic, readwrite) HBRangeType type;

@property (nonatomic, readwrite) int chapterStart;
@property (nonatomic, readwrite) int chapterStop;

@property (nonatomic, readwrite) int frameStart;
@property (nonatomic, readwrite) int frameStop;

@property (nonatomic, readwrite) int secondsStart;
@property (nonatomic, readwrite) int secondsStop;

@property (nonatomic, readonly) NSString *duration;

@property (nonatomic, readwrite, assign) HBTitle *title;

@end
