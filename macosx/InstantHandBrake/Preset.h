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

#import <Cocoa/Cocoa.h>


@interface Preset : NSObject {
    int             fMuxer;
    int             fVideoCodec;
    int             fVideoBitRate;
    NSString      * fVideoCodecOptions;

    int             fAudioCodec;
    int             fAudioBitRate;
    int             fAudioSampleRate;
    
    int             fMaxWidth;
    int             fMaxHeight;
    int             fAnamorphic;
}

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

- (int) muxer;
- (int) videoCodec;
- (NSString *) videoCodecOptions;
- (int) videoBitRate;
- (int) AudioCodec;
- (int) maxWidth;
- (int) maxHeight;

@end
