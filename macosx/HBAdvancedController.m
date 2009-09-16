/* HBAdvancedController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */
    
#import "HBAdvancedController.h"

@implementation HBAdvancedController

- (id)init
{
    [super init];
    [self loadMyNibFile];
    
    return self;
}

- (void) setView: (NSBox *) box
{
    fOptionsBox = box;
    [fOptionsBox setContentView:fX264optView];
}

- (BOOL) loadMyNibFile
{
    if(![NSBundle loadNibNamed:@"AdvancedView" owner:self])
    {
        NSLog(@"Warning! Could not load myNib file.\n");
        return NO;
    }
    
    return YES;
}

- (NSString *) optionsString
{
    return [fDisplayX264Options stringValue];
}

- (void) setOptions: (NSString *)string
{
    [fDisplayX264Options setStringValue:string];
    [self X264AdvancedOptionsSet:nil];
}

- (void) setHidden: (BOOL) hide
{
    if(hide)
    {
        [fOptionsBox setContentView:fEmptyView];
        [fX264optViewTitleLabel setStringValue: @"Only Used With The x264 (H.264) Codec"];
    }
    else
    {
        [fOptionsBox setContentView:fX264optView];
        [fX264optViewTitleLabel setStringValue: @""];
    }
    return;
}

 - (void) enableUI: (bool) b
{
    unsigned i;
    NSControl * controls[] =
      { fX264optViewTitleLabel,fDisplayX264Options,fDisplayX264OptionsLabel,fX264optBframesLabel,
        fX264optBframesPopUp,fX264optRefLabel,fX264optRefPopUp,fX264optNfpskipLabel,fX264optNfpskipSwitch,
        fX264optNodctdcmtLabel,fX264optNodctdcmtSwitch,fX264optSubmeLabel,fX264optSubmePopUp,
        fX264optTrellisLabel,fX264optTrellisPopUp,fX264optMixedRefsLabel,fX264optMixedRefsSwitch,
        fX264optMotionEstLabel,fX264optMotionEstPopUp,fX264optMERangeLabel,fX264optMERangePopUp,
        fX264optWeightBLabel,fX264optWeightBSwitch, fX264optBPyramidLabel,fX264optBPyramidSwitch,
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
                [tf setTextColor: b ? [NSColor controlTextColor] :
                    [NSColor disabledControlTextColor]];
                continue;
            }
        }
        [controls[i] setEnabled: b];

    }
    
    [fX264optView setWantsLayer:YES];
}

- (void)dealloc
{
    [super dealloc];
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
        @"Sane values are 1-6. B-Frames are smaller than other frames, so they let you pack in more quality at the same bitrate. Use more of them with animated material.";
    [fX264optBframesPopUp setToolTip: toolTip];
    [fX264optBframesLabel setToolTip: toolTip];
    
    /*Reference Frames fX264optRefPopUp*/
    [fX264optRefPopUp removeAllItems];
    [fX264optRefPopUp addItemWithTitle:@"Default (3)"];
    for (i=0; i<17;i++)
    {
        [fX264optRefPopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    toolTip =
        @"Sane values are 1-6. The more you add, the higher the quality — but the slower the encode. Be careful...too many and QuickTime struggle to play the video back.";
    [fX264optRefPopUp setToolTip: toolTip];
    [fX264optRefLabel setToolTip: toolTip];
    
    /*No Fast P-Skip fX264optNfpskipSwitch BOOLEAN*/
    [fX264optNfpskipSwitch setState:0];
    toolTip =
        @"This can help with blocking on solid colors like blue skies, but it also slows down the encode.";
    [fX264optNfpskipSwitch setToolTip: toolTip];
    [fX264optNfpskipLabel setToolTip: toolTip];
    
    /*No Dict Decimate fX264optNodctdcmtSwitch BOOLEAN*/
    [fX264optNodctdcmtSwitch setState:0];    
    toolTip =
        @"To save space, x264 will \"zero out\" blocks when it thinks they won't be perceptible by the viewer. This negligibly reduces quality, but in rare cases it can mess up and produce visible artifacts. This situation can be alleviated by telling x264 not to decimate DCT blocks.\n\nIt increases quality but also bitrate/file size, so if you use it when you've specified a target bitrate you will end up with a worse picture than without it. However, when used with constant quality encoding, or if you boost the average bitrate to compensate, you might get a better result.";
    [fX264optNodctdcmtSwitch setToolTip: toolTip];
    [fX264optNodctdcmtLabel setToolTip: toolTip];
    
    /*Sub Me fX264optSubmePopUp*/
    [fX264optSubmePopUp removeAllItems];
    [fX264optSubmePopUp addItemWithTitle:@"Default (7)"];
    for (i=0; i<10;i++)
    {
        [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    toolTip =
        @"This setting is finer-grained than the motion estimation settings above. Instead of dealing with whole pixels, it deals with 4 fractional pixels, or quarter pixels (qpel). Higher levels increase quality by further refining the motion prediction for these quarter pixels, but take longer to encode.\n\nLevel 6, turns on a feature called rate distortion optimization, including psychovisual enhancements. 7, the default, enables that rate distortion for B-frames. 8 refines those decisions for I and P frames, and 9 adds on refinement for B-frames as well.";
    [fX264optSubmePopUp setToolTip: toolTip];
    [fX264optSubmeLabel setToolTip: toolTip];
    
    /*Trellis fX264optTrellisPopUp*/
    [fX264optTrellisPopUp removeAllItems];
    [fX264optTrellisPopUp addItemWithTitle:@"Default (1)"];
    for (i=0; i<3;i++)
    {
        [fX264optTrellisPopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    [fX264optTrellisPopUp setWantsLayer:YES];
    toolTip =
        @"Trellis fine-tunes how bitrate is doled out, so it can reduce file size/bitrate or increase quality. A value of 1 means it only fine-tunes the final encode of a block of pixels, while 2 means it is considered during earlier phases of the decision-making process as well.";
    [fX264optTrellisPopUp setToolTip: toolTip];
    [fX264optTrellisLabel setToolTip: toolTip];
    
    /*Mixed-references fX264optMixedRefsSwitch BOOLEAN*/
    [fX264optMixedRefsSwitch setState:1];
    [fX264optMixedRefsSwitch setWantsLayer:YES];
    toolTip =
        @"With this on, different references can be used for different parts of each 16x16 pixel macroblock, increasing quality.";
    [fX264optMixedRefsSwitch setToolTip: toolTip];
    [fX264optMixedRefsLabel setToolTip: toolTip];
    
    /*Motion Estimation fX264optMotionEstPopUp*/
    [fX264optMotionEstPopUp removeAllItems];
    [fX264optMotionEstPopUp addItemWithTitle:@"Default (Hexagon)"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Diamond"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Hexagon"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Uneven Multi-Hexagon"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Exhaustive"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Transformed Exhaustive"];
    toolTip =
        @"Controls the motion estimation method. Motion estimation is how the encoder decides how each block of pixels in a frame has moved, compared to most similar blocks in the other frames it references. There are many ways of finding the most similar blocks, with varying speeds and accuracy.\n\nAt the most basic setting, dia, x264 will only consider a diamond-shaped region around each pixel.\n\nThe default setting, hex, is similar to dia but uses a hexagon shape.\n\nUneven multi-hexagon, umh, searches a number of different patterns across a wider area and thus is slower than hex and dia but further increases compression efficiency and quality.\n\nesa, an exhaustive search of a square around each pixel (whose size is controlled by the me-range parameter), is much slower and offers only minimal quality gains.\n\ntesa, transformed exhaustive search, which performs just as thorough a search, is slower still but offers further slight improvements to quality.";
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
        @"This range is the radius, in pixels, x264 should use for motion estimation searches. It only has an effect when you use Uneven Multi-Hexagonal, Exhaustive, or Transformed Exhaustive searching. 24, 32, and 64 are good values, with each being progressively smaller for progressively less improvement to picture quality.";
    [fX264optMERangePopUp setToolTip: toolTip];
    [fX264optMERangeLabel setToolTip: toolTip];
    
    /*Weighted B-Frame Prediction fX264optWeightBSwitch BOOLEAN*/
    [fX264optWeightBSwitch setState:1];
    [fX264optWeightBSwitch setWantsLayer:YES];
    toolTip =
        @"Sometimes x264 will base a B-frame's motion compensation on frames both before and after. With weighted B-frames, the amount of influence each frame has is related to its distance from the frame being encoded, instead of both having equal influence. The AppleTV can have issues with this.";
    [fX264optWeightBSwitch setToolTip: toolTip];
    [fX264optWeightBLabel setToolTip: toolTip];
    
    /*B-frame Pyramids fX264optBPyramidSwitch BOOLEAN*/
    [fX264optBPyramidSwitch setState:0];
    [fX264optBPyramidSwitch setWantsLayer:YES];
    toolTip =
        @"B-frame pyramids are a High Profile feature. Pyramidal B-frames mean that B-frames don't just reference surrounding reference frames — instead, it also treats a previous B-frame as a reference, improving quality/lowering bitrate at the expense of complexity. Logically, to reference an earlier B-frame, you must tell x264 to use at least 2 B-frames.";
    [fX264optBPyramidSwitch setToolTip: toolTip];
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
        @"Direct prediction tells x264 what method to use when guessing motion for certain parts of a B-frame. It can either look at other parts of the current frame (spatial) or compare against the following P-frameframe (temporal). You're best off setting this to automatic, so x264 decides which method is best on its own. Don't select none assuming it will be faster; instead it will take longer and look worse. If you're going to choose between spatial and temporal, spatial is usually better.";
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
        @"When adaptive B-Frames are disabled, the number of B-Frames you specify is the constant length of every B-Frame sequence. When one of the adaptive modes is enabled, the number of B-Frames is treated as a maximum, with the length of each sequence varying, but never exceeding the max.\n\nFast mode takes the same amount of time no matter how many B-frames you specify. However, it doesn't always make the best decisions on how many B-Frames to use in a sequence.\n\nOptimal mode gets slower as the maximum number of B-Frames increases, but does a much better job at deciding sequence length, which can mean smaller file sizes and better quality.";
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
        @"x264 includes an in-loop deblocking filter. What this means is that blocky compression artifacts are smoothed away when you play back the video. It has two settings: strength and threshold, just like a simple filter in Photoshop.\n\nStrength controls the amount of deblocking applied to the whole frame. If you drop down below 0, you reduce the amount of blurring. Go too negative, and you'll get an effect somewhat like oversharpening an image. Go into positive values, and the image may become too soft.\n\nThreshold controls how sensitive the filter is to whether something in a block is detail that needs to be preserved: lower numbers blur details less.\n\nThe default deblocking values are 0 and 0. This does not mean zero deblocking. It means x264 will apply the regular deblocking strength and thresholds the codec authors have selected as working the best in most cases.\n\nWhile many, many people stick with the default deblocking values of 0,0, and you should never change the deblocking without disabling adaptive quantization, other people disagree. Some prefer a slightly less blurred image for live action material, and use values like -2,-1 or -2,-2. Others will raise it to 1,1 or even 3,3 for animation. While the values for each setting extend from -6 to 6, the consensus is that going below -3 or above 3 is worthless.";
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
    [fX264optAnalysePopUp addItemWithTitle:@"Default (some)"]; /* 0=default */
    [fX264optAnalysePopUp addItemWithTitle:[NSString stringWithFormat:@"None"]]; /* 1=none */
    [fX264optAnalysePopUp addItemWithTitle:[NSString stringWithFormat:@"All"]]; /* 2=all */
    toolTip =
        @"Analysis controls how finely x264 divides up a frame to capture detail. Full macroblocks are 16x16 pixels, but x264 can go down all the way to 4x4 blocks if it judges it necessary. By default it only breaks up key frames that much. To give x264 the freedom to make the best decisions for all frames, use \"all\" analysis. If you want to create a high profile H.264 video (which is less compatible with the world at large than main profile), also check the \"8x8 DCT blocks\" box to add yet another block size for analysis.";
    [fX264optAnalysePopUp setToolTip: toolTip];
    [fX264optAnalyseLabel setToolTip: toolTip];

    /* 8x8 DCT fX264op8x8dctSwitch */
    [fX264opt8x8dctSwitch setState:1];
    [fX264opt8x8dctSwitch setWantsLayer:YES];
    toolTip =
        @"Checking this box lets x264 break key frames down into 8x8 blocks of pixels for analysis. This is a high profile feature of H.264, which makes it less compatible. It should slightly decrease bitrate or improve quality. Turn it on whenever possible.";
    [fX264opt8x8dctSwitch setToolTip: toolTip];
    [fX264opt8x8dctLabel setToolTip: toolTip];

    /* CABAC fX264opCabacSwitch */
    [fX264optCabacSwitch setState:1];
    toolTip =
        @"CABAC, or context adaptive binary arithmetic coding, is used by x264 to reduce the bitrate needed for a given quality by 15\%. This makes it very cool and very useful, and it should be left on whenever possible. However, it is incompatible with the iPod, and makes the AppleTV struggle. So turn it off for those.\n\nCABAC is a kind of entropy coding, which means that it compresses data by making shorthand symbols to represent long streams of data. The \"entropy\" part means that the symbols it uses the most often are the smallest. When you disable CABAC, another entropy coding scheme gets enabled, called CAVLC (context adaptive variable-length coding). CAVLC is a lot less efficient, which is why it needs 15\% more bitrate to achieve the same quality as CABAC.";
    [fX264optCabacSwitch setToolTip: toolTip];
    [fX264optCabacLabel setToolTip: toolTip];
    
    /* PsyRDO fX264optPsyRDSlider */
    [fX264optPsyRDSlider setMinValue:0.0];
    [fX264optPsyRDSlider setMaxValue:1.0];
    [fX264optPsyRDSlider setTickMarkPosition:NSTickMarkBelow];
    [fX264optPsyRDSlider setNumberOfTickMarks:10];
    [fX264optPsyRDSlider setAllowsTickMarkValuesOnly:YES];
    [fX264optPsyRDSlider setFloatValue:1.0];
    toolTip =
        @"Psychovisual Rate Distortion Optimization sure is a mouthful, isn't it? Basically, it means x264 tries to retain detail, for better quality to the human eye, as opposed to trying to maximize quality the way a computer understands it, through signal-to-noise ratios that have trouble telling apart fine detail and noise.";
    [fX264optPsyRDSlider setToolTip: toolTip];
    [fX264optPsyRDLabel setToolTip: toolTip];

    /* PsyTrellis fX264optPsyRDSlider */
    [fX264optPsyTrellisSlider setMinValue:0.0];
    [fX264optPsyTrellisSlider setMaxValue:1.0];
    [fX264optPsyTrellisSlider setTickMarkPosition:NSTickMarkBelow];
    [fX264optPsyTrellisSlider setNumberOfTickMarks:10];
    [fX264optPsyTrellisSlider setAllowsTickMarkValuesOnly:YES];
    [fX264optPsyTrellisSlider setFloatValue:0.0];
    toolTip =
        @"Psychovisual Trellis tries to retain more sharpness and detail, but can cause artifacting. It is considered experimental, which is why it's off by default. Good values are 0.1 to 0.2.";
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
        int loopcounter;
        int currentOptsArrayCount = [currentOptsArray count];
        for (loopcounter = 0; loopcounter < currentOptsArrayCount; loopcounter++)
        {
            thisOpt = [currentOptsArray objectAtIndex:loopcounter];
            
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
    
    /*No Fast PSkip nofast_pskip*/
    if ([cleanOptNameString isEqualToString:@"no-fast-pskip"] || [cleanOptNameString isEqualToString:@"no_fast_pskip"] || [cleanOptNameString isEqualToString:@"nofast_pskip"])
    {
        cleanOptNameString = @"no-fast-pskip";
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
    
    /*WeightB*/
    if ([cleanOptNameString isEqualToString:@"weight-b"] || [cleanOptNameString isEqualToString:@"weight_b"])
    {
        cleanOptNameString = @"weightb";
    }
    
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
       - CABAC (when 0 turn off trellis)
       - analysis (if none, turn off 8x8dct)
       - refs (under 2, disable mixed-refs)
       - subme (if under 6, turn off psy-rd and psy-trel)
       - trellis (if 0, turn off psy-trel)
    */
    
    if( sender == fX264optBframesPopUp || sender == nil || sender == fDisplayX264Options )
    {
        if ( [fX264optBframesPopUp indexOfSelectedItem ] > 0 &&
             [fX264optBframesPopUp indexOfSelectedItem ] < 2)
        {
            /* If the b-frame widget is at 0 or 1, the user has chosen
               not to use b-frames at all. So disable the options
               that can only be used when b-frames are enabled.        */
            
            if( [fX264optWeightBSwitch isHidden] == false)
            {
                [[fX264optWeightBSwitch animator] setHidden:YES];
                [[fX264optWeightBLabel animator] setHidden:YES];
                if ( [fX264optWeightBSwitch state] == 1 )
                    [fX264optWeightBSwitch performClick:self];
            }

            if( [fX264optBPyramidSwitch isHidden] == false )
            {
                [[fX264optBPyramidSwitch animator] setHidden:YES];
                [[fX264optBPyramidLabel animator] setHidden:YES];
                if ( [fX264optBPyramidSwitch state] == 1 )
                    [fX264optBPyramidSwitch performClick:self];
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
            if( [fX264optBPyramidSwitch isHidden] == false )
            {
                [[fX264optBPyramidSwitch animator] setHidden:YES];
                [[fX264optBPyramidLabel animator] setHidden:YES];
                if ( [fX264optBPyramidSwitch state] == 1 )
                    [fX264optBPyramidSwitch performClick:self];
            }

            if( [fX264optWeightBSwitch isHidden] == true )
            {
                [[fX264optWeightBSwitch animator] setHidden:NO];
                [[fX264optWeightBLabel animator] setHidden:NO];
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
            if( [fX264optBPyramidSwitch isHidden] == true )
            {
                [[fX264optBPyramidSwitch animator] setHidden:NO];
                [[fX264optBPyramidLabel animator] setHidden:NO];
            }

            if( [fX264optWeightBSwitch isHidden] == true )
            {
                [[fX264optWeightBSwitch animator] setHidden:NO];
                [[fX264optWeightBLabel animator] setHidden:NO];
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
    
    if( sender == fX264optCabacSwitch || sender == nil || sender == fDisplayX264Options )
    {
        if ( [fX264optCabacSwitch state] == false)
        {
            if( [fX264optTrellisPopUp isHidden] == false )
            {
                /* Without CABAC entropy coding, trellis doesn't run. */
                [[fX264optTrellisPopUp animator] setHidden:YES];
                [[fX264optTrellisLabel animator] setHidden:YES];
                [fX264optTrellisPopUp selectItemAtIndex:0];
                [[fX264optTrellisPopUp cell] performClick:self];
            }
        }
        else if( [fX264optTrellisPopUp isHidden] == true)
        {
            [[fX264optTrellisPopUp animator] setHidden:NO];
            [[fX264optTrellisLabel animator] setHidden:NO];
        }
    }
    
    if( sender == fX264optAnalysePopUp || sender == nil || sender == fDisplayX264Options )
    {
        if ( [fX264optAnalysePopUp indexOfSelectedItem] == 1)
        {
            /* No analysis? Disable 8x8dct */
            if( [fX264opt8x8dctSwitch isHidden] == false )
            {
                [[fX264opt8x8dctSwitch animator] setHidden:YES];
                [[fX264opt8x8dctLabel animator] setHidden:YES];
                if ( [fX264opt8x8dctSwitch state] == 1 )
                    [fX264opt8x8dctSwitch performClick:self];
            }
        }
        else
        {
            if( [fX264opt8x8dctSwitch isHidden] == true )
            {
                [[fX264opt8x8dctSwitch animator] setHidden:NO];
                [[fX264opt8x8dctLabel animator] setHidden:NO];
            }
        }
    }
    
    if( sender == fX264optRefPopUp || sender == nil || sender == fDisplayX264Options )
    {
        if ( [fX264optRefPopUp indexOfSelectedItem] > 0 &&
             [fX264optRefPopUp indexOfSelectedItem] < 3 )
        {
            if( [fX264optMixedRefsSwitch isHidden] == false )
            {
                /* Only do mixed-refs when there are at least 2 refs to mix. */
                [[fX264optMixedRefsSwitch animator] setHidden:YES];
                [[fX264optMixedRefsLabel animator] setHidden:YES];
                if( [fX264optMixedRefsSwitch state] == 1 )
                    [fX264optMixedRefsSwitch performClick:self];
            }
        }
        else
        {
            if( [fX264optMixedRefsSwitch isHidden] == true )
            {
                [[fX264optMixedRefsSwitch animator] setHidden:NO];
                [[fX264optMixedRefsLabel animator] setHidden:NO];
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
                [[fX264optPsyRDSlider animator] setFloatValue:1];
                if ( [fX264optPsyRDSlider floatValue] < 1.0 )
                {
                    [fX264optPsyRDSlider setFloatValue:1.0];
                    [[fX264optPsyRDSlider cell] performClick:self];            
                }
            }

            if( [fX264optPsyTrellisSlider isHidden] == false)
            {
                [[fX264optPsyTrellisSlider animator] setHidden:YES];
                [[fX264optPsyTrellisLabel animator] setHidden:YES];
                [[fX264optPsyTrellisSlider animator] setFloatValue:0];
                if ( [fX264optPsyTrellisSlider floatValue] > 0.0 )
                {
                    [fX264optPsyTrellisSlider setFloatValue:0.0];
                    [[fX264optPsyTrellisSlider cell] performClick:self];
                }
            }
        }
        else
        {
            if( [fX264optPsyRDSlider isHidden] == true )
            {
                [[fX264optPsyRDSlider animator] setHidden:NO];
                [[fX264optPsyRDLabel animator] setHidden:NO];
            }

            if( ( [fX264optTrellisPopUp indexOfSelectedItem] == 0 || [fX264optTrellisPopUp indexOfSelectedItem] >= 2 ) && [fX264optCabacSwitch state] == true && [fX264optPsyTrellisSlider isHidden] == true )
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
            if( ( [fX264optSubmePopUp indexOfSelectedItem] == 0 || [fX264optSubmePopUp indexOfSelectedItem] >= 7 ) && [fX264optCabacSwitch state] == true  && [fX264optPsyTrellisSlider isHidden] == true )
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
        int loopcounter;
        int currentOptsArrayCount = [currentOptsArray count];
        for (loopcounter = 0; loopcounter < currentOptsArrayCount; loopcounter++)
        {
            thisOpt = [currentOptsArray objectAtIndex:loopcounter];
            
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
                    [fX264optRefPopUp selectItemAtIndex:[optValue intValue]+1];
                }
                /*No Fast PSkip NSButton*/
                if ([optName isEqualToString:@"no-fast-pskip"])
                {
                    [fX264optNfpskipSwitch setState:[optValue intValue]];
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
                /*Mixed Refs NSButton*/
                if ([optName isEqualToString:@"mixed-refs"])
                {
                    [fX264optMixedRefsSwitch setState:[optValue intValue]];
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
                /*Weighted B-Frames NSButton*/
                if ([optName isEqualToString:@"weightb"])
                {
                    [fX264optWeightBSwitch setState:[optValue intValue]];
                }
                /*B Pyramid NSPButton*/
                if ([optName isEqualToString:@"b-pyramid"])
                {
                    [fX264optBPyramidSwitch setState:[optValue intValue]];
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
                        /* Default ("some") */
                        [fX264optAnalysePopUp selectItemAtIndex:0];
                    }
                    if ([optValue isEqualToString:@"none"])
                    {
                        [fX264optAnalysePopUp selectItemAtIndex:1];
                    }
                    if ([optValue isEqualToString:@"all"])
                    {
                        [fX264optAnalysePopUp selectItemAtIndex:2];
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
    if (widget == fX264optNfpskipSwitch)
    {
        optNameToChange = @"no-fast-pskip";
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
    if (widget == fX264optMixedRefsSwitch)
    {
        optNameToChange = @"mixed-refs";
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
    if (widget == fX264optWeightBSwitch)
    {
        optNameToChange = @"weightb";
    }
    if (widget == fX264optBPyramidSwitch)
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
            thisOpt = [NSString stringWithFormat:@"%@=%d,%d",optName, ([fX264optAlphaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optAlphaDeblockPopUp indexOfSelectedItem]-7 : 0,([fX264optBetaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optBetaDeblockPopUp indexOfSelectedItem]-7 : 0];
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
            /* Otherwise the format is deblock=a,b, where a and b both have an array
               offset of 7 because deblocking values start at -6 instead of at zero. */
            thisOpt = [NSString stringWithFormat:@"%@=%0.1f,%0.1f", optName, [fX264optPsyRDSlider floatValue], [fX264optPsyTrellisSlider floatValue] ];
        }
    }
    
    else if /*Boolean Switches*/ ( [optName isEqualToString:@"b-pyramid"] || [optName isEqualToString:@"no-fast-pskip"] || [optName isEqualToString:@"no-dct-decimate"] )
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
    
    else if ( [optName isEqualToString:@"8x8dct"] || [optName isEqualToString:@"weightb"] || [optName isEqualToString:@"mixed-refs"] || [optName isEqualToString:@"cabac"] )
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
                thisOpt = [NSString stringWithFormat:@"%@=%@",optName,@"all"];
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
        thisOpt = [NSString stringWithFormat:@"%@=%d",optName,[sender indexOfSelectedItem]+3];
    }
    
    else if ([optName isEqualToString:@"b-adapt"])
    {
        /* B-adapt starts at index 0 with default then goes 0, 1, 2)*/
        thisOpt = [NSString stringWithFormat:@"%@=%d", optName, [sender indexOfSelectedItem]-1];
    }
    
    else // we have a valid value to change, so change it
    {
        if ( [sender indexOfSelectedItem] != 0 )
        /* Here's our general case, that catches things like ref frames and b-frames.
           Basically, any options that are PopUp menus with index 0 as default and
           index 1 as 1, with numerical values, are all handled right here. All of
           the above stuff is for the exceptions to the general case.              */
            thisOpt = [NSString stringWithFormat:@"%@=%d",optName,[sender indexOfSelectedItem]-1];
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
    NSString * optValue = @""; // The option value such as "3"
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
        int loopcounter;
        int currentOptsArrayCount = [currentOptsArray count];
        for (loopcounter = 0; loopcounter < currentOptsArrayCount; loopcounter++)
        {
            thisOpt = [currentOptsArray objectAtIndex:loopcounter];
            NSRange splitOptRange = [thisOpt rangeOfString:@"="];
            
            if (splitOptRange.location != NSNotFound)
            {
                /* First off, it's time to handle option strings that
                   already have at least one option=value pair in them. */
                   
                optName = [thisOpt substringToIndex:splitOptRange.location];
                optValue = [thisOpt substringFromIndex:splitOptRange.location + 1];

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
}

@end
