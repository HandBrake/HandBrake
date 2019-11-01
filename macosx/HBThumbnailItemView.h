/*
 Copyright (C) 2017 Apple Inc. All Rights Reserved.
 See LICENSE.txt for this sample’s licensing information

 Abstract:
 Custom NSScrubberItemView used to display an image.
 */

#import <Cocoa/Cocoa.h>
#import "HBPreviewGenerator.h"

NS_ASSUME_NONNULL_BEGIN

@interface HBThumbnailItemView : NSScrubberItemView

@property (nonatomic, nullable) NSImage *thumbnail;
@property (nonatomic) NSUInteger thumbnailIndex;
@property (nonatomic, weak) HBPreviewGenerator *generator;

@end

NS_ASSUME_NONNULL_END

