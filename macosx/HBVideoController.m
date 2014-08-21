/*  HBVideoController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBVideoController.h"

#include "hb.h"

#import "Controller.h"
#import "HBAdvancedController.h"

NSString *HBVideoEncoderChangedNotification = @"HBVideoEncoderChangedNotification";

@interface HBVideoController () {
    /* Framerate */
    /* Radio Button Framerate Controls */
    IBOutlet NSMatrix            * fFramerateMatrix;
    IBOutlet NSButtonCell        * fFramerateVfrPfrCell;
    IBOutlet NSButtonCell        * fFramerateCfrCell;

    /* Video Encoder */
    IBOutlet NSTextField         * fVidRateField;
    IBOutlet NSPopUpButton       * fVidRatePopUp;
    IBOutlet NSTextField         * fVidEncoderField;
    IBOutlet NSPopUpButton       * fVidEncoderPopUp;
    IBOutlet NSTextField         * fVidQualityField;
    IBOutlet NSTextField         * fVidQualityRFLabel;
    IBOutlet NSTextField         * fVidQualityRFField;
    IBOutlet NSMatrix            * fVidQualityMatrix;
    IBOutlet NSButtonCell        * fVidBitrateCell;
    IBOutlet NSTextField         * fVidBitrateField;
    IBOutlet NSButtonCell        * fVidConstantCell;
    IBOutlet NSSlider            * fVidQualitySlider;
    IBOutlet NSButton            * fVidTwoPassCheck;
    IBOutlet NSButton            * fVidTurboPassCheck;

    /* Status read out fields for picture settings and video filters */
    IBOutlet NSTextField         * fPictureSettingsField;
    IBOutlet NSTextField         * fPictureFiltersField;


    /* Encoder options views */
    IBOutlet NSView         *fPresetView;
    IBOutlet NSView         *fSimplePresetView;

    /* Simple Presets Box */
    IBOutlet NSTextField    *fLavcOptionsTextField;
    IBOutlet NSTextField    *fLavcOptionsLabel;

    /* x264/x265 Presets Box */
    NSArray                      * fPresetNames;
    NSUInteger                     fMediumPresetIndex;
    IBOutlet NSButton            * fX264UseAdvancedOptionsCheck;
    IBOutlet NSBox               * fPresetsBox;
    IBOutlet NSSlider            * fPresetsSlider;
    IBOutlet NSTextField         * fPresetSliderLabel;
    IBOutlet NSTextField         * fPresetSelectedTextField;
    IBOutlet NSPopUpButton       * fTunePopUp;
    IBOutlet NSTextField         * fTunePopUpLabel;
    IBOutlet NSPopUpButton       * fProfilePopUp;
    IBOutlet NSTextField         * fProfilePopUpLabel;
    IBOutlet NSPopUpButton       * fLevelPopUp;
    IBOutlet NSTextField         * fLevelPopUpLabel;
    IBOutlet NSButton            * fFastDecodeCheck;
    IBOutlet NSTextField         * fDisplayPresetsAdditonalOptionsTextField;
    IBOutlet NSTextField         * fDisplayPresetsAdditonalOptionsLabel;
    // Text Field to show the expanded opts from unparse()
    IBOutlet NSTextField         * fDisplayX264PresetsUnparseTextField;
    char                         * fX264PresetsUnparsedUTF8String;
}

@property (nonatomic, readwrite) int codec;
@property (nonatomic, readwrite) int qualityType;

@end

@implementation HBVideoController

- (void)setPictureSettingsField:(NSString *)string
{
    if (string)
    {
        [_pictureSettingsField autorelease];
        _pictureSettingsField = [[NSString stringWithFormat:@"Picture Settings: %@", string] retain];
    }
}

- (void)setPictureFiltersField:(NSString *)string
{
    if (string)
    {
        [_pictureFiltersField autorelease];
        _pictureFiltersField = [[NSString stringWithFormat:@"Picture Filters: %@", string] retain];
    }
}

- (NSString *)selectedBitrate
{
    return [fVidBitrateField stringValue];
}

- (NSString *)selectedQuality
{
    return [fVidQualityRFField stringValue];
}


- (instancetype)init
{
    self = [super initWithNibName:@"Video" bundle:nil];
    if (self)
    {
        /*
         * initialize fX264PresetsUnparsedUTF8String as early as possible
         * avoids an invalid free
         */
        fX264PresetsUnparsedUTF8String = NULL;
        _pictureFiltersField = @"Picture Filters:";
        _pictureSettingsField = @"Picture Settings:";

        NSNotificationCenter *center = [NSNotificationCenter defaultCenter];

        /* register that we are interested in changes made to the video container */
        [center addObserver:self selector: @selector(containerChanged:) name:HBContainerChangedNotification object:nil];
        [center addObserver:self selector: @selector(titleChanged:) name:HBTitleChangedNotification object:nil];
    }

    return self;
}

- (void)loadView {
    [super loadView];

    /* Video encoder */
    [fVidEncoderPopUp removeAllItems];
    [fVidEncoderPopUp addItemWithTitle:@"H.264 (x264)"];

    /* setup our x264/x265 presets widgets */
    [self switchPresetViewForEncoder:HB_VCODEC_X264];

    /* Video quality */
	[fVidBitrateField setIntValue: 1000];
    self.qualityType = 0;

    [self videoMatrixChanged:nil];

    /* Video framerate */
    [fVidRatePopUp removeAllItems];
    NSMenuItem *menuItem = [[fVidRatePopUp menu] addItemWithTitle:@"Same as source"
                                               action:nil
                                        keyEquivalent:@""];
    [menuItem setTag:hb_video_framerate_get_from_name("Same as source")];
    for (const hb_rate_t *video_framerate = hb_video_framerate_get_next(NULL);
         video_framerate != NULL;
         video_framerate  = hb_video_framerate_get_next(video_framerate))
    {
        NSString *itemTitle;
        if (!strcmp(video_framerate->name, "23.976"))
        {
            itemTitle = @"23.976 (NTSC Film)";
        }
        else if (!strcmp(video_framerate->name, "25"))
        {
            itemTitle = @"25 (PAL Film/Video)";
        }
        else if (!strcmp(video_framerate->name, "29.97"))
        {
            itemTitle = @"29.97 (NTSC Video)";
        }
        else
        {
            itemTitle = @(video_framerate->name);
        }
        menuItem = [[fVidRatePopUp menu] addItemWithTitle:itemTitle
                                                   action:nil
                                            keyEquivalent:@""];
        [menuItem setTag:video_framerate->rate];
    }
    [fVidRatePopUp selectItemAtIndex:0];

    /* We disable the Turbo 1st pass checkbox since we are not x264 */
	[fVidTurboPassCheck setEnabled: NO];
	[fVidTurboPassCheck setState: NSOffState];
}

- (void)setUIEnabled:(BOOL)flag {
    NSControl *controls[] =
    {
        fFramerateMatrix,
        fVidRateField,
        fVidRatePopUp,
        fVidEncoderField,
        fVidEncoderPopUp,
        fVidQualityField,
        fVidQualityRFLabel,
        fVidQualityRFField,
        fVidQualityMatrix,
        fVidBitrateField,
        fVidQualitySlider,
        fVidTwoPassCheck,
        fVidTurboPassCheck,
        fPictureSettingsField,
        fPictureFiltersField
    };

    for (unsigned i = 0; i < (sizeof(controls) / sizeof(NSControl *)); i++)
    {
        if ([[controls[i] className] isEqualToString: @"NSTextField"])
        {
            NSTextField *tf = (NSTextField *)controls[i];
            if (![tf isBezeled])
            {
                [tf setTextColor: (flag ?
                                   [NSColor controlTextColor] :
                                   [NSColor disabledControlTextColor])];
                continue;
            }
        }
        [controls[i] setEnabled:flag];
    }

    [self videoMatrixChanged:nil];
    [self enableEncoderOptionsWidgets:flag];
}

- (void)containerChanged:(NSNotification *)aNotification
{
    NSDictionary *notDict = [aNotification userInfo];

    int videoContainer = [notDict[keyContainerTag] intValue];

    /* lets get the tag of the currently selected item first so we might reset it later */
    int selectedVidEncoderTag = self.codec;
    BOOL encoderSupported = NO;

    /* Note: we now store the video encoder int values from common.c in the tags of each popup for easy retrieval later */
    [fVidEncoderPopUp removeAllItems];
    for (const hb_encoder_t *video_encoder = hb_video_encoder_get_next(NULL);
         video_encoder != NULL;
         video_encoder  = hb_video_encoder_get_next(video_encoder))
    {
        if (video_encoder->muxers & videoContainer)
        {
            NSMenuItem *menuItem = [[fVidEncoderPopUp menu] addItemWithTitle:@(video_encoder->name)
                                                          action:nil
                                                   keyEquivalent:@""];
            [menuItem setTag:video_encoder->codec];

            if (selectedVidEncoderTag == video_encoder->codec)
            {
                encoderSupported = YES;
            }
        }
    }

    if (!encoderSupported)
    {
        self.codec = hb_video_encoder_get_default(videoContainer);
        [self videoEncoderPopUpChanged:nil];
    }
}

- (void)titleChanged:(NSNotification *)aNotification
{
    [fVidRatePopUp selectItemAtIndex: 0];
}

#pragma mark - apply settings

- (void)applyVideoSettingsFromQueue:(NSDictionary *)queueToApply
{
    /* video encoder */
    self.codec = [queueToApply[@"JobVideoEncoderVcodec"] intValue];

    self.lavcOptions = queueToApply[@"lavcOption"];

    /* advanced x264 options */
    if ([queueToApply[@"x264UseAdvancedOptions"] intValue])
    {
        // we are using the advanced panel
        [self.fAdvancedOptions setOptions:queueToApply[@"x264Option"]];
        // preset does not use the x264 preset system, reset the widgets
        [self setPreset:     nil];
        [self setTune:       nil];
        [self setOptionExtra:queueToApply[@"x264Option"]];
        [self setProfile:    nil];
        [self setLevel:      nil];
        // enable the advanced panel and update the widgets
        [fX264UseAdvancedOptionsCheck setState:NSOnState];
        [self updateEncoderOptionsWidgets:nil];
    }
    else
    {
        // we are using the x264 preset system
        [self setPreset:     queueToApply[@"VideoPreset"]];
        [self setTune:       queueToApply[@"VideoTune"]];
        [self setOptionExtra:queueToApply[@"VideoOptionExtra"]];
        [self setProfile:    queueToApply[@"VideoProfile"]];
        [self setLevel:      queueToApply[@"VideoLevel"]];
        // preset does not use the advanced panel, reset it
        [self.fAdvancedOptions setOptions:@""];
        // disable the advanced panel and update the widgets
        [fX264UseAdvancedOptionsCheck setState:NSOffState];
        [self updateEncoderOptionsWidgets:nil];
    }

    /* Lets run through the following functions to get variables set there */
    [self videoEncoderPopUpChanged:nil];

    /* Video quality */
    self.qualityType = [queueToApply[@"VideoQualityType"] intValue];

    [fVidBitrateField setStringValue:queueToApply[@"VideoAvgBitrate"]];

    int direction;
    float minValue, maxValue, granularity;

    hb_video_quality_get_limits(self.codec, &minValue, &maxValue, &granularity, &direction);

    if (!direction)
    {
        [fVidQualitySlider setFloatValue:[queueToApply[@"VideoQualitySlider"] floatValue]];
    }
    else
    {
        /*
         * Since ffmpeg and x264 use an "inverted" slider (lower values
         * indicate a higher quality) we invert the value on the slider
         */
        [fVidQualitySlider setFloatValue:([fVidQualitySlider minValue] +
                                          [fVidQualitySlider maxValue] -
                                          [queueToApply[@"VideoQualitySlider"] floatValue])];
    }

    [self videoMatrixChanged:nil];

    /* Video framerate */
    if ([queueToApply[@"VideoFramerate"] isEqualToString:@"Same as source"])
    {
        /* Now set the Video Frame Rate Mode to either vfr or cfr according to the preset */
        if ([queueToApply[@"VideoFramerateMode"] isEqualToString:@"vfr"])
        {
            [fFramerateMatrix selectCellAtRow:0 column:0]; // we want vfr
        }
        else
        {
            [fFramerateMatrix selectCellAtRow:1 column:0]; // we want cfr
        }
    }
    else
    {
        /* Now set the Video Frame Rate Mode to either pfr or cfr according to the preset */
        if ([queueToApply[@"VideoFramerateMode"] isEqualToString:@"pfr"])
        {
            [fFramerateMatrix selectCellAtRow:0 column:0]; // we want pfr
        }
        else
        {
            [fFramerateMatrix selectCellAtRow:1 column:0]; // we want cfr
        }
    }
    [fVidRatePopUp selectItemWithTitle:queueToApply[@"VideoFramerate"]];
    [self videoFrameRateChanged:nil];

    /* 2 Pass Encoding */
    [fVidTwoPassCheck setState:[queueToApply[@"VideoTwoPass"] intValue]];
    [self twoPassCheckboxChanged:nil];
    /* Turbo 1st pass for 2 Pass Encoding */
    [fVidTurboPassCheck setState:[queueToApply[@"VideoTurboTwoPass"] intValue]];
}

- (void)applySettingsFromPreset:(NSDictionary *)preset
{
    /* map legacy encoder names via libhb */
    const char *strValue = hb_video_encoder_sanitize_name([preset[@"VideoEncoder"] UTF8String]);

    self.codec = hb_video_encoder_get_from_name(strValue);
    [self videoEncoderPopUpChanged:nil];

    if (self.codec == HB_VCODEC_X264 || self.codec == HB_VCODEC_X265)
    {
        if (self.codec == HB_VCODEC_X264 &&
            (!preset[@"x264UseAdvancedOptions"] ||
            [preset[@"x264UseAdvancedOptions"] intValue]))
        {
            /*
             * x264UseAdvancedOptions is not set (legacy preset)
             * or set to 1 (enabled), so we use the old advanced panel
             */
            if (preset[@"x264Option"])
            {
                /* we set the advanced options string here if applicable */
                [self.fAdvancedOptions setOptions:preset[@"x264Option"]];
                [self setOptionExtra:preset[@"x264Option"]];
            }
            else
            {
                [self.fAdvancedOptions setOptions:        @""];
                [self             setOptionExtra:nil];
            }
            /* preset does not use the x264 preset system, reset the widgets */
            [self setPreset: nil];
            [self setTune:   nil];
            [self setProfile:nil];
            [self setLevel:  nil];
            /* we enable the advanced panel and update the widgets */
            [fX264UseAdvancedOptionsCheck setState:NSOnState];
            [self updateEncoderOptionsWidgets:nil];
        }
        else
        {
            /*
             * x264UseAdvancedOptions is set to 0 (disabled),
             * so we use the x264 preset system
             */
            if (preset[@"x264Preset"])
            {
                [self setPreset:     preset[@"x264Preset"]];
                [self setTune:       preset[@"x264Tune"]];
                [self setOptionExtra:preset[@"x264OptionExtra"]];
                [self setProfile:    preset[@"h264Profile"]];
                [self setLevel:      preset[@"h264Level"]];
            }
            else
            {
                [self setPreset:     preset[@"VideoPreset"]];
                [self setTune:       preset[@"VideoTune"]];
                [self setOptionExtra:preset[@"VideoOptionExtra"]];
                [self setProfile:    preset[@"VideoProfile"]];
                [self setLevel:      preset[@"VideoLevel"]];
            }
            /* preset does not use the advanced panel, reset it */
            [self.fAdvancedOptions setOptions:@""];
            /* we disable the advanced panel and update the widgets */
            [fX264UseAdvancedOptionsCheck setState:NSOffState];
            [self updateEncoderOptionsWidgets:nil];
        }
    }

    int qualityType = [preset[@"VideoQualityType"] intValue] - 1;
    /* Note since the removal of Target Size encoding, the possible values for VideoQuality type are 0 - 1.
     * Therefore any preset that uses the old 2 for Constant Quality would now use 1 since there is one less index
     * for the fVidQualityMatrix. It should also be noted that any preset that used the deprecated Target Size
     * setting of 0 would set us to 0 or ABR since ABR is now tagged 0. Fortunately this does not affect any built-in
     * presets since they all use Constant Quality or Average Bitrate.*/
    if (qualityType == -1)
    {
        qualityType = 0;
    }
    self.qualityType = qualityType;

    [fVidBitrateField setStringValue:preset[@"VideoAvgBitrate"]];

    int direction;
    float minValue, maxValue, granularity;

    hb_video_quality_get_limits(self.codec, &minValue, &maxValue, &granularity, &direction);

    if (!direction)
    {
        [fVidQualitySlider setFloatValue:[preset[@"VideoQualitySlider"] floatValue]];
    }
    else
    {
        /*
         * Since ffmpeg and x264 use an "inverted" slider (lower values
         * indicate a higher quality) we invert the value on the slider
         */
        [fVidQualitySlider setFloatValue:([fVidQualitySlider minValue] +
                                          [fVidQualitySlider maxValue] -
                                          [preset[@"VideoQualitySlider"] floatValue])];
    }

    [self videoMatrixChanged:nil];

    /* Video framerate */
    if ([preset[@"VideoFramerate"] isEqualToString:@"Same as source"])
    {
        /* Now set the Video Frame Rate Mode to either vfr or cfr according to the preset */
        if (!preset[@"VideoFramerateMode"] ||
            [preset[@"VideoFramerateMode"] isEqualToString:@"vfr"])
        {
            [fFramerateMatrix selectCellAtRow:0 column:0]; // we want vfr
        }
        else
        {
            [fFramerateMatrix selectCellAtRow:1 column:0]; // we want cfr
        }
    }
    else
    {
        /* Now set the Video Frame Rate Mode to either pfr or cfr according to the preset */
        if ([preset[@"VideoFramerateMode"] isEqualToString:@"pfr"] ||
            [preset[@"VideoFrameratePFR"]  intValue] == 1)
        {
            [fFramerateMatrix selectCellAtRow:0 column:0]; // we want pfr
        }
        else
        {
            [fFramerateMatrix selectCellAtRow:1 column:0]; // we want cfr
        }
    }
    /* map legacy names via libhb */
    int intValue = hb_video_framerate_get_from_name([preset[@"VideoFramerate"] UTF8String]);
    [fVidRatePopUp selectItemWithTag:intValue];
    [self videoFrameRateChanged:nil];

    /* 2 Pass Encoding */
    [fVidTwoPassCheck setState:[preset[@"VideoTwoPass"] intValue]];
    [self twoPassCheckboxChanged:nil];

    /* Turbo 1st pass for 2 Pass Encoding */
    [fVidTurboPassCheck setState:[preset[@"VideoTurboTwoPass"] intValue]];
}

- (void)prepareVideoForQueueFileJob:(NSMutableDictionary *)queueFileJob
{
    queueFileJob[@"VideoEncoder"] = @(hb_video_encoder_get_name(self.codec));

    /* x264 advanced options */
    if ([fX264UseAdvancedOptionsCheck state])
    {
        // we are using the advanced panel
        queueFileJob[@"x264UseAdvancedOptions"] = @1;
        queueFileJob[@"x264Option"] = [self.fAdvancedOptions optionsString];
    }
    else
    {
        // we are using the x264/x265 preset system
        queueFileJob[@"x264UseAdvancedOptions"] = @0;
        queueFileJob[@"VideoPreset"] = [self preset];
        queueFileJob[@"VideoTune"] = [self tune];
        queueFileJob[@"VideoOptionExtra"] = [self optionExtra];
        queueFileJob[@"VideoProfile"] = [self profile];
        queueFileJob[@"VideoLevel"] = [self level];
    }

    /* FFmpeg (lavc) Option String */
    queueFileJob[@"lavcOption"] = self.lavcOptions;

	queueFileJob[@"VideoQualityType"] = @(self.qualityType + 1);
	queueFileJob[@"VideoAvgBitrate"] = [fVidBitrateField stringValue];
	queueFileJob[@"VideoQualitySlider"] = @([fVidQualityRFField floatValue]);
    /* Framerate */
    queueFileJob[@"VideoFramerate"] = [fVidRatePopUp titleOfSelectedItem];
    /* Frame Rate Mode */
    if ([fFramerateMatrix selectedRow] == 1) // if selected we are cfr regardless of the frame rate popup
    {
        queueFileJob[@"VideoFramerateMode"] = @"cfr";
    }
    else
    {
        if ([fVidRatePopUp indexOfSelectedItem] == 0) // Same as source frame rate
        {
            queueFileJob[@"VideoFramerateMode"] = @"vfr";
        }
        else
        {
            queueFileJob[@"VideoFramerateMode"] = @"pfr";
        }

    }

	/* 2 Pass Encoding */
	queueFileJob[@"VideoTwoPass"] = @([fVidTwoPassCheck state]);
	/* Turbo 2 pass Encoding fVidTurboPassCheck*/
	queueFileJob[@"VideoTurboTwoPass"] = @([fVidTurboPassCheck state]);

    /* Video encoder */
	queueFileJob[@"JobVideoEncoderVcodec"] = @(self.codec);

    /* Framerate */
    queueFileJob[@"JobIndexVideoFramerate"] = @([[fVidRatePopUp selectedItem] tag]);
}

- (void)prepareVideoForJobPreview:(hb_job_t *)job andTitle:(hb_title_t *)title
{
    job->vcodec = self.codec;
    job->fastfirstpass = 0;

    job->chapter_markers = 0;

	if (job->vcodec == HB_VCODEC_X264)
    {
        /* advanced x264 options */
        NSString   *tmpString;
        // translate zero-length strings to NULL for libhb
        const char *encoder_preset  = NULL;
        const char *encoder_tune    = NULL;
        const char *encoder_options = NULL;
        const char *encoder_profile = NULL;
        const char *encoder_level   = NULL;
        if ([fX264UseAdvancedOptionsCheck state])
        {
            // we are using the advanced panel
            if ([(tmpString = [self.fAdvancedOptions optionsString]) length])
            {
                encoder_options = [tmpString UTF8String];
            }
        }
        else
        {
            // we are using the x264 preset system
            if ([(tmpString = [self tune]) length])
            {
                encoder_tune = [tmpString UTF8String];
            }
            if ([(tmpString = [self optionExtra]) length])
            {
                encoder_options = [tmpString UTF8String];
            }
            if ([(tmpString = [self profile]) length])
            {
                encoder_profile = [tmpString UTF8String];
            }
            if ([(tmpString = [self level]) length])
            {
                encoder_level = [tmpString UTF8String];
            }
            encoder_preset = [[self preset] UTF8String];
        }
        hb_job_set_encoder_preset (job, encoder_preset);
        hb_job_set_encoder_tune   (job, encoder_tune);
        hb_job_set_encoder_options(job, encoder_options);
        hb_job_set_encoder_profile(job, encoder_profile);
        hb_job_set_encoder_level  (job, encoder_level);
    }
    else if (job->vcodec & HB_VCODEC_FFMPEG_MASK)
    {
        hb_job_set_encoder_options(job, [self.lavcOptions UTF8String]);
    }

    /* Video settings */
    int fps_mode, fps_num, fps_den;
    if( [fVidRatePopUp indexOfSelectedItem] > 0 )
    {
        /* a specific framerate has been chosen */
        fps_num = 27000000;
        fps_den = (int)[[fVidRatePopUp selectedItem] tag];
        if ([fFramerateMatrix selectedRow] == 1)
        {
            // CFR
            fps_mode = 1;
        }
        else
        {
            // PFR
            fps_mode = 2;
        }
    }
    else
    {
        /* same as source */
        fps_num = title->rate;
        fps_den = title->rate_base;
        if ([fFramerateMatrix selectedRow] == 1)
        {
            // CFR
            fps_mode = 1;
        }
        else
        {
            // VFR
            fps_mode = 0;
        }
    }

    switch (self.qualityType)
    {
        case 0:
            /* ABR */
            job->vquality = -1.0;
            job->vbitrate = [fVidBitrateField intValue];
            break;
        case 1:
            /* Constant Quality */
            job->vquality = [fVidQualityRFField floatValue];
            job->vbitrate = 0;
            break;
    }

    /* Add framerate shaping filter */
    hb_filter_object_t *filter = hb_filter_init(HB_FILTER_VFR);
    hb_add_filter(job, filter, [[NSString stringWithFormat:@"%d:%d:%d",
                                 fps_mode, fps_num, fps_den] UTF8String]);
}

- (void)prepareVideoForPreset:(NSMutableDictionary *)preset
{
    preset[@"VideoEncoder"] = @(hb_video_encoder_get_name(self.codec));

    /* x264 Options, this will either be advanced panel or the video tabs x264 presets panel with modded option string */
    if ([fX264UseAdvancedOptionsCheck state] == NSOnState)
    {
        /* use the old advanced panel */
        preset[@"x264UseAdvancedOptions"] = @1;
        preset[@"x264Option"] = [self.fAdvancedOptions optionsString];
    }
    else
    {
        /* use the x264 preset system */
        preset[@"x264UseAdvancedOptions"] = @0;
        preset[@"VideoPreset"]      = [self preset];
        preset[@"VideoTune"]        = [self tune];
        preset[@"VideoOptionExtra"] = [self optionExtra];
        preset[@"VideoProfile"]     = [self profile];
        preset[@"VideoLevel"]       = [self level];

        /*
         * bonus: set the unparsed options to make the preset compatible
         * with old HB versions
         */
        if (fX264PresetsUnparsedUTF8String != NULL)
        {
            preset[@"x264Option"] = @(fX264PresetsUnparsedUTF8String);
        }
        else
        {
            preset[@"x264Option"] = @"";
        }
    }

    /* FFmpeg (lavc) Option String */
    preset[@"lavcOption"] = self.lavcOptions;

    /* though there are actually only 0 - 1 types available in the ui we need to map to the old 0 - 2
     * set of indexes from when we had 0 == Target , 1 == Abr and 2 == Constant Quality for presets
     * to take care of any legacy presets. */
    preset[@"VideoQualityType"] = @(self.qualityType +1);
    preset[@"VideoAvgBitrate"] = [fVidBitrateField stringValue];
    preset[@"VideoQualitySlider"] = @([fVidQualityRFField floatValue]);

    /* Video framerate and framerate mode */
    if ([fFramerateMatrix selectedRow] == 1)
    {
        preset[@"VideoFramerateMode"] = @"cfr";
    }
    if ([fVidRatePopUp indexOfSelectedItem] == 0) // Same as source is selected
    {
        preset[@"VideoFramerate"] = @"Same as source";

        if ([fFramerateMatrix selectedRow] == 0)
        {
            preset[@"VideoFramerateMode"] = @"vfr";
        }
    }
    else // translate the rate (selected item's tag) to the official libhb name
    {
        preset[@"VideoFramerate"] = [NSString stringWithFormat:@"%s",
                           hb_video_framerate_get_name((int)[[fVidRatePopUp selectedItem] tag])];

        if ([fFramerateMatrix selectedRow] == 0)
        {
            preset[@"VideoFramerateMode"] = @"pfr";
        }
    }



    /* 2 Pass Encoding */
    preset[@"VideoTwoPass"] = @([fVidTwoPassCheck state]);
    /* Turbo 2 pass Encoding fVidTurboPassCheck*/
    preset[@"VideoTurboTwoPass"] = @([fVidTurboPassCheck state]);
}

#pragma mark - Video

- (IBAction) videoEncoderPopUpChanged: (id) sender
{
    [self switchPresetViewForEncoder:self.codec];

    [[NSNotificationCenter defaultCenter] postNotificationName:HBVideoEncoderChangedNotification object:self];

    [self setupQualitySlider];
	[self twoPassCheckboxChanged: sender];
}

- (IBAction) twoPassCheckboxChanged: (id) sender
{
	/* check to see if x264 is chosen */
	if(self.codec == HB_VCODEC_X264)
    {
		if( [fVidTwoPassCheck state] == NSOnState)
		{
			[fVidTurboPassCheck setHidden: NO];
		}
		else
		{
			[fVidTurboPassCheck setHidden: YES];
			[fVidTurboPassCheck setState: NSOffState];
		}
		/* Make sure Two Pass is checked if Turbo is checked */
		if( [fVidTurboPassCheck state] == NSOnState)
		{
			[fVidTwoPassCheck setState: NSOnState];
		}
	}
	else
	{
		[fVidTurboPassCheck setHidden: YES];
		[fVidTurboPassCheck setState: NSOffState];
	}

	/* We call method method to change UI to reflect whether a preset is used or not*/
	[self.fHBController customSettingUsed: sender];
}

- (IBAction ) videoFrameRateChanged: (id) sender
{
    /* if no valid framerate is selected, use "Same as source" */
    if ([fVidRatePopUp selectedItem] == nil)
    {
        [fVidRatePopUp selectItemAtIndex:0];
    }

    /* Hide and set the PFR Checkbox to OFF if we are set to Same as Source */
    /* Depending on whether or not Same as source is selected modify the title for
     * fFramerateVfrPfrCell*/
    if ([fVidRatePopUp indexOfSelectedItem] == 0) // We are Same as Source
    {
        [fFramerateVfrPfrCell setTitle:@"Variable Framerate"];
    }
    else
    {
        [fFramerateVfrPfrCell setTitle:@"Peak Framerate (VFR)"];


    }

    /* We call method method to change UI to reflect whether a preset is used or not*/
	[self.fHBController customSettingUsed: sender];
}

- (IBAction) videoMatrixChanged: (id) sender;
{
    /* We use the selectedCell: tag of the fVidQualityMatrix instead of selectedRow
     * so that the order of the video controls can be switched around.
     * Constant quality is 1 and Average bitrate is 0 for reference. */
    bool bitrate, quality;
    bitrate = quality = false;
    if( [fVidQualityMatrix isEnabled] )
    {
        switch(self.qualityType)
        {
            case 0:
                bitrate = true;
                break;
            case 1:
                quality = true;
                break;
        }
    }

    [fVidBitrateField     setEnabled: bitrate];
    [fVidQualitySlider    setEnabled: quality];
    [fVidQualityRFField   setEnabled: quality];
    [fVidQualityRFLabel    setEnabled: quality];
    [fVidTwoPassCheck     setEnabled: !quality &&
     [fVidQualityMatrix isEnabled]];
    if( quality )
    {
        [fVidTwoPassCheck setState: NSOffState];
		[fVidTurboPassCheck setHidden: YES];
		[fVidTurboPassCheck setState: NSOffState];
    }

    [self qualitySliderChanged: sender];
	[self.fHBController customSettingUsed: sender];
}

/* Use this method to setup the quality slider for cq/rf values depending on
 * the video encoder selected.
 */
- (void) setupQualitySlider
{
    /*
     * Get the current slider maxValue to check for a change in slider scale
     * later so that we can choose a new similar value on the new slider scale
     */
    float previousMaxValue             = [fVidQualitySlider maxValue];
    float previousPercentOfSliderScale = ([fVidQualitySlider floatValue] /
                                          ([fVidQualitySlider maxValue] -
                                           [fVidQualitySlider minValue] + 1));
    [fVidQualityRFLabel setStringValue:[NSString stringWithFormat:@"%s",
                                        hb_video_quality_get_name(self.codec)]];
    int direction;
    float minValue, maxValue, granularity;
    hb_video_quality_get_limits([self codec],
                                &minValue, &maxValue, &granularity, &direction);
    if (granularity < 1.0f)
    {
        /*
         * Encoders that allow fractional CQ values often have a low granularity
         * which makes the slider hard to use, so use a value from preferences.
         */
        granularity = [[NSUserDefaults standardUserDefaults]
                       floatForKey:@"x264CqSliderFractional"];
    }
    [fVidQualitySlider setMinValue:minValue];
    [fVidQualitySlider setMaxValue:maxValue];
    [fVidQualitySlider setNumberOfTickMarks:((maxValue - minValue) *
                                             (1.0f / granularity)) + 1];

    /* check to see if we have changed slider scales */
    if (previousMaxValue != maxValue)
    {
        /*
         * if so, convert the old setting to the new scale as close as possible
         * based on percentages
         */
        [fVidQualitySlider setFloatValue:((maxValue - minValue + 1.) *
                                          (previousPercentOfSliderScale))];
    }

    [self qualitySliderChanged:nil];
}

- (IBAction) qualitySliderChanged: (id) sender
{
    /*
     * Our constant quality slider is in a range based
     * on each encoders qp/rf values. The range depends
     * on the encoder. Also, the range is inverse of quality
     * for all of the encoders *except* for theora
     * (ie. as the "quality" goes up, the cq or rf value
     * actually goes down). Since the IB sliders always set
     * their max value at the right end of the slider, we
     * will calculate the inverse, so as the slider floatValue
     * goes up, we will show the inverse in the rf field
     * so, the floatValue at the right for x264 would be 51
     * and our rf field needs to show 0 and vice versa.
     */
    int direction;
    float minValue, maxValue, granularity;
    float inverseValue = ([fVidQualitySlider minValue] +
                          [fVidQualitySlider maxValue] -
                          [fVidQualitySlider floatValue]);

    hb_video_quality_get_limits(self.codec, &minValue, &maxValue, &granularity, &direction);

    if (!direction)
    {
        [fVidQualityRFField setStringValue:[NSString stringWithFormat:@"%.2f",
                                            [fVidQualitySlider floatValue]]];
    }
    else
    {
        [fVidQualityRFField setStringValue:[NSString stringWithFormat:@"%.2f",
                                            inverseValue]];
    }
    /* Show a warning if x264 and rf 0 which is lossless */
    if (self.codec == HB_VCODEC_X264 && inverseValue == 0.0)
    {
        [fVidQualityRFField setStringValue:[NSString stringWithFormat:@"%.2f (Warning: Lossless)",
                                            inverseValue]];
    }

    [self.fHBController customSettingUsed: sender];
}

#pragma mark - Video x264/x265 Presets

- (void)switchPresetViewForEncoder:(int)encoder
{
    [self.fAdvancedOptions setHidden:YES];

    if (encoder == HB_VCODEC_X264 || encoder == HB_VCODEC_X265)
    {
        [fPresetsBox setContentView:fPresetView];
        [self setupPresetsWidgetsForEncoder:encoder];

        if (encoder == HB_VCODEC_X264)
        {
            [self.fAdvancedOptions setHidden:NO];
        }
    }
    else if (encoder & HB_VCODEC_FFMPEG_MASK)
    {
        [fPresetsBox setContentView:fSimplePresetView];
    }
    else
    {
        [fPresetsBox setContentView:nil];
    }
}

- (void) setupPresetsWidgetsForEncoder:(int)encoder
{

    if (encoder == HB_VCODEC_X264)
    {
        [fX264UseAdvancedOptionsCheck setHidden:NO];
        [fFastDecodeCheck setHidden:NO];
    }
    else
    {
        [fX264UseAdvancedOptionsCheck setHidden:YES];
        [fFastDecodeCheck setHidden:YES];
    }

    NSUInteger i;
    // populate the preset system widgets via hb_video_encoder_get_* functions.
    // store preset names
    const char* const *presets = hb_video_encoder_get_presets(encoder);
    NSMutableArray *tmp_array = [[NSMutableArray alloc] init];
    for (i = 0; presets[i] != NULL; i++)
    {
        [tmp_array addObject:@(presets[i])];
        if (!strcasecmp(presets[i], "medium"))
        {
            fMediumPresetIndex = i;
        }
    }
    fPresetNames = [[NSArray alloc] initWithArray:tmp_array];
    [tmp_array release];
    // setup the preset slider
    [fPresetsSlider setMinValue:0];
    [fPresetsSlider setMaxValue:[fPresetNames count]-1];
    [fPresetsSlider setNumberOfTickMarks:[fPresetNames count]];
    [fPresetsSlider setIntegerValue:fMediumPresetIndex];
    [fPresetsSlider setTickMarkPosition:NSTickMarkAbove];
    [fPresetsSlider setAllowsTickMarkValuesOnly:YES];
    [self presetsSliderChanged: nil];
    // setup the tune popup
    [fTunePopUp removeAllItems];
    [fTunePopUp addItemWithTitle: @"none"];
    const char* const *tunes = hb_video_encoder_get_tunes(encoder);
    for (int i = 0; tunes[i] != NULL; i++)
    {
        // we filter out "fastdecode" as we have a dedicated checkbox for it
        if (strcasecmp(tunes[i], "fastdecode") != 0)
        {
            [fTunePopUp addItemWithTitle: @(tunes[i])];
        }
    }
    // the fastdecode checkbox is off by default
    [fFastDecodeCheck setState: NSOffState];
    // setup the h264 profile popup
    [fProfilePopUp removeAllItems];
    const char* const *profiles = hb_video_encoder_get_profiles(encoder);
    for (int i = 0; profiles[i] != NULL; i++)
    {
        [fProfilePopUp addItemWithTitle: @(profiles[i])];
    }
    // setup the level popup
    [fLevelPopUp removeAllItems];
    const char* const *levels = hb_video_encoder_get_levels(encoder);
    for (int i = 0; levels != NULL && levels[i] != NULL; i++)
    {
        [fLevelPopUp addItemWithTitle: @(levels[i])];
    }
    if ([[fLevelPopUp itemArray] count] == 0)
    {
        [fLevelPopUp addItemWithTitle:@"auto"];
    }
    // clear the additional x264 options
    [fDisplayPresetsAdditonalOptionsTextField setStringValue:@""];
}

- (void) enableEncoderOptionsWidgets: (BOOL) enable
{
    NSControl *controls[] =
    {
        fPresetsSlider, fPresetSliderLabel, fPresetSelectedTextField,
        fTunePopUp, fTunePopUpLabel, fFastDecodeCheck,
        fDisplayPresetsAdditonalOptionsTextField, fDisplayPresetsAdditonalOptionsLabel,
        fProfilePopUp, fProfilePopUpLabel,
        fLevelPopUp, fLevelPopUpLabel,
        fDisplayX264PresetsUnparseTextField,
    };

    // check whether the x264 preset system and the advanced panel should be enabled
    BOOL enable_x264_controls  = (enable && [fX264UseAdvancedOptionsCheck state] == NSOffState);
    BOOL enable_advanced_panel = (enable && [fX264UseAdvancedOptionsCheck state] == NSOnState);

    // enable/disable the checkbox and advanced panel
    [fX264UseAdvancedOptionsCheck setEnabled:enable];
    [self.fAdvancedOptions setUIEnabled:enable_advanced_panel];

    // enable/disable the x264 preset system controls
    for (unsigned i = 0; i < (sizeof(controls) / sizeof(NSControl*)); i++)
    {
        if ([[controls[i] className] isEqualToString: @"NSTextField"])
        {
            NSTextField *tf = (NSTextField*)controls[i];
            if (![tf isBezeled])
            {
                [tf setTextColor:(enable_x264_controls       ?
                                  [NSColor controlTextColor] :
                                  [NSColor disabledControlTextColor])];
                continue;
            }
        }
        [controls[i] setEnabled:enable_x264_controls];
    }
}

- (IBAction) updateEncoderOptionsWidgets: (id) sender
{
    if ([fX264UseAdvancedOptionsCheck state] == NSOnState)
    {
        /*
         * we are using or switching to the advanced panel
         *
         * if triggered by selectPreset or applyQueueSettingToMainWindow,
         * the options string will have been specified explicitly - leave it.
         *
         * if triggered by the advanced panel on/off checkbox, set the options
         * string to the value of the unparsed x264 preset system string.
         */
        if (sender == fX264UseAdvancedOptionsCheck)
        {
            if (fX264PresetsUnparsedUTF8String != NULL)
            {
                [self.fAdvancedOptions setOptions:@(fX264PresetsUnparsedUTF8String)];
            }
            else
            {
                [self.fAdvancedOptions setOptions:@""];
            }
        }
    }
    // enable/disable, populate and update the various widgets
    [self             enableEncoderOptionsWidgets:       YES];
    [self             presetsSliderChanged:nil];
    [self.fAdvancedOptions X264AdvancedOptionsSet:  nil];
}

#pragma mark - Lavc presets

- (void) setLavcOptions: (NSString *)optionExtra
{
    if (!optionExtra)
    {
        [fLavcOptionsTextField setStringValue:@""];
        return;
    }
    [fLavcOptionsTextField setStringValue:optionExtra];
}

- (NSString *) lavcOptions
{
    return [fLavcOptionsTextField stringValue];
}

#pragma mark -
#pragma mark x264/x265 preset system

- (NSString *) preset
{
    return (NSString *)fPresetNames[[fPresetsSlider intValue]];
}

- (NSString *) tune
{
    NSString *tune = @"";
    if ([fTunePopUp indexOfSelectedItem])
    {
        tune = [tune stringByAppendingString:
                    [fTunePopUp titleOfSelectedItem]];
    }
    if ([fFastDecodeCheck state])
    {
        if ([tune length])
        {
            tune = [tune stringByAppendingString: @","];
        }
        tune = [tune stringByAppendingString: @"fastdecode"];
    }
    return tune;
}

- (NSString *) optionExtra
{
    return [fDisplayPresetsAdditonalOptionsTextField stringValue];
}

- (NSString *) profile
{
    if ([fProfilePopUp indexOfSelectedItem])
    {
        return [fProfilePopUp titleOfSelectedItem];
    }
    return @"";
}

- (NSString *) level
{
    if ([fLevelPopUp indexOfSelectedItem])
    {
        return [fLevelPopUp titleOfSelectedItem];
    }
    return @"";
}

- (void) setPreset: (NSString *)preset
{
    if (preset)
    {
        for (NSString *name in fPresetNames)
        {
            if ([name isEqualToString:preset])
            {
                [fPresetsSlider setIntegerValue:
                 [fPresetNames indexOfObject:name]];
                return;
            }
        }
    }
    [fPresetsSlider setIntegerValue:fMediumPresetIndex];
}

- (void) setTune: (NSString *)tune
{
    if (!tune)
    {
        [fTunePopUp selectItemAtIndex:0];
        [fFastDecodeCheck setState:NSOffState];
        return;
    }
    // handle fastdecode
    if ([tune rangeOfString:@"fastdecode"].location != NSNotFound)
    {
        [fFastDecodeCheck setState:NSOnState];
    }
    else
    {
        [fFastDecodeCheck setState:NSOffState];
    }
    // filter out fastdecode
    tune = [tune stringByReplacingOccurrencesOfString:@","
                                                   withString:@""];
    tune = [tune stringByReplacingOccurrencesOfString:@"fastdecode"
                                                   withString:@""];
    // set the tune
    [fTunePopUp selectItemWithTitle:tune];
    // fallback
    if ([fTunePopUp indexOfSelectedItem] == -1)
    {
        [fTunePopUp selectItemAtIndex:0];
    }
}

- (void) setOptionExtra: (NSString *)optionExtra
{
    if (!optionExtra)
    {
        [fDisplayPresetsAdditonalOptionsTextField setStringValue:@""];
        return;
    }
    [fDisplayPresetsAdditonalOptionsTextField setStringValue:optionExtra];
}

- (void) setProfile: (NSString *)profile
{
    if (!profile)
    {
        [fProfilePopUp selectItemAtIndex:0];
        return;
    }
    // set the profile
    [fProfilePopUp selectItemWithTitle:profile];
    // fallback
    if ([fProfilePopUp indexOfSelectedItem] == -1)
    {
        [fProfilePopUp selectItemAtIndex:0];
    }
}

- (void) setLevel: (NSString *)level
{
    if (!level)
    {
        [fLevelPopUp selectItemAtIndex:0];
        return;
    }
    // set the level
    [fLevelPopUp selectItemWithTitle:level];
    // fallback
    if ([fLevelPopUp indexOfSelectedItem] == -1)
    {
        [fLevelPopUp selectItemAtIndex:0];
    }
}

- (IBAction) presetsSliderChanged: (id) sender
{
    // we assume the preset names and slider were setup properly
    [fPresetSelectedTextField setStringValue: [self preset]];

    if (self.codec == HB_VCODEC_X264)
    {
        [self x264PresetsChangedDisplayExpandedOptions:nil];
        [fDisplayX264PresetsUnparseTextField setHidden:NO];
    }
    else
    {
        [fDisplayX264PresetsUnparseTextField setHidden:YES];
    }

}

/* This is called everytime a x264 widget in the video tab is changed to
 display the expanded options in a text field via outlet fDisplayX264PresetsUnparseTextField
 */
- (IBAction) x264PresetsChangedDisplayExpandedOptions: (id) sender

{
    if (self.codec != HB_VCODEC_X264)
    {
        return;
    }

    /* API reference:
     *
     * char * hb_x264_param_unparse(const char *x264_preset,
     *                              const char *x264_tune,
     *                              const char *x264_encopts,
     *                              const char *h264_profile,
     *                              const char *h264_level,
     *                              int width, int height);
     */
    NSString   *tmpString;
    const char *x264_preset   = [[self preset] UTF8String];
    const char *x264_tune     = NULL;
    const char *advanced_opts = NULL;
    const char *h264_profile  = NULL;
    const char *h264_level    = NULL;
    int         width         = 1;
    int         height        = 1;
    // prepare the tune, advanced options, profile and level
    if ([(tmpString = [self tune]) length])
    {
        x264_tune = [tmpString UTF8String];
    }
    if ([(tmpString = [self optionExtra]) length])
    {
        advanced_opts = [tmpString UTF8String];
    }
    if ([(tmpString = [self profile]) length])
    {
        h264_profile = [tmpString UTF8String];
    }
    if ([(tmpString = [self level]) length])
    {
        h264_level = [tmpString UTF8String];
    }
    // width and height must be non-zero
    if (_fPresetsWidthForUnparse && _fPresetsHeightForUnparse)
    {
        width  = (int)_fPresetsWidthForUnparse;
        height = (int)_fPresetsHeightForUnparse;
    }
    // free the previous unparsed string
    free(fX264PresetsUnparsedUTF8String);
    // now, unparse
    fX264PresetsUnparsedUTF8String = hb_x264_param_unparse(x264_preset,
                                                           x264_tune,
                                                           advanced_opts,
                                                           h264_profile,
                                                           h264_level,
                                                           width, height);
    // update the text field
    if (fX264PresetsUnparsedUTF8String != NULL)
    {
        [fDisplayX264PresetsUnparseTextField setStringValue:
         [NSString stringWithFormat:@"x264 Unparse: %s",
          fX264PresetsUnparsedUTF8String]];
    }
    else
    {
        [fDisplayX264PresetsUnparseTextField setStringValue:@"x264 Unparse:"];
    }
}

@end
