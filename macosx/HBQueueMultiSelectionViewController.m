/*  HBQueueEmptyViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueMultiSelectionViewController.h"

@interface HBQueueMultiSelectionViewController ()

@property (nonatomic, weak) IBOutlet NSTextField *label;

@end

@implementation HBQueueMultiSelectionViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self updateLabels];
}

- (void)updateLabels
{
    if (self.count == 0)
    {
        self.label.stringValue = NSLocalizedString(@"No job selected", @"");
    }
    else
    {
        self.label.stringValue = [NSString stringWithFormat:NSLocalizedString(@"%lu jobs selected", @""), (unsigned long)self.count];
    }
}

- (void)setCount:(NSUInteger)count
{
    _count = count;
    [self updateLabels];
}

@end
