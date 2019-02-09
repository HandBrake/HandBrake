/*  HBQueueItemView.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueItemView.h"
#import "HBQueueItem.h"

@interface HBQueueItemView ()

@property (nonatomic, weak) IBOutlet NSButton *removeButton;
@property (nonatomic, weak) IBOutlet NSButton *expandButton;

@end

@implementation HBQueueItemView

- (void)setItem:(HBQueueItem *)item
{
    _item = item;
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
    if (_item.expanded)
    {
        self.textField.attributedStringValue = _item.attributedDescription;
        self.expandButton.state = NSOnState;
    }
    else
    {
        self.textField.attributedStringValue = _item.attributedTitleDescription;
        self.expandButton.state = NSOffState;
    }
}

- (void)HB_updateState
{
    NSImage *state = nil;
    switch (_item.state) {
        case HBQueueItemStateCompleted:
            state = [NSImage imageNamed:@"EncodeComplete"];
            break;
        case HBQueueItemStateWorking:
            state = [NSImage imageNamed:@"EncodeWorking0"];
            break;
        case HBQueueItemStateCanceled:
            state = [NSImage imageNamed:@"EncodeCanceled"];
            break;
        case HBQueueItemStateFailed:
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
    BOOL darkAqua = NO;

    if (@available(macOS 10.14, *))
    {
        darkAqua = [self.effectiveAppearance bestMatchFromAppearancesWithNames:@[NSAppearanceNameDarkAqua]] == NSAppearanceNameDarkAqua ? YES : NO;
    }

    if (_item.state == HBQueueItemStateCompleted)
    {
        [_removeButton setAction: @selector(revealQueueItem:)];
        if (self.backgroundStyle == NSBackgroundStyleEmphasized)
        {
            [_removeButton setImage:[NSImage imageNamed:@"RevealHighlight"]];
            [_removeButton setAlternateImage:[NSImage imageNamed:@"RevealHighlightPressed"]];
        }
        else
        {
            [_removeButton setImage:[NSImage imageNamed:darkAqua ? @"RevealHighlight" : @"Reveal"]];
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
            [_removeButton setImage:[NSImage imageNamed:darkAqua ? @"DeleteHighlight" : @"Delete"]];
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
    self.textField.attributedStringValue = _item.attributedDescription;
}

- (void)collapse
{
    self.expandButton.state = NSOffState;
    self.textField.attributedStringValue = _item.attributedTitleDescription;
}

- (BOOL)isFlipped
{
    return YES;
}

- (void)viewDidChangeEffectiveAppearance
{
    [self HB_updateRightButton];
}

- (IBAction)revealQueueItem:(id)sender
{
    [self.delegate revealQueueItem:_item];
}

- (IBAction)removeQueueItem:(id)sender
{
    [self.delegate removeQueueItem:_item];
}

- (IBAction)toggleHeight:(id)sender
{
    [self.delegate toggleQueueItemHeight:_item];
}

@end
