/* HBFilePromiseProvider.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBFilePromiseProvider.h"

@implementation HBFilePromiseProvider

- (NSArray<NSPasteboardType> *)writableTypesForPasteboard:(NSPasteboard *)pasteboard
{
    NSMutableArray<NSPasteboardType> *types = [[super writableTypesForPasteboard:pasteboard] mutableCopy];
    [types addObject:kHandBrakeInternalPBoardType];
    return types;
}

- (NSPasteboardWritingOptions)writingOptionsForType:(NSPasteboardType)type pasteboard:(NSPasteboard *)pasteboard
{
    if ([type isEqualToString:kHandBrakeInternalPBoardType])
    {
        return 0;
    }

    return [super writingOptionsForType:type pasteboard:pasteboard];
}

- (id)pasteboardPropertyListForType:(NSPasteboardType)type
{
    if ([type isEqualToString:kHandBrakeInternalPBoardType])
    {
        return kHandBrakeInternalPBoardType;
    }

    return [super pasteboardPropertyListForType:type];
}

@end
