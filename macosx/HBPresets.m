/*  HBPresets.m $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "HBPresets.h"

@implementation HBPresets

- (id)init 
{
    self = [super init];
   return self;
}

/* Called by -addFactoryPresets in Controller.m */
- (NSMutableArray *)generateBuiltinPresets:(NSMutableArray *)UserPresets
{
    /*
     * We receive the user presets array of dictionaries from Controller.m
     *
     * Re-create new built-in presets programmatically and add them to the array
     *
     * Note: the built-in presets will *not* sort themselves alphabetically,
     * so they will appear in the order you create them.
     */
    
    /* Built-in preset folders at the root of the hierarchy */
    [UserPresets addObject:[self createDevicesPresetFolder]];
    [UserPresets addObject:[self createRegularPresetFolder]];
    
    /* Independent presets at the root hierarchy level would go here */
    
    /* Return the newly-regenerated preset array back to Controller.m */
    return UserPresets;
}

#pragma mark -
#pragma mark Preset Folder Definitions

- (NSDictionary *)createDevicesPresetFolder
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"Devices"                    forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]    forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]    forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:YES] forKey:@"Folder"];
    
    /* Initialize a child array, and add the individual presets to it */
    NSMutableArray *childrenArray = [[NSMutableArray alloc] init];
    [childrenArray addObject:[self createUniversalPreset]];
    [childrenArray addObject:[self createiPodPreset]];
    [childrenArray addObject:[self createiPhoneiPodtouchPreset]];
    [childrenArray addObject:[self createiPadPreset]];
    [childrenArray addObject:[self createAppleTVPreset]];
    [childrenArray addObject:[self createAppleTV2Preset]];
    [childrenArray addObject:[self createAppleTV3Preset]];
    [childrenArray addObject:[self createAndroidPreset]];
    [childrenArray addObject:[self createAndroidTabletPreset]];
    
    /* Add the individual presets to the folder */
    [preset setObject:[NSMutableArray arrayWithArray:childrenArray]
               forKey:@"ChildrenArray"];
    
    /* Clean up and return the folder */
    [childrenArray autorelease];
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createRegularPresetFolder
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"Regular"                    forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]    forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]    forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:YES] forKey:@"Folder"];
    
    /* Initialize a child array, and add the individual presets to it */
    NSMutableArray *childrenArray = [[NSMutableArray alloc] init];
    [childrenArray addObject:[self createNormalPreset]];
    [childrenArray addObject:[self createHighProfilePreset]];
    
    /* Add the individual presets to the folder */
    [preset setObject:[NSMutableArray arrayWithArray:childrenArray]
               forKey:@"ChildrenArray"];
    
    /* Clean up and return the folder */
    [childrenArray autorelease];
    [preset autorelease];
    return preset;
}

#pragma mark -
#pragma mark Individual Preset Definitions
/* These NSDictionary definitions contain settings for one built-in preset */

- (NSDictionary *)createUniversalPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"Universal"                 forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
    [preset setObject:@"HandBrake's settings for compatibility with all Apple devices (including the iPod 6G and later). Includes Dolby Digital audio for surround sound."
               forKey:@"PresetDescription"];
    
    /* Container format and related settings */
    [preset setObject:@"MP4 file"                forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4HttpOptimize"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter markers */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    
    /* Video encoder and advanced options */
    [preset setObject:@"H.264 (x264)"            forKey:@"VideoEncoder"];
    [preset setObject:@""                        forKey:@"lavcOption"];
    [preset setObject:@""                        forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"fast"                    forKey:@"x264Preset"];
    [preset setObject:@""                        forKey:@"x264Tune"];
    [preset setObject:@""                        forKey:@"x264OptionExtra"];
    [preset setObject:@"baseline"                forKey:@"h264Profile"];
    [preset setObject:@"3.0"                     forKey:@"h264Level"];
    
    /* Video rate control */
    [preset setObject:@"2500"                         forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTurboTwoPass"];
    [preset setObject:[NSNumber numberWithInt:2]      forKey:@"VideoQualityType"]; //cq
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];
    
    /* Video frame rate */
    [preset setObject:@"30"  forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* Picture size */
    [preset setObject:[NSNumber numberWithInt:1]   forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:720] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:576] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:2]   forKey:@"PicturePAR"]; //loose
    [preset setObject:[NSNumber numberWithInt:2]   forKey:@"PictureModulus"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"PictureKeepRatio"]; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDecomb"]; //off
    [preset setObject:@""                        forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"]; //decomb
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:@""                        forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:@""                        forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:@""                        forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* Picture crop */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Auto Passthru */
    [preset setObject:@"AC3 (ffmpeg)"            forKey:@"AudioEncoderFallback"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAACPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAC3Pass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSHDPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowMP3Pass"];
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)"                  forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"          forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru"                forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"None"                        forKey:@"AudioMixdown"];
    [audioTrack2Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack2Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
    /* Add the audio track(s) to the preset's audio list */
    [preset setObject:[NSMutableArray arrayWithArray:audioListArray] forKey:@"AudioList"];
    
    /* Subtitles (note: currently ignored) */
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createiPodPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"iPod"                      forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
    [preset setObject:@"HandBrake's settings for playback on the iPod with Video (all generations)."
               forKey:@"PresetDescription"];
    
    /* Container format and related settings */
    [preset setObject:@"MP4 file"                forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4HttpOptimize"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter markers */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    
    /* Video encoder and advanced options */
    [preset setObject:@"H.264 (x264)"            forKey:@"VideoEncoder"];
    [preset setObject:@""                        forKey:@"lavcOption"];
    [preset setObject:@""                        forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium"                  forKey:@"x264Preset"];
    [preset setObject:@""                        forKey:@"x264Tune"];
    [preset setObject:@""                        forKey:@"x264OptionExtra"];
    [preset setObject:@"baseline"                forKey:@"h264Profile"];
    [preset setObject:@"1.3"                     forKey:@"h264Level"];
    
    /* Video rate control */
    [preset setObject:@"2500"                         forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTurboTwoPass"];
    [preset setObject:[NSNumber numberWithInt:2]      forKey:@"VideoQualityType"]; //cq
    [preset setObject:[NSNumber numberWithFloat:22.0] forKey:@"VideoQualitySlider"];
    
    /* Video frame rate */
    [preset setObject:@"30"  forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* Picture size */
    [preset setObject:[NSNumber numberWithInt:1]   forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:320] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:240] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"PicturePAR"]; //none
    [preset setObject:[NSNumber numberWithInt:2]   forKey:@"PictureModulus"];
    [preset setObject:[NSNumber numberWithInt:1]   forKey:@"PictureKeepRatio"];
    
    /* Picture filters */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDecomb"]; //off
    [preset setObject:@""                        forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"]; //decomb
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:@""                        forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:@""                        forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:@""                        forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* Picture crop */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Auto Passthru */
    [preset setObject:@"AC3 (ffmpeg)"            forKey:@"AudioEncoderFallback"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAACPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAC3Pass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSHDPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowMP3Pass"];
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)"                  forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"          forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    [preset setObject:[NSMutableArray arrayWithArray:audioListArray] forKey:@"AudioList"];
    
    /* Subtitles (note: currently ignored) */
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createiPhoneiPodtouchPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"iPhone & iPod touch"       forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
    [preset setObject:@"HandBrake's settings for handheld iOS devices (iPhone 4, iPod touch 3G and later)."
               forKey:@"PresetDescription"];
    
    /* Container format and related settings */
    [preset setObject:@"MP4 file"                forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4HttpOptimize"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter markers */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    
    /* Video encoder and advanced options */
    [preset setObject:@"H.264 (x264)"            forKey:@"VideoEncoder"];
    [preset setObject:@""                        forKey:@"lavcOption"];
    [preset setObject:@""                        forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium"                  forKey:@"x264Preset"];
    [preset setObject:@""                        forKey:@"x264Tune"];
    [preset setObject:@""                        forKey:@"x264OptionExtra"];
    [preset setObject:@"high"                    forKey:@"h264Profile"];
    [preset setObject:@"3.1"                     forKey:@"h264Level"];
    
    /* Video rate control */
    [preset setObject:@"2500"                         forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTurboTwoPass"];
    [preset setObject:[NSNumber numberWithInt:2]      forKey:@"VideoQualityType"]; //cq
    [preset setObject:[NSNumber numberWithFloat:22.0] forKey:@"VideoQualitySlider"];
    
    /* Video frame rate */
    [preset setObject:@"30"  forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* Picture size */
    [preset setObject:[NSNumber numberWithInt:1]   forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:960] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:640] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:2]   forKey:@"PicturePAR"]; //loose
    [preset setObject:[NSNumber numberWithInt:2]   forKey:@"PictureModulus"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"PictureKeepRatio"]; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDecomb"]; //off
    [preset setObject:@""                        forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"]; //decomb
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:@""                        forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:@""                        forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:@""                        forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* Picture crop */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Auto Passthru */
    [preset setObject:@"AC3 (ffmpeg)"            forKey:@"AudioEncoderFallback"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAACPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAC3Pass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSHDPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowMP3Pass"];
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)"                  forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"          forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    [preset setObject:[NSMutableArray arrayWithArray:audioListArray] forKey:@"AudioList"];
    
    /* Subtitles (note: currently ignored) */
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createiPadPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"iPad"                      forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
    [preset setObject:@"HandBrake's settings for playback on the iPad (all generations)."
               forKey:@"PresetDescription"];
    
    /* Container format and related settings */
    [preset setObject:@"MP4 file"                forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4HttpOptimize"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter markers */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    
    /* Video encoder and advanced options */
    [preset setObject:@"H.264 (x264)"            forKey:@"VideoEncoder"];
    [preset setObject:@""                        forKey:@"lavcOption"];
    [preset setObject:@""                        forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium"                  forKey:@"x264Preset"];
    [preset setObject:@""                        forKey:@"x264Tune"];
    [preset setObject:@""                        forKey:@"x264OptionExtra"];
    [preset setObject:@"high"                    forKey:@"h264Profile"];
    [preset setObject:@"3.1"                     forKey:@"h264Level"];
    
    /* Video rate control */
    [preset setObject:@"2500"                         forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTurboTwoPass"];
    [preset setObject:[NSNumber numberWithInt:2]      forKey:@"VideoQualityType"]; //cq
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];
    
    /* Video frame rate */
    [preset setObject:@"30"  forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* Picture size */
    [preset setObject:[NSNumber numberWithInt:1]    forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1280] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:720]  forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:2]    forKey:@"PicturePAR"]; //loose
    [preset setObject:[NSNumber numberWithInt:2]    forKey:@"PictureModulus"];
    [preset setObject:[NSNumber numberWithInt:0]    forKey:@"PictureKeepRatio"]; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDecomb"]; //off
    [preset setObject:@""                        forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"]; //decomb
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:@""                        forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:@""                        forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:@""                        forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* Picture crop */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Auto Passthru */
    [preset setObject:@"AC3 (ffmpeg)"            forKey:@"AudioEncoderFallback"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAACPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAC3Pass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSHDPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowMP3Pass"];
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)"                  forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"          forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    [preset setObject:[NSMutableArray arrayWithArray:audioListArray] forKey:@"AudioList"];
    
    /* Subtitles (note: currently ignored) */
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createAppleTVPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"AppleTV"                   forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
    [preset setObject:@"HandBrake's settings for the original AppleTV. Includes Dolby Digital audio for surround sound. Also compatible with iOS devices released since 2009."
               forKey:@"PresetDescription"];
    
    /* Container format and related settings */
    [preset setObject:@"MP4 file"                forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4HttpOptimize"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter markers */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    
    /* Video encoder and advanced options */
    [preset setObject:@"H.264 (x264)"            forKey:@"VideoEncoder"];
    [preset setObject:@""                        forKey:@"lavcOption"];
    [preset setObject:@""                        forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium"                  forKey:@"x264Preset"];
    [preset setObject:@""                        forKey:@"x264Tune"];
    [preset setObject:@"qpmin=4:cabac=0:ref=2:b-pyramid=none:weightb=0:weightp=0:vbv-maxrate=9500:vbv-bufsize=9500"
               forKey:@"x264OptionExtra"];
    [preset setObject:@"high"                    forKey:@"h264Profile"];
    [preset setObject:@"3.1"                     forKey:@"h264Level"];
    
    /* Video rate control */
    [preset setObject:@"2500"                         forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTurboTwoPass"];
    [preset setObject:[NSNumber numberWithInt:2]      forKey:@"VideoQualityType"]; //cq
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];
    
    /* Video frame rate */
    [preset setObject:@"30"  forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* Picture size */
    [preset setObject:[NSNumber numberWithInt:1]   forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:960] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:720] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:2]   forKey:@"PicturePAR"]; //loose
    [preset setObject:[NSNumber numberWithInt:2]   forKey:@"PictureModulus"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"PictureKeepRatio"]; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDecomb"]; //off
    [preset setObject:@""                        forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"]; //decomb
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:@""                        forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:@""                        forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:@""                        forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* Picture crop */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Auto Passthru */
    [preset setObject:@"AC3 (ffmpeg)"            forKey:@"AudioEncoderFallback"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAACPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAC3Pass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSHDPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowMP3Pass"];
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)"                  forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"          forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru"                forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"None"                        forKey:@"AudioMixdown"];
    [audioTrack2Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack2Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
    /* Add the audio track(s) to the preset's audio list */
    [preset setObject:[NSMutableArray arrayWithArray:audioListArray] forKey:@"AudioList"];
    
    /* Subtitles (note: currently ignored) */
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createAppleTV2Preset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"AppleTV 2"                 forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
    [preset setObject:@"HandBrake's settings for the second-generation AppleTV. Includes Dolby Digital audio for surround sound. NOT compatible with the original AppleTV."
               forKey:@"PresetDescription"];
    
    /* Container format and related settings */
    [preset setObject:@"MP4 file"                forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4HttpOptimize"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter markers */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    
    /* Video encoder and advanced options */
    [preset setObject:@"H.264 (x264)"            forKey:@"VideoEncoder"];
    [preset setObject:@""                        forKey:@"lavcOption"];
    [preset setObject:@""                        forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium"                  forKey:@"x264Preset"];
    [preset setObject:@""                        forKey:@"x264Tune"];
    [preset setObject:@""                        forKey:@"x264OptionExtra"];
    [preset setObject:@"high"                    forKey:@"h264Profile"];
    [preset setObject:@"3.1"                     forKey:@"h264Level"];
    
    /* Video rate control */
    [preset setObject:@"2500"                         forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTurboTwoPass"];
    [preset setObject:[NSNumber numberWithInt:2]      forKey:@"VideoQualityType"]; //cq
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];
    
    /* Video frame rate */
    [preset setObject:@"30"  forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* Picture size */
    [preset setObject:[NSNumber numberWithInt:1]    forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1280] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:720]  forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:2]    forKey:@"PicturePAR"]; //loose
    [preset setObject:[NSNumber numberWithInt:2]    forKey:@"PictureModulus"];
    [preset setObject:[NSNumber numberWithInt:0]    forKey:@"PictureKeepRatio"]; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDecomb"]; //off
    [preset setObject:@""                        forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"]; //decomb
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:@""                        forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:@""                        forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:@""                        forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* Picture crop */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Auto Passthru */
    [preset setObject:@"AC3 (ffmpeg)"            forKey:@"AudioEncoderFallback"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAACPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAC3Pass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSHDPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowMP3Pass"];
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)"                  forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"          forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru"                forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"None"                        forKey:@"AudioMixdown"];
    [audioTrack2Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack2Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
    /* Add the audio track(s) to the preset's audio list */
    [preset setObject:[NSMutableArray arrayWithArray:audioListArray] forKey:@"AudioList"];
    
    /* Subtitles (note: currently ignored) */
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createAppleTV3Preset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"AppleTV 3"                 forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
    [preset setObject:@"HandBrake's settings for the third-generation AppleTV. Includes Dolby Digital audio for surround sound. NOT compatible with the original AppleTV. May stutter on the second-generation AppleTV."
               forKey:@"PresetDescription"];
    
    /* Container format and related settings */
    [preset setObject:@"MP4 file"                forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4HttpOptimize"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter markers */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    
    /* Video encoder and advanced options */
    [preset setObject:@"H.264 (x264)"            forKey:@"VideoEncoder"];
    [preset setObject:@""                        forKey:@"lavcOption"];
    [preset setObject:@""                        forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium"                  forKey:@"x264Preset"];
    [preset setObject:@""                        forKey:@"x264Tune"];
    [preset setObject:@""                        forKey:@"x264OptionExtra"];
    [preset setObject:@"high"                    forKey:@"h264Profile"];
    [preset setObject:@"4.0"                     forKey:@"h264Level"];
    
    /* Video rate control */
    [preset setObject:@"2500"                         forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTurboTwoPass"];
    [preset setObject:[NSNumber numberWithInt:2]      forKey:@"VideoQualityType"]; //cq
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];
    
    /* Video frame rate */
    [preset setObject:@"30"  forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* Picture size */
    [preset setObject:[NSNumber numberWithInt:1]    forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1920] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:1080] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:2]    forKey:@"PicturePAR"]; //loose
    [preset setObject:[NSNumber numberWithInt:2]    forKey:@"PictureModulus"];
    [preset setObject:[NSNumber numberWithInt:0]    forKey:@"PictureKeepRatio"]; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:3] forKey:@"PictureDecomb"]; //fast
    [preset setObject:@""                        forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"]; //decomb
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:@""                        forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:@""                        forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:@""                        forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* Picture crop */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Auto Passthru */
    [preset setObject:@"AC3 (ffmpeg)"            forKey:@"AudioEncoderFallback"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAACPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAC3Pass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSHDPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowMP3Pass"];
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)"                  forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"          forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru"                forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"None"                        forKey:@"AudioMixdown"];
    [audioTrack2Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack2Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
    /* Add the audio track(s) to the preset's audio list */
    [preset setObject:[NSMutableArray arrayWithArray:audioListArray] forKey:@"AudioList"];
    
    /* Subtitles (note: currently ignored) */
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createAndroidPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"Android"                   forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
    [preset setObject:@"HandBrake's settings for midrange devices running Android 2.3 or later."
               forKey:@"PresetDescription"];
    
    /* Container format and related settings */
    [preset setObject:@"MP4 file"                forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4HttpOptimize"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter markers */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"ChapterMarkers"];
    
    /* Video encoder and advanced options */
    [preset setObject:@"H.264 (x264)"            forKey:@"VideoEncoder"];
    [preset setObject:@""                        forKey:@"lavcOption"];
    [preset setObject:@""                        forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium"                  forKey:@"x264Preset"];
    [preset setObject:@""                        forKey:@"x264Tune"];
    [preset setObject:@""                        forKey:@"x264OptionExtra"];
    [preset setObject:@"main"                    forKey:@"h264Profile"];
    [preset setObject:@"3.0"                     forKey:@"h264Level"];
    
    /* Video rate control */
    [preset setObject:@"2500"                         forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTurboTwoPass"];
    [preset setObject:[NSNumber numberWithInt:2]      forKey:@"VideoQualityType"]; //cq
    [preset setObject:[NSNumber numberWithFloat:22.0] forKey:@"VideoQualitySlider"];
    
    /* Video frame rate */
    [preset setObject:@"30"  forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* Picture size */
    [preset setObject:[NSNumber numberWithInt:1]   forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:720] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:576] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:2]   forKey:@"PicturePAR"]; //loose
    [preset setObject:[NSNumber numberWithInt:2]   forKey:@"PictureModulus"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"PictureKeepRatio"]; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDecomb"]; //off
    [preset setObject:@""                        forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"]; //decomb
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:@""                        forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:@""                        forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:@""                        forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* Picture crop */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Auto Passthru */
    [preset setObject:@"AC3 (ffmpeg)"            forKey:@"AudioEncoderFallback"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAACPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAC3Pass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSHDPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowMP3Pass"];
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)"                  forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"          forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"128"                         forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    [preset setObject:[NSMutableArray arrayWithArray:audioListArray] forKey:@"AudioList"];
    
    /* Subtitles (note: currently ignored) */
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createAndroidTabletPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"Android Tablet"            forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
    [preset setObject:@"HandBrake's preset for tablets running Android 2.3 or later."
               forKey:@"PresetDescription"];
    
    /* Container format and related settings */
    [preset setObject:@"MP4 file"                forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4HttpOptimize"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter markers */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"ChapterMarkers"];
    
    /* Video encoder and advanced options */
    [preset setObject:@"H.264 (x264)"            forKey:@"VideoEncoder"];
    [preset setObject:@""                        forKey:@"lavcOption"];
    [preset setObject:@""                        forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium"                  forKey:@"x264Preset"];
    [preset setObject:@""                        forKey:@"x264Tune"];
    [preset setObject:@""                        forKey:@"x264OptionExtra"];
    [preset setObject:@"main"                    forKey:@"h264Profile"];
    [preset setObject:@"3.1"                     forKey:@"h264Level"];
    
    /* Video rate control */
    [preset setObject:@"2500"                         forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTurboTwoPass"];
    [preset setObject:[NSNumber numberWithInt:2]      forKey:@"VideoQualityType"]; //cq
    [preset setObject:[NSNumber numberWithFloat:22.0] forKey:@"VideoQualitySlider"];
    
    /* Video frame rate */
    [preset setObject:@"30"  forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* Picture size */
    [preset setObject:[NSNumber numberWithInt:1]    forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1280] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:720]  forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:2]    forKey:@"PicturePAR"]; //loose
    [preset setObject:[NSNumber numberWithInt:2]    forKey:@"PictureModulus"];
    [preset setObject:[NSNumber numberWithInt:0]    forKey:@"PictureKeepRatio"]; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDecomb"]; //off
    [preset setObject:@""                        forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"]; //decomb
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:@""                        forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:@""                        forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:@""                        forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* Picture crop */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Auto Passthru */
    [preset setObject:@"AC3 (ffmpeg)"            forKey:@"AudioEncoderFallback"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAACPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAC3Pass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSHDPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowMP3Pass"];
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)"                  forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"          forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"128"                         forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    [preset setObject:[NSMutableArray arrayWithArray:audioListArray] forKey:@"AudioList"];
    
    /* Subtitles (note: currently ignored) */
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createNormalPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"Normal"                    forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:1]   forKey:@"Default"]; //default
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
    [preset setObject:@"HandBrake's normal, default settings."
               forKey:@"PresetDescription"];
    
    /* Container format and related settings */
    [preset setObject:@"MP4 file"                forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4HttpOptimize"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter markers */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    
    /* Video encoder and advanced options */
    [preset setObject:@"H.264 (x264)"            forKey:@"VideoEncoder"];
    [preset setObject:@""                        forKey:@"lavcOption"];
    [preset setObject:@""                        forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"veryfast"                forKey:@"x264Preset"];
    [preset setObject:@""                        forKey:@"x264Tune"];
    [preset setObject:@""                        forKey:@"x264OptionExtra"];
    [preset setObject:@"main"                    forKey:@"h264Profile"];
    [preset setObject:@"4.0"                     forKey:@"h264Level"];
    
    /* Video rate control */
    [preset setObject:@"2500"                         forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTurboTwoPass"];
    [preset setObject:[NSNumber numberWithInt:2]      forKey:@"VideoQualityType"]; //cq
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];
    
    /* Video frame rate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];
    [preset setObject:@"vfr"            forKey:@"VideoFramerateMode"];
    
    /* Picture size */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"]; //loose
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"]; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDecomb"]; //off
    [preset setObject:@""                        forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"]; //decomb
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:@""                        forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:@""                        forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:@""                        forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* Picture crop */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Auto Passthru */
    [preset setObject:@"AC3 (ffmpeg)"            forKey:@"AudioEncoderFallback"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAACPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAC3Pass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSHDPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowMP3Pass"];
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)"                  forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"          forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Add the audio track(s) to the preset's audio list */
    [preset setObject:[NSMutableArray arrayWithArray:audioListArray] forKey:@"AudioList"];
    
    /* Subtitles (note: currently ignored) */
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createHighProfilePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Preset properties (name, type: factory/user, default, folder, tooltip) */
    [preset setObject:@"High Profile"              forKey:@"PresetName"];
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Type"]; //factory
    [preset setObject:[NSNumber numberWithInt:0]   forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool:NO] forKey:@"Folder"];
    [preset setObject:@"HandBrake's general-purpose preset for High Profile H.264 video."
               forKey:@"PresetDescription"];
    
    /* Container format and related settings */
    [preset setObject:@"MP4 file"                forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4HttpOptimize"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter markers */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    
    /* Video encoder and advanced options */
    [preset setObject:@"H.264 (x264)"            forKey:@"VideoEncoder"];
    [preset setObject:@""                        forKey:@"lavcOption"];
    [preset setObject:@""                        forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium"                  forKey:@"x264Preset"];
    [preset setObject:@""                        forKey:@"x264Tune"];
    [preset setObject:@""                        forKey:@"x264OptionExtra"];
    [preset setObject:@"high"                    forKey:@"h264Profile"];
    [preset setObject:@"4.1"                     forKey:@"h264Level"];
    
    /* Video rate control */
    [preset setObject:@"2500"                         forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0]      forKey:@"VideoTurboTwoPass"];
    [preset setObject:[NSNumber numberWithInt:2]      forKey:@"VideoQualityType"]; //cq
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];
    
    /* Video frame rate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];
    [preset setObject:@"vfr"            forKey:@"VideoFramerateMode"];
    
    /* Picture size */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"]; //loose
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"]; //set to 0 for Loose (FIXME: why?)
    
    /* Picture filters */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureDecomb"]; //default
    [preset setObject:@""                        forKey:@"PictureDecombCustom"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"]; //decomb
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:@""                        forKey:@"PictureDeinterlaceCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:@""                        forKey:@"PictureDetelecineCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:@""                        forKey:@"PictureDenoiseCustom"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* Picture crop */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Auto Passthru */
    [preset setObject:@"AC3 (ffmpeg)"            forKey:@"AudioEncoderFallback"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAACPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowAC3Pass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSHDPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowDTSPass"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"AudioAllowMP3Pass"];
    
    /* Audio track list - no need to add "None" at the end */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    /* Track 1 */
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)"                  forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"          forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1]     forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru"                forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"None"                        forKey:@"AudioMixdown"];
    [audioTrack2Array setObject:@"Auto"                        forKey:@"AudioSamplerate"];
    [audioTrack2Array setObject:@"160"                         forKey:@"AudioBitrate"];
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackGainSlider"];
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
    /* Add the audio track(s) to the preset's audio list */
    [preset setObject:[NSMutableArray arrayWithArray:audioListArray] forKey:@"AudioList"];
    
    /* Subtitles (note: currently ignored) */
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    /* Clean up and return the preset */
    [preset autorelease];
    return preset;
}

@end
