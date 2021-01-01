//
//  HBFilePromiseProvider.m
//  HandBrake
//
//  Created by Damiano Galassi on 09/01/2020.
//  Copyright © 2021 HandBrake. All rights reserved.
//

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
