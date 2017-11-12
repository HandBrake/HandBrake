/*  HBSummaryViewController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBJob;
@class HBPreviewGenerator;

NS_ASSUME_NONNULL_BEGIN

@interface HBSummaryViewController : NSViewController

@property (nonatomic, readwrite, weak, nullable) HBJob *job;
@property (nonatomic, readwrite, weak, nullable) HBPreviewGenerator *generator;

@end

NS_ASSUME_NONNULL_END
