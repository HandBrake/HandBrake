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
    [UserPresets addObject:[self createApplePresetFolder]];
    [UserPresets addObject:[self createRegularPresetFolder]];
    [UserPresets addObject:[self createLegacyPresetFolder]];
    
    /* Independent presets at the root hierarchy level would go here */
    
    /* return the newly regenerated preset array back to Controller.mm */
    return UserPresets;
}

#pragma mark -

#pragma mark Built In Preset Folder Definitions

- (NSDictionary *)createApplePresetFolder
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
/*Set whether or not this is a folder, 1 is bool for folder*/
    [preset setObject:[NSNumber numberWithBool: YES] forKey:@"Folder"];


    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Apple" forKey:@"PresetName"];

    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];

    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    
    /* Lets initalize the child array of dictionaries for folders, this
     is an array of dictionaries much like the root level of presets and
     may contain folders and presets alike, etc.*/
    NSMutableArray *childrenArray = [[NSMutableArray alloc] init];
    /* we actually call the methods for the nests here */
    [childrenArray addObject:[self createAppleUniversalPreset]];
    [childrenArray addObject:[self createIpodLowPreset]];
    [childrenArray addObject:[self createiPhonePreset]];
    [childrenArray addObject:[self createAppleTVPreset]];

    [preset setObject:[NSMutableArray arrayWithArray: childrenArray] forKey:@"ChildrenArray"];
    
    [childrenArray autorelease];



    [preset autorelease];
    return preset;
}

- (NSDictionary *)createRegularPresetFolder
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
/*Set whether or not this is a folder, 1 is bool for folder*/
    [preset setObject:[NSNumber numberWithBool: YES] forKey:@"Folder"];


    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Regular" forKey:@"PresetName"];

    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];

    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    
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

- (NSDictionary *)createLegacyPresetFolder
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
/*Set whether or not this is a folder, 1 is bool for folder*/
    [preset setObject:[NSNumber numberWithBool: YES] forKey:@"Folder"];


    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Legacy" forKey:@"PresetName"];

    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];

    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    
    /* Lets initalize the child array of dictionaries for folders, this
     is an array of dictionaries much like the root level of presets and
     may contain folders and presets alike, etc.*/
    NSMutableArray *childrenArray = [[NSMutableArray alloc] init];
    /* we actually call the methods for the nests here */
    [childrenArray addObject:[self createClassicPreset]];
    [childrenArray addObject:[self createAppleTVLegacyPreset]];
    [childrenArray addObject:[self createiPhoneLegacyPreset]];
    [childrenArray addObject:[self createIpodHighPreset]];
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
    /*Set whether or not this is a folder, 1 is bool for folder*/
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];
    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];

    /*Get the whether or not to apply pic settings in the AddPresetPanel*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings for the AppleTV, including Dolby Digital 5.1 AC3 sound. Provides a good balance between quality and file size, and pushes the AppleTV to its limits." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];

    /* 64-bit MP4 file */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];

    /* x264 Option String (We can use this to tweak the appleTV output)*/
    [preset setObject:@"cabac=0:ref=2:mixed-refs=1:bframes=3:me=umh:subme=7:b-adapt=2:8x8dct=1" forKey:@"x264Option"];

    /* Video quality */
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:@"700" forKey:@"VideoTargetSize"];
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];

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

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VFR"];
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
    [audioTrack1Array setObject:@"48" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];

    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioMixdown"];
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

- (NSDictionary *)createAppleTVLegacyPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"AppleTV Legacy" forKey:@"PresetName"];

    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];

    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];

    /*Get the whether or not to apply pic settings in the AddPresetPanel*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's deprecated settings for the AppleTV, including Dolby Digital 5.1 AC3 sound. Provides a good balance between quality and file size, and optimizes performance. This is the AppleTV preset from HandBrake 0.9.2, and while it is offered as a service to legacy users, it is no longer supported." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];

    /* 64-bit MP4 file */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4LargeFile"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];

    /* x264 Option String (We can use this to tweak the appleTV output)*/
    [preset setObject:@"bframes=3:ref=1:subme=5:me=umh:no-fast-pskip=1:cabac=0" forKey:@"x264Option"];

    /* Video quality */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
    [preset setObject:@"700" forKey:@"VideoTargetSize"];
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];

    /* Basic Picture Settings */
    /* Use Max Picture settings for whatever the dvd is.*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VFR"];
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
    [audioTrack1Array setObject:@"48" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];

    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioMixdown"];
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

- (NSDictionary *)createAppleUniversalPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Universal" forKey:@"PresetName"];

    /*Set whether or not this is a user preset where 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
    /*Set whether or not this is a folder, 1 is bool for folder*/
    [preset setObject:[NSNumber numberWithBool: NO] forKey:@"Folder"];
    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];

    /*Get the whether or not to apply pic settings in the AddPresetPanel*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's universally compatible, full resolution settings for all current Apple devices: iPod (6G and up), iPhone, AppleTV, and Macs" forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];

    /* 64-bit MP4 file */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4LargeFile"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];

    /* x264 Option String (We can use this to tweak the appleTV output)*/
    [preset setObject:@"cabac=0:ref=2:mixed-refs=1:me=umh" forKey:@"x264Option"];

    /* Video quality */
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:@"700" forKey:@"VideoTargetSize"];
    [preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];

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

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VFR"];
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
    [audioTrack1Array setObject:@"48" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];

    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioMixdown"];
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

- (NSDictionary *)createClassicPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Classic" forKey:@"PresetName"];

    /*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];

    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];

    /*Get the whether or not to apply pic settings in the AddPresetPanel*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's traditional, faster, lower-quality settings." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:0] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"MPEG-4 (FFmpeg)" forKey:@"VideoEncoder"];

    /* x264 Option String */
    [preset setObject:@"" forKey:@"x264Option"];

    /* Video quality */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
    [preset setObject:@"700"  forKey:@"VideoTargetSize"];
    [preset setObject:@"1000" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];

    /*Picture Settings*/
    /* Use Max Picture settings for whatever the dvd is.*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VFR"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];

    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
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
    [audioTrack1Array setObject:@"48" forKey:@"AudioSamplerate"];
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

- (NSDictionary *)createHighProfilePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"High Profile" forKey:@"PresetName"];

    /*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];

    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];

    /*Get the whether or not to apply pic settings in the AddPresetPanel*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's general-purpose preset for High Profile H.264 video, with all the bells and whistles." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];

    /* x264 Option String */
    [preset setObject:@"ref=3:mixed-refs:bframes=3:weightb:b-pyramid:b-adapt=2:me=umh:subme=9:analyse=all:8x8dct" forKey:@"x264Option"];

    /* Video quality */
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:@"700" forKey:@"VideoTargetSize"];
    [preset setObject:@"1800" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTurboTwoPass"];

    /*Picture Settings*/
    /* Use Max Picture settings for whatever the dvd is.*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"PicturePAR"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDetelecine"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDecomb"];

    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
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
    [audioTrack1Array setObject:@"48" forKey:@"AudioSamplerate"];
    [audioTrack1Array setObject:@"160" forKey:@"AudioBitrate"];
    [audioTrack1Array setObject:[NSNumber numberWithFloat:0.0] forKey:@"AudioTrackDRCSlider"];
    [audioTrack1Array autorelease];
    [audioListArray addObject:audioTrack1Array];

    /* Track 2 */
    NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
    [audioTrack2Array setObject:[NSNumber numberWithInt:1] forKey:@"AudioTrack"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioEncoder"];
    [audioTrack2Array setObject:@"AC3 Passthru" forKey:@"AudioMixdown"];
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

- (NSDictionary *)createiPhonePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"iPhone & iPod Touch" forKey:@"PresetName"];

    /*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];

    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];

    /*Get the whether or not to apply pic settings in the AddPresetPanel*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings for the iPhone and iPod Touch." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Mp4iPodCompatible"];
    /* x264 Option String */
    [preset setObject:@"cabac=0:ref=2:mixed-refs:me=umh" forKey:@"x264Option"];

    /* Video quality */
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:@"700" forKey:@"VideoTargetSize"];
    [preset setObject:@"960" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];

    /*Picture Settings*/
    /* Use a width of 480 for the iPhone*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:480] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VFR"];
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
    [audioTrack1Array setObject:@"48" forKey:@"AudioSamplerate"];
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

- (NSDictionary *)createiPhoneLegacyPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"iPhone Legacy" forKey:@"PresetName"];

    /*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];

    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];

    /*Get the whether or not to apply pic settings in the AddPresetPanel*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's deprecated settings for the iPhone and iPod Touch. This is the iPhone preset from HandBrake 0.9.2, and while it is offered as a service to legacy users, it is no longer supported." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4iPodCompatible"];
    /* x264 Option String */
    [preset setObject:@"level=30:cabac=0:ref=1:analyse=all:me=umh:no-fast-pskip=1:psy-rd=0,0" forKey:@"x264Option"];

    /* Video quality */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
    [preset setObject:@"700" forKey:@"VideoTargetSize"];
    [preset setObject:@"960" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];

    /*Picture Settings*/
    /* Use a width of 480 for the iPhone*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:480] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VFR"];
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
    [audioTrack1Array setObject:@"48" forKey:@"AudioSamplerate"];
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

- (NSDictionary *)createIpodHighPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"iPod Legacy" forKey:@"PresetName"];

    /*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];

    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];

    /*Get the whether or not to apply pic settings in the AddPresetPanel*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's high resolution settings for older 5 and 5.5G iPods. Good video quality, great for viewing on a TV using your iPod. This is the iPod High-Rez preset from 0.9.2." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4iPodCompatible"];
    /* x264 Option String */
    [preset setObject:@"level=30:bframes=0:cabac=0:ref=1:vbv-maxrate=1500:vbv-bufsize=2000:analyse=all:me=umh:no-fast-pskip=1:psy-rd=0,0" forKey:@"x264Option"];

    /* Video quality */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
    [preset setObject:@"700" forKey:@"VideoTargetSize"];
    [preset setObject:@"1500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];

    /*Picture Settings*/
    /* Use a width of 640 for iPod TV-out */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:640] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VFR"];
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
    [audioTrack1Array setObject:@"48" forKey:@"AudioSamplerate"];
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

- (NSDictionary *)createIpodLowPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];

    /* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"iPod" forKey:@"PresetName"];

    /*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];

    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];

    /*Get the whether or not to apply pic settings in the AddPresetPanel*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's low resolution settings for the iPod (5G and up). Optimized for great playback on the iPod screen, with smaller file size." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Mp4iPodCompatible"];
    /* x264 Option String */
    [preset setObject:@"level=30:bframes=0:cabac=0:ref=1:vbv-maxrate=768:vbv-bufsize=2000:analyse=all:me=umh:no-fast-pskip=1" forKey:@"x264Option"];

    /* Video quality */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
    [preset setObject:@"700" forKey:@"VideoTargetSize"];
    [preset setObject:@"700" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];

    /*Picture Settings*/
    /* Use a width of 320 for the iPod screen */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:320] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VFR"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];

    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
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
    [audioTrack1Array setObject:@"48" forKey:@"AudioSamplerate"];
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

    /*Set whether or not this is default, at creation set to 0*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"Default"];

    /*Get the whether or not to apply pic settings in the AddPresetPanel*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];

    /* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's normal, default settings." forKey:@"PresetDescription"];

    /* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];

    /* Chapter Markers*/
     [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];

    /* Video encoder */
    [preset setObject:@"H.264 (x264)" forKey:@"VideoEncoder"];

    /* x264 Option String */
    [preset setObject:@"ref=2:bframes=2:me=umh" forKey:@"x264Option"];

    /* Video quality */
    [preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
    [preset setObject:@"700" forKey:@"VideoTargetSize"];
    [preset setObject:@"1500" forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithFloat:20.0] forKey:@"VideoQualitySlider"];

    /* Video framerate */
    [preset setObject:@"Same as source" forKey:@"VideoFramerate"];

    /* GrayScale */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];

    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTurboTwoPass"];

    /*Picture Settings*/
    /* Use Max Picture settings for whatever the dvd is.*/
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];

    /* Explicitly set the filters for built-in presets */
    [preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureFilters"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"VFR"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeblock"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDetelecine"];

    /* Set crop settings here */
    /* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
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
    [audioTrack1Array setObject:@"48" forKey:@"AudioSamplerate"];
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


@end
