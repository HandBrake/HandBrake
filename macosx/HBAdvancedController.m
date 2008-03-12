/* HBAdvancedController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.m0k.org/>.
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
    [self X264AdvancedOptionsSet:NULL];
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
        fX264optWeightBLabel,fX264optWeightBSwitch,fX264optBRDOLabel,fX264optBRDOSwitch,
        fX264optBPyramidLabel,fX264optBPyramidSwitch,fX264optBiMELabel,fX264optBiMESwitch,
        fX264optDirectPredLabel,fX264optDirectPredPopUp,fX264optDeblockLabel,fX264optAnalyseLabel,
        fX264optAnalysePopUp,fX264opt8x8dctLabel,fX264opt8x8dctSwitch,fX264optCabacLabel,fX264optCabacSwitch,
        fX264optAlphaDeblockPopUp,fX264optBetaDeblockPopUp};

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
    
    /*B-Frames fX264optBframesPopUp*/
    int i;
    [fX264optBframesPopUp removeAllItems];
    [fX264optBframesPopUp addItemWithTitle:@"Default (0)"];
    for (i=0; i<17;i++)
    {
        [fX264optBframesPopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    
    /*Reference Frames fX264optRefPopUp*/
    [fX264optRefPopUp removeAllItems];
    [fX264optRefPopUp addItemWithTitle:@"Default (1)"];
    for (i=0; i<17;i++)
    {
        [fX264optRefPopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    
    /*No Fast P-Skip fX264optNfpskipSwitch BOOLEAN*/
    [fX264optNfpskipSwitch setState:0];
    
    /*No Dict Decimate fX264optNodctdcmtSwitch BOOLEAN*/
    [fX264optNodctdcmtSwitch setState:0];    
    
    /*Sub Me fX264optSubmePopUp*/
    [fX264optSubmePopUp removeAllItems];
    [fX264optSubmePopUp addItemWithTitle:@"Default (4)"];
    for (i=0; i<8;i++)
    {
        [fX264optSubmePopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    
    /*Trellis fX264optTrellisPopUp*/
    [fX264optTrellisPopUp removeAllItems];
    [fX264optTrellisPopUp addItemWithTitle:@"Default (0)"];
    for (i=0; i<3;i++)
    {
        [fX264optTrellisPopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    [fX264optTrellisPopUp setWantsLayer:YES];
    
    /*Mixed-references fX264optMixedRefsSwitch BOOLEAN*/
    [fX264optMixedRefsSwitch setState:0];
    [fX264optMixedRefsSwitch setWantsLayer:YES];
    
    /*Motion Estimation fX264optMotionEstPopUp*/
    [fX264optMotionEstPopUp removeAllItems];
    [fX264optMotionEstPopUp addItemWithTitle:@"Default (Hexagon)"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Diamond"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Hexagon"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Uneven Multi-Hexagon"];
    [fX264optMotionEstPopUp addItemWithTitle:@"Exhaustive"];
    
    /*Motion Estimation range fX264optMERangePopUp*/
    [fX264optMERangePopUp removeAllItems];
    [fX264optMERangePopUp addItemWithTitle:@"Default (16)"];
    for (i=4; i<65;i++)
    {
        [fX264optMERangePopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    
    /*Weighted B-Frame Prediction fX264optWeightBSwitch BOOLEAN*/
    [fX264optWeightBSwitch setState:0];
    [fX264optWeightBSwitch setWantsLayer:YES];
    
    /*B-Frame Rate Distortion Optimization fX264optBRDOSwitch BOOLEAN*/
    [fX264optBRDOSwitch setState:0];
    [fX264optBRDOSwitch setWantsLayer:YES];
    
    /*B-frame Pyramids fX264optBPyramidSwitch BOOLEAN*/
    [fX264optBPyramidSwitch setState:0];
    [fX264optBPyramidSwitch setWantsLayer:YES];
    
    /*Bidirectional Motion Estimation Refinement fX264optBiMESwitch BOOLEAN*/
    [fX264optBiMESwitch setState:0];
    [fX264optBiMESwitch setWantsLayer:YES];
    
    /*Direct B-Frame Prediction Mode fX264optDirectPredPopUp*/
    [fX264optDirectPredPopUp removeAllItems];
    [fX264optDirectPredPopUp addItemWithTitle:@"Default (Spatial)"];
    [fX264optDirectPredPopUp addItemWithTitle:@"None"];
    [fX264optDirectPredPopUp addItemWithTitle:@"Spatial"];
    [fX264optDirectPredPopUp addItemWithTitle:@"Temporal"];
    [fX264optDirectPredPopUp addItemWithTitle:@"Automatic"];
    [fX264optDirectPredPopUp setWantsLayer:YES];
    
    /*Alpha Deblock*/
    [fX264optAlphaDeblockPopUp removeAllItems];
    [fX264optAlphaDeblockPopUp addItemWithTitle:@"Default (0)"];
    for (i=-6; i<7;i++)
    {
        [fX264optAlphaDeblockPopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }
    
    /*Beta Deblock*/
    [fX264optBetaDeblockPopUp removeAllItems];
    [fX264optBetaDeblockPopUp addItemWithTitle:@"Default (0)"];
    for (i=-6; i<7;i++)
    {
        [fX264optBetaDeblockPopUp addItemWithTitle:[NSString stringWithFormat:@"%d",i]];
    }     
    
    /* Analysis fX264optAnalysePopUp */
    [fX264optAnalysePopUp removeAllItems];
    [fX264optAnalysePopUp addItemWithTitle:@"Default (some)"]; /* 0=default */
    [fX264optAnalysePopUp addItemWithTitle:[NSString stringWithFormat:@"None"]]; /* 1=none */
    [fX264optAnalysePopUp addItemWithTitle:[NSString stringWithFormat:@"All"]]; /* 2=all */
                
    /* 8x8 DCT fX264op8x8dctSwitch */
    [fX264opt8x8dctSwitch setState:0];
    [fX264opt8x8dctSwitch setWantsLayer:YES];
    
    /* CABAC fX264opCabacSwitch */
    [fX264optCabacSwitch setState:1];
    
    /* Standardize the option string */
    [self X264AdvancedOptionsStandardizeOptString: NULL];

    /* Set Current GUI Settings based on newly standardized string */
    [self X264AdvancedOptionsSetCurrentSettings: NULL];
    
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
    [fDisplayX264Options setStringValue:[NSString stringWithFormat:changedOptString]];
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
    
    /*BRDO*/
    if ([cleanOptNameString isEqualToString:@"b-rdo"] || [cleanOptNameString isEqualToString:@"b_rdo"])
    {
        cleanOptNameString = @"brdo";
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
       - subme  (if under 6 turn off brdo)
       - analysis (if none, turn off 8x8dct and direct pred)
       - refs (under 2, disable mixed-refs)
    */
    
    if ( [fX264optBframesPopUp indexOfSelectedItem ] < 2)
    {
        /* If the b-frame widget is at 0 or 1, the user has chosen
           not to use b-frames at all. So disable the options
           that can only be used when b-frames are enabled.        */
        [[fX264optWeightBSwitch animator] setHidden:YES];
        [[fX264optWeightBLabel animator] setHidden:YES];
        if ( [fX264optWeightBSwitch state] == 1 && sender != fX264optWeightBSwitch && sender != fX264optBRDOSwitch && sender != fX264optBPyramidSwitch && sender != fX264optBiMESwitch && sender != fX264optDirectPredPopUp)
            [fX264optWeightBSwitch performClick:self];
        
        [[fX264optBRDOSwitch animator] setHidden:YES];
        [[fX264optBRDOLabel animator] setHidden:YES];
        if ( [fX264optBRDOSwitch state] == 1 && sender != fX264optWeightBSwitch && sender != fX264optBRDOSwitch && sender != fX264optBPyramidSwitch && sender != fX264optBiMESwitch && sender != fX264optDirectPredPopUp)
            [fX264optBRDOSwitch performClick:self];

        [[fX264optBPyramidSwitch animator] setHidden:YES];
        [[fX264optBPyramidLabel animator] setHidden:YES];
        if ( [fX264optBPyramidSwitch state] == 1 && sender != fX264optWeightBSwitch && sender != fX264optBRDOSwitch && sender != fX264optBPyramidSwitch && sender != fX264optBiMESwitch && sender != fX264optDirectPredPopUp)
            [fX264optBPyramidSwitch performClick:self];

        [[fX264optBiMESwitch animator] setHidden:YES];
        [[fX264optBiMELabel animator] setHidden:YES];
        if ( [fX264optBiMESwitch state] == 1 && sender != fX264optWeightBSwitch && sender != fX264optBRDOSwitch && sender != fX264optBPyramidSwitch && sender != fX264optBiMESwitch && sender != fX264optDirectPredPopUp)
            [fX264optBiMESwitch performClick:self];

        [[fX264optDirectPredPopUp animator] setHidden:YES];
        [[fX264optDirectPredLabel animator] setHidden:YES];
        [fX264optDirectPredPopUp selectItemAtIndex: 0];        
        if ( [fX264optDirectPredPopUp indexOfSelectedItem] > 1 && sender != fX264optWeightBSwitch && sender != fX264optBRDOSwitch && sender != fX264optBPyramidSwitch && sender != fX264optBiMESwitch && sender != fX264optDirectPredPopUp)
            [[fX264optDirectPredPopUp cell] performClick:self];
    }
    else if ( [fX264optBframesPopUp indexOfSelectedItem ] == 2)
    {
        /* Only 1 b-frame? Disable b-pyramid. */
        [[fX264optBPyramidSwitch animator] setHidden:YES];
        [[fX264optBPyramidLabel animator] setHidden:YES];
        if ( [fX264optBPyramidSwitch state] == 1 && sender != fX264optBPyramidSwitch)
            [fX264optBPyramidSwitch performClick:self];

        [[fX264optWeightBSwitch animator] setHidden:NO];
        [[fX264optWeightBLabel animator] setHidden:NO];

        [[fX264optBiMESwitch animator] setHidden:NO];
        [[fX264optBiMELabel animator] setHidden:NO];
                
        if ( [fX264optSubmePopUp indexOfSelectedItem] >= 7)
        {
            /* Only show B-RDO if both bframes and subme allow it. */
            [[fX264optBRDOSwitch animator] setHidden:NO];
            [[fX264optBRDOLabel animator] setHidden:NO];
        }
        
        if ( [fX264optAnalysePopUp indexOfSelectedItem] != 1)
        {
            /* Only show direct pred when allowed by both bframes and analysis.*/
            [[fX264optDirectPredPopUp animator] setHidden:NO];
            [[fX264optDirectPredLabel animator] setHidden:NO];
        }
    }
    else
    {
        [[fX264optWeightBSwitch animator] setHidden:NO];
        [[fX264optWeightBLabel animator] setHidden:NO];

        [[fX264optBPyramidSwitch animator] setHidden:NO];
        [[fX264optBPyramidLabel animator] setHidden:NO];

        [[fX264optBiMESwitch animator] setHidden:NO];
        [[fX264optBiMELabel animator] setHidden:NO];

        if ( [fX264optSubmePopUp indexOfSelectedItem] >= 7)
        {
            /* Only show B-RDO if both bframes and subme allow it. */
            [[fX264optBRDOSwitch animator] setHidden:NO];
            [[fX264optBRDOLabel animator] setHidden:NO];
        }

        if ( [fX264optAnalysePopUp indexOfSelectedItem] != 1)
        {
            /* Only show direct pred when allowed by both bframes and analysis.*/
            [[fX264optDirectPredPopUp animator] setHidden:NO];
            [[fX264optDirectPredLabel animator] setHidden:NO];
        }
    }
    
    if ( [fX264optCabacSwitch state] == false)
    {
        /* Without CABAC entropy coding, trellis doesn't run. */
        
        [[fX264optTrellisPopUp animator] setHidden:YES];
        [[fX264optTrellisLabel animator] setHidden:YES];
        [fX264optTrellisPopUp selectItemAtIndex:0];
        if (sender != fX264optTrellisPopUp)
            [[fX264optTrellisPopUp cell] performClick:self];
    }
    else
    {
        [[fX264optTrellisPopUp animator] setHidden:NO];
        [[fX264optTrellisLabel animator] setHidden:NO];
    }
    
    if ( [fX264optSubmePopUp indexOfSelectedItem] < 7)
    {
        /* When subme < 6, B-RDO doesn't work. */
        [[fX264optBRDOSwitch animator] setHidden:YES];
        [[fX264optBRDOLabel animator] setHidden:YES];
        if ( [fX264optBRDOSwitch state] == 1 && sender != fX264optBRDOSwitch )
            [fX264optBRDOSwitch performClick:self];
    }
    else if ( [fX264optBframesPopUp indexOfSelectedItem ] >= 2 )
    {
        /* Make sure to only display B-RDO if allowed by both
           the subme and bframe option settings.               */
        [[fX264optBRDOSwitch animator] setHidden:NO]; 
        [[fX264optBRDOLabel animator] setHidden:NO]; 
    }
    
    if ( [fX264optAnalysePopUp indexOfSelectedItem] == 1)
    {
        /* No analysis? Disable 8x8dct and direct pred */
        [[fX264opt8x8dctSwitch animator] setHidden:YES];
        [[fX264opt8x8dctLabel animator] setHidden:YES];
        if ( [fX264opt8x8dctSwitch state] == 1 && sender != fX264opt8x8dctSwitch )
            [fX264opt8x8dctSwitch performClick:self];

        [[fX264optDirectPredPopUp animator] setHidden:YES];
        [[fX264optDirectPredLabel animator] setHidden:YES];
        [fX264optDirectPredPopUp selectItemAtIndex: 0];        
        if ( [fX264optDirectPredPopUp indexOfSelectedItem] > 1 && sender != fX264optDirectPredPopUp)
            [[fX264optDirectPredPopUp cell] performClick:self];
    }
    else
    {
        [[fX264opt8x8dctSwitch animator] setHidden:NO];
        [[fX264opt8x8dctLabel animator] setHidden:NO];

        if ( [fX264optBframesPopUp indexOfSelectedItem ] >= 2)
        {
            /* Onlt show direct pred when allowed by both analysis and bframes */
            [[fX264optDirectPredPopUp animator] setHidden:NO];
            [[fX264optDirectPredLabel animator] setHidden:NO];
        }
    }
    
    if ( [fX264optRefPopUp indexOfSelectedItem] < 3)
    {
        /* Only do mixed-refs when there are at least 2 refs to mix. */
        [[fX264optMixedRefsSwitch animator] setHidden:YES];
        [[fX264optMixedRefsLabel animator] setHidden:YES];
        if ( [fX264optMixedRefsSwitch state] == 1 && sender != fX264optMixedRefsSwitch )
            [fX264optMixedRefsSwitch performClick:self];
    }
    else
    {
        [[fX264optMixedRefsSwitch animator] setHidden:NO];
        [[fX264optMixedRefsLabel animator] setHidden:NO];
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
                }
                /*ME Range NSPopUpButton*/
                if ([optName isEqualToString:@"merange"])
                {
                    [fX264optMERangePopUp selectItemAtIndex:[optValue intValue]-3];
                }
                /*Weighted B-Frames NSButton*/
                if ([optName isEqualToString:@"weightb"])
                {
                    [fX264optWeightBSwitch setState:[optValue intValue]];
                }
                /*BRDO NSButton*/
                if ([optName isEqualToString:@"brdo"])
                {
                    [fX264optBRDOSwitch setState:[optValue intValue]];
                }
                /*B Pyramid NSPButton*/
                if ([optName isEqualToString:@"b-pyramid"])
                {
                    [fX264optBPyramidSwitch setState:[optValue intValue]];
                }
                /*Bidirectional Motion Estimation Refinement NSButton*/
                if ([optName isEqualToString:@"bime"])
                {
                    [fX264optBiMESwitch setState:[optValue intValue]];
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
            }
        }
    }
}

/**
 * Resets the option string to mirror the GUI widgets.
 */
- (IBAction) X264AdvancedOptionsChanged: (id) sender
{
    /*Determine which outlet is being used and set optName to process accordingly */
    NSString * optNameToChange = @""; // The option name such as "bframes"
    
    if (sender == fX264optBframesPopUp)
    {
        optNameToChange = @"bframes";
    }
    if (sender == fX264optRefPopUp)
    {
        optNameToChange = @"ref";
    }
    if (sender == fX264optNfpskipSwitch)
    {
        optNameToChange = @"no-fast-pskip";
    }
    if (sender == fX264optNodctdcmtSwitch)
    {
        optNameToChange = @"no-dct-decimate";
    }
    if (sender == fX264optSubmePopUp)
    {
        optNameToChange = @"subq";
    }
    if (sender == fX264optTrellisPopUp)
    {
        optNameToChange = @"trellis";
    }
    if (sender == fX264optMixedRefsSwitch)
    {
        optNameToChange = @"mixed-refs";
    }
    if (sender == fX264optMotionEstPopUp)
    {
        optNameToChange = @"me";
    }
    if (sender == fX264optMERangePopUp)
    {
        optNameToChange = @"merange";
    }
    if (sender == fX264optWeightBSwitch)
    {
        optNameToChange = @"weightb";
    }
    if (sender == fX264optBRDOSwitch)
    {
        optNameToChange = @"brdo";
    }
    if (sender == fX264optBPyramidSwitch)
    {
        optNameToChange = @"b-pyramid";
    }
    if (sender == fX264optBiMESwitch)
    {
        optNameToChange = @"bime";
    }
    if (sender == fX264optDirectPredPopUp)
    {
        optNameToChange = @"direct";
    }
    if (sender == fX264optAlphaDeblockPopUp)
    {
        optNameToChange = @"deblock";
    }
    if (sender == fX264optBetaDeblockPopUp)
    {
        optNameToChange = @"deblock";
    }        
    if (sender == fX264optAnalysePopUp)
    {
        optNameToChange = @"analyse";
    }
    if (sender == fX264opt8x8dctSwitch)
    {
        optNameToChange = @"8x8dct";
    }
    if (sender == fX264optCabacSwitch)
    {
        optNameToChange = @"cabac";
    }
    
    /* Set widgets depending on the opt string in field */
    NSString * thisOpt; // The separated option such as "bframes=3"
    NSString * optName = @""; // The option name such as "bframes"
    NSString * optValue = @"";// The option value such as "3"
    NSArray *currentOptsArray;
    
    /*First, we get an opt string to process */
    NSString *currentOptString = [fDisplayX264Options stringValue];
    
    /* There are going to be a few possibilities.
       - The option might start off the string.
       - The option might be in the middle of the string.
       - The option might not be in the string at all yet.
       - The string itself might not yet exist.
       
       Because each of these possibilities means constructing a different kind of string,
       they're all handled separately in a sea of messy, somewhat redundant code. =(     */
       
    /* If the option is in the string but not the beginning of it, it will be in the form of ":optName=value"
       so we really want to be looking for ":optNameToChange=" rather than "optNameToChange".                 */
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
    {
        /* If the option is in the string wth a semicolon, or starts the string, it's time to edit.
           This means parsing the whole string into an array of options and values. From there,
           iterate through the options, and when you reach the one that's been changed, edit it.   */
        
        /* Create new empty opt string*/
        NSString *changedOptString = @"";
        
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
                /* First off, it's time to handle option strings that
                   already have at least one option=value pair in them. */
                   
                optName = [thisOpt substringToIndex:splitOptRange.location];
                optValue = [thisOpt substringFromIndex:splitOptRange.location + 1];
                
                /*Run through the available widgets for x264 opts and set them, as you add widgets, 
                    they need to be added here. This should be moved to its own method probably*/
                
                /*If the optNameToChange is found, appropriately change the value or delete it if
                    "Unspecified" is set.*/
                if ([optName isEqualToString:optNameToChange])
                {
                    if ([optNameToChange isEqualToString:@"deblock"])
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
                    else if /*Boolean Switches*/ ([optNameToChange isEqualToString:@"mixed-refs"] || [optNameToChange isEqualToString:@"weightb"] || [optNameToChange isEqualToString:@"brdo"] || [optNameToChange isEqualToString:@"bime"] || [optNameToChange isEqualToString:@"b-pyramid"] || [optNameToChange isEqualToString:@"no-fast-pskip"] || [optNameToChange isEqualToString:@"no-dct-decimate"] || [optNameToChange isEqualToString:@"8x8dct"] )
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
                    else if ([optNameToChange isEqualToString:@"cabac"])
                    {
                        /* CABAC is odd, in that it defaults to being on. That means
                           it only needs to be included in the string when turned off. */
                        if ([sender state] == 1)
                        {
                            /* It's true so don't include it. */
                            thisOpt = @"";
                        }
                        else
                        {
                            /* Otherwise, include cabac=0 in the string to enable CAVLC. */
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
                    else if ([optNameToChange isEqualToString:@"me"])
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
                                
                            default:
                                break;
                        }
                    }
                    else if ([optNameToChange isEqualToString:@"direct"])
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
                    else if ([optNameToChange isEqualToString:@"analyse"])
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
                    else if ([optNameToChange isEqualToString:@"merange"])
                    {
                        /* Motion estimation range uses an odd array offset because in addition
                           to starting with index 0 as default, index 1 starts at 4 instead of 1,
                           because merange can't go below 4. So it has to be handled separately.  */
                        thisOpt = [NSString stringWithFormat:@"%@=%d",optName,[sender indexOfSelectedItem]+3];
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
        [fDisplayX264Options setStringValue:[NSString stringWithFormat:changedOptString]];    
    }
    else // if none exists, add it to the string
    {
        /* This is where options that aren't already in the string are handled. */
        if ([[fDisplayX264Options stringValue] isEqualToString: @""])
        {
            /* The option might not be in the string because the
               string is empty. Handle this possibility first.   */
            if ([optNameToChange isEqualToString:@"me"])
            {
                /* Special case for motion estimation, which uses string values
                   that need to be paired up with the equivalent widget index.  */
                switch ([sender indexOfSelectedItem])
                {   
                    case 1:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"dia"]]];
                        break;
                        
                    case 2:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"hex"]]];
                        break;
                        
                    case 3:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"umh"]]];
                        break;
                        
                    case 4:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"esa"]]];
                        break;
                        
                    default:
                        break;
                }
            }
            else if ([optNameToChange isEqualToString:@"direct"])
            {
                /* Special case for direct prediction, which uses string values
                   that need to be paired up with the equivalent widget index.  */
                switch ([sender indexOfSelectedItem])
                {   
                    case 1:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"none"]]];
                        break;
                        
                    case 2:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"spatial"]]];
                        break;
                        
                    case 3:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"temporal"]]];
                        break;
                        
                    case 4:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"auto"]]];
                        break;
                        
                    default:
                        break;
                }
            }
            else if ([optNameToChange isEqualToString:@"analyse"])
            {
                /* Special case for partition analysis, which uses string values
                   that need to be paired up with the equivalent widget index.  */
                switch ([sender indexOfSelectedItem])
                {   
                    case 1:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"none"]]];
                        break;
                        
                    case 2:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"all"]]];
                        break;
                        
                    default:
                        break;
                }
            }
            
            else if ([optNameToChange isEqualToString:@"merange"])
            {
                /* Special case for motion estimation range, which uses
                   a widget index offset of 3. This is because the
                   first valid value after default is four, not zero.   */
                [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                    [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender indexOfSelectedItem]+3]]];
            }
            else if ([optNameToChange isEqualToString:@"deblock"])
            {
                /* Very special case for deblock. Uses a weird widget index offset
                   of 7, because the first value after default is -6, rather than 0.
                   As well, deblock only goes to default when *both* alpha and beta
                   are zero. If only one is zero, you can't mark it down as default.
                   Instead, mark that one down as literally 0. This is because when
                   widgets are at default values, they aren't included in the string.
                   If only one filter is at 0, both need to be overtly specified.    */
                [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d,%d", ([fX264optAlphaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optAlphaDeblockPopUp indexOfSelectedItem]-7 : 0, ([fX264optBetaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optBetaDeblockPopUp indexOfSelectedItem]-7 : 0]]];                
            }
            else if /*Boolean Switches*/ ([optNameToChange isEqualToString:@"mixed-refs"] || [optNameToChange isEqualToString:@"weightb"] || [optNameToChange isEqualToString:@"brdo"] || [optNameToChange isEqualToString:@"bime"] || [optNameToChange isEqualToString:@"b-pyramid"] || [optNameToChange isEqualToString:@"no-fast-pskip"] || [optNameToChange isEqualToString:@"no-dct-decimate"] || [optNameToChange isEqualToString:@"8x8dct"] )
            {
                /* This covers all the boolean options that need to be specified only when true. */
                if ([sender state] == 0)
                {
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@""]];                    
                }
                else
                {
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                        [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender state]]]];
                }
            }
            else if ([optNameToChange isEqualToString:@"cabac"])
            {
                /* CABAC is weird in that you need the inverse. Only include in the string
                   when cabac=0, because cabac=1 is the default. Turning it off means CAVLC. */
                if ([sender state] == 1)
                {
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@""]];                                        
                }
                else
                {
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                        [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender state]]]];                    
                }
            }            
            else
            {
                if ( [sender indexOfSelectedItem] != 0 )
                /* General case to cover all the normal PopUp widgets, like ref and b-frames. */
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                    [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender indexOfSelectedItem]-1]]];
            }
        }
        else
        {
            /* The string isn't empty, and the option isn't already in it,
               so it will need to be appended to the string with a colon.  */
            if ([optNameToChange isEqualToString:@"me"])
            {
                /* Special case for motion estimation, which uses string values
                   that need to be paired up with the equivalent widget index.  */
                switch ([sender indexOfSelectedItem])
                {   
                    case 1:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", 
                            [NSString stringWithFormat:[fDisplayX264Options stringValue]],
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"dia"]]];
                        break;
                        
                    case 2:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", 
                            [NSString stringWithFormat:[fDisplayX264Options stringValue]],
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"hex"]]];
                        break;
                        
                    case 3:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", 
                            [NSString stringWithFormat:[fDisplayX264Options stringValue]],
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"umh"]]];
                        break;
                        
                    case 4:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", 
                            [NSString stringWithFormat:[fDisplayX264Options stringValue]],
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"esa"]]];
                        break;
                        
                    default:
                        break;
                }
            }
            else if ([optNameToChange isEqualToString:@"direct"])
            {
                /* Special case for direct prediction, which uses string values
                   that need to be paired up with the equivalent widget index.  */
                switch ([sender indexOfSelectedItem])
                {   
                    case 1:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", 
                            [NSString stringWithFormat:[fDisplayX264Options stringValue]],
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"none"]]];
                        break;
                        
                    case 2:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", 
                            [NSString stringWithFormat:[fDisplayX264Options stringValue]],
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"spatial"]]];
                        break;
                        
                    case 3:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", 
                            [NSString stringWithFormat:[fDisplayX264Options stringValue]],
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"temporal"]]];
                        break;
                        
                    case 4:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", 
                            [NSString stringWithFormat:[fDisplayX264Options stringValue]],
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"auto"]]];
                        break;
                        
                    default:
                        break;
                }
            }
            else if ([optNameToChange isEqualToString:@"analyse"])
            {
                /* Special case for partition analysis, which uses string values
                   that need to be paired up with the equivalent widget index.  */
                switch ([sender indexOfSelectedItem])
                {   
                    case 1:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", 
                            [NSString stringWithFormat:[fDisplayX264Options stringValue]],
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"none"]]];
                        break;
                        
                    case 2:
                        [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", 
                            [NSString stringWithFormat:[fDisplayX264Options stringValue]],
                            [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"all"]]];
                        break;
                        
                    default:
                        break;
                }
            }
            
            else if ([optNameToChange isEqualToString:@"merange"])
            {
                /* Motion estimation range uses a weird offset since its index goes
                   0: default, 1: 4, because the first valid value is 4, not 1.     */
                [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@",[NSString stringWithFormat:[fDisplayX264Options stringValue]], 
                    [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender indexOfSelectedItem]+3]]];
            }
            else if ([optNameToChange isEqualToString:@"deblock"])
            {
                /* Deblock is really weird because it has two values, and if only one is default, both
                   still need to be specified directly. with the default one at zero. To make deblock
                   just a little more fun, values start at -6 instead of at zero.                       */
                [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", [NSString stringWithFormat:[fDisplayX264Options stringValue]], [NSString stringWithFormat:optNameToChange], [NSString stringWithFormat:@"%d,%d", ([fX264optAlphaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optAlphaDeblockPopUp indexOfSelectedItem]-7 : 0, ([fX264optBetaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optBetaDeblockPopUp indexOfSelectedItem]-7 : 0]]];                
            }
            else if /*Boolean Switches*/ ([optNameToChange isEqualToString:@"mixed-refs"] || [optNameToChange isEqualToString:@"weightb"] || [optNameToChange isEqualToString:@"brdo"] || [optNameToChange isEqualToString:@"bime"] || [optNameToChange isEqualToString:@"b-pyramid"] || [optNameToChange isEqualToString:@"no-fast-pskip"] || [optNameToChange isEqualToString:@"no-dct-decimate"] || [optNameToChange isEqualToString:@"8x8dct"] )
            {
                /* Covers all the normal booleans, that only need to be included in the string when they're true. */
                if ([sender state] == 0)
                {
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@",[NSString stringWithFormat:[fDisplayX264Options stringValue]]]];                    
                }
                else
                {
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@",[NSString stringWithFormat:[fDisplayX264Options stringValue]], 
                        [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender state]]]];                
                }
            }
            else if ([optNameToChange isEqualToString:@"cabac"])
            {
                /* CABAC is weird, in that it's an inverse. Only include it in the string when it's false. */
                if ([sender state] == 1)
                {
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@",[NSString stringWithFormat:[fDisplayX264Options stringValue]]]];                    
                }
                else
                {
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@",[NSString stringWithFormat:[fDisplayX264Options stringValue]], 
                        [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender state]]]];
                }
            }
            else
            {
                /* General case to handle the normal PopUp widgets like ref and b-frames. */
                if ( [sender indexOfSelectedItem] != 0 )
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@",[NSString stringWithFormat:[fDisplayX264Options stringValue]], 
                    [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender indexOfSelectedItem]-1]]];
            }
        }
    }
    
    /* We now need to reset the opt widgets since we changed some stuff */        
    [self X264AdvancedOptionsSet:sender];        
}

@end
