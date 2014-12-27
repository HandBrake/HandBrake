/*  HBVideoController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

@class HBController;
@class HBAdvancedController;

@class HBVideo;

/**
 *  HBVideoController
 */
@interface HBVideoController : NSViewController

- (instancetype)initWithAdvancedController:(HBAdvancedController *)advancedController;

/**
 *  Summaries of the pictures and filters settings
 */
@property (nonatomic, copy, readwrite) NSString *pictureSettings;
@property (nonatomic, copy, readwrite) NSString *pictureFilters;

@property (nonatomic, readwrite, assign) HBVideo *video;

@end
