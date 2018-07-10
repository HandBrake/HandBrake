/*  HBEncodingProgressHUDController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBEncodingProgressHUDController.h"

@interface HBEncodingProgressHUDController ()

@property (weak) IBOutlet NSProgressIndicator *progressIndicator;
@property (weak) IBOutlet NSTextField *infoLabel;

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
