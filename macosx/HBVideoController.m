/*  HBVideoController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBVideoController.h"
#import "HBAdvancedController.h"
#import "HBVideo+UIAdditions.h"
#import "HBJob.h"

#include "hb.h"

static void *HBVideoControllerContext = &HBVideoControllerContext;

@interface HBVideoController () {
    // Framerate Radio Button Framerate Controls
    IBOutlet NSButtonCell *fFramerateVfrPfrCell;

    // Video Encoder
    IBOutlet NSSlider *fVidQualitySlider;

    // Encoder options views
    IBOutlet NSView *fPresetView;
    IBOutlet NSView *fSimplePresetView;

    IBOutlet NSTextField *fEncoderOptionsLabel;

    // x264/x265 Presets Box
    IBOutlet NSButton       *fX264UseAdvancedOptionsCheck;
    IBOutlet NSBox          *fDividerLine;
    IBOutlet NSBox          *fPresetsBox;
    IBOutlet NSSlider       *fPresetsSlider;

    // Text Field to show the expanded opts from unparse()
    IBOutlet NSTextField *fDisplayX264PresetsUnparseTextField;
}

@property (nonatomic, strong, readwrite) HBAdvancedController *advancedController;

@property (nonatomic, readwrite) BOOL presetViewEnabled;

@property (nonatomic, readwrite) NSColor *labelColor;

@end

@implementation HBVideoController

- (instancetype)initWithAdvancedController:(HBAdvancedController *)advancedController
{
    self = [self init];
    if (self)
    {
        _advancedController = advancedController;
    }
    return self;
}

- (instancetype)init
{
    self = [super initWithNibName:@"Video" bundle:nil];
    if (self)
    {
        _labelColor = [NSColor disabledControlTextColor];

        // Observe the advanced tab pref shown/hided state.
        [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
                                                                  forKeyPath:@"values.HBShowAdvancedTab"
                                                                     options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                                                                     context:HBVideoControllerContext];

        // Observe the x264 slider granularity, to update the slider when the pref is changed.
        [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
                                                                  forKeyPath:@"values.x264CqSliderFractional"
                                                                     options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                                                                     context:HBVideoControllerContext];

        // Observer a bunch of HBVideo properties to update the UI.
        [self addObserver:self forKeyPath:@"video.encoder" options:NSKeyValueObservingOptionInitial context:HBVideoControllerContext];
        [self addObserver:self forKeyPath:@"video.frameRate" options:NSKeyValueObservingOptionInitial context:HBVideoControllerContext];
        [self addObserver:self forKeyPath:@"video.unparseOptions" options:NSKeyValueObservingOptionInitial context:HBVideoControllerContext];
        [self addObserver:self forKeyPath:@"video.advancedOptions" options:NSKeyValueObservingOptionInitial context:HBVideoControllerContext];
    }

    return self;
}

- (void)setVideo:(HBVideo *)video
{
    _video = video;

    if (video)
    {
        self.labelColor = [NSColor controlTextColor];
    }
    else
    {
        self.labelColor = [NSColor disabledControlTextColor];
    }

    [self enableEncoderOptionsWidgets:(video != nil)];
}

#pragma mark - KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBVideoControllerContext)
    {
        if ([keyPath isEqualToString:@"video.encoder"])
        {
            [self switchPresetView];
            [self setupQualitySlider];
        }
        else if ([keyPath isEqualToString:@"video.frameRate"])
        {
            // Hide and set the PFR Checkbox to OFF if we are set to Same as Source
            // Depending on whether or not Same as source is selected modify the title for
            // fFramerateVfrPfrCell
            if (self.video.frameRate == 0) // We are Same as Source
            {
                [fFramerateVfrPfrCell setTitle:@"Variable Framerate"];
            }
            else
            {
                [fFramerateVfrPfrCell setTitle:@"Peak Framerate (VFR)"];
            }
        }
        else if ([keyPath isEqualToString:@"video.unparseOptions"])
        {
            if (self.video.encoder & HB_VCODEC_X264_MASK)
            {
                fDisplayX264PresetsUnparseTextField.stringValue = [NSString stringWithFormat:@"x264 Unparse: %@", self.video.unparseOptions];
            }
            else
            {
                fDisplayX264PresetsUnparseTextField.stringValue = @"";
            }
        }
        else if ([keyPath isEqualToString:@"video.advancedOptions"])
        {
            if (self.video.advancedOptions)
            {
                // Do not enable the advanced panel it isn't visible.
                if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBShowAdvancedTab"])
                {
                    self.advancedController.videoSettings = self.video.advancedOptions ? self.video : nil;
                }
                else
                {
                    self.video.advancedOptions = NO;
                }
            }
            // enable/disable, populate and update the various widgets
            [self enableEncoderOptionsWidgets:(self.video != nil)];

        } else if ([keyPath isEqualToString:@"values.HBShowAdvancedTab"])
        {
            [self toggleAdvancedOptionsCheckBoxForEncoder:self.video.encoder];
        }
        else if ([keyPath isEqualToString:@"values.x264CqSliderFractional"])
        {
            [self setupQualitySlider];
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

#pragma mark - Interface setup

/*
 * Use this method to setup the quality slider for cq/rf values depending on
 * the video encoder selected.
 */
- (void)setupQualitySlider
{
    int direction;
    float minValue, maxValue, granularity;
    hb_video_quality_get_limits(self.video.encoder,
                                &minValue, &maxValue, &granularity, &direction);
    if (granularity < 1.0f)
    {
         // Encoders that allow fractional CQ values often have a low granularity
         // which makes the slider hard to use, so use a value from preferences.
        granularity = [[NSUserDefaults standardUserDefaults]
                       floatForKey:@"x264CqSliderFractional"];
    }
    [fVidQualitySlider setNumberOfTickMarks:(int)((maxValue - minValue) *
                                             (1.0f / granularity)) + 1];

    // Replace the slider transformer with a new one,
    // configured with the new max/min/direction values.
    [fVidQualitySlider unbind:@"value"];
    HBQualityTransformer *transformer = [[HBQualityTransformer alloc] initWithReversedDirection:(direction != 0) min:minValue max:maxValue];
    [fVidQualitySlider bind:@"value" toObject:self withKeyPath:@"self.video.quality" options:@{NSValueTransformerBindingOption: transformer}];
}

#pragma mark - Video x264/x265 Presets

/**
 *  Shows/hides the right preset view for the current video encoder.
 */
- (void)switchPresetView
{
    self.advancedController.hidden = YES;

    if (hb_video_encoder_get_presets(self.video.encoder) != NULL)
    {
        [self toggleAdvancedOptionsCheckBoxForEncoder:self.video.encoder];

        fPresetsBox.contentView = fPresetView;
        [self setupPresetsSlider];

        if (self.video.encoder & HB_VCODEC_X264_MASK)
        {
            self.advancedController.hidden = NO;
        }
    }
    else if (self.video.encoder & HB_VCODEC_FFMPEG_MASK)
    {
        fPresetsBox.contentView = fSimplePresetView;
    }
    else
    {
        fPresetsBox.contentView = nil;
    }
}

/**
 *  Enables/disables the advanced panel and the preset panel.
 */
- (void)enableEncoderOptionsWidgets:(BOOL)enable
{
    // check whether the x264 preset system and the advanced panel should be enabled
    BOOL enable_x264_controls  = (enable && !self.video.advancedOptions);
    BOOL enable_advanced_panel = (enable && self.video.advancedOptions);

    // enable/disable the checkbox and advanced panel
    self.presetViewEnabled = enable_x264_controls;
    self.advancedController.enabled = enable_advanced_panel;
}

/**
 *  Shows/Hides the advanced options checkbox
 *
 *  @param encoder the current encoder
 */
- (void)toggleAdvancedOptionsCheckBoxForEncoder:(int)encoder
{
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"HBShowAdvancedTab"] && (encoder & HB_VCODEC_X264_MASK))
    {
        fX264UseAdvancedOptionsCheck.hidden = NO;
        fDividerLine.hidden = YES;
        fEncoderOptionsLabel.stringValue = NSLocalizedString(@"Encoder Options:", @"");
    }
    else
    {
        fX264UseAdvancedOptionsCheck.hidden =YES;
        fDividerLine.hidden = NO;
        fEncoderOptionsLabel.stringValue = NSLocalizedString(@"Encoder Options", @"");
        self.video.advancedOptions = NO;
    }
}

/**
 *  Setup the presets slider with the right
 *  number of ticks.
 */
- (void)setupPresetsSlider
{
    // setup the preset slider
    [fPresetsSlider setMaxValue:self.video.presets.count - 1];
    [fPresetsSlider setNumberOfTickMarks:self.video.presets.count];

    // Bind the slider value to a custom value transformer,
    // done here because it can't be done in IB.
    [fPresetsSlider unbind:@"value"];
    HBPresetsTransformer *transformer = [[HBPresetsTransformer alloc] initWithEncoder:self.video.encoder];
    [fPresetsSlider bind:@"value" toObject:self withKeyPath:@"self.video.preset" options:@{NSValueTransformerBindingOption: transformer}];
}


@end
