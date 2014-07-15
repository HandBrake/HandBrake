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

    /* x264 Presets Box */
    NSArray                      * fX264PresetNames;
    NSUInteger                     fX264MediumPresetIndex;
    IBOutlet NSButton            * fX264UseAdvancedOptionsCheck;
    IBOutlet NSBox               * fX264PresetsBox;
    IBOutlet NSSlider            * fX264PresetsSlider;
    IBOutlet NSTextField         * fX264PresetSliderLabel;
    IBOutlet NSTextField         * fX264PresetSelectedTextField;
    IBOutlet NSPopUpButton       * fX264TunePopUp;
    IBOutlet NSTextField         * fX264TunePopUpLabel;
    IBOutlet NSPopUpButton       * fX264ProfilePopUp;
    IBOutlet NSTextField         * fX264ProfilePopUpLabel;
    IBOutlet NSPopUpButton       * fX264LevelPopUp;
    IBOutlet NSTextField         * fX264LevelPopUpLabel;
    IBOutlet NSButton            * fX264FastDecodeCheck;
    IBOutlet NSTextField         * fDisplayX264PresetsAdditonalOptionsTextField;
    IBOutlet NSTextField         * fDisplayX264PresetsAdditonalOptionsLabel;
    // Text Field to show the expanded opts from unparse()
    IBOutlet NSTextField         * fDisplayX264PresetsUnparseTextField;
    char                         * fX264PresetsUnparsedUTF8String;
    NSUInteger                     _fX264PresetsHeightForUnparse;
    NSUInteger                     _fX264PresetsWidthForUnparse;
}

@end

@implementation HBVideoController

@synthesize fX264PresetsHeightForUnparse = _fX264PresetsHeightForUnparse;
@synthesize fX264PresetsWidthForUnparse = _fX264PresetsWidthForUnparse;

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

- (int)selectedCodec
{
    return (int)[[fVidEncoderPopUp selectedItem] tag];
}

- (int)selectedQualityType
{
    return (int)[[fVidQualityMatrix selectedCell] tag];
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
        _pictureFiltersField = @"Pictures Filters:";
        _pictureSettingsField = @"Pictures Settings:";

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
    [fVidEncoderPopUp addItemWithTitle: @"FFmpeg"];

    /* setup our x264 presets widgets - this only needs to be done once */
    [self setupX264PresetsWidgets];

    /* Video quality */
	[fVidBitrateField    setIntValue: 1000];
    [fVidQualityMatrix   selectCell: fVidBitrateCell];
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
            itemTitle = [NSString stringWithUTF8String:video_framerate->name];
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

- (void)enableUI:(BOOL)b {
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
                [tf setTextColor: (b ?
                                   [NSColor controlTextColor] :
                                   [NSColor disabledControlTextColor])];
                continue;
            }
        }
        [controls[i] setEnabled: b];
    }

    [self videoMatrixChanged:nil];
    [self enableX264Widgets:b];
}

- (void)containerChanged:(NSNotification *)aNotification
{
    NSDictionary *notDict = [aNotification userInfo];

    int videoContainer = [[notDict objectForKey: keyContainerTag] intValue];

    /* lets get the tag of the currently selected item first so we might reset it later */
    int selectedVidEncoderTag = (int)[[fVidEncoderPopUp selectedItem] tag];

    /* Note: we now store the video encoder int values from common.c in the tags of each popup for easy retrieval later */
    [fVidEncoderPopUp removeAllItems];
    for (const hb_encoder_t *video_encoder = hb_video_encoder_get_next(NULL);
         video_encoder != NULL;
         video_encoder  = hb_video_encoder_get_next(video_encoder))
    {
        if (video_encoder->muxers & videoContainer)
        {
            NSMenuItem *menuItem = [[fVidEncoderPopUp menu] addItemWithTitle:[NSString stringWithUTF8String:video_encoder->name]
                                                          action:nil
                                                   keyEquivalent:@""];
            [menuItem setTag:video_encoder->codec];
        }
    }

    /*
     * item 0 will be selected by default
     * deselect it so that we can detect whether the video encoder has changed
     */
    [fVidEncoderPopUp selectItem:nil];
    if (selectedVidEncoderTag)
    {
        // if we have a tag for previously selected encoder, try to select it
        // if this fails, [fVidEncoderPopUp selectedItem] will be nil
        // we'll handle that scenario further down
        [fVidEncoderPopUp selectItemWithTag:selectedVidEncoderTag];
    }

    if ([fVidEncoderPopUp selectedItem] == nil)
    {
        /* this means the above call to selectItemWithTag failed */
        [fVidEncoderPopUp selectItemAtIndex:0];
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
    [fVidEncoderPopUp selectItemWithTitle:[queueToApply objectForKey:@"VideoEncoder"]];
    [self.fAdvancedOptions setLavcOptions:     [queueToApply objectForKey:@"lavcOption"]];
    /* advanced x264 options */
    if ([[queueToApply objectForKey:@"x264UseAdvancedOptions"] intValue])
    {
        // we are using the advanced panel
        [self.fAdvancedOptions setOptions:[queueToApply objectForKey:@"x264Option"]];
        // preset does not use the x264 preset system, reset the widgets
        [self setX264Preset:     nil];
        [self setX264Tune:       nil];
        [self setX264OptionExtra:[queueToApply objectForKey:@"x264Option"]];
        [self setH264Profile:    nil];
        [self setH264Level:      nil];
        // enable the advanced panel and update the widgets
        [fX264UseAdvancedOptionsCheck setState:NSOnState];
        [self updateX264Widgets:nil];
    }
    else
    {
        // we are using the x264 preset system
        [self setX264Preset:     [queueToApply objectForKey:@"x264Preset"]];
        [self setX264Tune:       [queueToApply objectForKey:@"x264Tune"]];
        [self setX264OptionExtra:[queueToApply objectForKey:@"x264OptionExtra"]];
        [self setH264Profile:    [queueToApply objectForKey:@"h264Profile"]];
        [self setH264Level:      [queueToApply objectForKey:@"h264Level"]];
        // preset does not use the advanced panel, reset it
        [self.fAdvancedOptions setOptions:@""];
        // disable the advanced panel and update the widgets
        [fX264UseAdvancedOptionsCheck setState:NSOffState];
        [self updateX264Widgets:nil];
    }

    /* Lets run through the following functions to get variables set there */
    [self videoEncoderPopUpChanged:nil];

    /* Video quality */
    [fVidQualityMatrix selectCellAtRow:[[queueToApply objectForKey:@"VideoQualityType"] intValue] column:0];

    [fVidBitrateField setStringValue:[queueToApply objectForKey:@"VideoAvgBitrate"]];

    int direction;
    float minValue, maxValue, granularity;
    hb_video_quality_get_limits((int)[[fVidEncoderPopUp selectedItem] tag],
                                &minValue, &maxValue, &granularity, &direction);
    if (!direction)
    {
        [fVidQualitySlider setFloatValue:[[queueToApply objectForKey:@"VideoQualitySlider"] floatValue]];
    }
    else
    {
        /*
         * Since ffmpeg and x264 use an "inverted" slider (lower values
         * indicate a higher quality) we invert the value on the slider
         */
        [fVidQualitySlider setFloatValue:([fVidQualitySlider minValue] +
                                          [fVidQualitySlider maxValue] -
                                          [[queueToApply objectForKey:@"VideoQualitySlider"] floatValue])];
    }

    [self videoMatrixChanged:nil];

    /* Video framerate */
    if ([[queueToApply objectForKey:@"VideoFramerate"] isEqualToString:@"Same as source"])
    {
        /* Now set the Video Frame Rate Mode to either vfr or cfr according to the preset */
        if ([[queueToApply objectForKey:@"VideoFramerateMode"] isEqualToString:@"vfr"])
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
        if ([[queueToApply objectForKey:@"VideoFramerateMode"] isEqualToString:@"pfr"])
        {
            [fFramerateMatrix selectCellAtRow:0 column:0]; // we want pfr
        }
        else
        {
            [fFramerateMatrix selectCellAtRow:1 column:0]; // we want cfr
        }
    }
    [fVidRatePopUp selectItemWithTitle:[queueToApply objectForKey:@"VideoFramerate"]];
    [self videoFrameRateChanged:nil];

    /* 2 Pass Encoding */
    [fVidTwoPassCheck setState:[[queueToApply objectForKey:@"VideoTwoPass"] intValue]];
    [self twoPassCheckboxChanged:nil];
    /* Turbo 1st pass for 2 Pass Encoding */
    [fVidTurboPassCheck setState:[[queueToApply objectForKey:@"VideoTurboTwoPass"] intValue]];
}

- (void)applySettingsFromPreset:(NSDictionary *)preset
{
    /* map legacy encoder names via libhb */
    const char *strValue = hb_video_encoder_sanitize_name([[preset objectForKey:@"VideoEncoder"] UTF8String]);
    [fVidEncoderPopUp selectItemWithTitle:[NSString stringWithFormat:@"%s", strValue]];
    [self videoEncoderPopUpChanged:nil];

    if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_X264)
    {
        if (![preset objectForKey:@"x264UseAdvancedOptions"] ||
            [[preset objectForKey:@"x264UseAdvancedOptions"] intValue])
        {
            /*
             * x264UseAdvancedOptions is not set (legacy preset)
             * or set to 1 (enabled), so we use the old advanced panel
             */
            if ([preset objectForKey:@"x264Option"])
            {
                /* we set the advanced options string here if applicable */
                [self.fAdvancedOptions setOptions:[preset objectForKey:@"x264Option"]];
                [self setX264OptionExtra:[preset objectForKey:@"x264Option"]];
            }
            else
            {
                [self.fAdvancedOptions setOptions:        @""];
                [self             setX264OptionExtra:nil];
            }
            /* preset does not use the x264 preset system, reset the widgets */
            [self setX264Preset: nil];
            [self setX264Tune:   nil];
            [self setH264Profile:nil];
            [self setH264Level:  nil];
            /* we enable the advanced panel and update the widgets */
            [fX264UseAdvancedOptionsCheck setState:NSOnState];
            [self updateX264Widgets:nil];
        }
        else
        {
            /*
             * x264UseAdvancedOptions is set to 0 (disabled),
             * so we use the x264 preset system
             */
            [self setX264Preset:     [preset objectForKey:@"x264Preset"]];
            [self setX264Tune:       [preset objectForKey:@"x264Tune"]];
            [self setX264OptionExtra:[preset objectForKey:@"x264OptionExtra"]];
            [self setH264Profile:    [preset objectForKey:@"h264Profile"]];
            [self setH264Level:      [preset objectForKey:@"h264Level"]];
            /* preset does not use the advanced panel, reset it */
            [self.fAdvancedOptions setOptions:@""];
            /* we disable the advanced panel and update the widgets */
            [fX264UseAdvancedOptionsCheck setState:NSOffState];
            [self updateX264Widgets:nil];
        }
    }

    int qualityType = [[preset objectForKey:@"VideoQualityType"] intValue] - 1;
    /* Note since the removal of Target Size encoding, the possible values for VideoQuality type are 0 - 1.
     * Therefore any preset that uses the old 2 for Constant Quality would now use 1 since there is one less index
     * for the fVidQualityMatrix. It should also be noted that any preset that used the deprecated Target Size
     * setting of 0 would set us to 0 or ABR since ABR is now tagged 0. Fortunately this does not affect any built-in
     * presets since they all use Constant Quality or Average Bitrate.*/
    if (qualityType == -1)
    {
        qualityType = 0;
    }
    [fVidQualityMatrix selectCellWithTag:qualityType];

    [fVidBitrateField setStringValue:[preset objectForKey:@"VideoAvgBitrate"]];

    int direction;
    float minValue, maxValue, granularity;
    hb_video_quality_get_limits((int)[[fVidEncoderPopUp selectedItem] tag],
                                &minValue, &maxValue, &granularity, &direction);
    if (!direction)
    {
        [fVidQualitySlider setFloatValue:[[preset objectForKey:@"VideoQualitySlider"] floatValue]];
    }
    else
    {
        /*
         * Since ffmpeg and x264 use an "inverted" slider (lower values
         * indicate a higher quality) we invert the value on the slider
         */
        [fVidQualitySlider setFloatValue:([fVidQualitySlider minValue] +
                                          [fVidQualitySlider maxValue] -
                                          [[preset objectForKey:@"VideoQualitySlider"] floatValue])];
    }

    [self videoMatrixChanged:nil];

    /* Video framerate */
    if ([[preset objectForKey:@"VideoFramerate"] isEqualToString:@"Same as source"])
    {
        /* Now set the Video Frame Rate Mode to either vfr or cfr according to the preset */
        if (![preset objectForKey:@"VideoFramerateMode"] ||
            [[preset objectForKey:@"VideoFramerateMode"] isEqualToString:@"vfr"])
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
        if ([[preset objectForKey:@"VideoFramerateMode"] isEqualToString:@"pfr"] ||
            [[preset objectForKey:@"VideoFrameratePFR"]  intValue] == 1)
        {
            [fFramerateMatrix selectCellAtRow:0 column:0]; // we want pfr
        }
        else
        {
            [fFramerateMatrix selectCellAtRow:1 column:0]; // we want cfr
        }
    }
    /* map legacy names via libhb */
    int intValue = hb_video_framerate_get_from_name([[preset objectForKey:@"VideoFramerate"] UTF8String]);
    [fVidRatePopUp selectItemWithTag:intValue];
    [self videoFrameRateChanged:nil];

    /* 2 Pass Encoding */
    [fVidTwoPassCheck setState:[[preset objectForKey:@"VideoTwoPass"] intValue]];
    [self twoPassCheckboxChanged:nil];

    /* Turbo 1st pass for 2 Pass Encoding */
    [fVidTurboPassCheck setState:[[preset objectForKey:@"VideoTurboTwoPass"] intValue]];
}

- (void)prepareVideoForQueueFileJob:(NSMutableDictionary *)queueFileJob
{
	[queueFileJob setObject:[fVidEncoderPopUp titleOfSelectedItem] forKey:@"VideoEncoder"];

    /* x264 advanced options */
    if ([fX264UseAdvancedOptionsCheck state])
    {
        // we are using the advanced panel
        [queueFileJob setObject:[NSNumber numberWithInt:1]       forKey: @"x264UseAdvancedOptions"];
        [queueFileJob setObject:[self.fAdvancedOptions optionsString] forKey:@"x264Option"];
    }
    else
    {
        // we are using the x264 preset system
        [queueFileJob setObject:[NSNumber numberWithInt:0] forKey: @"x264UseAdvancedOptions"];
        [queueFileJob setObject:[self x264Preset]          forKey: @"x264Preset"];
        [queueFileJob setObject:[self x264Tune]            forKey: @"x264Tune"];
        [queueFileJob setObject:[self x264OptionExtra]     forKey: @"x264OptionExtra"];
        [queueFileJob setObject:[self h264Profile]         forKey: @"h264Profile"];
        [queueFileJob setObject:[self h264Level]           forKey: @"h264Level"];
    }

    /* FFmpeg (lavc) Option String */
    [queueFileJob setObject:[self.fAdvancedOptions optionsStringLavc] forKey:@"lavcOption"];

	[queueFileJob setObject:[NSNumber numberWithInteger:[[fVidQualityMatrix selectedCell] tag] + 1] forKey:@"VideoQualityType"];
	[queueFileJob setObject:[fVidBitrateField stringValue] forKey:@"VideoAvgBitrate"];
	[queueFileJob setObject:[NSNumber numberWithFloat:[fVidQualityRFField floatValue]] forKey:@"VideoQualitySlider"];
    /* Framerate */
    [queueFileJob setObject:[fVidRatePopUp titleOfSelectedItem] forKey:@"VideoFramerate"];
    /* Frame Rate Mode */
    if ([fFramerateMatrix selectedRow] == 1) // if selected we are cfr regardless of the frame rate popup
    {
        [queueFileJob setObject:@"cfr" forKey:@"VideoFramerateMode"];
    }
    else
    {
        if ([fVidRatePopUp indexOfSelectedItem] == 0) // Same as source frame rate
        {
            [queueFileJob setObject:@"vfr" forKey:@"VideoFramerateMode"];
        }
        else
        {
            [queueFileJob setObject:@"pfr" forKey:@"VideoFramerateMode"];
        }

    }

	/* 2 Pass Encoding */
	[queueFileJob setObject:[NSNumber numberWithInteger:[fVidTwoPassCheck state]] forKey:@"VideoTwoPass"];
	/* Turbo 2 pass Encoding fVidTurboPassCheck*/
	[queueFileJob setObject:[NSNumber numberWithInteger:[fVidTurboPassCheck state]] forKey:@"VideoTurboTwoPass"];

    /* Video encoder */
	[queueFileJob setObject:[NSNumber numberWithInteger:[[fVidEncoderPopUp selectedItem] tag]] forKey:@"JobVideoEncoderVcodec"];

    /* Framerate */
    [queueFileJob setObject:[NSNumber numberWithInteger:[[fVidRatePopUp selectedItem] tag]] forKey:@"JobIndexVideoFramerate"];
}

- (void)prepareVideoForJobPreview:(hb_job_t *)job andTitle:(hb_title_t *)title
{
    job->vcodec = (int)[[fVidEncoderPopUp selectedItem] tag];
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
            if ([(tmpString = [self x264Tune]) length])
            {
                encoder_tune = [tmpString UTF8String];
            }
            if ([(tmpString = [self x264OptionExtra]) length])
            {
                encoder_options = [tmpString UTF8String];
            }
            if ([(tmpString = [self h264Profile]) length])
            {
                encoder_profile = [tmpString UTF8String];
            }
            if ([(tmpString = [self h264Level]) length])
            {
                encoder_level = [tmpString UTF8String];
            }
            encoder_preset = [[self x264Preset] UTF8String];
        }
        hb_job_set_encoder_preset (job, encoder_preset);
        hb_job_set_encoder_tune   (job, encoder_tune);
        hb_job_set_encoder_options(job, encoder_options);
        hb_job_set_encoder_profile(job, encoder_profile);
        hb_job_set_encoder_level  (job, encoder_level);
    }
    else if (job->vcodec & HB_VCODEC_FFMPEG_MASK)
    {
        hb_job_set_encoder_options(job,
                                   [[self.fAdvancedOptions optionsStringLavc]
                                    UTF8String]);
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

    switch( [[fVidQualityMatrix selectedCell] tag] )
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
    [preset setObject:[fVidEncoderPopUp titleOfSelectedItem] forKey:@"VideoEncoder"];
    /* x264 Options, this will either be advanced panel or the video tabs x264 presets panel with modded option string */

    if ([fX264UseAdvancedOptionsCheck state] == NSOnState)
    {
        /* use the old advanced panel */
        [preset setObject:[NSNumber numberWithInt:1]       forKey:@"x264UseAdvancedOptions"];
        [preset setObject:[self.fAdvancedOptions optionsString] forKey:@"x264Option"];
    }
    else
    {
        /* use the x264 preset system */
        [preset setObject:[NSNumber numberWithInt:0] forKey:@"x264UseAdvancedOptions"];
        [preset setObject:[self x264Preset]          forKey:@"x264Preset"];
        [preset setObject:[self x264Tune]            forKey:@"x264Tune"];
        [preset setObject:[self x264OptionExtra]     forKey:@"x264OptionExtra"];
        [preset setObject:[self h264Profile]         forKey:@"h264Profile"];
        [preset setObject:[self h264Level]           forKey:@"h264Level"];
        /*
         * bonus: set the unparsed options to make the preset compatible
         * with old HB versions
         */
        if (fX264PresetsUnparsedUTF8String != NULL)
        {
            [preset setObject:[NSString stringWithUTF8String:fX264PresetsUnparsedUTF8String]
                       forKey:@"x264Option"];
        }
        else
        {
            [preset setObject:@"" forKey:@"x264Option"];
        }
    }

    /* FFmpeg (lavc) Option String */
    [preset setObject:[self.fAdvancedOptions optionsStringLavc] forKey:@"lavcOption"];

    /* though there are actually only 0 - 1 types available in the ui we need to map to the old 0 - 2
     * set of indexes from when we had 0 == Target , 1 == Abr and 2 == Constant Quality for presets
     * to take care of any legacy presets. */
    [preset setObject:[NSNumber numberWithInteger:[[fVidQualityMatrix selectedCell] tag] +1 ] forKey:@"VideoQualityType"];
    [preset setObject:[fVidBitrateField stringValue] forKey:@"VideoAvgBitrate"];
    [preset setObject:[NSNumber numberWithFloat:[fVidQualityRFField floatValue]] forKey:@"VideoQualitySlider"];

    /* Video framerate and framerate mode */
    if ([fFramerateMatrix selectedRow] == 1)
    {
        [preset setObject:@"cfr" forKey:@"VideoFramerateMode"];
    }
    if ([fVidRatePopUp indexOfSelectedItem] == 0) // Same as source is selected
    {
        [preset setObject:@"Same as source" forKey:@"VideoFramerate"];

        if ([fFramerateMatrix selectedRow] == 0)
        {
            [preset setObject:@"vfr" forKey:@"VideoFramerateMode"];
        }
    }
    else // translate the rate (selected item's tag) to the official libhb name
    {
        [preset setObject:[NSString stringWithFormat:@"%s",
                           hb_video_framerate_get_name((int)[[fVidRatePopUp selectedItem] tag])]
                   forKey:@"VideoFramerate"];

        if ([fFramerateMatrix selectedRow] == 0)
        {
            [preset setObject:@"pfr" forKey:@"VideoFramerateMode"];
        }
    }



    /* 2 Pass Encoding */
    [preset setObject:[NSNumber numberWithInteger:[fVidTwoPassCheck state]] forKey:@"VideoTwoPass"];
    /* Turbo 2 pass Encoding fVidTurboPassCheck*/
    [preset setObject:[NSNumber numberWithInteger:[fVidTurboPassCheck state]] forKey:@"VideoTurboTwoPass"];
}

#pragma mark - Video

- (IBAction) videoEncoderPopUpChanged: (id) sender
{
    /* if no valid encoder is selected, use the first one */
    if ([fVidEncoderPopUp selectedItem] == nil)
    {
        [fVidEncoderPopUp selectItemAtIndex:0];
    }

    int videoEncoder = (int)[[fVidEncoderPopUp selectedItem] tag];

    [self.fAdvancedOptions setHidden:YES];
    /* If we are using x264 then show the x264 advanced panel and the x264 presets box */
    if (videoEncoder == HB_VCODEC_X264)
    {
        [self.fAdvancedOptions setHidden:NO];

        // show the x264 presets box
        [fX264PresetsBox setHidden:NO];
    }
    else // we are FFmpeg (lavc) or Theora
    {
        [self.fAdvancedOptions setHidden:YES];
        [fX264PresetsBox setHidden:YES];

        // We Are Lavc
        if ([[fVidEncoderPopUp selectedItem] tag] & HB_VCODEC_FFMPEG_MASK)
        {
            [self.fAdvancedOptions setLavcOptsEnabled:YES];
        }
        else /// We are Theora
        {
            [self.fAdvancedOptions setLavcOptsEnabled:NO];
        }
    }

    [[NSNotificationCenter defaultCenter] postNotificationName:HBVideoEncoderChangedNotification object:self];

    [self setupQualitySlider];
	[self twoPassCheckboxChanged: sender];
}


- (IBAction) twoPassCheckboxChanged: (id) sender
{
	/* check to see if x264 is chosen */
	if([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_X264)
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
        switch( [[fVidQualityMatrix selectedCell] tag] )
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
                                        hb_video_quality_get_name((int)[[fVidEncoderPopUp
                                                                         selectedItem] tag])]];
    int direction;
    float minValue, maxValue, granularity;
    hb_video_quality_get_limits((int)[[fVidEncoderPopUp selectedItem] tag],
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
    hb_video_quality_get_limits((int)[[fVidEncoderPopUp selectedItem] tag],
                                &minValue, &maxValue, &granularity, &direction);
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
    if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_X264 && inverseValue == 0.0)
    {
        [fVidQualityRFField setStringValue:[NSString stringWithFormat:@"%.2f (Warning: Lossless)",
                                            inverseValue]];
    }

    [self.fHBController customSettingUsed: sender];
}

#pragma mark - Video x264 Presets

- (void) setupX264PresetsWidgets
{
    NSUInteger i;
    // populate the preset system widgets via hb_video_encoder_get_* functions.
    // store x264 preset names
    const char* const *x264_presets = hb_video_encoder_get_presets(HB_VCODEC_X264);
    NSMutableArray *tmp_array = [[NSMutableArray alloc] init];
    for (i = 0; x264_presets[i] != NULL; i++)
    {
        [tmp_array addObject:[NSString stringWithUTF8String:x264_presets[i]]];
        if (!strcasecmp(x264_presets[i], "medium"))
        {
            fX264MediumPresetIndex = i;
        }
    }
    fX264PresetNames = [[NSArray alloc] initWithArray:tmp_array];
    [tmp_array release];
    // setup the x264 preset slider
    [fX264PresetsSlider setMinValue:0];
    [fX264PresetsSlider setMaxValue:[fX264PresetNames count]-1];
    [fX264PresetsSlider setNumberOfTickMarks:[fX264PresetNames count]];
    [fX264PresetsSlider setIntegerValue:fX264MediumPresetIndex];
    [fX264PresetsSlider setTickMarkPosition:NSTickMarkAbove];
    [fX264PresetsSlider setAllowsTickMarkValuesOnly:YES];
    [self x264PresetsSliderChanged: nil];
    // setup the x264 tune popup
    [fX264TunePopUp removeAllItems];
    [fX264TunePopUp addItemWithTitle: @"none"];
    const char* const *x264_tunes = hb_video_encoder_get_tunes(HB_VCODEC_X264);
    for (int i = 0; x264_tunes[i] != NULL; i++)
    {
        // we filter out "fastdecode" as we have a dedicated checkbox for it
        if (strcasecmp(x264_tunes[i], "fastdecode") != 0)
        {
            [fX264TunePopUp addItemWithTitle: [NSString stringWithUTF8String:x264_tunes[i]]];
        }
    }
    // the fastdecode checkbox is off by default
    [fX264FastDecodeCheck setState: NSOffState];
    // setup the h264 profile popup
    [fX264ProfilePopUp removeAllItems];
    const char* const *h264_profiles = hb_video_encoder_get_profiles(HB_VCODEC_X264);
    for (int i = 0; h264_profiles[i] != NULL; i++)
    {
        [fX264ProfilePopUp addItemWithTitle: [NSString stringWithUTF8String:h264_profiles[i]]];
    }
    // setup the h264 level popup
    [fX264LevelPopUp removeAllItems];
    const char* const *h264_levels = hb_video_encoder_get_levels(HB_VCODEC_X264);
    for (int i = 0; h264_levels[i] != NULL; i++)
    {
        [fX264LevelPopUp addItemWithTitle: [NSString stringWithUTF8String:h264_levels[i]]];
    }
    // clear the additional x264 options
    [fDisplayX264PresetsAdditonalOptionsTextField setStringValue:@""];
}

- (void) enableX264Widgets: (bool) enable
{
    NSControl *controls[] =
    {
        fX264PresetsSlider, fX264PresetSliderLabel, fX264PresetSelectedTextField,
        fX264TunePopUp, fX264TunePopUpLabel, fX264FastDecodeCheck,
        fDisplayX264PresetsAdditonalOptionsTextField, fDisplayX264PresetsAdditonalOptionsLabel,
        fX264ProfilePopUp, fX264ProfilePopUpLabel,
        fX264LevelPopUp, fX264LevelPopUpLabel,
        fDisplayX264PresetsUnparseTextField,
    };

    // check whether the x264 preset system and the advanced panel should be enabled
    BOOL enable_x264_controls  = (enable && [fX264UseAdvancedOptionsCheck state] == NSOffState);
    BOOL enable_advanced_panel = (enable && [fX264UseAdvancedOptionsCheck state] == NSOnState);

    // enable/disable the checkbox and advanced panel
    [fX264UseAdvancedOptionsCheck setEnabled:enable];
    [self.fAdvancedOptions enableUI:enable_advanced_panel];

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

- (IBAction) updateX264Widgets: (id) sender
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
                [self.fAdvancedOptions setOptions:
                 [NSString stringWithUTF8String:fX264PresetsUnparsedUTF8String]];
            }
            else
            {
                [self.fAdvancedOptions setOptions:@""];
            }
        }
    }
    // enable/disable, populate and update the various widgets
    [self             enableX264Widgets:       YES];
    [self             x264PresetsSliderChanged:nil];
    [self.fAdvancedOptions X264AdvancedOptionsSet:  nil];
}

#pragma mark -
#pragma mark x264 preset system

- (NSString *) x264Preset
{
    return (NSString *)[fX264PresetNames objectAtIndex:[fX264PresetsSlider intValue]];
}

- (NSString *) x264Tune
{
    NSString *x264Tune = @"";
    if ([fX264TunePopUp indexOfSelectedItem])
    {
        x264Tune = [x264Tune stringByAppendingString:
                    [fX264TunePopUp titleOfSelectedItem]];
    }
    if ([fX264FastDecodeCheck state])
    {
        if ([x264Tune length])
        {
            x264Tune = [x264Tune stringByAppendingString: @","];
        }
        x264Tune = [x264Tune stringByAppendingString: @"fastdecode"];
    }
    return x264Tune;
}

- (NSString*) x264OptionExtra
{
    return [fDisplayX264PresetsAdditonalOptionsTextField stringValue];
}

- (NSString*) h264Profile
{
    if ([fX264ProfilePopUp indexOfSelectedItem])
    {
        return [fX264ProfilePopUp titleOfSelectedItem];
    }
    return @"";
}

- (NSString*) h264Level
{
    if ([fX264LevelPopUp indexOfSelectedItem])
    {
        return [fX264LevelPopUp titleOfSelectedItem];
    }
    return @"";
}

- (void) setX264Preset: (NSString*)x264Preset
{
    if (x264Preset)
    {
        NSString *name;
        NSEnumerator *enumerator = [fX264PresetNames objectEnumerator];
        while ((name = (NSString *)[enumerator nextObject]))
        {
            if ([name isEqualToString:x264Preset])
            {
                [fX264PresetsSlider setIntegerValue:
                 [fX264PresetNames indexOfObject:name]];
                return;
            }
        }
    }
    [fX264PresetsSlider setIntegerValue:fX264MediumPresetIndex];
}

- (void) setX264Tune: (NSString*)x264Tune
{
    if (!x264Tune)
    {
        [fX264TunePopUp selectItemAtIndex:0];
        [fX264FastDecodeCheck setState:NSOffState];
        return;
    }
    // handle fastdecode
    if ([x264Tune rangeOfString:@"fastdecode"].location != NSNotFound)
    {
        [fX264FastDecodeCheck setState:NSOnState];
    }
    else
    {
        [fX264FastDecodeCheck setState:NSOffState];
    }
    // filter out fastdecode
    x264Tune = [x264Tune stringByReplacingOccurrencesOfString:@","
                                                   withString:@""];
    x264Tune = [x264Tune stringByReplacingOccurrencesOfString:@"fastdecode"
                                                   withString:@""];
    // set the tune
    [fX264TunePopUp selectItemWithTitle:x264Tune];
    // fallback
    if ([fX264TunePopUp indexOfSelectedItem] == -1)
    {
        [fX264TunePopUp selectItemAtIndex:0];
    }
}

- (void) setX264OptionExtra: (NSString*)x264OptionExtra
{
    if (!x264OptionExtra)
    {
        [fDisplayX264PresetsAdditonalOptionsTextField setStringValue:@""];
        return;
    }
    [fDisplayX264PresetsAdditonalOptionsTextField setStringValue:x264OptionExtra];
}

- (void) setH264Profile: (NSString*)h264Profile
{
    if (!h264Profile)
    {
        [fX264ProfilePopUp selectItemAtIndex:0];
        return;
    }
    // set the profile
    [fX264ProfilePopUp selectItemWithTitle:h264Profile];
    // fallback
    if ([fX264ProfilePopUp indexOfSelectedItem] == -1)
    {
        [fX264ProfilePopUp selectItemAtIndex:0];
    }
}

- (void) setH264Level: (NSString*)h264Level
{
    if (!h264Level)
    {
        [fX264LevelPopUp selectItemAtIndex:0];
        return;
    }
    // set the level
    [fX264LevelPopUp selectItemWithTitle:h264Level];
    // fallback
    if ([fX264LevelPopUp indexOfSelectedItem] == -1)
    {
        [fX264LevelPopUp selectItemAtIndex:0];
    }
}


- (IBAction) x264PresetsSliderChanged: (id) sender
{
    // we assume the preset names and slider were setup properly
    [fX264PresetSelectedTextField setStringValue: [self x264Preset]];
    [self x264PresetsChangedDisplayExpandedOptions:nil];

}

/* This is called everytime a x264 widget in the video tab is changed to
 display the expanded options in a text field via outlet fDisplayX264PresetsUnparseTextField
 */
- (IBAction) x264PresetsChangedDisplayExpandedOptions: (id) sender

{
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
    const char *x264_preset   = [[self x264Preset] UTF8String];
    const char *x264_tune     = NULL;
    const char *advanced_opts = NULL;
    const char *h264_profile  = NULL;
    const char *h264_level    = NULL;
    int         width         = 1;
    int         height        = 1;
    // prepare the tune, advanced options, profile and level
    if ([(tmpString = [self x264Tune]) length])
    {
        x264_tune = [tmpString UTF8String];
    }
    if ([(tmpString = [self x264OptionExtra]) length])
    {
        advanced_opts = [tmpString UTF8String];
    }
    if ([(tmpString = [self h264Profile]) length])
    {
        h264_profile = [tmpString UTF8String];
    }
    if ([(tmpString = [self h264Level]) length])
    {
        h264_level = [tmpString UTF8String];
    }
    // width and height must be non-zero
    if (_fX264PresetsWidthForUnparse && _fX264PresetsHeightForUnparse)
    {
        width  = (int)_fX264PresetsWidthForUnparse;
        height = (int)_fX264PresetsHeightForUnparse;
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
