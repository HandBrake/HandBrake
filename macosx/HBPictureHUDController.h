/*  HBPictureHUDController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */
//

#import <Cocoa/Cocoa.h>
#import "HBHUD.h"

@protocol HBPictureHUDControllerDelegate <NSObject>

- (void)displayPreviewAtIndex:(NSUInteger)idx;
- (void)toggleScaleToScreen;
- (void)showPictureSettings;
- (void)createMoviePreviewWithPictureIndex:(NSUInteger)index duration:(NSUInteger)duration;

@end

@interface HBPictureHUDController : NSViewController <HBHUD>

@property (nonatomic, nullable, assign) id<HBPictureHUDControllerDelegate> delegate;

@property (nonatomic, nonnull) NSString *info;
@property (nonatomic, nonnull) NSString *scale;

@property (nonatomic) NSUInteger pictureCount;
@property (nonatomic) NSUInteger selectedIndex;

@end
