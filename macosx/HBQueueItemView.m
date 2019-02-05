/*  HBQueueItemView.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueItemView.h"
#import "HBJob.h"
#import "HBJob+UIAdditions.h"

@interface HBQueueItemView ()

@property (nonatomic, weak) IBOutlet NSButton *removeButton;
@property (nonatomic, weak) IBOutlet NSButton *expandButton;

@end

@implementation HBQueueItemView

- (void)setJob:(HBJob *)job
{
    _job = job;
    [self reload];
}

- (void)reload
{
    [self HB_updateLabel];
    [self HB_updateState];
    [self HB_updateRightButton];

    self.removeButton.target = self;
    self.expandButton.target = self;
    self.expandButton.action = @selector(toggleHeight:);
}

- (void)HB_updateLabel
{
    if (_expanded)
    {
        self.textField.attributedStringValue = self.job.attributedDescription;
        self.expandButton.state = NSOnState;
    }
    else
    {
        self.textField.attributedStringValue = self.job.attributedTitleDescription;
        self.expandButton.state = NSOffState;
    }
}

- (void)HB_updateState
{
    NSImage *state = nil;
    switch (self.job.state) {
        case HBJobStateCompleted:
            state = [NSImage imageNamed:@"EncodeComplete"];
            break;
        case HBJobStateWorking:
            state = [NSImage imageNamed:@"EncodeWorking0"];
            break;
        case HBJobStateCanceled:
            state = [NSImage imageNamed:@"EncodeCanceled"];
            break;
        case HBJobStateFailed:
            state = [NSImage imageNamed:@"EncodeFailed"];
            break;
        default:
            state = [NSImage imageNamed:@"JobSmall"];
            break;
    }

    self.imageView.image = state;
}

- (void)HB_updateRightButton
{
    if (self.job.state == HBJobStateCompleted)
    {
        [_removeButton setAction: @selector(revealQueueItem:)];
        if (self.backgroundStyle == NSBackgroundStyleEmphasized)
        {
            [_removeButton setImage:[NSImage imageNamed:@"RevealHighlight"]];
            [_removeButton setAlternateImage:[NSImage imageNamed:@"RevealHighlightPressed"]];
        }
        else
        {
            [_removeButton setImage:[NSImage imageNamed:@"Reveal"]];
        }
    }
    else
    {
        [_removeButton setAction: @selector(removeQueueItem:)];
        if (self.backgroundStyle == NSBackgroundStyleEmphasized)
        {
            [_removeButton setImage:[NSImage imageNamed:@"DeleteHighlight"]];
            [_removeButton setAlternateImage:[NSImage imageNamed:@"DeleteHighlightPressed"]];
        }
        else
        {
            [_removeButton setImage:[NSImage imageNamed:@"Delete"]];
        }
    }
}

- (void)setBackgroundStyle:(NSBackgroundStyle)backgroundStyle
{
    [super setBackgroundStyle:backgroundStyle];
    [self HB_updateRightButton];
}

- (void)expand
{
    self.expandButton.state = NSOnState;
    self.expanded = YES;
    self.textField.attributedStringValue = self.job.attributedDescription;
}

- (void)collapse
{
    self.expandButton.state = NSOffState;
    self.expanded = NO;
    self.textField.attributedStringValue = self.job.attributedTitleDescription;
}

- (BOOL)isFlipped
{
    return YES;
}

- (IBAction)revealQueueItem:(id)sender
{
    [self.delegate revealQueueItem:self.job];
}

- (IBAction)removeQueueItem:(id)sender
{
    [self.delegate removeQueueItem:self.job];
}

- (IBAction)toggleHeight:(id)sender
{
    [self.delegate toggleQueueItemHeight:self.job];
}

@end
