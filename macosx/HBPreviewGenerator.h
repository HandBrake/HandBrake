/*  HBPreviewGenerator.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@class HBCore;
@class HBJob;

@protocol HBPreviewGeneratorDelegate <NSObject>

- (void) reloadPreviews;

- (void) didCreateMovieAtURL: (NSURL *) fileURL;
- (void) didCancelMovieCreation;

- (void) updateProgress: (double) progress info: (NSString *) progressInfo;

@end

@interface HBPreviewGenerator : NSObject

@property (nonatomic, assign) id <HBPreviewGeneratorDelegate> delegate;

- (instancetype)initWithCore:(HBCore *)core job:(HBJob *)job;

/* Still image generator */
- (CGImageRef) imageAtIndex: (NSUInteger) index shouldCache: (BOOL) cache;
- (NSUInteger) imagesCount;
- (void) purgeImageCache;

- (NSString *)info;

/* Video generator */
- (BOOL) createMovieAsyncWithImageAtIndex: (NSUInteger) index duration: (NSUInteger) seconds;
- (void) cancel;

@end
