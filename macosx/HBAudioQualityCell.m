/*  HBAudioQualityCell.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAudioQualityCell.h"
#import "HBAudioTrack.h"

#define HB_INVALID_AUDIO_QUALITY (-3.)

static void *HBAudioQualityCellContext = &HBAudioQualityCellContext;
static void *HBAudioQualityCellItemContext = &HBAudioQualityCellItemContext;

@interface HBAudioQualityCell ()
@property (nonatomic, readwrite) NSArray<NSNumber *> *bitRates;
@property (nonatomic, readwrite) NSArray<NSNumber *> *qualities;
@end

@implementation HBAudioQualityCell

- (void)setObjectValue:(id)objectValue
{
    [self.objectValue removeObserver:self forKeyPath:@"bitRates" context:HBAudioQualityCellContext];
    [self.objectValue removeObserver:self forKeyPath:@"bitRate" context:HBAudioQualityCellItemContext];
    [self.objectValue removeObserver:self forKeyPath:@"quality" context:HBAudioQualityCellItemContext];
    [self.objectValue removeObserver:self forKeyPath:@"mode" context:HBAudioQualityCellItemContext];

    [super setObjectValue:objectValue];

    [self buildMenu];

    [objectValue addObserver:self
                  forKeyPath:@"bitRates"
                     options:0
                     context:HBAudioQualityCellContext];

    [objectValue addObserver:self
                  forKeyPath:@"bitRate"
                     options:0
                     context:HBAudioQualityCellItemContext];

    [objectValue addObserver:self
                  forKeyPath:@"quality"
                     options:0
                     context:HBAudioQualityCellItemContext];

    [objectValue addObserver:self
                  forKeyPath:@"mode"
                     options:0
                     context:HBAudioQualityCellItemContext];

}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBAudioQualityCellContext)
    {
        [self buildMenu];
    }
    else if (context == HBAudioQualityCellItemContext)
    {
        [self setSelectedItem];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)buildMenu
{
    if (self.objectValue != nil)
    {
        NSArray<NSNumber *> *bitRates = [self.objectValue bitRates];
        NSArray<NSNumber *> *qualities = [self.objectValue qualities];

        if ([bitRates isEqualToArray:self.bitRates] &&
            [qualities isEqualToArray:self.qualities])
        {
            [self setSelectedItem];
        }
        else
        {
            NSMenu *menu = self.popUp.menu;
            [menu removeAllItems];

            self.bitRates = bitRates;
            self.qualities = qualities;

            if (bitRates.count)
            {
                if (@available(macOS 14.0, *))
                {
                    NSMenuItem *bitRateHeader = [NSMenuItem sectionHeaderWithTitle:NSLocalizedString(@"Average Bitrate", @"Audio bitrates")];
                    [menu addItem:bitRateHeader];
                }

                for (NSNumber *bitRate in bitRates)
                {
                    NSMenuItem *bitRateItem = [[NSMenuItem alloc] init];
                    bitRateItem.title = [NSString stringWithFormat:@"%@ kbps", bitRate];
                    bitRateItem.tag = bitRate.intValue;
                    bitRateItem.action = @selector(setTrackBitrate:);
                    bitRateItem.target = self;
                    [menu addItem:bitRateItem];
                }
            }

            if (bitRates.count && qualities.count)
            {
                [menu addItem:[NSMenuItem separatorItem]];
            }

            if (qualities.count)
            {
                if (@available(macOS 14.0, *))
                {
                    NSMenuItem *qualityHeader = [NSMenuItem sectionHeaderWithTitle:NSLocalizedString(@"Constant Quality", @"Audio quality")];
                    [menu addItem:qualityHeader];
                }

                for (NSNumber *quality in qualities)
                {
                    NSMenuItem *qualityItem = [[NSMenuItem alloc] init];
                    qualityItem.title = [NSString stringWithFormat:@"%@ CQ", quality];
                    qualityItem.tag = quality.doubleValue * 1000 ;
                    qualityItem.action = @selector(setTrackQuality:);
                    qualityItem.target = self;
                    [menu addItem:qualityItem];
                }
            }

            if (bitRates.count == 0 && qualities.count == 0)
            {
                [menu addItemWithTitle:NSLocalizedString(@"N/A", @"Audio quality")
                                action:nil
                         keyEquivalent:@""];
                [self.popUp selectItemAtIndex:0];
            }
            else
            {
                [self setSelectedItem];
            }
        }
    }
}

- (void)setSelectedItem
{
    if ((int)[self.objectValue mode] == (int)HBAudioEncoderModeABR)
    {
        [self.popUp selectItemWithTag:[self.objectValue bitRate]];
    }
    else
    {
        [self.popUp selectItemWithTag:[self.objectValue quality] * 1000];
    }
}

- (IBAction)setTrackBitrate:(NSMenuItem *)sender
{
    [self.objectValue setBitRate:(int)sender.tag];
}

- (IBAction)setTrackQuality:(NSMenuItem *)sender
{
    [self.objectValue setQuality:(double)sender.title.doubleValue];
}

@end
