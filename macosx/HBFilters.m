/*  HBFilters.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilters.h"
#import "HBFilter.h"
#import "HBCodingUtilities.h"
#import "HBMutablePreset.h"
#import "HBJob+Private.h"

#include "handbrake/handbrake.h"

NSString * const HBFiltersChangedNotification = @"HBFiltersChangedNotification";

@implementation HBFilterGroup : NSObject

- (instancetype)initWithFilterID:(int)filterID
{
    self = [super init];
    if (self)
    {
        _filters = @[@(filterID)];
    }
    return self;
}

- (instancetype)initWithFilters:(NSArray<NSNumber *> *)filters groupName:(NSString *)name
{
    self = [super init];
    if (self)
    {
        _name = [name copy];
        _filters = [filters copy];
    }
    return self;
}

@end


@interface HBFilters ()

@property (nonatomic, readwrite, getter=areNotificationsEnabled) BOOL notificationsEnabled;

@end

@implementation HBFilters

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _filters = [[NSMutableArray alloc] init];
        _notificationsEnabled = YES;
    }
    return self;
}

- (void)addFilterWithID:(int)filterID
{
    for (HBFilter *filter in self.filters)
    {
        if (filter.filterID == filterID)
        {
            return;
        }
    }

    NSUInteger idx = 0;
    for (HBFilter *filter in self.filters)
    {
        if (filter.filterID > filterID)
        {
            break;
        }
        else
        {
            idx++;
        }
    }

    HBFilter *filter = [[HBFilter alloc] initWithFilterID:filterID];
    filter.delegate = self;
    filter.undo = self.undo;
    filter.notificationsEnabled = self.notificationsEnabled;

    [self insertObject:filter inFiltersAtIndex:idx];
}

- (BOOL)containsFilterWithIDs:(NSArray<NSNumber *> *)filterIDs
{
    for (HBFilter *filter in self.filters)
    {
        for (NSNumber *filterID in filterIDs)
        {
            if (filter.filterID == filterID.intValue)
            {
                return YES;
            }
        }
    }
    return NO;
}

- (BOOL)containsFilterWithID:(int)filterID
{
    for (HBFilter *filter in self.filters)
    {
        if (filter.filterID == filterID)
        {
            return YES;
        }
    }
    return NO;
}

- (void)removeFilter:(nonnull HBFilter *)filterToRemove
{
    NSUInteger idx = 0;
    for (HBFilter *filter in self.filters)
    {
        if ([filter isEqualTo:filterToRemove])
        {
            [self removeObjectFromFiltersAtIndex:idx];
            break;
        }
        idx++;
    }
}

- (void)removeFilterWithID:(int)filterID
{
    NSUInteger idx = 0;
    for (HBFilter *filter in self.filters)
    {
        if (filter.filterID == filterID)
        {
            [self removeObjectFromFiltersAtIndex:idx];
            break;
        }
        idx++;
    }
}

- (void)removeAll
{
    [self removeFiltersAtIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, self.filters.count)]];
}

- (NSArray<NSNumber *> *)availableFilters
{
    return @[];
}

- (NSArray<HBFilterGroup *> *)availableFilterGroups
{
    return @[];
}

- (HBFilterGroup *)groupWithFilterID:(int)filterID
{
    for (HBFilterGroup *group in self.availableFilterGroups)
    {
        for (NSNumber *groupFilterID in group.filters)
        {
            if (groupFilterID.intValue == filterID)
            {
                return group;
            }
        }
    }
    return nil;
}

- (void)setUndo:(NSUndoManager *)undo
{
    _undo = undo;
    for (HBFilter *filter in self.filters)
    {
        filter.undo = undo;
    }
}

- (void)setNotificationsEnabled:(BOOL)notificationsEnabled
{
    _notificationsEnabled = notificationsEnabled;

    for (HBFilter *filter in self.filters)
    {
        filter.notificationsEnabled = notificationsEnabled;
    }
}

- (void)postChangedNotification
{
    if (self.areNotificationsEnabled)
    {
        [NSNotificationCenter.defaultCenter postNotification:[NSNotification notificationWithName:HBFiltersChangedNotification
                                                                                           object:self userInfo:nil]];
    }
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBFilters *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_filters = [[NSMutableArray alloc] init];

        for (HBFilter *filter in _filters)
        {
            HBFilter *filterCopy = [filter copy];
            [copy->_filters addObject:filterCopy];
        }
    }

    return copy;
}

#pragma mark - NSCoding

+ (BOOL)supportsSecureCoding
{
    return YES;
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:3 forKey:@"HBFiltersVersion"];
    encodeObject(_filters);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeCollectionOfObjectsOrFail(_filters, NSMutableArray, HBFilter);

    _notificationsEnabled = YES;
    for (HBFilter *filter in _filters)
    {
        filter.delegate = self;
        filter.notificationsEnabled = YES;
    }

    return self;

fail:
    return nil;
}

#pragma mark - Presets

- (void)writeToPreset:(HBMutablePreset *)preset {}

- (void)applyPreset:(HBPreset *)preset jobSettings:(NSDictionary *)settings {}

#pragma mark - KVC

- (NSUInteger)countOfFilters
{
    return self.filters.count;
}

- (HBFilter *)objectInFiltersAtIndex:(NSUInteger)index
{
    return self.filters[index];
}

- (void)insertObject:(HBFilter *)filter inFiltersAtIndex:(NSUInteger)index
{
    [[self.undo prepareWithInvocationTarget:self] removeObjectFromFiltersAtIndex:index];
    [self.filters insertObject:filter atIndex:index];
    [self postChangedNotification];
}

- (void)insertFilters:(NSArray<HBFilter *> *)array atIndexes:(NSIndexSet *)indexes
{
    [[self.undo prepareWithInvocationTarget:self] removeFiltersAtIndexes:indexes];
    [self.filters insertObjects:array atIndexes:indexes];
    [self postChangedNotification];
}

- (void)removeObjectFromFiltersAtIndex:(NSUInteger)index
{
    HBFilter *filter = self.filters[index];
    [[self.undo prepareWithInvocationTarget:self] insertObject:filter inFiltersAtIndex:index];
    [self.filters removeObjectAtIndex:index];
    [self postChangedNotification];
}

- (void)removeFiltersAtIndexes:(NSIndexSet *)indexes
{
    NSArray<HBFilter *> *filters = [self.filters objectsAtIndexes:indexes];
    [[self.undo prepareWithInvocationTarget:self] insertFilters:filters atIndexes:indexes];
    [self.filters removeObjectsAtIndexes:indexes];
    [self postChangedNotification];
}

@end
