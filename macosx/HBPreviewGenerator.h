/*  HBPreviewGenerator.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import <CoreVideo/CoreVideo.h>

NS_ASSUME_NONNULL_BEGIN

@class HBCore;
@class HBJob;

@protocol HBPreviewGeneratorDelegate <NSObject>

- (void) reloadPreviews;

- (void) didCreateMovieAtURL: (NSURL *) fileURL;
- (void) didCancelMovieCreation;

- (void) updateProgress: (double) progress info: (NSString *) progressInfo;

@end

@interface HBPreviewGenerator : NSObject

@property (nonatomic, assign, nullable) id <HBPreviewGeneratorDelegate> delegate;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithCore:(HBCore *)core job:(HBJob *)job NS_DESIGNATED_INITIALIZER;

/**
 * Wait until all the asynchronous operations are done.
 */
- (void)invalidate;

#pragma mark - Still image generator


/**
 * Returns the picture preview CVPixelBuffer at the specified index
 *
 * @param index picture index in title.
 */
- (nullable CVPixelBufferRef)copyPixelBufferAtIndex:(NSUInteger)index shouldCache:(BOOL)cache;

/**
 * Returns the picture preview at the specified index
 *
 * @param index picture index in title.
 */
- (nullable CGImageRef)copyImageAtIndex:(NSUInteger)index shouldCache:(BOOL)cache CF_RETURNS_RETAINED;

/**
 * Returns a small picture preview at the specified index asynchronously
 *
 * @param index picture index in title.
 */
- (void) copySmallImageAtIndex: (NSUInteger) index completionHandler:(void (^)(__nullable CGImageRef result))handler;

@property (nonatomic, readonly) NSUInteger imagesCount;
@property (nonatomic, readonly) CGSize imageSize;
- (void) purgeImageCache;

@property (nonatomic, readonly, copy) NSString *info;

#pragma mark -  Video generator
- (BOOL) createMovieAsyncWithImageAtIndex: (NSUInteger) index duration: (NSUInteger) seconds;
- (void) cancel;

@end

NS_ASSUME_NONNULL_END
