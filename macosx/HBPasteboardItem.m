/* HBPasteboardItem.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPasteboardItem.h"

NSPasteboardType const tableViewIndex = @"fr.handbrake.tableViewIndex";

@implementation HBPasteboardItem

- (instancetype)initWithIndex:(NSInteger)index
{
    self = [super init];
    if (self)
    {
        _index = index;
    }
    return self;
}

- (NSArray<NSPasteboardType> *)writableTypesForPasteboard:(NSPasteboard *)pasteboard
{
    return @[tableViewIndex];
}

- (nullable id)pasteboardPropertyListForType:(nonnull NSPasteboardType)type
{
    if ([type isEqualTo:tableViewIndex])
    {
        return @(self.index);
    }
    else
    {
        return nil;
    }
}

@end
