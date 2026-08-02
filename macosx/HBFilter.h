/*  HBFilter.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <HandBrakeKit/HBPresetCoding.h>

@class HBFilter;

NS_ASSUME_NONNULL_BEGIN

@protocol HBFilterDelegate <NSObject>
- (void)removeFilter:(HBFilter *)filter;
@end

@interface HBFilter : NSObject <NSSecureCoding, NSCopying, HBPresetCoding>

@property (nonatomic, readonly) int filterID;
@property (nonatomic, copy) NSString *preset;
@property (nonatomic, copy) NSString *tune;
@property (nonatomic, copy) NSString *custom;

@property (nonatomic, weak, nullable) id<HBFilterDelegate> delegate;
@property (nonatomic, weak, nullable) NSUndoManager *undo;

- (instancetype)initWithFilterID:(int)filterID;

- (instancetype)initWithFilter:(NSString *)filter
                        preset:(NSString *)preset
                          tune:(nullable NSString *)tune
                        custom:(nullable NSString *)custom;

@end

NS_ASSUME_NONNULL_END
