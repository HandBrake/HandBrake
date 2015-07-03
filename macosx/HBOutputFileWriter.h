/*  HBOutputFileWriter.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBOutputRedirect.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * This class is used to listen to HBOutputRedirect
 * and write the output to a file.
 */
@interface HBOutputFileWriter : NSObject <HBOutputRedirectListening>

- (nullable instancetype)initWithFileURL:(NSURL *)url;

- (void)write:(NSString *)text;
- (void)clear;

@property (nonatomic, readonly) NSURL *url;

@end

NS_ASSUME_NONNULL_END
