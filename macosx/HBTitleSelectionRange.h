/* HBTitleSelectionRange.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@import HandBrakeKit;

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, HBTitleSelectionRangeType)
{
    HBTitleSelectionRangeTypeChapters,
    HBTitleSelectionRangeTypeSeconds,
    HBTitleSelectionRangeTypeFrames
};

typedef NS_ENUM(NSUInteger, HBTitleSelectionTrimType)
{
    HBTitleSelectionTrimTypeNormal,
    HBTitleSelectionTrimTypeEnd
};

@interface HBTitleSelectionRange : NSObject <NSCopying>

- (instancetype)initWithTitles:(NSArray<HBTitle *> *)titles;

@property (nonatomic) HBTitleSelectionRangeType type;
@property (nonatomic) HBTitleSelectionTrimType trim;

@property (nonatomic) int chapterStart;
@property (nonatomic) int chapterStop;

@property (nonatomic) int frameStart;
@property (nonatomic) int frameStop;

@property (nonatomic) int secondsStart;
@property (nonatomic) int secondsStop;

@property (nonatomic, weak, nullable) NSUndoManager *undo;

@end

@interface HBTitleSelectionRange (UIAdditions)

@property (nonatomic, readonly) NSArray<NSString *> *chapters;
@property (nonatomic, readonly) NSArray<NSString *> *types;

@property (nonatomic, readonly) BOOL chaptersSelected;
@property (nonatomic, readonly) BOOL secondsSelected;
@property (nonatomic, readonly) BOOL framesSelected;

@end

@interface HBJob (HBTitleSelectionRangeAdditions)

- (void)applySelectionRange:(nullable HBTitleSelectionRange *)range;

@end

NS_ASSUME_NONNULL_END
