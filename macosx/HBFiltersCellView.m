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

@end

