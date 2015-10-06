/* QTKit+HBQTMovieExtensions.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "QTKit+HBQTMovieExtensions.h"

@implementation QTMovieView (HBQTMovieViewExtensions)

- (void)mouseMoved:(NSEvent *)theEvent
{
    [super mouseMoved:theEvent];
}

@end

@implementation QTMovie (HBQTMovieExtensions)

- (BOOL)isPlaying
{
    if (self.rate > 0)
    {
        return YES;
    }
    else
    {
        return NO;
    }
}

- (NSString *)timecode
{
    QTTime time = [self currentTime];
    double timeInSeconds = (double)time.timeValue / time.timeScale;
    UInt16 seconds = (UInt16)fmod(timeInSeconds, 60.0);
    UInt16 minutes = (UInt16)fmod(timeInSeconds / 60.0, 60.0);
    UInt16 hours = (UInt16)(timeInSeconds / (60.0 * 60.0));
    UInt16 milliseconds = (UInt16)(timeInSeconds - (int) timeInSeconds) * 1000;
    return [NSString stringWithFormat:@"%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds];
}

- (void)setCurrentTimeDouble:(double)value
{
    long timeScale = [[self attributeForKey:QTMovieTimeScaleAttribute] longValue];
    [self setCurrentTime:QTMakeTime((long long)value * timeScale, timeScale)];
}

@end