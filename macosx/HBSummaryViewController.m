/*  HBSummaryViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSummaryViewController.h"
#import "HBPreviewView.h"
#import "HBPreviewGenerator.h"

@import HandBrakeKit;

@interface HBSummaryViewController ()

@property (strong) IBOutlet HBPreviewView *previewView;

@end

@implementation HBSummaryViewController

- (void)loadView {
    [super loadView];
    self.previewView.showShadow = NO;
}

- (void)setGenerator:(HBPreviewGenerator *)generator
{
    _generator = generator;

    if (generator)
    {
        NSUInteger index = generator.imagesCount > 1 ? 1 : 0;
        CGImageRef fPreviewImage = [generator copyImageAtIndex:index shouldCache:NO];
        self.previewView.image = fPreviewImage;
        CFRelease(fPreviewImage);
    }
    else
    {
        self.previewView.image = nil;
    }
}


@end
