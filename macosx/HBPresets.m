/*  HBPresets.m $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
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
    [UserPresets addObject:[self createNormalPreset]];
    [UserPresets addObject:[self createClassicPreset]];
    [UserPresets addObject:[self createQuickTimePreset]];
	[UserPresets addObject:[self createIpodLowPreset]];
	[UserPresets addObject:[self createIpodHighPreset]];
	[UserPresets addObject:[self createAppleTVPreset]];
    [UserPresets addObject:[self createiPhonePreset]];
	[UserPresets addObject:[self createPSThreePreset]];
	[UserPresets addObject:[self createPSPPreset]];
	[UserPresets addObject:[self createFilmPreset]];
    [UserPresets addObject:[self createTelevisionPreset]];
    [UserPresets addObject:[self createAnimationPreset]];
    [UserPresets addObject:[self createBedlamPreset]];
    [UserPresets addObject:[self createDeuxSixQuatrePreset]];
    [UserPresets addObject:[self createBrokePreset]];
    [UserPresets addObject:[self createBlindPreset]];
    [UserPresets addObject:[self createCRFPreset]];
    /* return the newly regenerated preset array back to Controller.mm */
    return UserPresets;
}



#pragma mark -
#pragma mark Built In Preset Definitions
/* These NSDictionary Buit-In Preset definitions contain all of the settings for one built in preset */
/* Note: For now, you can no longer have reference to any main window fields in your key values */
- (NSDictionary *)createIpodLowPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"iPod Low-Rez" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's low resolution settings for the iPod. Optimized for great playback on the iPod screen, with smaller file size." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 iPod)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"keyint=300:keyint-min=30:bframes=0:cabac=0:ref=1:vbv-maxrate=768:vbv-bufsize=2000:analyse=all:me=umh:subme=6:no-fast-pskip=1" forKey:@"x264Option"];
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
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:320] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createIpodHighPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"iPod High-Rez" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's high resolution settings for the iPod. Good video quality, great for viewing on a TV using your iPod" forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 iPod)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"keyint=300:keyint-min=30:bframes=0:cabac=0:ref=1:vbv-maxrate=1500:vbv-bufsize=2000:analyse=all:me=umh:subme=6:no-fast-pskip=1" forKey:@"x264Option"];
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
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:640] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createAppleTVPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"AppleTV" forKey:@"PresetName"];
	/*Set whether or not this is a user preset where 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:2] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings for the AppleTV. Provides a good balance between quality and file size, and optimizes performance." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
	/* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String (We can use this to tweak the appleTV output)*/
	[preset setObject:@"bframes=3:ref=1:subme=5:me=umh:no-fast-pskip=1:trellis=2:cabac=0" forKey:@"x264Option"];
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
	
	/*Picture Settings*/
	/* For AppleTV we only want to retain UsesMaxPictureSettings
	which depend on the source dvd picture settings, so we don't
	record the current dvd's picture info since it will vary from
	source to source*/
	//hb_job_t * job = fTitle->job;
	//hb_job_t * job = title->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createPSThreePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"PS3" forKey:@"PresetName"];
	/*Set whether or not this is a user preset where 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:2] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings for the Sony PlayStation 3." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:0] forKey:@"ChapterMarkers"];
	/* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String (We can use this to tweak the appleTV output)*/
	[preset setObject:@"level=41:subme=5:me=umh" forKey:@"x264Option"];
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
	
	/*Picture Settings*/
	/* For PS3 we only want to retain UsesMaxPictureSettings
	which depend on the source dvd picture settings, so we don't
	record the current dvd's picture info since it will vary from
	source to source*/
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}
- (NSDictionary *)createPSPPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"PSP" forKey:@"PresetName"];
	/*Set whether or not this is a user preset where 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings for the Sony PlayStation Portable." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
	/* Codecs */
	[preset setObject:@"MPEG-4 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"FFmpeg" forKey:@"VideoEncoder"];
	/* x264 Option String (We can use this to tweak the appleTV output)*/
	[preset setObject:@"" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
	[preset setObject:@"700" forKey:@"VideoTargetSize"];
	[preset setObject:@"1024" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
	[preset setObject:@"Same as source" forKey:@"VideoFramerate"];
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	/* 2 Pass Encoding */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];
	
	/*Picture Settings*/
	/* For PS3 we only want to retain UsesMaxPictureSettings
	which depend on the source dvd picture settings, so we don't
	record the current dvd's picture info since it will vary from
	source to source*/
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:@"368" forKey:@"PictureWidth"];
	[preset setObject:@"208" forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"128" forKey:@"AudioBitRate"];
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
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"ref=2:bframes=2:subme=5:me=umh" forKey:@"x264Option"];
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
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTwoPass"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTurboTwoPass"];
	
	/*Picture Settings*/
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
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
    /* Codecs */
	[preset setObject:@"MPEG-4 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"FFmpeg" forKey:@"VideoEncoder"];
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
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createFilmPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Film" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's preset for feature films." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MKV file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AC-3 Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"ref=3:mixed-refs:bframes=16:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=6:analyse=all:8x8dct:trellis=1:no-fast-pskip" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
	[preset setObject:@"700" forKey:@"VideoTargetSize"];
	[preset setObject:@"1800" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
	[preset setObject:@"Same as source" forKey:@"VideoFramerate"];
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	/* 2 Pass Encoding */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTwoPass"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTurboTwoPass"];
	
	/*Picture Settings*/
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createTelevisionPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Television" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings for video from television." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MKV file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"ref=3:mixed-refs:bframes=16:bime:weightb:direct=auto:b-pyramid:me=umh:subme=6:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
	[preset setObject:@"700" forKey:@"VideoTargetSize"];
	[preset setObject:@"1300" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
	[preset setObject:@"Same as source" forKey:@"VideoFramerate"];
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	/* 2 Pass Encoding */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTwoPass"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTurboTwoPass"];
	
	/*Picture Settings*/
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createAnimationPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Animation" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings for cartoons, anime, and CGI." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MKV file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"ref=5:mixed-refs:bframes=16:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=5:analyse=all:8x8dct:trellis=1:nr=150:no-fast-pskip:filter=2,2" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
	[preset setObject:@"700" forKey:@"VideoTargetSize"];
	[preset setObject:@"1000" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
	[preset setObject:@"Same as source" forKey:@"VideoFramerate"];
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	/* 2 Pass Encoding */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTwoPass"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTurboTwoPass"];
	
	/*Picture Settings*/
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createQuickTimePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"QuickTime" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's high quality settings for use with QuickTime. It can be slow, so use it when the Normal preset doesn't look good enough." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"ref=3:mixed-refs:bframes=3:bime:weightb:b-rdo:direct=auto:me=umh:subme=5:analyse=all:trellis=1:no-fast-pskip" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
	[preset setObject:@"700" forKey:@"VideoTargetSize"];
	[preset setObject:@"2000" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
	[preset setObject:@"Same as source" forKey:@"VideoFramerate"];
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	/* 2 Pass Encoding */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTwoPass"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTurboTwoPass"];
	
	/*Picture Settings*/
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createBedlamPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Bedlam" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings maxed out for slowest encoding and highest quality. Use at your own risk. So slow it's not just insane...it's a trip to the looney bin." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MKV file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AC-3 Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"ref=16:mixed-refs:bframes=16:bime:weightb:b-rdo:direct=auto:b-pyramid:me=umh:subme=7:me-range=64:analyse=all:8x8dct:trellis=2:no-fast-pskip:no-dct-decimate:filter=-2,-1" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
	[preset setObject:@"700" forKey:@"VideoTargetSize"];
	[preset setObject:@"1800" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
	[preset setObject:@"Same as source" forKey:@"VideoFramerate"];
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	/* 2 Pass Encoding */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTwoPass"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTurboTwoPass"];
	
	/*Picture Settings*/
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createiPhonePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"iPhone / iPod Touch" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's settings for the iPhone." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 iPod)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"cabac=0:ref=1:analyse=all:me=umh:subme=6:no-fast-pskip=1:trellis=1" forKey:@"x264Option"];
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
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:480] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"128" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createDeuxSixQuatrePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Deux Six Quatre" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's preset for true high profile x264 quality. A good balance of quality and speed, based on community standards found in the wild. This preset will give you a much better sense of x264's capabilities than vanilla main profile." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MKV file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AC-3 Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"ref=5:mixed-refs:bframes=3:bime:weightb:b-rdo:b-pyramid:me=umh:subme=7:trellis=1:analyse=all:8x8dct:no-fast-pskip" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
	[preset setObject:@"700" forKey:@"VideoTargetSize"];
	[preset setObject:@"1600" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
	[preset setObject:@"Same as source" forKey:@"VideoFramerate"];
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	/* 2 Pass Encoding */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTwoPass"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTurboTwoPass"];
	
	/*Picture Settings*/
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createBrokePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Broke" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's preset for people without a lot of money to waste on hard drives. Tries to maximize quality for burning to CDs, so you can party like it's 1999." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"ref=3:mixed-refs:bframes=16:bime:weightb:b-rdo:b-pyramid:direct=auto:me=umh:subme=6:trellis=1:analyse=all:8x8dct:no-fast-pskip" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoQualityType"];
	[preset setObject:@"695" forKey:@"VideoTargetSize"];
	[preset setObject:@"1600" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
	[preset setObject:@"Same as source" forKey:@"VideoFramerate"];
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	/* 2 Pass Encoding */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTwoPass"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoTurboTwoPass"];
	
	/*Picture Settings*/
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:640] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"128" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createBlindPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Blind" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's preset for impatient people who don't care about picture quality." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"MPEG-4 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"FFmpeg" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
	[preset setObject:@"700" forKey:@"VideoTargetSize"];
	[preset setObject:@"512" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
	[preset setObject:@"Same as source" forKey:@"VideoFramerate"];
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	/* 2 Pass Encoding */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTurboTwoPass"];
	
	/*Picture Settings*/
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:512] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"128" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)createCRFPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"Constant Quality Rate" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* Get the New Preset Description from the field in the AddPresetPanel */
    [preset setObject:@"HandBrake's preset for consistently excellent quality in one pass, with the downside of entirely unpredictable file sizes and bitrates." forKey:@"PresetDescription"];
	/* File Format */
    [preset setObject:@"MKV file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AC-3 Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"ref=3:mixed-refs:bframes=3:b-pyramid:b-rdo:bime:weightb:filter=-2,-1:subme=6:trellis=1:analyse=all:8x8dct:me=umh" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:2] forKey:@"VideoQualityType"];
	[preset setObject:@"700" forKey:@"VideoTargetSize"];
	[preset setObject:@"2000" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:0.6471] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
	[preset setObject:@"Same as source" forKey:@"VideoFramerate"];
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoGrayScale"];
	/* 2 Pass Encoding */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTwoPass"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"VideoTurboTwoPass"];
	
	/*Picture Settings*/
	//hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PictureAutoCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:@"48" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}



@end
