//
//  Preset.h
//  InstantHandBrake
//
//  Created by Damiano Galassi on 15/01/08.
//  This file is part of the HandBrake source code.
//  Homepage: <http://handbrake.m0k.org/>.
//  It may be used under the terms of the GNU General Public License.
//
//

#import "Preset.h"


@implementation Preset

- (id) initWithMuxer: (int) muxer
          videoCodec: (int) videoCodec
        videoBitRate: (int) videoBitRate
   videoCodecOptions: (NSString *) videoCodecOptions
          audioCodec: (int) audioCodec
        audioBitrate: (int) audioBitrate
     audioSampleRate: (int) audioSampleRate
            maxWidth: (int) maxWidth
           maxHeight: (int) maxHeight
          anamorphic: (int) anamorphic;
{
    if (self = [super init])
    {
        fMuxer = muxer;
        fVideoCodec = videoCodec;
        fVideoBitRate = videoBitRate;
        fVideoCodecOptions = videoCodecOptions;
        fAudioCodec = audioCodec;
        fAudioBitRate = audioBitrate;
        fAudioSampleRate = audioSampleRate;
        fMaxWidth = maxWidth;
        fMaxHeight = maxHeight;
        fAnamorphic = anamorphic;
    }
    return self;
}

- (void) dealloc
{
    [fVideoCodecOptions release];
    [super dealloc];
}

- (int) muxer
{
    return fMuxer;
}

- (int) videoCodec;
{
    return fVideoCodec;
}

- (NSString *) videoCodecOptions
{
    return fVideoCodecOptions;
}

- (int) videoBitRate
{
    return fVideoBitRate;
}

- (int) AudioCodec;
{
    return fAudioCodec;
}

- (int) maxWidth;
{
    return fMaxWidth;
}

- (int) maxHeight;
{
    return fMaxHeight;
}

@end
