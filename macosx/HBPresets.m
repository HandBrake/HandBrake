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

/* Called by -addFactoryPresets in Controller.mm */
- (NSMutableArray *) generateBuiltinPresets: (NSMutableArray *) UserPresets
{
    /* We receive the user presets array of dictionaries from controller.mm */
    /* We re-create new built in presets programmatically and add them to our presets array */
    
    /* Note: the built in presets will *not* sort themselves alphabetically, so they will
     * appear in the order you create them
     */
    /* Built in preset folders at the root of the hierarchy */
    [UserPresets addObject:[self createDevicesPresetFolder]];
    [UserPresets addObject:[self createRegularPresetFolder]];
    
    /* Independent presets at the root hierarchy level would go here */
    
    /* return the newly regenerated preset array back to Controller.mm */
    return UserPresets;
}

#pragma mark -

#pragma mark Built In Preset Folder Definitions

- (NSDictionary *)createDevicesPresetFolder
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Devices" forKey:@"PresetName"];

    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: YES] forKey:@"Folder"];
    
    /* Lets initalize the child array of dictionaries for folders, this
     is an array of dictionaries much like the root level of presets and
     may contain folders and presets alike, etc.*/
    NSMutableArray *childrenArray = [[NSMutableArray alloc] init];
    /* we actually call the methods for the nests here */
    [childrenArray addObject:[self createUniversalPreset]];
    [childrenArray addObject:[self createiPodPreset]];
    [childrenArray addObject:[self createiPhoneiPodtouchPreset]];
    [childrenArray addObject:[self createiPadPreset]];
    [childrenArray addObject:[self createAppleTVPreset]];
    [childrenArray addObject:[self createAppleTV2Preset]];
    [childrenArray addObject:[self createAppleTV3Preset]];
    [childrenArray addObject:[self createAndroidPreset]];
    [childrenArray addObject:[self createAndroidTabletPreset]];

    [preset setObject:[NSMutableArray arrayWithArray: childrenArray] forKey:@"ChildrenArray"];
    
    [childrenArray autorelease];



    [preset autorelease];
    return preset;
}

- (NSDictionary *)createRegularPresetFolder
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Regular" forKey:@"PresetName"];

    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: YES] forKey:@"Folder"];
    
    /* Lets initalize the child array of dictionaries for folders, this
     is an array of dictionaries much like the root level of presets and
     may contain folders and presets alike, etc.*/
    NSMutableArray *childrenArray = [[NSMutableArray alloc] init];
    /* we actually call the methods for the nests here */
    [childrenArray addObject:[self createNormalPreset]];
    [childrenArray addObject:[self createHighProfilePreset]];
    [preset setObject:[NSMutableArray arrayWithArray: childrenArray] forKey:@"ChildrenArray"];
    
    [childrenArray autorelease];



    [preset autorelease];
    return preset;
}

#pragma mark -

#pragma mark Built In Preset Definitions

/* These NSDictionary Buit-In Preset definitions contain all of the settings for one built in preset */
/* Note: For now, you can no longer have reference to any main window fields in your key values */

- (NSDictionary *)createAppleTVPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"AppleTV" forKey:@"PresetName"];

    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings for the AppleTV and 2009's iPhone and iPod Touch lineup. Provides a good balance between quality and file size, and pushes the devices to their limits. Includes Dolby Digital 5.1 AC3 sound for the AppleTV." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];

    /* x264 Option String (We can use this to tweak the appleTV output)*/
    [preset setObject:@"" forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium" forKey:@"x264Preset"];
    [preset setObject:@"" forKey:@"x264Tune"];
    [preset setObject:@"cabac=0:ref=2:b-pyramid=none:weightb=0:weightp=0:vbv-maxrate=9500:vbv-bufsize=9500" forKey:@"x264OptionExtra"];
    [preset setObject:@"high" forKey:@"h264Profile"];
    [preset setObject:@"3.1" forKey:@"h264Level"];

    /* Video quality */
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"30" forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];

    /* Basic Picture Settings */
    /* Use Max Picture settings for whatever the dvd is.*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:960] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];

    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];    
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];

    /* Audio - Is done on a track by track basis, ONLY specifiy the tracks we want set as any track
     * not listed will be set to "None" and not encoded */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    
    /* Track 1 */        
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)" forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"  forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];

    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"None" forKey:@"AudioMixdown"];
    [audioTrack2Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack2Array setObject:@"160" forKey:@"AudioBitrate"];
    /* Note: we ignore specified bitrate for AC3 Passthru in libhb and use
     * the sources bitrate, however we need to initially set the value to something so
     * the macgui doesnt barf, so 160 seems as good as anything */
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];

    [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];

    /* Subtitles*/
    [preset setObject:@"None" forKey:@"Subtitles"];

    [preset autorelease];
    return preset;
}

- (NSDictionary *)createUniversalPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Universal" forKey:@"PresetName"];

    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's universally compatible, full resolution settings for all current Apple devices: iPod (6G and up), iPhone, AppleTV, and Macs" forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];

    /* x264 Option String (We can use this to tweak the appleTV output)*/
    [preset setObject:@"" forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"fast" forKey:@"x264Preset"];
    [preset setObject:@"" forKey:@"x264Tune"];
    [preset setObject:@"" forKey:@"x264OptionExtra"];
    [preset setObject:@"baseline" forKey:@"h264Profile"];
    [preset setObject:@"3.0" forKey:@"h264Level"];

    /* Video quality */
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"30" forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];

    /* Basic Picture Settings */
    /* Use Max Picture settings for whatever the dvd is.*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:720] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];

    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];    
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];

    /* Audio - Is done on a track by track basis, ONLY specifiy the tracks we want set as any track
     * not listed will be set to "None" and not encoded */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    
    /* Track 1 */        
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)" forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"  forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];

    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"None" forKey:@"AudioMixdown"];
    [audioTrack2Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack2Array setObject:@"160" forKey:@"AudioBitrate"];
    /* Note: we ignore specified bitrate for AC3 Passthru in libhb and use
     * the sources bitrate, however we need to initially set the value to something so
     * the macgui doesnt barf, so 160 seems as good as anything */
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];

    [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];

    /* Subtitles*/
    [preset setObject:@"None" forKey:@"Subtitles"];

    [preset autorelease];
    return preset;
}

- (NSDictionary *)createiPadPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"iPad" forKey:@"PresetName"];

    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's preset for the iPad (all generations) is optimized for a good balance between quality and filesize." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];

    /* x264 Option String (We can use this to tweak the output)*/
    [preset setObject:@"" forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium" forKey:@"x264Preset"];
    [preset setObject:@"" forKey:@"x264Tune"];
    [preset setObject:@"" forKey:@"x264OptionExtra"];
    [preset setObject:@"high" forKey:@"h264Profile"];
    [preset setObject:@"3.1" forKey:@"h264Level"];

    /* Video quality */
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"29.97 (NTSC Video)" forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];

    /* Basic Picture Settings */
    /* Use Max Picture settings for whatever the dvd is.*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1280] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];

    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];    
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];

    /* Audio - Is done on a track by track basis, ONLY specifiy the tracks we want set as any track
     * not listed will be set to "None" and not encoded */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    
    /* Track 1 */        
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)" forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"  forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];

    [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];

    /* Subtitles*/
    [preset setObject:@"None" forKey:@"Subtitles"];

    [preset autorelease];
    return preset;
}

- (NSDictionary *)createAppleTV2Preset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	
    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"AppleTV 2" forKey:@"PresetName"];
	
    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];
	
    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's preset for the Apple TV (2nd gen) is optimized for viewing on its 1280x720 display." forKey:@"PresetDescription"];
	
    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
	
    /* Chapter Markers*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
	
    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];
	
    /* x264 Option String (We can use this to tweak the output)*/
    [preset setObject:@"" forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium" forKey:@"x264Preset"];
    [preset setObject:@"" forKey:@"x264Tune"];
    [preset setObject:@"" forKey:@"x264OptionExtra"];
    [preset setObject:@"high" forKey:@"h264Profile"];
    [preset setObject:@"3.1" forKey:@"h264Level"];
	
    /* Video quality */
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];
	
    /* Video framerate */
    [preset setObject:@"29.97 (NTSC Video)" forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	
    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];
	
    /* Basic Picture Settings */
    /* Use Max Picture settings for whatever the dvd is.*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1280] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];
	
    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
	
    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];    
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
    /* Audio - Is done on a track by track basis, ONLY specifiy the tracks we want set as any track
     * not listed will be set to "None" and not encoded */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    
    /* Track 1 */        
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)" forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"  forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
	
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"None" forKey:@"AudioMixdown"];
    [audioTrack2Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack2Array setObject:@"160" forKey:@"AudioBitrate"];
    /* Note: we ignore specified bitrate for AC3 Passthru in libhb and use
     * the sources bitrate, however we need to initially set the value to something so
     * the macgui doesnt barf, so 160 seems as good as anything */
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
	
	
    [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];
	
    /* Subtitles*/
    [preset setObject:@"None" forKey:@"Subtitles"];
	
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createAppleTV3Preset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	
    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"AppleTV 3" forKey:@"PresetName"];
	
    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];
	
    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's preset for the Apple TV (3rd gen) is optimized for up to 1080p playback." forKey:@"PresetDescription"];
	
    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
	
    /* Chapter Markers*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
	
    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];
	
    /* x264 Option String (We can use this to tweak the output)*/
    [preset setObject:@"" forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium" forKey:@"x264Preset"];
    [preset setObject:@"" forKey:@"x264Tune"];
    [preset setObject:@"" forKey:@"x264OptionExtra"];
    [preset setObject:@"high" forKey:@"h264Profile"];
    [preset setObject:@"4.0" forKey:@"h264Level"];
	
    /* Video quality */
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];
	
    /* Video framerate */
    [preset setObject:@"30" forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	
    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];
	
    /* Basic Picture Settings */
    /* Use Max Picture settings for whatever the dvd is.*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1920] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    
    /* We use Loose Anamorphic with a Modulus of 2 */
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];
	
    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    /* Note: decided to use Fast Decomb. This makes this the only device preset using
     * decomb that we have. Fast Decomb is better than no decomb imo and has basically no
     * speed hit on progressive sources. Once Default decomb is sped up, we can switch */
    [preset setObject:[NSNumber numberWithInt:3] forKey:@"PictureDecomb"];
	
    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];    
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
    /* Audio - Is done on a track by track basis, ONLY specifiy the tracks we want set as any track
     * not listed will be set to "None" and not encoded */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    
    /* Track 1 */        
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)" forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"  forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
	
    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"None" forKey:@"AudioMixdown"];
    [audioTrack2Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack2Array setObject:@"160" forKey:@"AudioBitrate"];
    /* Note: we ignore specified bitrate for AC3 Passthru in libhb and use
     * the sources bitrate, however we need to initially set the value to something so
     * the macgui doesnt barf, so 160 seems as good as anything */
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];
	
	
    [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];
	
    /* Subtitles*/
    [preset setObject:@"None" forKey:@"Subtitles"];
	
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createHighProfilePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"High Profile" forKey:@"PresetName"];

    /*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's general-purpose preset for High Profile H.264 video, with all the bells and whistles." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];

    /* x264 Option String */
    [preset setObject:@"" forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium" forKey:@"x264Preset"];
    [preset setObject:@"" forKey:@"x264Tune"];
    [preset setObject:@"" forKey:@"x264OptionExtra"];
    [preset setObject:@"high" forKey:@"h264Profile"];
    [preset setObject:@"4.1" forKey:@"h264Level"];

    /* Video quality */
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];
    [preset setObject:@"vfr" forKey:@"VideoFramerateMode"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTurboTwoPass"];

    /*Picture Settings*/
    /* Use Max Picture settings for whatever the dvd is.*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecombDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureDecomb"];

    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];

    /* Audio - Is done on a track by track basis, ONLY specifiy the tracks we want set as any track
     * not listed will be set to "None" and not encoded */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    
    /* Track 1 */        
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)" forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"  forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];

    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"None" forKey:@"AudioMixdown"];
    [audioTrack2Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack2Array setObject:@"160" forKey:@"AudioBitrate"];
    /* Note: we ignore specified bitrate for AC3 Passthru in libhb and use
     * the sources bitrate, however we need to initially set the value to something so
     * the macgui doesnt barf, so 160 seems as good as anything */
    [audioTrack2Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack2Array autorelease];
    [audioListArray addObject:audioTrack2Array];

    [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];

    /* Subtitles*/
    [preset setObject:@"None" forKey:@"Subtitles"];

    [preset autorelease];
    return preset;
}

- (NSDictionary *)createiPhoneiPodtouchPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"iPhone & iPod Touch" forKey:@"PresetName"];

    /*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings for all iPhones and iPod Touches going back to the original iPhone 2G." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];
    
    /* x264 Option String */
    [preset setObject:@"" forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium" forKey:@"x264Preset"];
    [preset setObject:@"" forKey:@"x264Tune"];
    [preset setObject:@"" forKey:@"x264OptionExtra"];
    [preset setObject:@"high" forKey:@"h264Profile"];
    [preset setObject:@"3.1" forKey:@"h264Level"];

    /* Video quality */
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:[NSNumber numberWithFloat:22.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"29.97 (NTSC Video)" forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];

    /*Picture Settings*/
    /* Use a width of 960 for the iPhone 4 and later */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:960] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];

    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];

    /* Audio - Is done on a track by track basis, ONLY specifiy the tracks we want set as any track
     * not listed will be set to "None" and not encoded */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    
    /* Track 1 */        
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)" forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"  forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];

    [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];

    /* Subtitles*/
    [preset setObject:@"None" forKey:@"Subtitles"];

    [preset autorelease];
    return preset;
}

- (NSDictionary *)createiPodPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"iPod" forKey:@"PresetName"];

    /*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's low resolution settings for the iPod (5G and up). Optimized for great playback on the iPod screen, with smaller file size." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4iPodCompatible"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];
    
    /* x264 Option String */
    [preset setObject:@"" forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium" forKey:@"x264Preset"];
    [preset setObject:@"" forKey:@"x264Tune"];
    [preset setObject:@"" forKey:@"x264OptionExtra"];
    [preset setObject:@"baseline" forKey:@"h264Profile"];
    [preset setObject:@"1.3" forKey:@"h264Level"];

    /* Video quality */
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:[NSNumber numberWithFloat:22.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"30" forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];

    /*Picture Settings*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:320] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];

    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];

    /* Audio - Is done on a track by track basis, ONLY specifiy the tracks we want set as any track
     * not listed will be set to "None" and not encoded */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    
    /* Track 1 */        
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)" forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"  forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];

    [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];

    /* Subtitles*/
    [preset setObject:@"None" forKey:@"Subtitles"];

    [preset autorelease];
    return preset;
}

- (NSDictionary *)createNormalPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Normal" forKey:@"PresetName"];

    /*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's normal, default settings." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];

    /* x264 Option String */
    [preset setObject:@"" forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"veryfast" forKey:@"x264Preset"];
    [preset setObject:@"" forKey:@"x264Tune"];
    [preset setObject:@"" forKey:@"x264OptionExtra"];
    [preset setObject:@"main" forKey:@"h264Profile"];
    [preset setObject:@"4.0" forKey:@"h264Level"];

    /* Video quality */
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];
    [preset setObject:@"vfr" forKey:@"VideoFramerateMode"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTurboTwoPass"];

    /*Picture Settings*/
    /* Use Max Picture settings for whatever the dvd is.*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];

    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];

    /* Audio - Is done on a track by track basis, ONLY specifiy the tracks we want set as any track
     * not listed will be set to "None" and not encoded */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    
    /* Track 1 */        
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)" forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"  forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];

    [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];

    /* Subtitles*/
    [preset setObject:@"None" forKey:@"Subtitles"];

    [preset autorelease];
    return preset;
}

- (NSDictionary *)createAndroidPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    
    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Android" forKey:@"PresetName"];
    
    /*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];
    
    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings for Mid-range Android 2.3 or better devices." forKey:@"PresetDescription"];
    
    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    
    /* Chapter Markers*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"ChapterMarkers"];
    
    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];
    
    /* x264 Option String */
    [preset setObject:@"" forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium" forKey:@"x264Preset"];
    [preset setObject:@"" forKey:@"x264Tune"];
    [preset setObject:@"" forKey:@"x264OptionExtra"];
    [preset setObject:@"main" forKey:@"h264Profile"];
    [preset setObject:@"2.2" forKey:@"h264Level"];
    
    /* Video quality */
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:[NSNumber numberWithFloat:22.0] forKey:@"VideoQualitySlider"];
    
    /* Video framerate */
    [preset setObject:@"29.97 (NTSC Video)" forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
    
    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];
    
    /*Picture Settings*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:720] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];
    
    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
    
    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
    
    /* Audio - Is done on a track by track basis, ONLY specifiy the tracks we want set as any track
     * not listed will be set to "None" and not encoded */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    
    /* Track 1 */        
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)" forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"  forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"128" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
    
    [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];
    
    /* Subtitles*/
    [preset setObject:@"None" forKey:@"Subtitles"];
    
    [preset autorelease];
    return preset;
}

- (NSDictionary *)createAndroidTabletPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	
    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Android Tablet" forKey:@"PresetName"];
	
    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];
	
    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's preset for the Higher end Anroid 2.3 or better devices." forKey:@"PresetDescription"];
	
    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4LargeFile"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
	
    /* Chapter Markers*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"ChapterMarkers"];
	
    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];
	
    /* x264 Option String (We can use this to tweak the output)*/
    [preset setObject:@"" forKey:@"x264Option"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
    [preset setObject:@"medium" forKey:@"x264Preset"];
    [preset setObject:@"" forKey:@"x264Tune"];
    [preset setObject:@"" forKey:@"x264OptionExtra"];
    [preset setObject:@"main" forKey:@"h264Profile"];
    [preset setObject:@"3.1" forKey:@"h264Level"];
	
    /* Video quality */
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:[NSNumber numberWithFloat:22.0] forKey:@"VideoQualitySlider"];
	
    /* Video framerate */
    [preset setObject:@"29.97 (NTSC Video)" forKey:@"VideoFramerate"];
    [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
    
    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	
    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];
	
    /* Basic Picture Settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1280] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PictureModulus"];
	
    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];
	
    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];    
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
    /* Audio - Is done on a track by track basis, ONLY specifiy the tracks we want set as any track
     * not listed will be set to "None" and not encoded */
    NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
    
    /* Track 1 */        
    NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
    [audioTrack1Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack1Array setObject:@"AAC (faac)" forKey:@"AudioEncoder"];
    [audioTrack1Array setObject:@"Dolby Pro Logic II"  forKey:@"AudioMixdown"];
    [audioTrack1Array setObject:@"Auto" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"128" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];
	
    [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];
	
    /* Subtitles*/
    [preset setObject:@"None" forKey:@"Subtitles"];
	
    [preset autorelease];
    return preset;
}


@end
