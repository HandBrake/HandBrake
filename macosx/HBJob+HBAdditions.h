/*  HBJob+HBAdditions.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@import HandBrakeKit;

NS_ASSUME_NONNULL_BEGIN

@interface HBJob (HBAdditions)

@property (nonatomic, readonly) NSString *automaticName;
@property (nonatomic, readonly) NSString *automaticExt;
@property (nonatomic, readonly) NSString *defaultName;

/**
 *  Generates a file name automatically based on the inputs,
 *  it can be configured with NSUserDefaults.
 *
 *  @param sourceName    the name of the source file
 *  @param title         the title number
 *  @param chaptersRange the selected chapters range
 *  @param quality       the video encoder quality
 *  @param bitrate       the video encoder bitrate
 *  @param videoCodec    the video encoder type
 *
 *  @return a NSString containing the required info
 */
- (NSString *)automaticNameForSource:(NSString *)sourceName
                               title:(NSUInteger)title
                            chapters:(NSRange)chaptersRange
                             quality:(double)quality
                             bitrate:(int)bitrate
                          videoCodec:(uint32_t)codec
                        creationDate:(NSDate *)creationDate;

@end

NS_ASSUME_NONNULL_END
