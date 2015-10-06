/* QTKit+HBQTMovieExtensions.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <QTKit/QTKit.h>

@interface QTMovieView (HBQTMovieViewExtensions)

- (void)mouseMoved:(NSEvent *)theEvent;

@end

@interface QTMovie (HBQTMovieExtensions)

- (BOOL)isPlaying;
- (NSString *)timecode;
- (void)setCurrentTimeDouble:(double)value;

@end
