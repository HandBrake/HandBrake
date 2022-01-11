/*  HBQueueItem.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN


/// A flag to indicate the item's state
typedef NS_ENUM(NSUInteger, HBQueueItemState) {
    HBQueueItemStateReady,
    HBQueueItemStateWorking,
    HBQueueItemStateCompleted,
    HBQueueItemStateCanceled,
    HBQueueItemStateFailed,
    HBQueueItemStateRescanning
};

@protocol HBQueueItem <NSObject, NSSecureCoding>

/// Current state of the item.
@property (nonatomic) HBQueueItemState state;

/// Whether the item has a file representation on disk or not.
@property (nonatomic, readonly) BOOL hasFileRepresentation;

@property (nonatomic, readonly, copy, nullable) NSURL *destinationURL;

/// The title of the item.
@property (nonatomic, readonly) NSString *title;
@property (nonatomic, readonly) NSAttributedString *attributedDescription;

@property (nonatomic, readonly) NSImage *image;

@end

NS_ASSUME_NONNULL_END
