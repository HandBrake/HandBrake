/*  HBEncodingProgressHUDController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBEncodingProgressHUDController.h"

@interface HBEncodingProgressHUDController ()

@property (nonatomic, weak) IBOutlet NSProgressIndicator *progressIndicator;
@property (nonatomic, weak) IBOutlet NSTextField *infoLabel;

@end

@implementation HBEncodingProgressHUDController

- (NSString *)nibName
{
    return @"HBEncodingProgressHUDController";
}

- (BOOL)canBeHidden
{
    return NO;
}

- (void)setInfo:(NSString *)info
{
    self.infoLabel.stringValue = info;
}

- (void)setProgress:(double)progress
{
    self.progressIndicator.doubleValue = progress;
}

- (IBAction)cancelEncoding:(id)sender
{
    [self.delegate cancelEncoding];
}

- (BOOL)HB_keyDown:(NSEvent *)event
{
    return NO;
}

- (BOOL)HB_scrollWheel:(NSEvent *)theEvent
{
    return NO;
}

@end

@interface HBEncodingProgressHUDController (TouchBar) <NSTouchBarProvider, NSTouchBarDelegate>
@end

@implementation HBEncodingProgressHUDController (TouchBar)

@dynamic touchBar;

static NSTouchBarItemIdentifier HBTouchBarCancelEncoding = @"fr.handbrake.cancelEncoding";

- (NSTouchBar *)makeTouchBar
{
    NSTouchBar *bar = [[NSTouchBar alloc] init];
    bar.delegate = self;

    bar.escapeKeyReplacementItemIdentifier = HBTouchBarCancelEncoding;

    return bar;
}

- (NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
    if ([identifier isEqualTo:HBTouchBarCancelEncoding])
    {
        NSCustomTouchBarItem *item = [[NSCustomTouchBarItem alloc] initWithIdentifier:identifier];
        item.customizationLabel = NSLocalizedString(@"Cancel Encoding", @"Touch bar");

        NSButton *button = [NSButton buttonWithTitle:NSLocalizedString(@"Cancel", @"Touch bar") target:self action:@selector(cancelEncoding:)];
        button.bezelColor = NSColor.systemRedColor;

        item.view = button;
        return item;
    }

    return nil;
}

@end
