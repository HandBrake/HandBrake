/* HBAdvancedController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */
    
#import "HBAdvancedController.h"
#import "HBVideo.h"
#import "HBVideo+UIAdditions.h"

@interface HBAdvancedController ()
{
    /* Advanced Tab for opts fX264optView*/
    IBOutlet NSBox              * fOptionsBox;

    IBOutlet NSView             * fEmptyView;

    IBOutlet NSView             * fX264optView;
    IBOutlet NSTextField        * fX264optViewTitleLabel;
    IBOutlet NSTextField        * fDisplayX264OptionsLabel;
    IBOutlet NSTextField        * fDisplayX264Options;

    IBOutlet NSTextField        * fX264optBframesLabel;
    IBOutlet NSPopUpButton      * fX264optBframesPopUp;
    IBOutlet NSTextField        * fX264optRefLabel;
    IBOutlet NSPopUpButton      * fX264optRefPopUp;
    IBOutlet NSButton           * fX264optWeightPSwitch;
    IBOutlet NSTextField        * fX264optWeightPLabel;
    IBOutlet NSTextField        * fX264optNodctdcmtLabel;
    IBOutlet NSButton           * fX264optNodctdcmtSwitch;
    IBOutlet NSTextField        * fX264optSubmeLabel;
    IBOutlet NSPopUpButton      * fX264optSubmePopUp;
    IBOutlet NSTextField        * fX264optTrellisLabel;
    IBOutlet NSPopUpButton      * fX264optTrellisPopUp;
    IBOutlet NSTextField        * fX264optMotionEstLabel;
    IBOutlet NSPopUpButton      * fX264optMotionEstPopUp;
    IBOutlet NSTextField        * fX264optMERangeLabel;
    IBOutlet NSPopUpButton      * fX264optMERangePopUp;
    IBOutlet NSTextField        * fX264optBPyramidLabel;
    IBOutlet NSPopUpButton      * fX264optBPyramidPopUp;
    IBOutlet NSTextField        * fX264optDirectPredLabel;
    IBOutlet NSPopUpButton      * fX264optDirectPredPopUp;
    IBOutlet NSTextField        * fX264optDeblockLabel;
    IBOutlet NSPopUpButton      * fX264optAlphaDeblockPopUp;
    IBOutlet NSPopUpButton      * fX264optBetaDeblockPopUp;
    IBOutlet NSTextField        * fX264optAnalyseLabel;
    IBOutlet NSPopUpButton      * fX264optAnalysePopUp;
    IBOutlet NSTextField        * fX264opt8x8dctLabel;
    IBOutlet NSButton           * fX264opt8x8dctSwitch;
    IBOutlet NSTextField        * fX264optCabacLabel;
    IBOutlet NSButton           * fX264optCabacSwitch;
    IBOutlet NSSlider           * fX264optAqSlider;
    IBOutlet NSTextField        * fX264optAqLabel;
    IBOutlet NSSlider           * fX264optPsyRDSlider;
    IBOutlet NSTextField        * fX264optPsyRDLabel;
    IBOutlet NSSlider           * fX264optPsyTrellisSlider;
    IBOutlet NSTextField        * fX264optPsyTrellisLabel;
    IBOutlet NSPopUpButton      * fX264optBAdaptPopUp;
    IBOutlet NSTextField        * fX264optBAdaptLabel;
}

- (IBAction) X264AdvancedOptionsAnimate: (id) sender;
- (IBAction) X264AdvancedOptionsSet: (id) sender;
- (IBAction) X264AdvancedOptionsStandardizeOptString: (id) sender;
- (IBAction) X264AdvancedOptionsSetCurrentSettings: (id) sender;
- (NSString *)  X264AdvancedOptionsStandardizeOptNames:(NSString *) cleanOptNameString;
- (NSString *)  X264AdvancedOptionsOptIDToString: (id) sender;
- (NSString *)  X264AdvancedOptionsWidgetToString: (NSString *) optName withID: (id) sender;
- (BOOL) X264AdvancedOptionsIsOpt: (NSString *) optNameToChange inString: (NSString *) currentOptString;
- (IBAction) X264AdvancedOptionsChanged: (id) sender;

@end

@implementation HBAdvancedController

@synthesize enabled = _enabled;

- (instancetype)init
{
    self = [super initWithNibName:@"AdvancedView" bundle:nil];
    if (self)
    {

    }

    return self;
}

- (void)loadView
{
    [super loadView];
    [self setHidden:NO];
}

- (void)setVideoSettings:(HBVideo *)videoSettings
{
    _videoSettings = videoSettings;

    if (_videoSettings)
    {
        fDisplayX264Options.stringValue = _videoSettings.unparseOptions;
    }
    else
    {
        fDisplayX264Options.stringValue = @"";
    }
    [self X264AdvancedOptionsSet:nil];
}

- (void)setHidden:(BOOL)hidden
{
    if (hidden)
    {
        [fOptionsBox setContentView:fEmptyView];
    }
    else
    {
        [fOptionsBox setContentView:fX264optView];
    }
}

 - (void)setEnabled:(BOOL)flag
{
    _enabled = flag;

    unsigned i;
    NSControl * controls[] =
      { fX264optViewTitleLabel,fDisplayX264Options,fDisplayX264OptionsLabel,fX264optBframesLabel,
        fX264optBframesPopUp,fX264optRefLabel,fX264optRefPopUp,
        fX264optNodctdcmtLabel,fX264optNodctdcmtSwitch,fX264optSubmeLabel,fX264optSubmePopUp,
        fX264optTrellisLabel,fX264optTrellisPopUp, fX264optWeightPLabel, fX264optWeightPSwitch,
        fX264optMotionEstLabel,fX264optMotionEstPopUp,fX264optMERangeLabel,fX264optMERangePopUp,
        fX264optBPyramidLabel,fX264optBPyramidPopUp, fX264optAqLabel, fX264optAqSlider,
        fX264optDirectPredLabel,fX264optDirectPredPopUp,fX264optDeblockLabel,fX264optAnalyseLabel,
        fX264optAnalysePopUp,fX264opt8x8dctLabel,fX264opt8x8dctSwitch,fX264optCabacLabel,fX264optCabacSwitch,
        fX264optAlphaDeblockPopUp,fX264optBetaDeblockPopUp, fX264optPsyRDSlider, fX264optPsyRDLabel, fX264optPsyTrellisSlider, fX264optPsyTrellisLabel, fX264optBAdaptPopUp, fX264optBAdaptLabel };

    for( i = 0; i < sizeof( controls ) / sizeof( NSControl * ); i++ )
    {
        if( [[controls[i] className] isEqualToString: @"NSTextField"] )
        {
            NSTextField * tf = (NSTextField *) controls[i];
            if( ![tf isBezeled] )
            {
                [tf setTextColor: flag ? [NSColor controlTextColor] :
                    [NSColor disabledControlTextColor]];
                continue;
            }
        }
        [controls[i] setEnabled: flag];

    }
}


/**
 * Populates the option widgets
 */
- (IBAction) X264AdvancedOptionsSet: (id) sender
{
    /*Set opt widget values here*/
    
    NSString * toolTip = @"";
    
    /*B-Frames fX264optBframesPopUp*/
    int i;
    [fX264optBframesPopUp removeAllItems];
    [fX264optBframesPopUp addItemWithTitle:@"Default (3)"];
    for (i=0; i<17;i++)
    {
        [fX264optBframesPopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    toolTip =
        @"Sane values are ~2-5.  This specifies the maximum number of sequential B-frames that the encoder can use.  Large numbers generally won't help significantly unless Adaptive B-frames is set to Optimal.  Cel-animated source material and B-pyramid also significantly increase the usefulness of larger values. Baseline profile, as required for iPods and similar devices, requires B-frames to be set to 0 (off).";
    [fX264optBframesPopUp setToolTip: toolTip];
    [fX264optBframesLabel setToolTip: toolTip];
    
    /*Reference Frames fX264optRefPopUp*/
    [fX264optRefPopUp removeAllItems];
    [fX264optRefPopUp addItemWithTitle:@"Default (3)"];
    for (i=1; i<17;i++)
    {
        [fX264optRefPopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    toolTip =
        @"Sane values are ~1-6.  The more you add, the better the compression, but the slower the encode.  Cel animation tends to benefit from more reference frames a lot more than film content.  Note that many hardware devices have limitations on the number of supported reference frames, so if you're encoding for a handheld or standalone player, don't touch this unless you're absolutely sure you know what you're doing!";
    [fX264optRefPopUp setToolTip: toolTip];
    [fX264optRefLabel setToolTip: toolTip];

    /*Weight-P fX264optWeightPSwitch BOOLEAN*/
    [fX264optWeightPSwitch setState:1];
    toolTip = 
        @"Performs extra analysis to decide upon weighting parameters for each frame.  This improves overall compression slightly and improves the quality of fades greatly. Baseline profile, as required for iPods and similar devices, requires weighted P-frame prediction to be disabled.  Note that some devices and players, even those that support Main Profile, may have problems with Weighted P-frame prediction: the Apple TV is completely incompatible with it, for example.";
    [fX264optWeightPSwitch setToolTip: toolTip];
    [fX264optWeightPLabel setToolTip: toolTip];
    
    /*No Dict Decimate fX264optNodctdcmtSwitch BOOLEAN*/
    [fX264optNodctdcmtSwitch setState:0];    
    toolTip =
        @"x264 normally zeroes out nearly-empty data blocks to save bits to be better used for some other purpose in the video.  However, this can sometimes have slight negative effects on retention of subtle grain and dither.  Don't touch this unless you're having banding issues or other such cases where you are having trouble keeping fine noise.";
    [fX264optNodctdcmtSwitch setToolTip: toolTip];
    [fX264optNodctdcmtLabel setToolTip: toolTip];
    
    /*Sub Me fX264optSubmePopUp*/
    [fX264optSubmePopUp removeAllItems];
    [fX264optSubmePopUp addItemWithTitle:@"Default (7)"];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"0: SAD, no subpel (super fast!)"]];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"1: SAD, qpel"]];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"2: SATD, qpel"]];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"3: SATD, multi-qpel"]];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"4: SATD, qpel on all"]];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"5: SATD, multi-qpel on all"]];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"6: RD in I/P-frames"]];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"7: RD in all frames"]];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"8: RD refine in I/P-frames"]];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"9: RD refine in all frames"]];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"10: QPRD in all frames"]];
    [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"11: No early terminations in analysis"]];
    toolTip =
        @"This setting controls both subpixel-precision motion estimation and mode decision methods.\n\nSubpixel motion estimation is used for refining motion estimates beyond mere pixel accuracy, improving compression.\n\nMode decision is the method used to choose how to encode each block of the frame: a very important decision.\n\nSAD is the fastest method, followed by SATD, RD, RD refinement, and the slowest, QPRD.\n\n6 or higher is strongly recommended: Psy-RD, a very powerful psy optimization that helps retain detail, requires RD.\n\n11 disables all early terminations in analysis.\n\n10 and 11, the most powerful and slowest options, require adaptive quantization (aq-mode > 0) and trellis 2 (always).";
    [fX264optSubmePopUp setToolTip: toolTip];
    [fX264optSubmeLabel setToolTip: toolTip];
    
    /*Trellis fX264optTrellisPopUp*/
    [fX264optTrellisPopUp removeAllItems];
    [fX264optTrellisPopUp addItemWithTitle:@"Default (Encode only)"];
    [fX264optTrellisPopUp addItemWithTitle:[NSString stringWithFormat:@"Off"]];
    [fX264optTrellisPopUp addItemWithTitle:[NSString stringWithFormat:@"Encode only"]];
    [fX264optTrellisPopUp addItemWithTitle:[NSString stringWithFormat:@"Always"]];
    [fX264optTrellisPopUp setWantsLayer:YES];
    toolTip =
        @"Trellis fine-tunes the rounding of transform coefficients to squeeze out 3-5% more compression at the cost of some speed. \"Always\" uses trellis not only during the main encoding process, but also during analysis, which improves compression even more, albeit at great speed cost. Trellis costs more speed at higher bitrates.";
    [fX264optTrellisPopUp setToolTip: toolTip];
    [fX264optTrellisLabel setToolTip: toolTip];
    
    /*Motion Estimation fX264optMotionEstPopUp*/
    [fX264optMotionEstPopUp removeAllItems];
    [fX264optMotionEstPopUp addItemWithTitle:@"Default (Hexagon)"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Diamond"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Hexagon"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Uneven Multi-Hexagon"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Exhaustive"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Transformed Exhaustive"];
    toolTip =
        @"Controls the motion estimation method. Motion estimation is how the encoder estimates how each block of pixels in a frame has moved.  A better motion search method improves compression at the cost of speed.\n\nDiamond: performs an extremely fast and simple search using a diamond pattern.\n\nHexagon: performs a somewhat more effective but slightly slower search using a hexagon pattern.\n\nUneven Multi-Hex: performs a very wide search using a variety of patterns, more accurately capturing complex motion.\n\nExhaustive: performs a \"dumb\" search of every pixel in a wide area.  Significantly slower for only a small compression gain.\n\nTransformed Exhaustive: Like exhaustive, but makes even more accurate decisions. Accordingly, somewhat slower, also for only a small improvement.";
    [fX264optMotionEstPopUp setToolTip: toolTip];
    [fX264optMotionEstLabel setToolTip: toolTip];
    
    /*Motion Estimation range fX264optMERangePopUp*/
    [fX264optMERangePopUp removeAllItems];
    [fX264optMERangePopUp addItemWithTitle:@"Default (16)"];
    for (i=4; i<65;i++)
    {
        [fX264optMERangePopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    toolTip =
        @"This is the distance x264 searches from its best guess at the motion of a block in order to try to find its actual motion.  Doesn't apply to Diamond or Hexagon search options.  The default is fine for most content, but extremely high motion video, especially at HD resolutions, may benefit from higher ranges, albeit at a high speed cost.";
    [fX264optMERangePopUp setToolTip: toolTip];
    [fX264optMERangeLabel setToolTip: toolTip];
    
    /*B-frame Pyramids fX264optBPyramidPopUp*/
    [fX264optBPyramidPopUp removeAllItems];
    [fX264optBPyramidPopUp addItemWithTitle:@"Default (Normal)"];
    [fX264optBPyramidPopUp addItemWithTitle:@"Off"];
    [fX264optBPyramidPopUp addItemWithTitle:@"Strict"];
    [fX264optBPyramidPopUp setWantsLayer:YES];
    toolTip =
        @"B-pyramid improves compression by creating a pyramidal structure (hence the name) of B-frames, allowing B-frames to reference each other to improve compression.  Requires Max B-frames greater than 1; optimal adaptive B-frames is strongly recommended for full compression benefit.";
    [fX264optBPyramidPopUp setToolTip: toolTip];
    [fX264optBPyramidLabel setToolTip: toolTip];
    
    /*Direct B-Frame Prediction Mode fX264optDirectPredPopUp*/
    [fX264optDirectPredPopUp removeAllItems];
    [fX264optDirectPredPopUp addItemWithTitle:@"Default (Spatial)"];
    [fX264optDirectPredPopUp addItemWithTitle:@"None"];
    [fX264optDirectPredPopUp addItemWithTitle:@"Spatial"];
    [fX264optDirectPredPopUp addItemWithTitle:@"Temporal"];
    [fX264optDirectPredPopUp addItemWithTitle:@"Automatic"];
    [fX264optDirectPredPopUp setWantsLayer:YES];
    toolTip =
        @"H.264 allows for two different prediction modes, spatial and temporal, in B-frames.\n\nSpatial, the default, is almost always better, but temporal is sometimes useful too.\n\nx264 can, at the cost of a small amount of speed (and accordingly for a small compression gain), adaptively select which is better for each particular frame.";
    [fX264optDirectPredPopUp setToolTip: toolTip];
    [fX264optDirectPredLabel setToolTip: toolTip];
    
    /* Adaptive B-Frames Mode fX264optBAdaptPopUp */
    [fX264optBAdaptPopUp removeAllItems];
    [fX264optBAdaptPopUp addItemWithTitle:@"Default (Fast)"];
    [fX264optBAdaptPopUp addItemWithTitle:@"Off"];
    [fX264optBAdaptPopUp addItemWithTitle:@"Fast"];
    [fX264optBAdaptPopUp addItemWithTitle:@"Optimal"];
    [fX264optBAdaptPopUp setWantsLayer:YES];
    toolTip =
        @"x264 has a variety of algorithms to decide when to use B-frames and how many to use.\n\nFast mode takes roughly the same amount of time no matter how many B-frames you specify.  However, while fast, its decisions are often suboptimal.\n\nOptimal mode gets slower as the maximum number of B-Frames increases, but makes much more accurate decisions, especially when used with B-pyramid.";
    [fX264optBAdaptPopUp setToolTip: toolTip];
    [fX264optBAdaptLabel setToolTip: toolTip];
    
    /*Alpha Deblock*/
    [fX264optAlphaDeblockPopUp removeAllItems];
    [fX264optAlphaDeblockPopUp addItemWithTitle:@"Default (0)"];
    for (i=-6; i<7;i++)
    {
        [fX264optAlphaDeblockPopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    toolTip =
        @"H.264 has a built-in deblocking filter that smooths out blocking artifacts after decoding each frame.  This not only improves visual quality, but also helps compression significantly. The deblocking filter takes a lot of CPU power, so if you're looking to minimize CPU requirements for video playback, disable it.\n\nThe deblocking filter has two adjustable parameters, \"strength\" and \"threshold\". The former controls how strong (or weak) the deblocker is, while the latter controls how many (or few) edges it applies to. Lower values mean less deblocking, higher values mean more deblocking. The default is 0 (normal strength) for both parameters.";
    [fX264optAlphaDeblockPopUp setToolTip: toolTip];
    [fX264optDeblockLabel setToolTip: toolTip];

    /*Beta Deblock*/
    [fX264optBetaDeblockPopUp removeAllItems];
    [fX264optBetaDeblockPopUp addItemWithTitle:@"Default (0)"];
    for (i=-6; i<7;i++)
    {
        [fX264optBetaDeblockPopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    [fX264optBetaDeblockPopUp setToolTip: toolTip];
    [fX264optDeblockLabel setToolTip: toolTip];

    /* Analysis fX264optAnalysePopUp */
    [fX264optAnalysePopUp removeAllItems];
    [fX264optAnalysePopUp addItemWithTitle:@"Default (Most)"]; /* 0=default */
    [fX264optAnalysePopUp addItemWithTitle:[NSString stringWithFormat:@"None"]]; /* 1=none */
    [fX264optAnalysePopUp addItemWithTitle:[NSString stringWithFormat:@"Some"]]; /* 2=some */
    [fX264optAnalysePopUp addItemWithTitle:[NSString stringWithFormat:@"All"]]; /* 3=all */
    toolTip =
        @"Mode decision picks from a variety of options to make its decision: this option chooses what options those are.  Fewer partitions to check means faster encoding, at the cost of worse decisions, since the best option might have been one that was turned off.";
    [fX264optAnalysePopUp setToolTip: toolTip];
    [fX264optAnalyseLabel setToolTip: toolTip];

    /* 8x8 DCT fX264op8x8dctSwitch */
    [fX264opt8x8dctSwitch setState:1];
    [fX264opt8x8dctSwitch setWantsLayer:YES];
    toolTip =
        @"The 8x8 transform is the single most useful feature of x264 in terms of compression-per-speed.  It improves compression by at least 5% at a very small speed cost and may provide an unusually high visual quality benefit compared to its compression gain.  However, it requires High Profile, which many devices may not support.";
    [fX264opt8x8dctSwitch setToolTip: toolTip];
    [fX264opt8x8dctLabel setToolTip: toolTip];

    /* CABAC fX264opCabacSwitch */
    [fX264optCabacSwitch setState:1];
    toolTip =
        @"After the encoder has done its work, it has a bunch of data that needs to be compressed losslessly, similar to ZIP or RAR.  H.264 provides two options for this: CAVLC and CABAC.  CABAC decodes a lot slower but compresses significantly better (10-30%), especially at lower bitrates.  If you're looking to minimize CPU requirements for video playback, disable this option. Baseline profile, as required for iPods and similar devices, requires CABAC to be disabled.";
    [fX264optCabacSwitch setToolTip: toolTip];
    [fX264optCabacLabel setToolTip: toolTip];

    /* Adaptive Quantization Strength fX264opAqSlider */
    [fX264optAqSlider setMinValue:0.0];
    [fX264optAqSlider setMaxValue:2.0];
    [fX264optAqSlider setTickMarkPosition:NSTickMarkBelow];
    [fX264optAqSlider setNumberOfTickMarks:21];
    [fX264optAqSlider setAllowsTickMarkValuesOnly:YES];
    [fX264optAqSlider setFloatValue:1.0];
    toolTip =
        @"Adaptive quantization controls how the encoder distributes bits across the frame.  Higher values take more bits away from edges and complex areas to improve areas with finer detail.";
    [fX264optAqSlider setToolTip: toolTip];
    [fX264optAqLabel setToolTip: toolTip];
    
    /* PsyRDO fX264optPsyRDSlider */
    [fX264optPsyRDSlider setMinValue:0.0];
    [fX264optPsyRDSlider setMaxValue:2.0];
    [fX264optPsyRDSlider setTickMarkPosition:NSTickMarkBelow];
    [fX264optPsyRDSlider setNumberOfTickMarks:21];
    [fX264optPsyRDSlider setAllowsTickMarkValuesOnly:YES];
    [fX264optPsyRDSlider setFloatValue:1.0];
    toolTip =
        @"Psychovisual rate-distortion optimization takes advantage of the characteristics of human vision to dramatically improve apparent detail and sharpness.  The effect can be made weaker or stronger by adjusting the strength.  Being an RD algorithm, it requires mode decision to be at least \"6\".";
    [fX264optPsyRDSlider setToolTip: toolTip];
    [fX264optPsyRDLabel setToolTip: toolTip];

    /* PsyTrellis fX264optPsyRDSlider */
    [fX264optPsyTrellisSlider setMinValue:0.0];
    [fX264optPsyTrellisSlider setMaxValue:1.0];
    [fX264optPsyTrellisSlider setTickMarkPosition:NSTickMarkBelow];
    [fX264optPsyTrellisSlider setNumberOfTickMarks:21];
    [fX264optPsyTrellisSlider setAllowsTickMarkValuesOnly:YES];
    [fX264optPsyTrellisSlider setFloatValue:0.0];
    toolTip =
        @"Psychovisual trellis is an experimental algorithm to further improve sharpness and detail retention beyond what Psychovisual RD does.  Recommended values are around 0.2, though higher values may help for very grainy video or lower bitrate encodes.  Not recommended for cel animation and other sharp-edged graphics.";
    [fX264optPsyTrellisSlider setToolTip: toolTip];
    [fX264optPsyTrellisLabel setToolTip: toolTip];

    /* Standardize the option string */
    [self X264AdvancedOptionsStandardizeOptString:nil];

    /* Set Current GUI Settings based on newly standardized string */
    [self X264AdvancedOptionsSetCurrentSettings:sender];

    /* Fade out options that don't apply */
    [self X264AdvancedOptionsAnimate: sender];
}

/**
 * Cleans the option string to use a standard format of option=value
 */
- (IBAction) X264AdvancedOptionsStandardizeOptString: (id) sender
{
    /* Set widgets depending on the opt string in field */
    NSString * thisOpt; // The separated option such as "bframes=3"
    NSString * optName = @""; // The option name such as "bframes"
    NSString * optValue = @"";// The option value such as "3"
    NSString * changedOptString = @"";
    NSArray *currentOptsArray;
    
    /*First, we get an opt string to process */
    NSString *currentOptString = [fDisplayX264Options stringValue];
    
    /* Verify there is an opt string to process by making sure an
       option is getting its value set. If so, start to process it. */
    NSRange currentOptRange = [currentOptString rangeOfString:@"="];
    if (currentOptRange.location != NSNotFound)
    {
        /*Put individual options into an array based on the ":" separator for processing, result is "<opt>=<value>"*/
        currentOptsArray = [currentOptString componentsSeparatedByString:@":"];
        
        /*iterate through the array and get <opts> and <values*/
        NSUInteger loopcounter;
        NSUInteger currentOptsArrayCount = [currentOptsArray count];
        for (loopcounter = 0; loopcounter < currentOptsArrayCount; loopcounter++)
        {
            thisOpt = currentOptsArray[loopcounter];
            
            NSRange splitOptRange = [thisOpt rangeOfString:@"="];
            if (splitOptRange.location != NSNotFound)
            {
                optName = [thisOpt substringToIndex:splitOptRange.location];
                optValue = [thisOpt substringFromIndex:splitOptRange.location + 1];
                
                /* Standardize the names here depending on whats in the string */
                optName = [self X264AdvancedOptionsStandardizeOptNames:optName];
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,optValue];    
            }
            else // No value given so we use a default of "1"
            {
                optName = thisOpt;

                /* Standardize the names here depending on whats in the string */
                optName = [self X264AdvancedOptionsStandardizeOptNames:optName];
                thisOpt = [NSString stringWithFormat:@"%@=%d",optName,1];
            }
            
            /* Construct New String for opts here.*/
            if ([thisOpt isEqualToString:@""])
            {
                /* Blank option, just add it to the string. (Why?) */
                changedOptString = [NSString stringWithFormat:@"%@%@",changedOptString,thisOpt];
            }
            else
            {
                if ([changedOptString isEqualToString:@""])
                {
                    /* Blank string, output the current option. */
                    changedOptString = [NSString stringWithFormat:@"%@",thisOpt];
                }
                else
                {
                    /* Option exists and string exists, so append the option
                       to the string with a semi-colon inbetween them.       */
                    changedOptString = [NSString stringWithFormat:@"%@:%@",changedOptString,thisOpt];
                }
            }
        }
    }
    
    /* Change the option string to reflect the new standardized option string */
    [fDisplayX264Options setStringValue:changedOptString];
    self.videoSettings.videoOptionExtra = changedOptString;
}

/**
 * Cleans the option string to use a standard set of option names, by conflating synonyms.
 */
- (NSString *) X264AdvancedOptionsStandardizeOptNames:(NSString *) cleanOptNameString
{
    /* Reference Frames */
    if ([cleanOptNameString isEqualToString:@"ref"] || [cleanOptNameString isEqualToString:@"frameref"])
    {
        cleanOptNameString = @"ref";
    }
    
    /*No Dict Decimate*/
    if ([cleanOptNameString isEqualToString:@"no-dct-decimate"] || [cleanOptNameString isEqualToString:@"no_dct_decimate"] || [cleanOptNameString isEqualToString:@"nodct_decimate"])
    {
        cleanOptNameString = @"no-dct-decimate";
    }
    
    /*Subme*/
    if ([cleanOptNameString isEqualToString:@"subme"])
    {
        cleanOptNameString = @"subq";
    }
    
    /*ME Range*/
    if ([cleanOptNameString isEqualToString:@"me-range"] || [cleanOptNameString isEqualToString:@"me_range"])
        cleanOptNameString = @"merange";
    
    /*B Pyramid*/
    if ([cleanOptNameString isEqualToString:@"b_pyramid"])
    {
        cleanOptNameString = @"b-pyramid";
    }
    
    /*Direct Prediction*/
    if ([cleanOptNameString isEqualToString:@"direct-pred"] || [cleanOptNameString isEqualToString:@"direct_pred"])
    {
        cleanOptNameString = @"direct";
    }
    
    /*Deblocking*/
    if ([cleanOptNameString isEqualToString:@"filter"])
    {
        cleanOptNameString = @"deblock";
    }
    
    /*Analysis*/
    if ([cleanOptNameString isEqualToString:@"partitions"])
    {
        cleanOptNameString = @"analyse";
    }
    
    return cleanOptNameString;    
}

/**
 * Fades options in and out depending on whether they're available..
 */
- (IBAction) X264AdvancedOptionsAnimate: (id) sender
{
    /* Lots of situations to cover.
       - B-frames (when 0 turn of b-frame specific stuff, when < 2 disable b-pyramid)
       - CABAC (when 0 turn off trellis and psy-trel)
       - subme (if under 6, turn off psy-rd and psy-trel)
       - trellis (if 0, turn off psy-trel)
    */
    
    if( sender == fX264optBframesPopUp || sender == nil || sender == fDisplayX264Options )
    {
        if ( [fX264optBframesPopUp indexOfSelectedItem ] == 1 )
        {
            /* If the b-frame widget is at 1, the user has chosen
               not to use b-frames at all. So disable the options
               that can only be used when b-frames are enabled.        */
            
            if( [fX264optBPyramidPopUp isHidden] == false )
            {
                [[fX264optBPyramidPopUp animator] setHidden:YES];
                [[fX264optBPyramidLabel animator] setHidden:YES];
                if ( [fX264optBPyramidPopUp indexOfSelectedItem] > 0 )
                {
                    [fX264optBPyramidPopUp selectItemAtIndex: 0];
                    [[fX264optBPyramidPopUp cell] performClick:self];
                }
            }

            if( [fX264optDirectPredPopUp isHidden] == false )
            {
                [[fX264optDirectPredPopUp animator] setHidden:YES];
                [[fX264optDirectPredLabel animator] setHidden:YES];
                if ( [fX264optDirectPredPopUp indexOfSelectedItem] > 0 )
                {
                    [fX264optDirectPredPopUp selectItemAtIndex: 0];
                    [[fX264optDirectPredPopUp cell] performClick:self];
                }
            }

            if( [fX264optBAdaptPopUp isHidden] == false )
            {
                [[fX264optBAdaptPopUp animator] setHidden:YES];
                [[fX264optBAdaptLabel animator] setHidden:YES];
                if ( [fX264optBAdaptPopUp indexOfSelectedItem] > 0 )
                {
                    [fX264optBAdaptPopUp selectItemAtIndex: 0];
                    [[fX264optBAdaptPopUp cell] performClick:self];
                }
            }
        }
        else if ( [fX264optBframesPopUp indexOfSelectedItem ] == 2)
        {
            /* Only 1 b-frame? Disable b-pyramid. */
            if( [fX264optBPyramidPopUp isHidden] == false )
            {
                [[fX264optBPyramidPopUp animator] setHidden:YES];
                [[fX264optBPyramidLabel animator] setHidden:YES];
                if ( [fX264optBPyramidPopUp indexOfSelectedItem] > 0 )
                {
                    [fX264optBPyramidPopUp selectItemAtIndex: 0];
                    [[fX264optBPyramidPopUp cell] performClick:self];
                }
            }

            if( [fX264optDirectPredPopUp isHidden] == true )
            {
                [[fX264optDirectPredPopUp animator] setHidden:NO];
                [[fX264optDirectPredLabel animator] setHidden:NO];
            }
            
            if( [fX264optBAdaptPopUp isHidden] == true )
            {
                [[fX264optBAdaptPopUp animator] setHidden:NO];
                [[fX264optBAdaptLabel animator] setHidden:NO];
            }
        }
        else
        {
            if( [fX264optBPyramidPopUp isHidden] == true )
            {
                [[fX264optBPyramidPopUp animator] setHidden:NO];
                [[fX264optBPyramidLabel animator] setHidden:NO];
            }

            if( [fX264optDirectPredPopUp isHidden] == true )
            {
                [[fX264optDirectPredPopUp animator] setHidden:NO];
                [[fX264optDirectPredLabel animator] setHidden:NO];
            }
            
            if( [fX264optBAdaptPopUp isHidden] == true )
            {
                [[fX264optBAdaptPopUp animator] setHidden:NO];
                [[fX264optBAdaptLabel animator] setHidden:NO];
            }
        }
    }
    
    if( sender == fX264optMotionEstPopUp || sender == nil || sender == fDisplayX264Options )
    {
        if ( [fX264optMotionEstPopUp indexOfSelectedItem] < 3 )
        {
            /* ME-range can only be above 16 if me >= umh
              and changing it to < 16 is idiotic so hide it . */
            if( [fX264optMERangePopUp isHidden] == false )
            {
                [[fX264optMERangePopUp animator] setHidden:YES];
                [[fX264optMERangeLabel animator] setHidden:YES];
                if ( [fX264optMERangePopUp indexOfSelectedItem] > 0 )
                {
                    [fX264optMERangePopUp selectItemAtIndex:0];
                    [[fX264optMERangePopUp cell] performClick:self];
                }
            }
        }
        else
        {
            if( [fX264optMERangePopUp isHidden] == true )
            {
                [[fX264optMERangePopUp animator] setHidden:NO];
                [[fX264optMERangeLabel animator] setHidden:NO];
            }
        }
    }
    
    if( sender == fX264optSubmePopUp || sender == nil || sender == fDisplayX264Options )
    {
        if( [fX264optSubmePopUp indexOfSelectedItem] != 0 && [fX264optSubmePopUp indexOfSelectedItem] < 7 )
        {
            /* No Psy-RDO or Psy=trel if subme < 6. */
            if( [fX264optPsyRDSlider isHidden] == false )
            {
                [[fX264optPsyRDSlider animator] setHidden:YES];
                [[fX264optPsyRDLabel animator] setHidden:YES];
                [fX264optPsyRDSlider setFloatValue:1.0];
                [[fX264optPsyRDSlider cell] performClick:self];
            }

            if( [fX264optPsyTrellisSlider isHidden] == false)
            {
                [[fX264optPsyTrellisSlider animator] setHidden:YES];
                [[fX264optPsyTrellisLabel animator] setHidden:YES];
                [fX264optPsyTrellisSlider setFloatValue:0.0];
                [[fX264optPsyTrellisSlider cell] performClick:self];
            }
        }
        else
        {
            if( [fX264optPsyRDSlider isHidden] == true )
            {
                [[fX264optPsyRDSlider animator] setHidden:NO];
                [[fX264optPsyRDLabel animator] setHidden:NO];
            }

            if( ( [fX264optTrellisPopUp indexOfSelectedItem] == 0 || [fX264optTrellisPopUp indexOfSelectedItem] >= 2 ) && [fX264optPsyTrellisSlider isHidden] == true )
            {
                [[fX264optPsyTrellisSlider animator] setHidden:NO];
                [[fX264optPsyTrellisLabel animator] setHidden:NO];
            }
        }
    }
    
    if( sender == fX264optTrellisPopUp || sender == nil || sender == fDisplayX264Options )
    {
        if( [fX264optTrellisPopUp indexOfSelectedItem] > 0 && [fX264optTrellisPopUp indexOfSelectedItem] < 2 )
        {
            if( [fX264optPsyTrellisSlider isHidden] == false )
            {
                /* No Psy-trellis without trellis. */
                [[fX264optPsyTrellisSlider animator] setHidden:YES];
                [[fX264optPsyTrellisLabel animator] setHidden:YES];
                [[fX264optPsyTrellisSlider animator] setFloatValue:0.0];
                [[fX264optPsyTrellisSlider cell] performClick:self];
            }
        }
        else
        {
            if( ( [fX264optSubmePopUp indexOfSelectedItem] == 0 || [fX264optSubmePopUp indexOfSelectedItem] >= 7 ) && [fX264optPsyTrellisSlider isHidden] == true )
            {
                [[fX264optPsyTrellisSlider animator] setHidden:NO];
                [[fX264optPsyTrellisLabel animator] setHidden:NO];
            }
        }
    }
}

/**
 * Resets the GUI widgets to the contents of the option string.
 */
- (IBAction) X264AdvancedOptionsSetCurrentSettings: (id) sender
{
    /* Set widgets depending on the opt string in field */
    NSString * thisOpt; // The separated option such as "bframes=3"
    NSString * optName = @""; // The option name such as "bframes"
    NSString * optValue = @"";// The option value such as "3"
    NSArray *currentOptsArray;
    
    /*First, we get an opt string to process */
    NSString *currentOptString = [fDisplayX264Options stringValue];
    
    /* Verify there is an opt string to process by making sure an
       option is getting its value set. If so, start to process it. */
    NSRange currentOptRange = [currentOptString rangeOfString:@"="];
    if (currentOptRange.location != NSNotFound)
    {
        /*Put individual options into an array based on the ":" separator for processing, result is "<opt>=<value>"*/
        currentOptsArray = [currentOptString componentsSeparatedByString:@":"];
        
        /*iterate through the array and get <opts> and <values*/
        NSUInteger loopcounter;
        NSUInteger currentOptsArrayCount = [currentOptsArray count];
        for (loopcounter = 0; loopcounter < currentOptsArrayCount; loopcounter++)
        {
            thisOpt = currentOptsArray[loopcounter];
            
            /* Verify the option sets a value */
            NSRange splitOptRange = [thisOpt rangeOfString:@"="];            
            if (splitOptRange.location != NSNotFound)
            {
                /* Split thisOpt into an optName setting an optValue. */
                optName = [thisOpt substringToIndex:splitOptRange.location];
                optValue = [thisOpt substringFromIndex:splitOptRange.location + 1];
                
                /*Run through the available widgets for x264 opts and set them, as you add widgets, 
                    they need to be added here. This should be moved to its own method probably*/
                
                /*bframes NSPopUpButton*/
                if ([optName isEqualToString:@"bframes"])
                {
                    [fX264optBframesPopUp selectItemAtIndex:[optValue intValue]+1];
                }
                /*ref NSPopUpButton*/
                if ([optName isEqualToString:@"ref"])
                {
                    // Clamp values to a minimum of 1 and a maximum of 16
                    if ( [optValue intValue] < 1 )
                    {
                        [fX264optRefPopUp selectItemAtIndex:1];
                        [ self X264AdvancedOptionsChanged: fX264optRefPopUp];
                    }
                    else if ( [optValue intValue] > 16 )
                    {
                        [fX264optRefPopUp selectItemAtIndex:16];
                        [ self X264AdvancedOptionsChanged: fX264optRefPopUp];
                    }
                    else
                    {
                        [fX264optRefPopUp selectItemAtIndex:[optValue intValue]];
                    }
                }
                /*WeightP NSButton*/
                if ([optName isEqualToString:@"weightp"])
                {
                    if ([optValue intValue] < 1)
                        [fX264optWeightPSwitch setState:0];
                    else
                        [fX264optWeightPSwitch setState:1];
                }
                /*No Dict Decimate NSButton*/
                if ([optName isEqualToString:@"no-dct-decimate"])
                {
                    [fX264optNodctdcmtSwitch setState:[optValue intValue]];
                }
                /*Sub Me NSPopUpButton*/
                if ([optName isEqualToString:@"subq"])
                {
                    [fX264optSubmePopUp selectItemAtIndex:[optValue intValue]+1];
                }
                /*Trellis NSPopUpButton*/
                if ([optName isEqualToString:@"trellis"])
                {
                    [fX264optTrellisPopUp selectItemAtIndex:[optValue intValue]+1];
                }
                /*Motion Estimation NSPopUpButton*/
                if ([optName isEqualToString:@"me"])
                {
                    if ([optValue isEqualToString:@"dia"])
                        [fX264optMotionEstPopUp selectItemAtIndex:1];
                    else if ([optValue isEqualToString:@"hex"])
                        [fX264optMotionEstPopUp selectItemAtIndex:2];
                    else if ([optValue isEqualToString:@"umh"])
                        [fX264optMotionEstPopUp selectItemAtIndex:3];
                    else if ([optValue isEqualToString:@"esa"])
                        [fX264optMotionEstPopUp selectItemAtIndex:4];
                    else if ([optValue isEqualToString:@"tesa"])
                        [fX264optMotionEstPopUp selectItemAtIndex:5];
                }
                /*ME Range NSPopUpButton*/
                if ([optName isEqualToString:@"merange"])
                {
                    [fX264optMERangePopUp selectItemAtIndex:[optValue intValue]-3];
                }
                /* Adaptive B-Frames NSPopUpButton*/
                if ([optName isEqualToString:@"b-adapt"])
                {
                    [fX264optBAdaptPopUp selectItemAtIndex:[optValue intValue]+1];
                }
                /*B Pyramid NSPButton*/
                if ([optName isEqualToString:@"b-pyramid"])
                {
                    
                    if( [optValue isEqualToString:@"normal"] )
                    {
                        [self X264AdvancedOptionsChanged: fX264optBPyramidPopUp];
                        [fX264optBPyramidPopUp selectItemAtIndex:0];
                    }
                    else if( [optValue isEqualToString:@"2"] )
                    {
                        [fX264optBPyramidPopUp selectItemAtIndex:0];
                        [self X264AdvancedOptionsChanged: fX264optBPyramidPopUp];
                    }
                    if( [optValue isEqualToString:@"strict"] )
                    {
                        [fX264optBPyramidPopUp selectItemAtIndex:2];
                    }
                    else if( [optValue isEqualToString:@"1"] )
                    {
                        [fX264optBPyramidPopUp selectItemAtIndex:2];
                        [self X264AdvancedOptionsChanged: fX264optBPyramidPopUp];
                    }
                    if( [optValue isEqualToString:@"none"] )
                    {
                        [fX264optBPyramidPopUp selectItemAtIndex:1];
                    }
                    else if( [optValue isEqualToString:@"0"] )
                    {
                        [fX264optBPyramidPopUp selectItemAtIndex:1];
                        [self X264AdvancedOptionsChanged: fX264optBPyramidPopUp];
                    }
                }                
                /*Direct B-frame Prediction NSPopUpButton*/
                                if ([optName isEqualToString:@"direct"])
                {
                    if ([optValue isEqualToString:@"none"])
                        [fX264optDirectPredPopUp selectItemAtIndex:1];
                    else if ([optValue isEqualToString:@"spatial"])
                        [fX264optDirectPredPopUp selectItemAtIndex:2];
                    else if ([optValue isEqualToString:@"temporal"])
                        [fX264optDirectPredPopUp selectItemAtIndex:3];
                    else if ([optValue isEqualToString:@"auto"])
                        [fX264optDirectPredPopUp selectItemAtIndex:4];                        
                }
                /*Deblocking NSPopUpButtons*/
                if ([optName isEqualToString:@"deblock"])
                {
                    NSString * alphaDeblock = @"";
                    NSString * betaDeblock = @"";
                    
                    NSRange splitDeblock = [optValue rangeOfString:@","];
                    alphaDeblock = [optValue substringToIndex:splitDeblock.location];
                    betaDeblock = [optValue substringFromIndex:splitDeblock.location + 1];
                    
                    if ([alphaDeblock isEqualToString:@"0"] && [betaDeblock isEqualToString:@"0"])
                    {
                        /* When both filters are at 0, default */
                        [fX264optAlphaDeblockPopUp selectItemAtIndex:0];                        
                        [fX264optBetaDeblockPopUp selectItemAtIndex:0];                               
                    }
                    else
                    {
                        if (![alphaDeblock isEqualToString:@"0"])
                        {
                            /* Alpha isn't 0, so set it. The offset of 7 is
                               because filters start at -6 instead of at 0. */
                            [fX264optAlphaDeblockPopUp selectItemAtIndex:[alphaDeblock intValue]+7];
                        }
                        else
                        {
                            /* Set alpha filter to 0, which is 7 up
                               because filters start at -6, not 0. */
                            [fX264optAlphaDeblockPopUp selectItemAtIndex:7];                        
                        }
                        
                        if (![betaDeblock isEqualToString:@"0"])
                        {
                            /* Beta isn't 0, so set it. */
                            [fX264optBetaDeblockPopUp selectItemAtIndex:[betaDeblock intValue]+7];
                        }
                        else
                        {
                            /* Set beta filter to 0. */
                            [fX264optBetaDeblockPopUp selectItemAtIndex:7];                        
                        }
                    }
                }
                /* Analysis NSPopUpButton */
                if ([optName isEqualToString:@"analyse"])
                {
                    if ([optValue isEqualToString:@"p8x8,b8x8,i8x8,i4x4"])
                    {
                        /* Default ("most") */
                        [fX264optAnalysePopUp selectItemAtIndex:0];
                    }
                    else if ([optValue isEqualToString:@"i4x4,i8x8"] ||
                             [optValue isEqualToString:@"i8x8,i4x4"] )
                    {
                        /* Some */
                        [fX264optAnalysePopUp selectItemAtIndex:2];
                    }
                    else if ([optValue isEqualToString:@"none"])
                    {
                        [fX264optAnalysePopUp selectItemAtIndex:1];
                    }
                    else if ([optValue isEqualToString:@"all"])
                    {
                        [fX264optAnalysePopUp selectItemAtIndex:3];
                    }
                    
                }
                /* 8x8 DCT NSButton */
                if ([optName isEqualToString:@"8x8dct"])
                {
                    [fX264opt8x8dctSwitch setState:[optValue intValue]];
                }
                /* CABAC NSButton */
                if ([optName isEqualToString:@"cabac"])
                {
                    [fX264optCabacSwitch setState:[optValue intValue]];
                }
                /* Adaptive Quantization Strength NSSlider */
                if ([optName isEqualToString:@"aq-strength"])
                {
                    [fX264optAqSlider setFloatValue:[optValue floatValue]];
                }                                                              
                /* Psy-RD and Psy-Trellis NSSliders */
                if ([optName isEqualToString:@"psy-rd"])
                {
                    NSString * rdOpt = @"";
                    NSString * trellisOpt = @"";
                    
                    NSRange splitRD = [optValue rangeOfString:@","];
                    rdOpt = [optValue substringToIndex:splitRD.location];
                    trellisOpt = [optValue substringFromIndex:splitRD.location + 1];
                    
                    [fX264optPsyRDSlider setFloatValue:[rdOpt floatValue]];
                    [fX264optPsyTrellisSlider setFloatValue:[trellisOpt floatValue]];
                }                                                              
            }
        }
    }
}

- (NSString *) X264AdvancedOptionsOptIDToString: (id) widget
{
    /*Determine which outlet is being used and set optName to process accordingly */
    NSString * optNameToChange = @""; // The option name such as "bframes"
    
    if (widget == fX264optBframesPopUp)
    {
        optNameToChange = @"bframes";
    }
    if (widget == fX264optRefPopUp)
    {
        optNameToChange = @"ref";
    }
    if (widget == fX264optWeightPSwitch)
    {
        optNameToChange = @"weightp";
    }
    if (widget == fX264optNodctdcmtSwitch)
    {
        optNameToChange = @"no-dct-decimate";
    }
    if (widget == fX264optSubmePopUp)
    {
        optNameToChange = @"subq";
    }
    if (widget == fX264optTrellisPopUp)
    {
        optNameToChange = @"trellis";
    }
    if (widget == fX264optMotionEstPopUp)
    {
        optNameToChange = @"me";
    }
    if (widget == fX264optMERangePopUp)
    {
        optNameToChange = @"merange";
    }
    if (widget == fX264optBAdaptPopUp)
    {
        optNameToChange = @"b-adapt";
    }
    if (widget == fX264optBPyramidPopUp)
    {
        optNameToChange = @"b-pyramid";
    }
    if (widget == fX264optDirectPredPopUp)
    {
        optNameToChange = @"direct";
    }
    if (widget == fX264optAlphaDeblockPopUp)
    {
        optNameToChange = @"deblock";
    }
    if (widget == fX264optBetaDeblockPopUp)
    {
        optNameToChange = @"deblock";
    }        
    if (widget == fX264optAnalysePopUp)
    {
        optNameToChange = @"analyse";
    }
    if (widget == fX264opt8x8dctSwitch)
    {
        optNameToChange = @"8x8dct";
    }
    if (widget == fX264optCabacSwitch)
    {
        optNameToChange = @"cabac";
    }
    if( widget == fX264optAqSlider)
    {
        optNameToChange = @"aq-strength";
    }
    if( widget == fX264optPsyRDSlider)
    {
        optNameToChange = @"psy-rd";
    }
    if( widget == fX264optPsyTrellisSlider)
    {
        optNameToChange = @"psy-rd";
    }
    
    return optNameToChange;
}

- (NSString *) X264AdvancedOptionsWidgetToString: (NSString *) optName withID: (id) sender
{
    NSString * thisOpt = @""; // The option=value string the method will return
    
    if ([optName isEqualToString:@"deblock"])
    {
        if ((([fX264optAlphaDeblockPopUp indexOfSelectedItem] == 0) || ([fX264optAlphaDeblockPopUp indexOfSelectedItem] == 7)) && (([fX264optBetaDeblockPopUp indexOfSelectedItem] == 0) || ([fX264optBetaDeblockPopUp indexOfSelectedItem] == 7)))
        {
            /* When both deblock widgets are 0 or default or a mix of the two,
               use a blank string, since deblocking defaults to 0,0.           */
            thisOpt = @"";                                
        }
        else
        {
            /* Otherwise the format is deblock=a,b, where a and b both have an array
               offset of 7 because deblocking values start at -6 instead of at zero. */
            thisOpt = [NSString stringWithFormat:@"%@=%ld,%ld",optName, ([fX264optAlphaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optAlphaDeblockPopUp indexOfSelectedItem]-7 : 0,([fX264optBetaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optBetaDeblockPopUp indexOfSelectedItem]-7 : 0];
        }
    }
    
    else if ([optName isEqualToString:@"aq-strength"])
    {
        if( [fX264optAqSlider floatValue] == 1.0 ) 
        {
            /* When Aq is 1 it's the default value and can be ignored. */
            thisOpt = @"";                                
        }
        else
        {
            thisOpt = [NSString stringWithFormat:@"%@=%0.1f", optName, [fX264optAqSlider floatValue] ];
        }
    }

    else if ([optName isEqualToString:@"psy-rd"])
    {
        if( [fX264optPsyRDSlider floatValue] == 1.0 && [fX264optPsyTrellisSlider floatValue] == 0.0 ) 
        {
            /* When  PsyRD is 1 and PsyTrel is 0 they're default values and can be ignored. */
            thisOpt = @"";                                
        }
        else
        {
            /* Otherwise the format is psy-rd=a,b */
            thisOpt = [NSString stringWithFormat:@"%@=%0.1f,%0.2f", optName, [fX264optPsyRDSlider floatValue], [fX264optPsyTrellisSlider floatValue] ];
        }
    }
    
    else if /*Boolean Switches*/ ( [optName isEqualToString:@"no-dct-decimate"] )
    {
        /* Here is where we take care of the boolean options that work overtly:
           no-dct-decimate being on means no-dct-decimate=1, etc. Some options
           require the inverse, but those will be handled a couple lines down. */
        if ([sender state] == 0)
        {
            /* When these options are false, don't include them. They all default
               to being set off, so they don't need to be mentioned at all.       */
            thisOpt = @"";
        }
        else
        {
            /* Otherwise, include them as optioname=1 */
            thisOpt = [NSString stringWithFormat:@"%@=%d",optName,1];
        }
    }
    
    else if ( [optName isEqualToString:@"8x8dct"] || [optName isEqualToString:@"cabac"] || [optName isEqualToString:@"weightp"] )
    {
        /* These options default to being on. That means they
           only need to be included in the string when turned off. */
        if ([sender state] == 1)
        {
            /* It's true so don't include it. */
            thisOpt = @"";
        }
        else
        {
            /* Otherwise, include cabac=0, etc, in the string. */
            thisOpt = [NSString stringWithFormat:@"%@=%d",optName,0];
        }
    }
                                            
    else if (([sender indexOfSelectedItem] == 0) && (sender != fX264optAlphaDeblockPopUp) && (sender != fX264optBetaDeblockPopUp) ) // means that "unspecified" is chosen, lets then remove it from the string
    {
        /* When a widget is at index 0, it's default. Default means don't add to the string.
           The exception for deblocking is because for those, *both* need to at index 0
           for it to default, so it's handled separately, above this section.                */
        thisOpt = @"";
    }
    
    else if ([optName isEqualToString:@"me"])
    {
        /* Motion estimation uses string values, so this switch
           pairs the widget index with the right value string.  */
        switch ([sender indexOfSelectedItem])
        {   
            case 1:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"dia"];
                break;
                
            case 2:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"hex"];
                break;
                
            case 3:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"umh"];
                break;
                
            case 4:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"esa"];
                break;
            
            case 5:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"tesa"];
            
            default:
                break;
        }
    }
    
    else if ([optName isEqualToString:@"direct"])
    {
        /* Direct prediction uses string values, so this switch
           pairs the right string value with the right widget index. */
        switch ([sender indexOfSelectedItem])
        {   
            case 1:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"none"];
                break;
                
            case 2:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"spatial"];
                break;
                
            case 3:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"temporal"];
                break;
                
            case 4:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"auto"];
                break;
                
            default:
                break;
        }
    }
    
    else if ([optName isEqualToString:@"analyse"])
    {
        /* Analysis uses string values as well. */
        switch ([sender indexOfSelectedItem])
        {   
            case 1:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"none"];
                break;
            case 2:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"i4x4,i8x8"];
                break;
            case 3:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"all"];
                break;
                
            default:
                break;
        }
    }

    else if ([optName isEqualToString:@"b-pyramid"])
    {
        /* B-pyramid uses string values too. */
        switch ([sender indexOfSelectedItem])
        {   
            case 1:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"none"];
                break;
            case 2:
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"strict"];
                break;
            case 0:
                thisOpt = @"";
                break;
                
            default:
                break;
        }
    }
    
    else if ([optName isEqualToString:@"merange"])
    {
        /* Motion estimation range uses an odd array offset because in addition
           to starting with index 0 as default, index 1 starts at 4 instead of 1,
           because merange can't go below 4. So it has to be handled separately.  */
        thisOpt = [NSString stringWithFormat:@"%@=%ld",optName,[sender indexOfSelectedItem]+3];
    }
    
    else if ([optName isEqualToString:@"b-adapt"])
    {
        /* B-adapt starts at index 0 with default then goes 0, 1, 2)*/
        thisOpt = [NSString stringWithFormat:@"%@=%ld", optName, [sender indexOfSelectedItem]-1];
    }
    
    else if ([optName isEqualToString:@"ref"])
    {
        /* Refs use actual index numbers */
        thisOpt = [NSString stringWithFormat:@"%@=%ld",optName,(long)[sender indexOfSelectedItem]];
    }
    
    else // we have a valid value to change, so change it
    {
        if ( [sender indexOfSelectedItem] != 0 )
        /* Here's our general case, that catches things like b-frames.
           Basically, any options that are PopUp menus with index 0 as default and
           index 1 as 0, with numerical values, are all handled right here. All of
           the above stuff is for the exceptions to the general case.              */
            thisOpt = [NSString stringWithFormat:@"%@=%ld",optName,[sender indexOfSelectedItem]-1];
    }
    
    return thisOpt;
}

- (BOOL) X264AdvancedOptionsIsOpt: (NSString *) optNameToChange inString: (NSString *) currentOptString
{
    /* If the option is in the string but not the beginning of it,
       it will be in the form of ":optName=value" so we really want
       to be looking for ":optNameToChange=" rather than "optNameToChange". */
    NSString *checkOptNameToChange = [NSString stringWithFormat:@":%@=",optNameToChange];
    
    /* Now we store the part of the string up through the option name in currentOptRange. */
    NSRange currentOptRange = [currentOptString rangeOfString:checkOptNameToChange];

    /*  We need to know if the option is at the beginning of the string.
        If it is at the start, it won't be preceded by a colon.
        To figure this out, we'll use the rangeOfString method. First,
        store what the option name would be if if it was at the beginning,
        in checkOptNameToChangeBeginning. Then, find its range in the string.
        If the range is 0, it's the first option listed in the string.       */        
    NSString *checkOptNameToChangeBeginning = [NSString stringWithFormat:@"%@=",optNameToChange];
    NSRange currentOptRangeBeginning = [currentOptString rangeOfString:checkOptNameToChangeBeginning];

    if (currentOptRange.location != NSNotFound || currentOptRangeBeginning.location == 0)
        return true;
    else
        return false;
} 

/**
 * Resets the option string to mirror the GUI widgets.
 */
- (IBAction) X264AdvancedOptionsChanged: (id) sender
{
    /* Look up the equivalent string option name of the calling widget. */
    NSString * optNameToChange = [self X264AdvancedOptionsOptIDToString: sender];
    
    NSString * thisOpt = @"";  // The separated option such as "bframes=3"
    NSString * optName = @"";  // The option name such as "bframes"
    NSArray *currentOptsArray;
    
    /* Get the current opt string being displayed. */
    NSString *currentOptString = [fDisplayX264Options stringValue];
    
    /* There are going to be a few possibilities.
       - The option might start off the string.
       - The option might be in the middle of the string.
       - The option might not be in the string at all yet.
       - The string itself might not yet exist.             */
    
    if( [self X264AdvancedOptionsIsOpt: optNameToChange inString: currentOptString] )
    {
        /* If the option is in the string wth a semicolon, or starts the string, it's time to edit.
           This means parsing the whole string into an array of options and values. From there,
           iterate through the options, and when you reach the one that's been changed, edit it.   */
        
        /* Create new empty opt string*/
        NSString *changedOptString = @"";
        
        /* Put individual options into an array based on the ":"
           separator for processing, result is "<opt>=<value>"   */
        currentOptsArray = [currentOptString componentsSeparatedByString:@":"];
        
        /* Iterate through the array and get <opts> and <values*/
        NSUInteger loopcounter;
        NSUInteger currentOptsArrayCount = [currentOptsArray count];
        for (loopcounter = 0; loopcounter < currentOptsArrayCount; loopcounter++)
        {
            thisOpt = currentOptsArray[loopcounter];
            NSRange splitOptRange = [thisOpt rangeOfString:@"="];
            
            if (splitOptRange.location != NSNotFound)
            {
                /* First off, it's time to handle option strings that
                   already have at least one option=value pair in them. */
                   
                optName = [thisOpt substringToIndex:splitOptRange.location];

                /*If the optNameToChange is found, appropriately change the value or delete it if
                    "Unspecified" is set.*/
                if ([optName isEqualToString:optNameToChange])
                {
                    thisOpt = [self X264AdvancedOptionsWidgetToString: optName withID: sender];
                }
            }
            
            /* Construct New String for opts here */
            if ([thisOpt isEqualToString:@""])
            {
                /* Blank option, so just add it to the string. (Why?) */
                changedOptString = [NSString stringWithFormat:@"%@%@",changedOptString,thisOpt];
            }
            else
            {
                if ([changedOptString isEqualToString:@""])
                {
                    /* No existing string, make the string this option. */
                    changedOptString = [NSString stringWithFormat:@"%@",thisOpt];
                }
                else
                {
                    /* Existing string, existing option. Append the
                       option to the string, preceding it with a colon. */
                    changedOptString = [NSString stringWithFormat:@"%@:%@",changedOptString,thisOpt];
                }
            }
        }
        
        /* Change the dislayed option string to reflect the new modified settings */
        [fDisplayX264Options setStringValue:changedOptString];
    }
    else // if none exists, add it to the string
    {
        /* This is where options that aren't already in the string are handled. */
        if ([[fDisplayX264Options stringValue] isEqualToString: @""])
        {
            
            [fDisplayX264Options setStringValue:
                [self X264AdvancedOptionsWidgetToString: optNameToChange withID: sender]];
        }
        else
        {
            /* The string isn't empty, and the option isn't already in it, so
               it will need to be appended to the current string with a colon,
               as long as the string to be appended isn't just blank (default). */
            if( [[self X264AdvancedOptionsWidgetToString: optNameToChange withID: sender] isEqualToString: @""] == false )
            {
                [fDisplayX264Options setStringValue:
                    [NSString stringWithFormat:@"%@:%@",
                        currentOptString,
                        [self X264AdvancedOptionsWidgetToString: optNameToChange withID: sender] ]];                
            }
        }
    }
    
    /* We now need to reset the opt widgets since we changed some stuff */        
    [self X264AdvancedOptionsSet:sender];
    self.videoSettings.videoOptionExtra = fDisplayX264Options.stringValue;
}

@end
