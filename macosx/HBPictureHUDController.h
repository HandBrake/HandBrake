/*  HBPictureHUDController.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */
//

#import <Cocoa/Cocoa.h>

#import "HBHUD.h"
#import "HBPreviewGenerator.h"

NS_ASSUME_NONNULL_BEGIN

@protocol HBPictureHUDControllerDelegate <NSObject>

- (void)displayPreviewAtIndex:(NSUInteger)idx;

- (void)setScaleToScreen:(BOOL)scaleToscreen;

- (void)showCroppingSettings:(id)sender;

- (void)createMoviePreviewWithPictureIndex:(NSUInteger)index duration:(NSUInteger)duration;

@end

@interface HBPictureHUDController : NSViewController <HBHUD>

@property (nonatomic, nullable, assign) id<HBPictureHUDControllerDelegate> delegate;

@property (nonatomic, copy) NSString *info;
@property (nonatomic, copy) NSString *scale;

@property (nonatomic, weak) HBPreviewGenerator *generator;
@property (nonatomic) NSUInteger selectedIndex;

@end

NS_ASSUME_NONNULL_END
