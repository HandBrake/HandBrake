/*  HBPictureViewController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBVideoFilters;

NS_ASSUME_NONNULL_BEGIN

@interface HBFiltersViewController : NSViewController

@property (nonatomic, readwrite, weak) HBVideoFilters *filters;

@end

NS_ASSUME_NONNULL_END

