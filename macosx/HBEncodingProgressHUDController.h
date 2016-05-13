/*  HBEncodingProgressHUDController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import "HBHUD.h"

@protocol HBEncodingProgressHUDControllerDelegate <NSObject>

- (void)cancelEncoding;

@end

@interface HBEncodingProgressHUDController : NSViewController <HBHUD>

@property (nonatomic, nullable, assign) id<HBEncodingProgressHUDControllerDelegate> delegate;

@property (nonatomic, nonnull) NSString *info;
@property (nonatomic) double progress;

@end
