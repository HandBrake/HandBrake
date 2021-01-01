//
//  HBPasteboardWriter.h
//  HandBrake
//
//  Created by Damiano Galassi on 04/10/20.
//  Copyright © 2021 HandBrake. All rights reserved.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

extern NSPasteboardType const tableViewIndex;

@interface HBPasteboardItem : NSObject<NSPasteboardWriting>

- (instancetype)initWithIndex:(NSInteger)index;

@property (nonatomic, readonly) NSInteger index;

@end

NS_ASSUME_NONNULL_END
