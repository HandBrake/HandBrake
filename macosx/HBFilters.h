/*  HBFilters.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBPresetCoding.h>
#import <HandBrakeKit/HBFilter.h>

NS_ASSUME_NONNULL_BEGIN

extern NSString * const HBFiltersChangedNotification;

@interface HBFilterGroup : NSObject

@property (nonatomic, readonly, nullable) NSString *name;
@property (nonatomic, readonly) NSArray<NSNumber *> *filters;

@end

/**
 *  Filters settings.
 */
@interface HBFilters : NSObject <HBFilterDelegate, NSSecureCoding, NSCopying>

@property (nonatomic, readonly) NSMutableArray<HBFilter *> *filters;

- (void)addFilterWithID:(int)filterID;

- (BOOL)containsFilterWithIDs:(NSArray<NSNumber *> *)filterIDs;
- (BOOL)containsFilterWithID:(int)filterID;

- (void)removeFilterWithID:(int)filterID;

- (nullable HBFilterGroup *)groupWithFilterID:(int)filterID;

- (void)removeAll;

@property (class, nonatomic, readonly) NSArray<NSNumber *> *availableFilters;
@property (class, nonatomic, readonly) NSArray<HBFilterGroup *> *availableFilterGroups;

@property (nonatomic, readwrite, weak, nullable) NSUndoManager *undo;

@property (nonatomic, readonly) NSUInteger countOfFilters;
- (HBFilter *)objectInFiltersAtIndex:(NSUInteger)index;
- (void)insertObject:(HBFilter *)filter inFiltersAtIndex:(NSUInteger)index;
- (void)removeObjectFromFiltersAtIndex:(NSUInteger)index;
- (void)removeFiltersAtIndexes:(NSIndexSet *)indexes;

@end

NS_ASSUME_NONNULL_END
