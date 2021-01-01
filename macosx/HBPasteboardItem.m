//
//  HBPasteboardWriter.m
//  HandBrake
//
//  Created by Damiano Galassi on 04/10/20.
//  Copyright © 2021 HandBrake. All rights reserved.
//

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
