/*  HBFiltersCellView.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFiltersCellView.h"

@import HandBrakeKit;

@implementation HBFiltersCellView

- (void)setObjectValue:(id)objectValue
{
    [super setObjectValue:objectValue];

    [self didChangeValueForKey:@"objectValue"];

    if ([objectValue isKindOfClass:[HBFilter class]])
    {
        HBFilter *filter = (HBFilter *)objectValue;

        HBFilterPresetTransformer *presetsTransformer = [[HBFilterPresetTransformer alloc] initWithWithFilterID:filter.filterID];
        HBFilterTuneTransformer   *tunesTransformer = [[HBFilterTuneTransformer alloc] initWithWithFilterID:filter.filterID];

        [self.presetsPopUpButton bind:@"selectedValue" toObject:self
                          withKeyPath:@"objectValue.preset"
                              options:@{NSValueTransformerBindingOption: presetsTransformer}];

        [self.tunesPopUpButton bind:@"selectedValue" toObject:self
                        withKeyPath:@"objectValue.tune"
                            options:@{NSValueTransformerBindingOption: tunesTransformer}];
    }
}

- (IBAction)remove:(id)sender
{
    if ([self.objectValue isKindOfClass:[HBFilter class]])
    {
        HBFilter *filter = (HBFilter *)self.objectValue;
        [filter.delegate removeFilter:filter];
    }
}

- (void)HB_updateUI
{
    BOOL darkAqua = NO;

    if (@available(macOS 10.14, *))
    {
        darkAqua = [self.effectiveAppearance bestMatchFromAppearancesWithNames:@[NSAppearanceNameDarkAqua]] == NSAppearanceNameDarkAqua ? YES : NO;
    }

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

- (void)setBackgroundStyle:(NSBackgroundStyle)backgroundStyle
{
    [super setBackgroundStyle:backgroundStyle];
    [self HB_updateUI];
}

- (void)viewDidChangeEffectiveAppearance
{
    [self HB_updateUI];
}

@end

