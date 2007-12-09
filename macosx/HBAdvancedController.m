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
}

- (void)dealloc
{
    [super dealloc];
}

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
    
    /*Mixed-references fX264optMixedRefsSwitch BOOLEAN*/
    [fX264optMixedRefsSwitch setState:0];
    
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
    
    /*B-Frame Rate Distortion Optimization fX264optBRDOSwitch BOOLEAN*/
    [fX264optBRDOSwitch setState:0];
    
    /*B-frame Pyramids fX264optBPyramidSwitch BOOLEAN*/
    [fX264optBPyramidSwitch setState:0];
    
    /*Bidirectional Motion Estimation Refinement fX264optBiMESwitch BOOLEAN*/
    [fX264optBiMESwitch setState:0];
    
    /*Direct B-Frame Prediction Mode fX264optDirectPredPopUp*/
    [fX264optDirectPredPopUp removeAllItems];
    [fX264optDirectPredPopUp addItemWithTitle:@"Default (Spatial)"];
    [fX264optDirectPredPopUp addItemWithTitle:@"None"];
    [fX264optDirectPredPopUp addItemWithTitle:@"Spatial"];
    [fX264optDirectPredPopUp addItemWithTitle:@"Temporal"];
    [fX264optDirectPredPopUp addItemWithTitle:@"Automatic"];
    
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
    
    /* CABAC fX264opCabacSwitch */
    [fX264optCabacSwitch setState:1];
    
    /* Standardize the option string */
    [self X264AdvancedOptionsStandardizeOptString: NULL];
    /* Set Current GUI Settings based on newly standardized string */
    [self X264AdvancedOptionsSetCurrentSettings: NULL];
}

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
    
    /*verify there is an opt string to process */
    NSRange currentOptRange = [currentOptString rangeOfString:@"="];
    if (currentOptRange.location != NSNotFound)
    {
        /*Put individual options into an array based on the ":" separator for processing, result is "<opt>=<value>"*/
        currentOptsArray = [currentOptString componentsSeparatedByString:@":"];
        
        /*iterate through the array and get <opts> and <values*/
        //NSEnumerator * enumerator = [currentOptsArray objectEnumerator];
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
            
            /* Construct New String for opts here */
            if ([thisOpt isEqualToString:@""])
            {
                changedOptString = [NSString stringWithFormat:@"%@%@",changedOptString,thisOpt];
            }
            else
            {
                if ([changedOptString isEqualToString:@""])
                {
                    changedOptString = [NSString stringWithFormat:@"%@",thisOpt];
                }
                else
                {
                    changedOptString = [NSString stringWithFormat:@"%@:%@",changedOptString,thisOpt];
                }
            }
        }
    }
    
    /* Change the option string to reflect the new standardized option string */
    [fDisplayX264Options setStringValue:[NSString stringWithFormat:changedOptString]];
}

- (NSString *) X264AdvancedOptionsStandardizeOptNames:(NSString *) cleanOptNameString
{
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

- (IBAction) X264AdvancedOptionsSetCurrentSettings: (id) sender
{
    /* Set widgets depending on the opt string in field */
    NSString * thisOpt; // The separated option such as "bframes=3"
    NSString * optName = @""; // The option name such as "bframes"
    NSString * optValue = @"";// The option value such as "3"
    NSArray *currentOptsArray;
    
    /*First, we get an opt string to process */
    //NSString *currentOptString = @"bframes=3:ref=1:subme=5:me=umh:no-fast-pskip=1:no-dct-decimate=1:trellis=2";
    NSString *currentOptString = [fDisplayX264Options stringValue];
    
    /*verify there is an opt string to process */
    NSRange currentOptRange = [currentOptString rangeOfString:@"="];
    if (currentOptRange.location != NSNotFound)
    {
        /* lets clean the opt string here to standardize any names*/
        /*Put individual options into an array based on the ":" separator for processing, result is "<opt>=<value>"*/
        currentOptsArray = [currentOptString componentsSeparatedByString:@":"];
        
        /*iterate through the array and get <opts> and <values*/
        //NSEnumerator * enumerator = [currentOptsArray objectEnumerator];
        int loopcounter;
        int currentOptsArrayCount = [currentOptsArray count];
        
        /*iterate through the array and get <opts> and <values*/
        for (loopcounter = 0; loopcounter < currentOptsArrayCount; loopcounter++)
        {
            thisOpt = [currentOptsArray objectAtIndex:loopcounter];
            NSRange splitOptRange = [thisOpt rangeOfString:@"="];
            
            if (splitOptRange.location != NSNotFound)
            {
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
                /*No Fast PSkip NSPopUpButton*/
                if ([optName isEqualToString:@"no-fast-pskip"])
                {
                    [fX264optNfpskipSwitch setState:[optValue intValue]];
                }
                /*No Dict Decimate NSPopUpButton*/
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
                /*Weighted B-Frames NSPopUpButton*/
                if ([optName isEqualToString:@"weightb"])
                {
                    [fX264optWeightBSwitch setState:[optValue intValue]];
                }
                /*BRDO NSPopUpButton*/
                if ([optName isEqualToString:@"brdo"])
                {
                    [fX264optBRDOSwitch setState:[optValue intValue]];
                }
                /*B Pyramid NSPopUpButton*/
                if ([optName isEqualToString:@"b-pyramid"])
                {
                    [fX264optBPyramidSwitch setState:[optValue intValue]];
                }
                /*Bidirectional Motion Estimation Refinement NSPopUpButton*/
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
                        [fX264optAlphaDeblockPopUp selectItemAtIndex:0];                        
                        [fX264optBetaDeblockPopUp selectItemAtIndex:0];                               
                    }
                    else
                    {
                        if (![alphaDeblock isEqualToString:@"0"])
                        {
                            [fX264optAlphaDeblockPopUp selectItemAtIndex:[alphaDeblock intValue]+7];
                        }
                        else
                        {
                            [fX264optAlphaDeblockPopUp selectItemAtIndex:7];                        
                        }
                        
                        if (![betaDeblock isEqualToString:@"0"])
                        {
                            [fX264optBetaDeblockPopUp selectItemAtIndex:[betaDeblock intValue]+7];
                        }
                        else
                        {
                            [fX264optBetaDeblockPopUp selectItemAtIndex:7];                        
                        }
                    }
                }
                /* Analysis NSPopUpButton */
                if ([optName isEqualToString:@"analyse"])
                {
                    if ([optValue isEqualToString:@"p8x8,b8x8,i8x8,i4x4"])
                    {
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
    //EXAMPLE: NSString *currentOptString = @"bframes=3:ref=1:subme=5:me=umh:no-fast-pskip=1:no-dct-decimate=1:trellis=2";
    NSString *currentOptString = [fDisplayX264Options stringValue];
    
    /*verify there is an occurrence of the opt specified by the sender to change */
    /*take care of any multi-value opt names here. This is extremely kludgy, but test for functionality
        and worry about pretty later */
    
    /*First, we create a pattern to check for ":"optNameToChange"=" to modify the option if the name falls after
        the first character of the opt string (hence the ":") */
    NSString *checkOptNameToChange = [NSString stringWithFormat:@":%@=",optNameToChange];
    NSRange currentOptRange = [currentOptString rangeOfString:checkOptNameToChange];
    /*Then we create a pattern to check for "<beginning of line>"optNameToChange"=" to modify the option to
        see if the name falls at the beginning of the line, where we would not have the ":" as a pattern to test against*/
    NSString *checkOptNameToChangeBeginning = [NSString stringWithFormat:@"%@=",optNameToChange];
    NSRange currentOptRangeBeginning = [currentOptString rangeOfString:checkOptNameToChangeBeginning];
    if (currentOptRange.location != NSNotFound || currentOptRangeBeginning.location == 0)
    {
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
                            thisOpt = @"";                                
                        }
                        else
                        {
                            thisOpt = [NSString stringWithFormat:@"%@=%d,%d",optName, ([fX264optAlphaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optAlphaDeblockPopUp indexOfSelectedItem]-7 : 0,([fX264optBetaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optBetaDeblockPopUp indexOfSelectedItem]-7 : 0];
                        }
                    }
                    else if /*Boolean Switches*/ ([optNameToChange isEqualToString:@"mixed-refs"] || [optNameToChange isEqualToString:@"weightb"] || [optNameToChange isEqualToString:@"brdo"] || [optNameToChange isEqualToString:@"bime"] || [optNameToChange isEqualToString:@"b-pyramid"] || [optNameToChange isEqualToString:@"no-fast-pskip"] || [optNameToChange isEqualToString:@"no-dct-decimate"] || [optNameToChange isEqualToString:@"8x8dct"] )
                    {
                        if ([sender state] == 0)
                        {
                            thisOpt = @"";
                        }
                        else
                        {
                            thisOpt = [NSString stringWithFormat:@"%@=%d",optName,1];
                        }
                    }
                    else if ([optNameToChange isEqualToString:@"cabac"])
                    {
                        if ([sender state] == 1)
                        {
                            thisOpt = @"";
                        }
                        else
                        {
                            thisOpt = [NSString stringWithFormat:@"%@=%d",optName,0];
                        }
                    }                                        
                    else if (([sender indexOfSelectedItem] == 0) && (sender != fX264optAlphaDeblockPopUp) && (sender != fX264optBetaDeblockPopUp) ) // means that "unspecified" is chosen, lets then remove it from the string
                    {
                        thisOpt = @"";
                    }
                    else if ([optNameToChange isEqualToString:@"me"])
                    {
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
                        thisOpt = [NSString stringWithFormat:@"%@=%d",optName,[sender indexOfSelectedItem]+3];
                    }
                    else // we have a valid value to change, so change it
                    {
                        if ( [sender indexOfSelectedItem] != 0 )
                            thisOpt = [NSString stringWithFormat:@"%@=%d",optName,[sender indexOfSelectedItem]-1];
                    }
                }
            }
            
            /* Construct New String for opts here */
            if ([thisOpt isEqualToString:@""])
            {
                changedOptString = [NSString stringWithFormat:@"%@%@",changedOptString,thisOpt];
            }
            else
            {
                if ([changedOptString isEqualToString:@""])
                {
                    changedOptString = [NSString stringWithFormat:@"%@",thisOpt];
                }
                else
                {
                    changedOptString = [NSString stringWithFormat:@"%@:%@",changedOptString,thisOpt];
                }
            }
        }
        
        /* Change the option string to reflect the new mod settings */
        [fDisplayX264Options setStringValue:[NSString stringWithFormat:changedOptString]];    
    }
    else // if none exists, add it to the string
    {
        if ([[fDisplayX264Options stringValue] isEqualToString: @""])
        {
            if ([optNameToChange isEqualToString:@"me"])
            {
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
                [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                    [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender indexOfSelectedItem]+3]]];
            }
            else if ([optNameToChange isEqualToString:@"deblock"])
            {
                [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d,%d", ([fX264optAlphaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optAlphaDeblockPopUp indexOfSelectedItem]-7 : 0, ([fX264optBetaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optBetaDeblockPopUp indexOfSelectedItem]-7 : 0]]];                
            }
            else if /*Boolean Switches*/ ([optNameToChange isEqualToString:@"mixed-refs"] || [optNameToChange isEqualToString:@"weightb"] || [optNameToChange isEqualToString:@"brdo"] || [optNameToChange isEqualToString:@"bime"] || [optNameToChange isEqualToString:@"b-pyramid"] || [optNameToChange isEqualToString:@"no-fast-pskip"] || [optNameToChange isEqualToString:@"no-dct-decimate"] || [optNameToChange isEqualToString:@"8x8dct"] )            {
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
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@=%@", 
                    [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender indexOfSelectedItem]-1]]];
            }
        }
        else
        {
            if ([optNameToChange isEqualToString:@"me"])
            {
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
                [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@",[NSString stringWithFormat:[fDisplayX264Options stringValue]], 
                    [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender indexOfSelectedItem]+3]]];
            }
            else if ([optNameToChange isEqualToString:@"deblock"])
            {
                [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@", [NSString stringWithFormat:[fDisplayX264Options stringValue]], [NSString stringWithFormat:optNameToChange], [NSString stringWithFormat:@"%d,%d", ([fX264optAlphaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optAlphaDeblockPopUp indexOfSelectedItem]-7 : 0, ([fX264optBetaDeblockPopUp indexOfSelectedItem] != 0) ? [fX264optBetaDeblockPopUp indexOfSelectedItem]-7 : 0]]];                
            }
            else if /*Boolean Switches*/ ([optNameToChange isEqualToString:@"mixed-refs"] || [optNameToChange isEqualToString:@"weightb"] || [optNameToChange isEqualToString:@"brdo"] || [optNameToChange isEqualToString:@"bime"] || [optNameToChange isEqualToString:@"b-pyramid"] || [optNameToChange isEqualToString:@"no-fast-pskip"] || [optNameToChange isEqualToString:@"no-dct-decimate"] || [optNameToChange isEqualToString:@"8x8dct"] )
            {
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
                if ( [sender indexOfSelectedItem] != 0 )
                    [fDisplayX264Options setStringValue:[NSString stringWithFormat:@"%@:%@=%@",[NSString stringWithFormat:[fDisplayX264Options stringValue]], 
                    [NSString stringWithFormat:optNameToChange],[NSString stringWithFormat:@"%d",[sender indexOfSelectedItem]-1]]];
            }
        }
    }
    
    /* We now need to reset the opt widgets since we changed some stuff */        
    [self X264AdvancedOptionsSet:NULL];        
}

@end
