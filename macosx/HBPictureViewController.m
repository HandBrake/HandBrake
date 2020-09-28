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
    }
    return self;
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

@end
