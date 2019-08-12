/*  HBPictureViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPictureViewController.h"

@import HandBrakeKit;

static void *HBPictureViewControllerContext = &HBPictureViewControllerContext;

@interface HBPictureViewController ()

@property (nonatomic, weak) IBOutlet NSStepper *widthStepper;
@property (nonatomic, weak) IBOutlet NSStepper *heightStepper;

@property (nonatomic, readwrite) NSColor *labelColor;

@end

@implementation HBPictureViewController

- (instancetype)init
{
    self = [super initWithNibName:@"HBPictureViewController" bundle:nil];
    if (self)
    {
        _labelColor = [NSColor disabledControlTextColor];
        [self addObserver:self forKeyPath:@"self.picture.modulus" options:NSKeyValueObservingOptionInitial context:HBPictureViewControllerContext];
    }
    return self;
}

- (void)dealloc
{
    @try
    {
        [self removeObserver:self forKeyPath:@"self.picture.modulus" context:HBPictureViewControllerContext];
    }
    @catch (NSException * __unused exception) {}
}

- (void)setPicture:(HBPicture *)picture
{
    _picture = picture;

    if (_picture)
    {
        self.labelColor = [NSColor controlTextColor];
    }
    else
    {
        self.labelColor = [NSColor disabledControlTextColor];
    }

}

#pragma mark - KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBPictureViewControllerContext)
    {
        // Set the increment here, it's not possible with bindings.
        if ([keyPath isEqualToString:@"self.picture.modulus"])
        {
            [self.widthStepper setIncrement:self.picture.modulus];
            [self.heightStepper setIncrement:self.picture.modulus];
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}


@end
