/*  HBPlayerTrack.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface HBPlayerTrack : NSObject

- (instancetype)initWithTrackName:(NSString *)name object:(id)representedObject enabled:(BOOL)enabled;

@property (nonatomic, readonly) NSString *name;
@property (nonatomic, readwrite) BOOL enabled;
@property (nonatomic, readonly) id representedObject;

@end

NS_ASSUME_NONNULL_END

