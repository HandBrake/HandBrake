//
//  Preset.h
//  InstantHandBrake
//
//  Created by Damiano Galassi on 15/01/08.
//  This file is part of the HandBrake source code.
//  Homepage: <http://handbrake.fr/>.
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

- (id) initWithCoder:(NSCoder *) coder
{
    presetName         = [[coder decodeObjectForKey:@"Name"] retain];
    fMuxer             = [coder decodeIntForKey:@"Muxer"];
    fVideoCodec        = [coder decodeIntForKey:@"VideoCodec"];
    fVideoBitRate      = [coder decodeIntForKey:@"VideoBitRate"];
    fVideoCodecOptions = [[coder decodeObjectForKey:@"VideoCodecOptions"] retain];
    fAudioCodec        = [coder decodeIntForKey:@"AudioCodec"];
    fAudioBitRate      = [coder decodeIntForKey:@"AudioBitRate"];
    fAudioSampleRate   = [coder decodeIntForKey:@"AudioSampleRate"];
    fMaxWidth          = [coder decodeIntForKey:@"MaxWidth"];
    fMaxHeight         = [coder decodeIntForKey:@"MaxHeight"];
    fAnamorphic        = [coder decodeIntForKey:@"Anarmophic"];
        
    return self;
}

- (void) encodeWithCoder:(NSCoder *)encoder
{
    [encoder encodeObject:presetName forKey:@"Name"];
    [encoder encodeInt:fMuxer forKey:@"Muxer"];
    [encoder encodeInt:fVideoCodec forKey:@"VideoCodec"];
    [encoder encodeInt:fVideoBitRate forKey:@"VideoBitRate"];
    [encoder encodeObject:fVideoCodecOptions forKey:@"VideoCodecOptions"];
    [encoder encodeInt:fAudioCodec forKey:@"AudioCodec"];
    [encoder encodeInt:fAudioBitRate forKey:@"AudioBitRate"];
    [encoder encodeInt:fAudioSampleRate forKey:@"AudioSampleRate"];
    [encoder encodeInt:fMaxWidth forKey:@"MaxWidth"];
    [encoder encodeInt:fMaxHeight forKey:@"MaxHeight"];
    [encoder encodeInt:fAnamorphic forKey:@"Anarmophic"];
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
