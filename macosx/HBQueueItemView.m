/*  HBQueueItemView.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueItemView.h"
#import "HBQueueItem.h"

@interface HBQueueItemView ()

@property (nonatomic, weak) IBOutlet NSButton *removeButton;

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
}

- (void)HB_updateLabel
{
    self.textField.stringValue = _item.outputFileName;
}

- (void)HB_updateState
{
    NSImage *state = nil;
    NSString *label = nil;
    switch (_item.state) {
        case HBQueueItemStateCompleted:
            state = [NSImage imageNamed:@"EncodeComplete"];
            label = NSLocalizedString(@"Encode complete", @"HBQueueItemView -> Encode state accessibility label");
            break;
        case HBQueueItemStateWorking:
            state = [NSImage imageNamed:@"EncodeWorking0"];
            label = NSLocalizedString(@"Encode working", @"HBQueueItemView -> Encode state accessibility label");
            break;
        case HBQueueItemStateCanceled:
            state = [NSImage imageNamed:@"EncodeCanceled"];
            label = NSLocalizedString(@"Encode canceled", @"HBQueueItemView -> Encode state accessibility label");
            break;
        case HBQueueItemStateFailed:
            state = [NSImage imageNamed:@"EncodeFailed"];
            label = NSLocalizedString(@"Encode failed", @"HBQueueItemView -> Encode state accessibility label");
            break;
        default:
            state = [NSImage imageNamed:@"JobSmall"];
            NSLocalizedString(@"Encode ready", @"HBQueueItemView -> Encode state accessibility label");
            break;
    }

    self.imageView.image = state;
    self.imageView.accessibilityLabel = label;
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
        _removeButton.accessibilityTitle = NSLocalizedString(@"Reveal destination in Finder", @"HBQueueItemView -> Reveal button accessibility label");
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
        _removeButton.accessibilityTitle = NSLocalizedString(@"Remove job", @"HBQueueItemView -> Remove button accessibility label");
    }
}

- (void)setBackgroundStyle:(NSBackgroundStyle)backgroundStyle
{
    [super setBackgroundStyle:backgroundStyle];
    [self HB_updateRightButton];
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

@end
