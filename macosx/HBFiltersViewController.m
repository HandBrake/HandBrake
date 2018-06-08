/*  HBPictureViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFiltersViewController.h"

@import HandBrakeKit;

@interface HBFiltersViewController ()

@property (nonatomic, readwrite) NSColor *labelColor;

@end

@implementation HBFiltersViewController

- (instancetype)init
{
    self = [super initWithNibName:@"HBFiltersViewController" bundle:nil];
    if (self)
    {
        _labelColor = [NSColor disabledControlTextColor];
    }
    return self;
}

- (void)setFilters:(HBFilters *)filters
{
    _filters = filters;

    if (_filters)
    {
        self.labelColor = [NSColor controlTextColor];
    }
    else
    {
        self.labelColor = [NSColor disabledControlTextColor];
    }

}

@end
