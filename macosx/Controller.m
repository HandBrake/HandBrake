/* $Id: Controller.mm,v 1.79 2005/11/04 19:41:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "Controller.h"
#import "HBOutputPanelController.h"
#import "HBPreferencesController.h"
#import "HBDVDDetector.h"
#import "HBPresets.h"
#import "HBPreviewController.h"

#define DragDropSimplePboardType 	@"MyCustomOutlineViewPboardType"

/* We setup the toolbar values here ShowPreviewIdentifier */
static NSString *        ToggleDrawerIdentifier             = @"Toggle Drawer Item Identifier";
static NSString *        StartEncodingIdentifier            = @"Start Encoding Item Identifier";
static NSString *        PauseEncodingIdentifier            = @"Pause Encoding Item Identifier";
static NSString *        ShowQueueIdentifier                = @"Show Queue Item Identifier";
static NSString *        AddToQueueIdentifier               = @"Add to Queue Item Identifier";
static NSString *        ShowPictureIdentifier             = @"Show Picture Window Item Identifier";
static NSString *        ShowPreviewIdentifier             = @"Show Preview Window Item Identifier";
static NSString *        ShowActivityIdentifier             = @"Debug Output Item Identifier";
static NSString *        ChooseSourceIdentifier             = @"Choose Source Item Identifier";


/*******************************
 * HBController implementation *
 *******************************/
@implementation HBController

- (id)init
{
    self = [super init];
    if( !self )
    {
        return nil;
    }

    /* replace bundled app icon with one which is 32/64-bit savvy */
#if defined( __LP64__ )
    fApplicationIcon = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForImageResource:@"HandBrake-64.icns"]];
#else
    fApplicationIcon = [[NSImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForImageResource:@"HandBrake.icns"]];
#endif
    if( fApplicationIcon != nil )
        [NSApp setApplicationIconImage:fApplicationIcon];
    
    [HBPreferencesController registerUserDefaults];
    fHandle = NULL;
    fQueueEncodeLibhb = NULL;
    /* Check for check for the app support directory here as
     * outputPanel needs it right away, as may other future methods
     */
    NSString *libraryDir = [NSSearchPathForDirectoriesInDomains( NSLibraryDirectory,
                                                                NSUserDomainMask,
                                                                YES ) objectAtIndex:0];
    AppSupportDirectory = [[libraryDir stringByAppendingPathComponent:@"Application Support"]
                           stringByAppendingPathComponent:@"HandBrake"];
    if( ![[NSFileManager defaultManager] fileExistsAtPath:AppSupportDirectory] )
    {
        [[NSFileManager defaultManager] createDirectoryAtPath:AppSupportDirectory
                                                   attributes:nil];
    }
    /* Check for and create the App Support Preview directory if necessary */
    NSString *PreviewDirectory = [AppSupportDirectory stringByAppendingPathComponent:@"Previews"];
    if( ![[NSFileManager defaultManager] fileExistsAtPath:PreviewDirectory] )
    {
        [[NSFileManager defaultManager] createDirectoryAtPath:PreviewDirectory
                                                   attributes:nil];
    }                                                            
    outputPanel = [[HBOutputPanelController alloc] init];
    fPictureController = [[PictureController alloc] init];
    fQueueController = [[HBQueueController alloc] init];
    fAdvancedOptions = [[HBAdvancedController alloc] init];
    /* we init the HBPresets class which currently is only used
     * for updating built in presets, may move more functionality
     * there in the future
     */
    fPresetsBuiltin = [[HBPresets alloc] init];
    fPreferencesController = [[HBPreferencesController alloc] init];
    /* Lets report the HandBrake version number here to the activity log and text log file */
    NSString *versionStringFull = [[NSString stringWithFormat: @"Handbrake Version: %@", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"]] stringByAppendingString: [NSString stringWithFormat: @" (%@)", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"]]];
    [self writeToActivityLog: "%s", [versionStringFull UTF8String]];    
    
    return self;
}


- (void) applicationDidFinishLaunching: (NSNotification *) notification
{
    /* Init libhb with check for updates libhb style set to "0" so its ignored and lets sparkle take care of it */
    int loggingLevel = [[[NSUserDefaults standardUserDefaults] objectForKey:@"LoggingLevel"] intValue];
    fHandle = hb_init(loggingLevel, 0);
    /* Optional dvd nav UseDvdNav*/
    hb_dvd_set_dvdnav([[[NSUserDefaults standardUserDefaults] objectForKey:@"UseDvdNav"] boolValue]);
    /* Init a separate instance of libhb for user scanning and setting up jobs */
    fQueueEncodeLibhb = hb_init(loggingLevel, 0);
    
	// Set the Growl Delegate
    [GrowlApplicationBridge setGrowlDelegate: self];
    /* Init others controllers */
    [fPictureController SetHandle: fHandle];
    [fPictureController   setHBController: self];
    
    [fQueueController   setHandle: fQueueEncodeLibhb];
    [fQueueController   setHBController: self];

    fChapterTitlesDelegate = [[ChapterTitles alloc] init];
    [fChapterTable setDataSource:fChapterTitlesDelegate];
    [fChapterTable setDelegate:fChapterTitlesDelegate];
    
    /* setup the subtitles delegate and connections to table */
    fSubtitlesDelegate = [[HBSubtitles alloc] init];
    [fSubtitlesTable setDataSource:fSubtitlesDelegate];
    [fSubtitlesTable setDelegate:fSubtitlesDelegate];
    [fSubtitlesTable setRowHeight:25.0];
    
    [fPresetsOutlineView setAutosaveName:@"Presets View"];
    [fPresetsOutlineView setAutosaveExpandedItems:YES];
    
    dockIconProgress = 0;

    /* Call UpdateUI every 1/2 sec */
    [[NSRunLoop currentRunLoop] addTimer:[NSTimer
                                          scheduledTimerWithTimeInterval:0.5 target:self
                                          selector:@selector(updateUI:) userInfo:nil repeats:YES]
                                 forMode:NSDefaultRunLoopMode];

    // Open debug output window now if it was visible when HB was closed
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"OutputPanelIsOpen"])
        [self showDebugOutputPanel:nil];

    // Open queue window now if it was visible when HB was closed
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QueueWindowIsOpen"])
        [self showQueueWindow:nil];

	[self openMainWindow:nil];
    
    /* We have to set the bool to tell hb what to do after a scan
     * Initially we set it to NO until we start processing the queue
     */
     applyQueueToScan = NO;
    
    /* Now we re-check the queue array to see if there are
     * any remaining encodes to be done in it and ask the
     * user if they want to reload the queue */
    if ([QueueFileArray count] > 0)
	{
        /* run  getQueueStats to see whats in the queue file */
        [self getQueueStats];
        /* this results in these values
         * fEncodingQueueItem = 0;
         * fPendingCount = 0;
         * fCompletedCount = 0;
         * fCanceledCount = 0;
         * fWorkingCount = 0;
         */
        
        /*On Screen Notification*/
        NSString * alertTitle;
        
        /* We check to see if there is already another instance of hb running.
         * Note: hbInstances == 1 means we are the only instance of HandBrake.app
         */
        if ([self hbInstances] > 1)
        {
        alertTitle = [NSString stringWithFormat:
                         NSLocalizedString(@"There is already an instance of HandBrake running.", @"")];
        NSBeginCriticalAlertSheet(
                                      alertTitle,
                                      NSLocalizedString(@"Reload Queue", nil),
                                      nil,
                                      nil,
                                      fWindow, self,
                                      nil, @selector(didDimissReloadQueue:returnCode:contextInfo:), nil,
                                      NSLocalizedString(@" HandBrake will now load up the existing queue.", nil));    
        }
        else
        {
            if (fWorkingCount > 0)
            {
                alertTitle = [NSString stringWithFormat:
                              NSLocalizedString(@"HandBrake Has Detected %d Previously Encoding Item and %d Pending Item(s) In Your Queue.", @""),
                              fWorkingCount,fPendingCount];
            }
            else
            {
                alertTitle = [NSString stringWithFormat:
                              NSLocalizedString(@"HandBrake Has Detected %d Pending Item(s) In Your Queue.", @""),
                              fPendingCount];
            }
            
            NSBeginCriticalAlertSheet(
                                      alertTitle,
                                      NSLocalizedString(@"Reload Queue", nil),
                                      nil,
                                      NSLocalizedString(@"Empty Queue", nil),
                                      fWindow, self,
                                      nil, @selector(didDimissReloadQueue:returnCode:contextInfo:), nil,
                                      NSLocalizedString(@" Do you want to reload them ?", nil));
        }
        
        // call didDimissReloadQueue: (NSWindow *)sheet returnCode: (int)returnCode contextInfo: (void *)contextInfo
        // right below to either clear the old queue or keep it loaded up.
    }
    else
    {
        /* We show whichever open source window specified in LaunchSourceBehavior preference key */
        if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"LaunchSourceBehavior"] isEqualToString: @"Open Source"])
        {
            [self browseSources:nil];
        }
        
        if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"LaunchSourceBehavior"] isEqualToString: @"Open Source (Title Specific)"])
        {
            [self browseSources:(id)fOpenSourceTitleMMenu];
        }
    }
}

- (int) hbInstances
{
    /* check to see if another instance of HandBrake.app is running */
    NSArray *runningAppDictionaries = [[NSWorkspace sharedWorkspace] launchedApplications];
    NSDictionary *aDictionary;
    int hbInstances = 0;
    for (aDictionary in runningAppDictionaries)
	{
        //	NSLog(@"Open App: %@", [aDictionary valueForKey:@"NSApplicationName"]);
        
        if ([[aDictionary valueForKey:@"NSApplicationName"] isEqualToString:@"HandBrake"])
		{
            hbInstances++;
		}
	}
    return hbInstances;
}

- (void) didDimissReloadQueue: (NSWindow *)sheet returnCode: (int)returnCode contextInfo: (void *)contextInfo
{
    if (returnCode == NSAlertOtherReturn)
    {
        [self clearQueueAllItems];
        /* We show whichever open source window specified in LaunchSourceBehavior preference key */
        if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"LaunchSourceBehavior"] isEqualToString: @"Open Source"])
        {
            [self browseSources:nil];
        }
        
        if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"LaunchSourceBehavior"] isEqualToString: @"Open Source (Title Specific)"])
        {
            [self browseSources:(id)fOpenSourceTitleMMenu];
        }
    }
    else
    {
        if ([self hbInstances] == 1)
        {
            [self setQueueEncodingItemsAsPending];
        }
        [self showQueueWindow:NULL];
    }
}

- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication *) app
{
    /* if we are in preview full screen mode, we need to go to
     * windowed mode and release the display before we terminate.
     * We do it here (instead of applicationWillTerminate) so we 
     * release the displays and can then see the alerts below.
     */
    if ([fPictureController previewFullScreenMode] == YES)
    {
        [fPictureController previewGoWindowed:nil];
    }
    
    hb_state_t s;
    hb_get_state( fQueueEncodeLibhb, &s );
    
    if ( s.state != HB_STATE_IDLE )
    {
        int result = NSRunCriticalAlertPanel(
                                             NSLocalizedString(@"Are you sure you want to quit HandBrake?", nil),
                                             NSLocalizedString(@"If you quit HandBrake your current encode will be reloaded into your queue at next launch. Do you want to quit anyway?", nil),
                                             NSLocalizedString(@"Quit", nil), NSLocalizedString(@"Don't Quit", nil), nil, @"A movie" );
        
        if (result == NSAlertDefaultReturn)
        {
            return NSTerminateNow;
        }
        else
            return NSTerminateCancel;
    }
    
    // Warn if items still in the queue
    else if ( fPendingCount > 0 )
    {
        int result = NSRunCriticalAlertPanel(
                                             NSLocalizedString(@"Are you sure you want to quit HandBrake?", nil),
                                             NSLocalizedString(@"There are pending encodes in your queue. Do you want to quit anyway?",nil),
                                             NSLocalizedString(@"Quit", nil), NSLocalizedString(@"Don't Quit", nil), nil);
        
        if ( result == NSAlertDefaultReturn )
            return NSTerminateNow;
        else
            return NSTerminateCancel;
    }
    
    return NSTerminateNow;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    
    [browsedSourceDisplayName release];
    [outputPanel release];
	[fQueueController release];
    [fPreviewController release];
    [fPictureController release];
    [fApplicationIcon release];

	hb_close(&fHandle);
    hb_close(&fQueueEncodeLibhb);
}


- (void) awakeFromNib
{
    [fWindow center];
    [fWindow setExcludedFromWindowsMenu:YES];
    [fAdvancedOptions setView:fAdvancedView];
    
    /* lets setup our presets drawer for drag and drop here */
    [fPresetsOutlineView registerForDraggedTypes: [NSArray arrayWithObject:DragDropSimplePboardType] ];
    [fPresetsOutlineView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:YES];
    [fPresetsOutlineView setVerticalMotionCanBeginDrag: YES];
    
    /* Initialize currentScanCount so HB can use it to
     evaluate successive scans */
	currentScanCount = 0;
    
    
    /* Init UserPresets .plist */
	[self loadPresets];
    
    /* Init QueueFile .plist */
    [self loadQueueFile];
	
    fRipIndicatorShown = NO;  // initially out of view in the nib
    
    /* For 64 bit builds, the threaded animation in the progress
     * indicators conflicts with the animation in the advanced tab
     * for reasons not completely clear. jbrjake found a note in the
     * 10.5 dev notes regarding this possiblility. It was also noted
     * that unless specified, setUsesThreadedAnimation defaults to true.
     * So, at least for now we set the indicator animation to NO for
     * both the scan and regular progress indicators for both 32 and 64 bit
     * as it test out fine on both and there is no reason our progress indicators
     * should require their own thread.
     */

    [fScanIndicator setUsesThreadedAnimation:NO];
    [fRipIndicator setUsesThreadedAnimation:NO];
  
    
    
	/* Show/Dont Show Presets drawer upon launch based
     on user preference DefaultPresetsDrawerShow*/
	if( [[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultPresetsDrawerShow"] > 0 )
	{
        [fPresetDrawer setDelegate:self];
        NSSize drawerSize = NSSizeFromString( [[NSUserDefaults standardUserDefaults] 
                                               stringForKey:@"Drawer Size"] );
        if( drawerSize.width )
            [fPresetDrawer setContentSize: drawerSize];
		[fPresetDrawer open];
	}
    
    /* Initially set the dvd angle widgets to hidden (dvdnav only) */
    [fSrcAngleLabel setHidden:YES];
    [fSrcAnglePopUp setHidden:YES];
    
    /* Destination box*/
    NSMenuItem *menuItem;
    [fDstFormatPopUp removeAllItems];
    // MP4 file
    menuItem = [[fDstFormatPopUp menu] addItemWithTitle:@"MP4 file" action: NULL keyEquivalent: @""];
    [menuItem setTag: HB_MUX_MP4];
	// MKV file
    menuItem = [[fDstFormatPopUp menu] addItemWithTitle:@"MKV file" action: NULL keyEquivalent: @""];
    [menuItem setTag: HB_MUX_MKV];
    
    [fDstFormatPopUp selectItemAtIndex: 0];
    
    [self formatPopUpChanged:nil];
    
	/* We enable the create chapters checkbox here since we are .mp4 */
	[fCreateChapterMarkers setEnabled: YES];
	if ([fDstFormatPopUp indexOfSelectedItem] == 0 && [[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultChapterMarkers"] > 0)
	{
		[fCreateChapterMarkers setState: NSOnState];
	}
    
    
    
    
    [fDstFile2Field setStringValue: [NSString stringWithFormat:
                                     @"%@/Desktop/Movie.mp4", NSHomeDirectory()]];
    
    /* Video encoder */
    [fVidEncoderPopUp removeAllItems];
    [fVidEncoderPopUp addItemWithTitle: @"FFmpeg"];
    
    
    
    /* Video quality */
    [fVidTargetSizeField setIntValue: 700];
	[fVidBitrateField    setIntValue: 1000];
    
    [fVidQualityMatrix   selectCell: fVidBitrateCell];
    [self videoMatrixChanged:nil];
    
    /* Video framerate */
    [fVidRatePopUp removeAllItems];
	[fVidRatePopUp addItemWithTitle: NSLocalizedString( @"Same as source", @"" )];
    for( int i = 0; i < hb_video_rates_count; i++ )
    {
        if ([[NSString stringWithCString: hb_video_rates[i].string] isEqualToString: [NSString stringWithFormat: @"%.3f",23.976]])
		{
			[fVidRatePopUp addItemWithTitle:[NSString stringWithFormat: @"%@%@",
                                             [NSString stringWithCString: hb_video_rates[i].string], @" (NTSC Film)"]];
		}
		else if ([[NSString stringWithCString: hb_video_rates[i].string] isEqualToString: [NSString stringWithFormat: @"%d",25]])
		{
			[fVidRatePopUp addItemWithTitle:[NSString stringWithFormat: @"%@%@",
                                             [NSString stringWithCString: hb_video_rates[i].string], @" (PAL Film/Video)"]];
		}
		else if ([[NSString stringWithCString: hb_video_rates[i].string] isEqualToString: [NSString stringWithFormat: @"%.2f",29.97]])
		{
			[fVidRatePopUp addItemWithTitle:[NSString stringWithFormat: @"%@%@",
                                             [NSString stringWithCString: hb_video_rates[i].string], @" (NTSC Video)"]];
		}
		else
		{
			[fVidRatePopUp addItemWithTitle:
             [NSString stringWithCString: hb_video_rates[i].string]];
		}
    }
    [fVidRatePopUp selectItemAtIndex: 0];
	
	/* Set Auto Crop to On at launch */
    [fPictureController setAutoCrop:YES];
	
	/* Audio bitrate */
    [fAudTrack1BitratePopUp removeAllItems];
    for( int i = 0; i < hb_audio_bitrates_count; i++ )
    {
        [fAudTrack1BitratePopUp addItemWithTitle:
         [NSString stringWithCString: hb_audio_bitrates[i].string]];
        
    }
    [fAudTrack1BitratePopUp selectItemAtIndex: hb_audio_bitrates_default];
	
    /* Audio samplerate */
    [fAudTrack1RatePopUp removeAllItems];
    for( int i = 0; i < hb_audio_rates_count; i++ )
    {
        [fAudTrack1RatePopUp addItemWithTitle:
         [NSString stringWithCString: hb_audio_rates[i].string]];
    }
    [fAudTrack1RatePopUp selectItemAtIndex: hb_audio_rates_default];
	
    /* Bottom */
    [fStatusField setStringValue: @""];
    
    [self enableUI: NO];
	[self setupToolbar];
    
	/* We disable the Turbo 1st pass checkbox since we are not x264 */
	[fVidTurboPassCheck setEnabled: NO];
	[fVidTurboPassCheck setState: NSOffState];
    
    
	/* lets get our default prefs here */
	[self getDefaultPresets:nil];
	/* lets initialize the current successful scancount here to 0 */
	currentSuccessfulScanCount = 0;
    
    
}

- (void) enableUI: (bool) b
{
    NSControl * controls[] =
    { fSrcTitleField, fSrcTitlePopUp,
        fSrcChapterField, fSrcChapterStartPopUp, fSrcChapterToField,
        fSrcChapterEndPopUp, fSrcDuration1Field, fSrcDuration2Field,
        fDstFormatField, fDstFormatPopUp, fDstFile1Field, fDstFile2Field,
        fDstBrowseButton, fVidRateField, fVidRatePopUp,fVidEncoderField, fVidEncoderPopUp, fVidQualityField,
        fPictureSizeField,fPictureCroppingField, fVideoFiltersField,fVidQualityMatrix, fSubField, fSubPopUp,
        fAudSourceLabel, fAudCodecLabel, fAudMixdownLabel, fAudSamplerateLabel, fAudBitrateLabel,
        fAudTrack1Label, fAudTrack2Label, fAudTrack3Label, fAudTrack4Label,
        fAudLang1PopUp, fAudLang2PopUp, fAudLang3PopUp, fAudLang4PopUp,
        fAudTrack1CodecPopUp, fAudTrack2CodecPopUp, fAudTrack3CodecPopUp, fAudTrack4CodecPopUp,
        fAudTrack1MixPopUp, fAudTrack2MixPopUp, fAudTrack3MixPopUp, fAudTrack4MixPopUp,
        fAudTrack1RatePopUp, fAudTrack2RatePopUp, fAudTrack3RatePopUp, fAudTrack4RatePopUp,
        fAudTrack1BitratePopUp, fAudTrack2BitratePopUp, fAudTrack3BitratePopUp, fAudTrack4BitratePopUp,
        fAudDrcLabel, fAudTrack1DrcSlider, fAudTrack1DrcField, fAudTrack2DrcSlider,
        fAudTrack2DrcField, fAudTrack3DrcSlider, fAudTrack3DrcField, fAudTrack4DrcSlider,fAudTrack4DrcField,
        fQueueStatus,fPresetsAdd,fPresetsDelete,fSrcAngleLabel,fSrcAnglePopUp,
		fCreateChapterMarkers,fVidTurboPassCheck,fDstMp4LargeFileCheck,fSubForcedCheck,fPresetsOutlineView,
    fAudDrcLabel,fDstMp4HttpOptFileCheck,fDstMp4iPodFileCheck,fVidQualityRFField,fVidQualityRFLabel};
    
    for( unsigned i = 0;
        i < sizeof( controls ) / sizeof( NSControl * ); i++ )
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
    
	if (b) {
        
        /* if we're enabling the interface, check if the audio mixdown controls need to be enabled or not */
        /* these will have been enabled by the mass control enablement above anyway, so we're sense-checking it here */
        [self setEnabledStateOfAudioMixdownControls:nil];
        /* we also call calculatePictureSizing here to sense check if we already have vfr selected */
        [self calculatePictureSizing:nil];
        
	} else {
        
		[fPresetsOutlineView setEnabled: NO];
        
	}
    
    [self videoMatrixChanged:nil];
    [fAdvancedOptions enableUI:b];
}


/***********************************************************************
 * UpdateDockIcon
 ***********************************************************************
 * Shows a progression bar on the dock icon, filled according to
 * 'progress' (0.0 <= progress <= 1.0).
 * Called with progress < 0.0 or progress > 1.0, restores the original
 * icon.
 **********************************************************************/
- (void) UpdateDockIcon: (float) progress
{
    NSData * tiff;
    NSBitmapImageRep * bmp;
    uint32_t * pen;
    uint32_t black = htonl( 0x000000FF );
    uint32_t red   = htonl( 0xFF0000FF );
    uint32_t white = htonl( 0xFFFFFFFF );
    int row_start, row_end;
    int i, j;

    if( progress < 0.0 || progress > 1.0 )
    {
        [NSApp setApplicationIconImage: fApplicationIcon];
        return;
    }

    /* Get it in a raw bitmap form */
    tiff = [fApplicationIcon TIFFRepresentationUsingCompression:
            NSTIFFCompressionNone factor: 1.0];
    bmp = [NSBitmapImageRep imageRepWithData: tiff];
    
    /* Draw the progression bar */
    /* It's pretty simple (ugly?) now, but I'm no designer */

    row_start = 3 * (int) [bmp size].height / 4;
    row_end   = 7 * (int) [bmp size].height / 8;

    for( i = row_start; i < row_start + 2; i++ )
    {
        pen = (uint32_t *) ( [bmp bitmapData] + i * [bmp bytesPerRow] );
        for( j = 0; j < (int) [bmp size].width; j++ )
        {
            pen[j] = black;
        }
    }
    for( i = row_start + 2; i < row_end - 2; i++ )
    {
        pen = (uint32_t *) ( [bmp bitmapData] + i * [bmp bytesPerRow] );
        pen[0] = black;
        pen[1] = black;
        for( j = 2; j < (int) [bmp size].width - 2; j++ )
        {
            if( j < 2 + (int) ( ( [bmp size].width - 4.0 ) * progress ) )
            {
                pen[j] = red;
            }
            else
            {
                pen[j] = white;
            }
        }
        pen[j]   = black;
        pen[j+1] = black;
    }
    for( i = row_end - 2; i < row_end; i++ )
    {
        pen = (uint32_t *) ( [bmp bitmapData] + i * [bmp bytesPerRow] );
        for( j = 0; j < (int) [bmp size].width; j++ )
        {
            pen[j] = black;
        }
    }

    /* Now update the dock icon */
    tiff = [bmp TIFFRepresentationUsingCompression:
            NSTIFFCompressionNone factor: 1.0];
    NSImage* icon = [[NSImage alloc] initWithData: tiff];
    [NSApp setApplicationIconImage: icon];
    [icon release];
}

- (void) updateUI: (NSTimer *) timer
{
    
    /* Update UI for fHandle (user scanning instance of libhb ) */
    
    hb_list_t  * list;
    list = hb_get_titles( fHandle );
    /* check to see if there has been a new scan done
     this bypasses the constraints of HB_STATE_WORKING
     not allowing setting a newly scanned source */
	int checkScanCount = hb_get_scancount( fHandle );
	if( checkScanCount > currentScanCount )
	{
		currentScanCount = checkScanCount;
        [fScanIndicator setIndeterminate: NO];
        [fScanIndicator setDoubleValue: 0.0];
        [fScanIndicator setHidden: YES];
		[self showNewScan:nil];
	}
    
    hb_state_t s;
    hb_get_state( fHandle, &s );
    
    switch( s.state )
    {
        case HB_STATE_IDLE:
            break;
#define p s.param.scanning
        case HB_STATE_SCANNING:
		{
            [fSrcDVD2Field setStringValue: [NSString stringWithFormat:
                                            NSLocalizedString( @"Scanning title %d of %d...", @"" ),
                                            p.title_cur, p.title_count]];
            [fScanIndicator setHidden: NO];
            [fScanIndicator setDoubleValue: 100.0 * ((double)( p.title_cur - 1 ) / p.title_count)];
            break;
		}
#undef p
            
#define p s.param.scandone
        case HB_STATE_SCANDONE:
        {
            [fScanIndicator setIndeterminate: NO];
            [fScanIndicator setDoubleValue: 0.0];
            [fScanIndicator setHidden: YES];
			[self writeToActivityLog:"ScanDone state received from fHandle"];
            [self showNewScan:nil];
            [[fWindow toolbar] validateVisibleItems];
            
			break;
        }
#undef p
            
#define p s.param.working
        case HB_STATE_WORKING:
        {
            
            break;
        }
#undef p
            
#define p s.param.muxing
        case HB_STATE_MUXING:
        {
            
            break;
        }
#undef p
            
        case HB_STATE_PAUSED:
            break;
            
        case HB_STATE_WORKDONE:
        {
            break;
        }
    }
    
    
    /* Update UI for fQueueEncodeLibhb */
    // hb_list_t  * list;
    // list = hb_get_titles( fQueueEncodeLibhb ); //fQueueEncodeLibhb
    /* check to see if there has been a new scan done
     this bypasses the constraints of HB_STATE_WORKING
     not allowing setting a newly scanned source */
	
    checkScanCount = hb_get_scancount( fQueueEncodeLibhb );
	if( checkScanCount > currentScanCount )
	{
		currentScanCount = checkScanCount;
	}
    
    //hb_state_t s;
    hb_get_state( fQueueEncodeLibhb, &s );
    
    switch( s.state )
    {
        case HB_STATE_IDLE:
            break;
#define p s.param.scanning
        case HB_STATE_SCANNING:
		{
            [fStatusField setStringValue: [NSString stringWithFormat:
                                           NSLocalizedString( @"Queue Scanning title %d of %d...", @"" ),
                                           p.title_cur, p.title_count]];
            
            /* Set the status string in fQueueController as well */                               
            [fQueueController setQueueStatusString: [NSString stringWithFormat:
                                                     NSLocalizedString( @"Queue Scanning title %d of %d...", @"" ),
                                                     p.title_cur, p.title_count]];
            break;
		}
#undef p
            
#define p s.param.scandone
        case HB_STATE_SCANDONE:
        {
			[self writeToActivityLog:"ScanDone state received from fQueueEncodeLibhb"];
            [self processNewQueueEncode];
            [[fWindow toolbar] validateVisibleItems];
            
			break;
        }
#undef p
            
#define p s.param.working
        case HB_STATE_WORKING:
        {
            NSMutableString * string;
            NSString * pass_desc;
			/* Update text field */
            if (p.job_cur == 1 && p.job_count > 1)
            {
                if ([[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SubtitleList"] && [[[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex]objectForKey:@"SubtitleList"] objectAtIndex:0] objectForKey:@"subtitleSourceTrackNum"] intValue] == 1)
                {
                    pass_desc = @"(subtitle scan)";   
                }
                else
                {
                    pass_desc = @"";
                }
            }
            else
            {
                pass_desc = @"";
            }
            
			string = [NSMutableString stringWithFormat: NSLocalizedString( @"Encoding: pass %d %@ of %d, %.2f %%", @"" ), p.job_cur, pass_desc, p.job_count, 100.0 * p.progress];
            
			if( p.seconds > -1 )
            {
                [string appendFormat:
                 NSLocalizedString( @" (%.2f fps, avg %.2f fps, ETA %02dh%02dm%02ds)", @"" ),
                 p.rate_cur, p.rate_avg, p.hours, p.minutes, p.seconds];
            }
            
            [fStatusField setStringValue: string];
            /* Set the status string in fQueueController as well */
            [fQueueController setQueueStatusString: string];
            /* Update slider */
            CGFloat progress_total = ( p.progress + p.job_cur - 1 ) / p.job_count;
            [fRipIndicator setIndeterminate: NO];
            [fRipIndicator setDoubleValue:100.0 * progress_total];
            
            // If progress bar hasn't been revealed at the bottom of the window, do
            // that now. This code used to be in doRip. I moved it to here to handle
            // the case where hb_start is called by HBQueueController and not from
            // HBController.
            if( !fRipIndicatorShown )
            {
                NSRect frame = [fWindow frame];
                if( frame.size.width <= 591 )
                    frame.size.width = 591;
                frame.size.height += 36;
                frame.origin.y -= 36;
                [fWindow setFrame:frame display:YES animate:YES];
                fRipIndicatorShown = YES;
                
            }

            /* Update dock icon */
            if( dockIconProgress < 100.0 * progress_total )
            {
                [self UpdateDockIcon: progress_total];
                dockIconProgress += 5;
            }

            break;
        }
#undef p
            
#define p s.param.muxing
        case HB_STATE_MUXING:
        {
            /* Update text field */
            [fStatusField setStringValue: NSLocalizedString( @"Muxing...", @"" )];
            /* Set the status string in fQueueController as well */
            [fQueueController setQueueStatusString: NSLocalizedString( @"Muxing...", @"" )];
            /* Update slider */
            [fRipIndicator setIndeterminate: YES];
            [fRipIndicator startAnimation: nil];
            
            /* Update dock icon */
            [self UpdateDockIcon: 1.0];
            
			break;
        }
#undef p
            
        case HB_STATE_PAUSED:
		    [fStatusField setStringValue: NSLocalizedString( @"Paused", @"" )];
            [fQueueController setQueueStatusString: NSLocalizedString( @"Paused", @"" )];
            
			break;
            
        case HB_STATE_WORKDONE:
        {
            // HB_STATE_WORKDONE happpens as a result of libhb finishing all its jobs
            // or someone calling hb_stop. In the latter case, hb_stop does not clear
            // out the remaining passes/jobs in the queue. We'll do that here.
            
            // Delete all remaining jobs of this encode.
            [fStatusField setStringValue: NSLocalizedString( @"Encode Finished.", @"" )];
            /* Set the status string in fQueueController as well */
            [fQueueController setQueueStatusString: NSLocalizedString( @"Encode Finished.", @"" )];
            [fRipIndicator setIndeterminate: NO];
            [fRipIndicator stopAnimation: nil];
            [fRipIndicator setDoubleValue: 0.0];
            [[fWindow toolbar] validateVisibleItems];
            
            /* Restore dock icon */
            [self UpdateDockIcon: -1.0];
            dockIconProgress = 0;
            
            if( fRipIndicatorShown )
            {
                NSRect frame = [fWindow frame];
                if( frame.size.width <= 591 )
				    frame.size.width = 591;
                frame.size.height += -36;
                frame.origin.y -= -36;
                [fWindow setFrame:frame display:YES animate:YES];
				fRipIndicatorShown = NO;
			}
            /* Since we are done with this encode, tell output to stop writing to the
             * individual encode log
             */
			[outputPanel endEncodeLog];
            /* Check to see if the encode state has not been cancelled
             to determine if we should check for encode done notifications */
			if( fEncodeState != 2 )
            {
                NSString *pathOfFinishedEncode;
                /* Get the output file name for the finished encode */
                pathOfFinishedEncode = [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"DestinationPath"];
                
                /* Both the Growl Alert and Sending to MetaX can be done as encodes roll off the queue */
                /* Growl alert */
                [self showGrowlDoneNotification:pathOfFinishedEncode];
                /* Send to MetaX */
                [self sendToMetaX:pathOfFinishedEncode];
                
                /* since we have successfully completed an encode, we increment the queue counter */
                [self incrementQueueItemDone:nil]; 
                
                /* all end of queue actions below need to be done after all queue encodes have finished 
                 * and there are no pending jobs left to process
                 */
                if (fPendingCount == 0)
                {
                    /* If Alert Window or Window and Growl has been selected */
                    if( [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window"] ||
                       [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window And Growl"] )
                    {
                        /*On Screen Notification*/
                        int status;
                        NSBeep();
                        status = NSRunAlertPanel(@"Put down that cocktail...",@"Your HandBrake queue is done!", @"OK", nil, nil);
                        [NSApp requestUserAttention:NSCriticalRequest];
                    }
                    
                    /* If sleep has been selected */
                    if( [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Put Computer To Sleep"] )
                    {
                        /* Sleep */
                        NSDictionary* errorDict;
                        NSAppleEventDescriptor* returnDescriptor = nil;
                        NSAppleScript* scriptObject = [[NSAppleScript alloc] initWithSource:
                                                       @"tell application \"Finder\" to sleep"];
                        returnDescriptor = [scriptObject executeAndReturnError: &errorDict];
                        [scriptObject release];
                    }
                    /* If Shutdown has been selected */
                    if( [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Shut Down Computer"] )
                    {
                        /* Shut Down */
                        NSDictionary* errorDict;
                        NSAppleEventDescriptor* returnDescriptor = nil;
                        NSAppleScript* scriptObject = [[NSAppleScript alloc] initWithSource:
                                                       @"tell application \"Finder\" to shut down"];
                        returnDescriptor = [scriptObject executeAndReturnError: &errorDict];
                        [scriptObject release];
                    }
                    
                }
                
                
            }
            
            break;
        }
    }
    
}

/* We use this to write messages to stderr from the macgui which show up in the activity window and log*/
- (void) writeToActivityLog:(const char *) format, ...
{
    va_list args;
    va_start(args, format);
    if (format != nil)
    {
        char str[1024];
        vsnprintf( str, 1024, format, args );

        time_t _now = time( NULL );
        struct tm * now  = localtime( &_now );
        fprintf(stderr, "[%02d:%02d:%02d] macgui: %s\n", now->tm_hour, now->tm_min, now->tm_sec, str );
    }
    va_end(args);
}

#pragma mark -
#pragma mark Toolbar
// ============================================================
// NSToolbar Related Methods
// ============================================================

- (void) setupToolbar {
    NSToolbar *toolbar = [[[NSToolbar alloc] initWithIdentifier: @"HandBrake Toolbar"] autorelease];

    [toolbar setAllowsUserCustomization: YES];
    [toolbar setAutosavesConfiguration: YES];
    [toolbar setDisplayMode: NSToolbarDisplayModeIconAndLabel];

    [toolbar setDelegate: self];

    [fWindow setToolbar: toolbar];
}

- (NSToolbarItem *) toolbar: (NSToolbar *)toolbar itemForItemIdentifier:
    (NSString *) itemIdent willBeInsertedIntoToolbar:(BOOL) willBeInserted {
    NSToolbarItem * item = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdent] autorelease];

    if ([itemIdent isEqualToString: ToggleDrawerIdentifier])
    {
        [item setLabel: @"Toggle Presets"];
        [item setPaletteLabel: @"Toggler Presets"];
        [item setToolTip: @"Open/Close Preset Drawer"];
        [item setImage: [NSImage imageNamed: @"Drawer"]];
        [item setTarget: self];
        [item setAction: @selector(toggleDrawer:)];
        [item setAutovalidates: NO];
    }
    else if ([itemIdent isEqualToString: StartEncodingIdentifier])
    {
        [item setLabel: @"Start"];
        [item setPaletteLabel: @"Start Encoding"];
        [item setToolTip: @"Start Encoding"];
        [item setImage: [NSImage imageNamed: @"Play"]];
        [item setTarget: self];
        [item setAction: @selector(Rip:)];
    }
    else if ([itemIdent isEqualToString: ShowQueueIdentifier])
    {
        [item setLabel: @"Show Queue"];
        [item setPaletteLabel: @"Show Queue"];
        [item setToolTip: @"Show Queue"];
        [item setImage: [NSImage imageNamed: @"Queue"]];
        [item setTarget: self];
        [item setAction: @selector(showQueueWindow:)];
        [item setAutovalidates: NO];
    }
    else if ([itemIdent isEqualToString: AddToQueueIdentifier])
    {
        [item setLabel: @"Add to Queue"];
        [item setPaletteLabel: @"Add to Queue"];
        [item setToolTip: @"Add to Queue"];
        [item setImage: [NSImage imageNamed: @"AddToQueue"]];
        [item setTarget: self];
        [item setAction: @selector(addToQueue:)];
    }
    else if ([itemIdent isEqualToString: PauseEncodingIdentifier])
    {
        [item setLabel: @"Pause"];
        [item setPaletteLabel: @"Pause Encoding"];
        [item setToolTip: @"Pause Encoding"];
        [item setImage: [NSImage imageNamed: @"Pause"]];
        [item setTarget: self];
        [item setAction: @selector(Pause:)];
    }
    else if ([itemIdent isEqualToString: ShowPictureIdentifier])
    {
        [item setLabel: @"Picture Settings"];
        [item setPaletteLabel: @"Show Picture Settings"];
        [item setToolTip: @"Show Picture Settings"];
        [item setImage: [NSImage imageNamed: @"pref-picture"]];
        [item setTarget: self];
        [item setAction: @selector(showPicturePanel:)];
    }
    else if ([itemIdent isEqualToString: ShowPreviewIdentifier])
    {
        [item setLabel: @"Preview Window"];
        [item setPaletteLabel: @"Show Preview"];
        [item setToolTip: @"Show Preview"];
        //[item setImage: [NSImage imageNamed: @"pref-picture"]];
        [item setImage: [NSImage imageNamed: @"Brushed_Window"]];
        [item setTarget: self];
        [item setAction: @selector(showPreviewWindow:)];
    }
    else if ([itemIdent isEqualToString: ShowActivityIdentifier]) 
    {
        [item setLabel: @"Activity Window"];
        [item setPaletteLabel: @"Show Activity Window"];
        [item setToolTip: @"Show Activity Window"];
        [item setImage: [NSImage imageNamed: @"ActivityWindow"]];
        [item setTarget: self];
        [item setAction: @selector(showDebugOutputPanel:)];
        [item setAutovalidates: NO];
    }
    else if ([itemIdent isEqualToString: ChooseSourceIdentifier])
    {
        [item setLabel: @"Source"];
        [item setPaletteLabel: @"Source"];
        [item setToolTip: @"Choose Video Source"];
        [item setImage: [NSImage imageNamed: @"Source"]];
        [item setTarget: self];
        [item setAction: @selector(browseSources:)];
    }
    else
    {
        return nil;
    }

    return item;
}

- (NSArray *) toolbarDefaultItemIdentifiers: (NSToolbar *) toolbar
{
    return [NSArray arrayWithObjects: ChooseSourceIdentifier, NSToolbarSeparatorItemIdentifier, StartEncodingIdentifier,
        PauseEncodingIdentifier, AddToQueueIdentifier, ShowQueueIdentifier, NSToolbarFlexibleSpaceItemIdentifier, 
		NSToolbarSpaceItemIdentifier, ShowPictureIdentifier, ShowPreviewIdentifier, ShowActivityIdentifier, ToggleDrawerIdentifier, nil];
}

- (NSArray *) toolbarAllowedItemIdentifiers: (NSToolbar *) toolbar
{
    return [NSArray arrayWithObjects:  StartEncodingIdentifier, PauseEncodingIdentifier, AddToQueueIdentifier,
        ChooseSourceIdentifier, ShowQueueIdentifier, ShowPictureIdentifier, ShowPreviewIdentifier, ShowActivityIdentifier, ToggleDrawerIdentifier,
        NSToolbarCustomizeToolbarItemIdentifier, NSToolbarFlexibleSpaceItemIdentifier,
        NSToolbarSpaceItemIdentifier, NSToolbarSeparatorItemIdentifier, nil];
}

- (BOOL) validateToolbarItem: (NSToolbarItem *) toolbarItem
{
    NSString * ident = [toolbarItem itemIdentifier];
        
    if (fHandle)
    {
        hb_state_t s;
        hb_get_state2( fQueueEncodeLibhb, &s );
        
        if (s.state == HB_STATE_WORKING || s.state == HB_STATE_MUXING)
        {
            if ([ident isEqualToString: StartEncodingIdentifier])
            {
                [toolbarItem setImage: [NSImage imageNamed: @"Stop"]];
                [toolbarItem setLabel: @"Stop"];
                [toolbarItem setPaletteLabel: @"Stop"];
                [toolbarItem setToolTip: @"Stop Encoding"];
                return YES;
            }
            if ([ident isEqualToString: PauseEncodingIdentifier])
            {
                [toolbarItem setImage: [NSImage imageNamed: @"Pause"]];
                [toolbarItem setLabel: @"Pause"];
                [toolbarItem setPaletteLabel: @"Pause Encoding"];
                [toolbarItem setToolTip: @"Pause Encoding"];
                return YES;
            }
            if (SuccessfulScan)
            {
                if ([ident isEqualToString: AddToQueueIdentifier])
                    return YES;
                if ([ident isEqualToString: ShowPictureIdentifier])
                    return YES;
                if ([ident isEqualToString: ShowPreviewIdentifier])
                    return YES;
            }
        }
        else if (s.state == HB_STATE_PAUSED)
        {
            if ([ident isEqualToString: PauseEncodingIdentifier])
            {
                [toolbarItem setImage: [NSImage imageNamed: @"Play"]];
                [toolbarItem setLabel: @"Resume"];
                [toolbarItem setPaletteLabel: @"Resume Encoding"];
                [toolbarItem setToolTip: @"Resume Encoding"];
                return YES;
            }
            if ([ident isEqualToString: StartEncodingIdentifier])
                return YES;
            if ([ident isEqualToString: AddToQueueIdentifier])
                return YES;
            if ([ident isEqualToString: ShowPictureIdentifier])
                return YES;
            if ([ident isEqualToString: ShowPreviewIdentifier])
                return YES;
        }
        else if (s.state == HB_STATE_SCANNING)
            return NO;
        else if (s.state == HB_STATE_WORKDONE || s.state == HB_STATE_SCANDONE || SuccessfulScan)
        {
            if ([ident isEqualToString: StartEncodingIdentifier])
            {
                [toolbarItem setImage: [NSImage imageNamed: @"Play"]];
                if (hb_count(fHandle) > 0)
                    [toolbarItem setLabel: @"Start Queue"];
                else
                    [toolbarItem setLabel: @"Start"];
                [toolbarItem setPaletteLabel: @"Start Encoding"];
                [toolbarItem setToolTip: @"Start Encoding"];
                return YES;
            }
            if ([ident isEqualToString: AddToQueueIdentifier])
                return YES;
            if ([ident isEqualToString: ShowPictureIdentifier])
                return YES;
            if ([ident isEqualToString: ShowPreviewIdentifier])
                return YES;
        }

    }
    /* If there are any pending queue items, make sure the start/stop button is active */
    if ([ident isEqualToString: StartEncodingIdentifier] && fPendingCount > 0)
        return YES;
    if ([ident isEqualToString: ShowQueueIdentifier])
        return YES;
    if ([ident isEqualToString: ToggleDrawerIdentifier])
        return YES;
    if ([ident isEqualToString: ChooseSourceIdentifier])
        return YES;
    if ([ident isEqualToString: ShowActivityIdentifier])
        return YES;
    
    return NO;
}

- (BOOL) validateMenuItem: (NSMenuItem *) menuItem
{
    SEL action = [menuItem action];
    
    hb_state_t s;
    hb_get_state2( fHandle, &s );
    
    if (fHandle)
    {
        if (action == @selector(addToQueue:) || action == @selector(showPicturePanel:) || action == @selector(showAddPresetPanel:))
            return SuccessfulScan && [fWindow attachedSheet] == nil;
        
        if (action == @selector(browseSources:))
        {
            if (s.state == HB_STATE_SCANNING)
                return NO;
            else
                return [fWindow attachedSheet] == nil;
        }
        if (action == @selector(selectDefaultPreset:))
            return [fPresetsOutlineView selectedRow] >= 0 && [fWindow attachedSheet] == nil;
        if (action == @selector(Pause:))
        {
            if (s.state == HB_STATE_WORKING)
            {
                if(![[menuItem title] isEqualToString:@"Pause Encoding"])
                    [menuItem setTitle:@"Pause Encoding"];
                return YES;
            }
            else if (s.state == HB_STATE_PAUSED)
            {
                if(![[menuItem title] isEqualToString:@"Resume Encoding"])
                    [menuItem setTitle:@"Resume Encoding"];
                return YES;
            }
            else
                return NO;
        }
        if (action == @selector(Rip:))
        {
            if (s.state == HB_STATE_WORKING || s.state == HB_STATE_MUXING || s.state == HB_STATE_PAUSED)
            {
                if(![[menuItem title] isEqualToString:@"Stop Encoding"])
                    [menuItem setTitle:@"Stop Encoding"];
                return YES;
            }
            else if (SuccessfulScan)
            {
                if(![[menuItem title] isEqualToString:@"Start Encoding"])
                    [menuItem setTitle:@"Start Encoding"];
                return [fWindow attachedSheet] == nil;
            }
            else
                return NO;
        }
    }
    if( action == @selector(setDefaultPreset:) )
    {
        return [fPresetsOutlineView selectedRow] != -1;
    }

    return YES;
}

#pragma mark -
#pragma mark Encode Done Actions
// register a test notification and make
// it enabled by default
#define SERVICE_NAME @"Encode Done"
- (NSDictionary *)registrationDictionaryForGrowl 
{ 
    NSDictionary *registrationDictionary = [NSDictionary dictionaryWithObjectsAndKeys: 
    [NSArray arrayWithObjects:SERVICE_NAME,nil], GROWL_NOTIFICATIONS_ALL, 
    [NSArray arrayWithObjects:SERVICE_NAME,nil], GROWL_NOTIFICATIONS_DEFAULT, 
    nil]; 

    return registrationDictionary; 
} 

-(void)showGrowlDoneNotification:(NSString *) filePath
{
    /* This end of encode action is called as each encode rolls off of the queue */
    NSString * finishedEncode = filePath;
    /* strip off the path to just show the file name */
    finishedEncode = [finishedEncode lastPathComponent];
    if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Growl Notification"] || 
        [[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window And Growl"])
    {
        NSString * growlMssg = [NSString stringWithFormat: @"your HandBrake encode %@ is done!",finishedEncode];
        [GrowlApplicationBridge 
         notifyWithTitle:@"Put down that cocktail..." 
         description:growlMssg 
         notificationName:SERVICE_NAME
         iconData:nil 
         priority:0 
         isSticky:1 
         clickContext:nil];
    }
    
}
-(void)sendToMetaX:(NSString *) filePath
{
    /* This end of encode action is called as each encode rolls off of the queue */
    if([[NSUserDefaults standardUserDefaults] boolForKey: @"sendToMetaX"] == YES)
    {
        NSAppleScript *myScript = [[NSAppleScript alloc] initWithSource: [NSString stringWithFormat: @"%@%@%@", @"tell application \"MetaX\" to open (POSIX file \"", filePath, @"\")"]];
        [myScript executeAndReturnError: nil];
        [myScript release];
    }
}
#pragma mark -
#pragma mark Get New Source

/*Opens the source browse window, called from Open Source widgets */
- (IBAction) browseSources: (id) sender
{
    NSOpenPanel * panel;
	
    panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: YES ];
    NSString * sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastSourceDirectory"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastSourceDirectory"];
	}
	else
	{
		sourceDirectory = @"~/Desktop";
		sourceDirectory = [sourceDirectory stringByExpandingTildeInPath];
	}
    /* we open up the browse sources sheet here and call for browseSourcesDone after the sheet is closed
        * to evaluate whether we want to specify a title, we pass the sender in the contextInfo variable
        */
    [panel beginSheetForDirectory: sourceDirectory file: nil types: nil
                   modalForWindow: fWindow modalDelegate: self
                   didEndSelector: @selector( browseSourcesDone:returnCode:contextInfo: )
                      contextInfo: sender]; 
}

- (void) browseSourcesDone: (NSOpenPanel *) sheet
                returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    /* we convert the sender content of contextInfo back into a variable called sender
     * mostly just for consistency for evaluation later
     */
    id sender = (id)contextInfo;
    /* User selected a file to open */
	if( returnCode == NSOKButton )
    {
            /* Free display name allocated previously by this code */
        [browsedSourceDisplayName release];
       
        NSString *scanPath = [[sheet filenames] objectAtIndex: 0];
        /* we set the last searched source directory in the prefs here */
        NSString *sourceDirectory = [scanPath stringByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setObject:sourceDirectory forKey:@"LastSourceDirectory"];
        /* we order out sheet, which is the browse window as we need to open
         * the title selection sheet right away
         */
        [sheet orderOut: self];
        
        if (sender == fOpenSourceTitleMMenu)
        {
            /* We put the chosen source path in the source display text field for the
             * source title selection sheet in which the user specifies the specific title to be
             * scanned  as well as the short source name in fSrcDsplyNameTitleScan just for display
             * purposes in the title panel
             */
            /* Full Path */
            [fScanSrcTitlePathField setStringValue:scanPath];
            NSString *displayTitlescanSourceName;

            if ([[scanPath lastPathComponent] isEqualToString: @"VIDEO_TS"])
            {
                /* If VIDEO_TS Folder is chosen, choose its parent folder for the source display name
                 we have to use the title->dvd value so we get the proper name of the volume if a physical dvd is the source*/
                displayTitlescanSourceName = [[scanPath stringByDeletingLastPathComponent] lastPathComponent];
            }
            else
            {
                /* if not the VIDEO_TS Folder, we can assume the chosen folder is the source name */
                displayTitlescanSourceName = [scanPath lastPathComponent];
            }
            /* we set the source display name in the title selection dialogue */
            [fSrcDsplyNameTitleScan setStringValue:displayTitlescanSourceName];
            /* we set the attempted scans display name for main window to displayTitlescanSourceName*/
            browsedSourceDisplayName = [displayTitlescanSourceName retain];
            /* We show the actual sheet where the user specifies the title to be scanned
             * as we are going to do a title specific scan
             */
            [self showSourceTitleScanPanel:nil];
        }
        else
        {
            /* We are just doing a standard full source scan, so we specify "0" to libhb */
            NSString *path = [[sheet filenames] objectAtIndex: 0];
            
            /* We check to see if the chosen file at path is a package */
            if ([[NSWorkspace sharedWorkspace] isFilePackageAtPath:path])
            {
                [self writeToActivityLog: "trying to open a package at: %s", [path UTF8String]];
                /* We check to see if this is an .eyetv package */
                if ([[path pathExtension] isEqualToString: @"eyetv"])
                {
                    [self writeToActivityLog:"trying to open eyetv package"];
                    /* We're looking at an EyeTV package - try to open its enclosed
                     .mpg media file */
                     browsedSourceDisplayName = [[[path stringByDeletingPathExtension] lastPathComponent] retain];
                    NSString *mpgname;
                    int n = [[path stringByAppendingString: @"/"]
                             completePathIntoString: &mpgname caseSensitive: YES
                             matchesIntoArray: nil
                             filterTypes: [NSArray arrayWithObject: @"mpg"]];
                    if (n > 0)
                    {
                        /* Found an mpeg inside the eyetv package, make it our scan path 
                        and call performScan on the enclosed mpeg */
                        path = mpgname;
                        [self writeToActivityLog:"found mpeg in eyetv package"];
                        [self performScan:path scanTitleNum:0];
                    }
                    else
                    {
                        /* We did not find an mpeg file in our package, so we do not call performScan */
                        [self writeToActivityLog:"no valid mpeg in eyetv package"];
                    }
                }
                /* We check to see if this is a .dvdmedia package */
                else if ([[path pathExtension] isEqualToString: @"dvdmedia"])
                {
                    /* path IS a package - but dvdmedia packages can be treaded like normal directories */
                    browsedSourceDisplayName = [[[path stringByDeletingPathExtension] lastPathComponent] retain];
                    [self writeToActivityLog:"trying to open dvdmedia package"];
                    [self performScan:path scanTitleNum:0];
                }
                else
                {
                    /* The package is not an eyetv package, so we do not call performScan */
                    [self writeToActivityLog:"unable to open package"];
                }
            }
            else // path is not a package, so we treat it as a dvd parent folder or VIDEO_TS folder
            {
                /* path is not a package, so we call perform scan directly on our file */
                if ([[path lastPathComponent] isEqualToString: @"VIDEO_TS"])
                {
                    [self writeToActivityLog:"trying to open video_ts folder (video_ts folder chosen)"];
                    /* If VIDEO_TS Folder is chosen, choose its parent folder for the source display name*/
                    browsedSourceDisplayName = [[[path stringByDeletingLastPathComponent] lastPathComponent] retain];
                }
                else
                {
                    [self writeToActivityLog:"trying to open video_ts folder (parent directory chosen)"];
                    /* if not the VIDEO_TS Folder, we can assume the chosen folder is the source name */
                    /* make sure we remove any path extension as this can also be an '.mpg' file */
                    browsedSourceDisplayName = [[path lastPathComponent] retain];
                }
                [self performScan:path scanTitleNum:0];
            }

        }

    }
}

- (IBAction)showAboutPanel:(id)sender
{
    NSMutableDictionary* d = [[NSMutableDictionary alloc] initWithObjectsAndKeys:
        fApplicationIcon, @"ApplicationIcon",
        nil ];
    [NSApp orderFrontStandardAboutPanelWithOptions:d];
    [d release];
}

/* Here we open the title selection sheet where we can specify an exact title to be scanned */
- (IBAction) showSourceTitleScanPanel: (id) sender
{
    /* We default the title number to be scanned to "0" which results in a full source scan, unless the
    * user changes it
    */
    [fScanSrcTitleNumField setStringValue: @"0"];
	/* Show the panel */
	[NSApp beginSheet:fScanSrcTitlePanel modalForWindow:fWindow modalDelegate:nil didEndSelector:NULL contextInfo:NULL];
}

- (IBAction) closeSourceTitleScanPanel: (id) sender
{
    [NSApp endSheet: fScanSrcTitlePanel];
    [fScanSrcTitlePanel orderOut: self];

    if(sender == fScanSrcTitleOpenButton)
    {
        /* We setup the scan status in the main window to indicate a source title scan */
        [fSrcDVD2Field setStringValue: @"Opening a new source title ..."];
		[fScanIndicator setHidden: NO];
        [fScanIndicator setIndeterminate: YES];
        [fScanIndicator startAnimation: nil];
		
        /* We use the performScan method to actually perform the specified scan passing the path and the title
            * to be scanned
            */
        [self performScan:[fScanSrcTitlePathField stringValue] scanTitleNum:[fScanSrcTitleNumField intValue]];
    }
}

/* Here we actually tell hb_scan to perform the source scan, using the path to source and title number*/
- (void) performScan:(NSString *) scanPath scanTitleNum: (int) scanTitleNum
{
    /* set the bool applyQueueToScan so that we dont apply a queue setting to the final scan */
    applyQueueToScan = NO;
    /* use a bool to determine whether or not we can decrypt using vlc */
    BOOL cancelScanDecrypt = 0;
    NSString *path = scanPath;
    HBDVDDetector *detector = [HBDVDDetector detectorForPath:path];
    
    // Notify ChapterTitles that there's no title
    [fChapterTitlesDelegate resetWithTitle:nil];
    [fChapterTable reloadData];
    
    // Notify Subtitles that there's no title
    [fSubtitlesDelegate resetWithTitle:nil];
    [fSubtitlesTable reloadData];
    
    [self enableUI: NO];
    
    if( [detector isVideoDVD] )
    {
        // The chosen path was actually on a DVD, so use the raw block
        // device path instead.
        path = [detector devicePath];
        [self writeToActivityLog: "trying to open a physical dvd at: %s", [scanPath UTF8String]];
        
#if defined( __LP64__ )
        /* If we are 64 bit, we cannot read encrypted dvd's as vlc is 32 bit only */
        cancelScanDecrypt = 1;
        [self writeToActivityLog: "64 bit mode cannot read dvd's, scan cancelled"];
        /*On Screen Notification*/
        int status;
        NSBeep();
        status = NSRunAlertPanel(@"64-bit HandBrake cannot read encrypted dvds!",@"", @"Cancel Scan", @"Attempt Scan Anyway", nil);
        [NSApp requestUserAttention:NSCriticalRequest];
        
        if (status == NSAlertDefaultReturn)
        {
            /* User chose to cancel the scan */
            [self writeToActivityLog: "cannot open physical dvd , scan cancelled"];
            cancelScanDecrypt = 1;
        }
        else
        {
            [self writeToActivityLog: "user overrode 64-bit warning trying to open physical dvd without decryption"];
            cancelScanDecrypt = 0;
        }

#else
        /* lets check for vlc here to make sure we have a dylib available to use for decrypting */
        NSString *vlcPath = @"/Applications/VLC.app/Contents/MacOS/lib/libdvdcss.2.dylib";
        NSFileManager * fileManager = [NSFileManager defaultManager];
	    if ([fileManager fileExistsAtPath:vlcPath] == 0) 
	    {
            /*vlc not found in /Applications so we set the bool to cancel scanning to 1 */
            cancelScanDecrypt = 1;
            [self writeToActivityLog: "VLC app not found for decrypting physical dvd"];
            int status;
            status = NSRunAlertPanel(@"HandBrake could not find VLC or your VLC is out of date.",@"Please download and install VLC media player in your /Applications folder if you wish to read encrypted DVDs.", @"Get VLC", @"Cancel Scan", @"Attempt Scan Anyway");
            [NSApp requestUserAttention:NSCriticalRequest];
            
            if (status == NSAlertDefaultReturn)
            {
                /* User chose to go download vlc (as they rightfully should) so we send them to the vlc site */
                [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://www.videolan.org/"]];
            }
            else if (status == NSAlertAlternateReturn)
            {
                /* User chose to cancel the scan */
                [self writeToActivityLog: "cannot open physical dvd , scan cancelled"];
            }
            else
            {
                /* User chose to override our warning and scan the physical dvd anyway, at their own peril. on an encrypted dvd this produces massive log files and fails */
                cancelScanDecrypt = 0;
                [self writeToActivityLog: "user overrode vlc warning -trying to open physical dvd without decryption"];
            }
            
        }
        else
        {
            /* VLC was found in /Applications so all is well, we can carry on using vlc's libdvdcss.dylib for decrypting if needed */
            [self writeToActivityLog: "VLC app found for decrypting physical dvd"];
        }
#endif 
    }
    
    if (cancelScanDecrypt == 0)
    {
        /* we actually pass the scan off to libhb here */
        /* If there is no title number passed to scan, we use "0"
         * which causes the default behavior of a full source scan
         */
        if (!scanTitleNum)
        {
            scanTitleNum = 0;
        }
        if (scanTitleNum > 0)
        {
            [self writeToActivityLog: "scanning specifically for title: %d", scanTitleNum];
        }
        /* We use our advance pref to determine how many previews to scan */
        int hb_num_previews = [[[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewsNumber"] intValue];
        /* set title to NULL */
        //fTitle = NULL;
        hb_scan( fHandle, [path UTF8String], scanTitleNum, hb_num_previews, 1 );
        [fSrcDVD2Field setStringValue:@"Scanning new source ..."];
    }
}

- (IBAction) showNewScan:(id)sender
{
    hb_list_t  * list;
	hb_title_t * title;
	int indxpri=0; 	  // Used to search the longuest title (default in combobox)
	int longuestpri=0; // Used to search the longuest title (default in combobox)
    

        list = hb_get_titles( fHandle );
        
        if( !hb_list_count( list ) )
        {
            /* We display a message if a valid dvd source was not chosen */
            [fSrcDVD2Field setStringValue: @"No Valid Source Found"];
            SuccessfulScan = NO;
            
            // Notify ChapterTitles that there's no title
            [fSubtitlesDelegate resetWithTitle:nil];
            [fSubtitlesTable reloadData];
            
            // Notify Subtitles that there's no title
            [fChapterTitlesDelegate resetWithTitle:nil];
            [fChapterTable reloadData];
        }
        else
        {
            /* We increment the successful scancount here by one,
             which we use at the end of this function to tell the gui
             if this is the first successful scan since launch and whether
             or not we should set all settings to the defaults */
            
            currentSuccessfulScanCount++;
            
            [[fWindow toolbar] validateVisibleItems];
            
            [fSrcTitlePopUp removeAllItems];
            for( int i = 0; i < hb_list_count( list ); i++ )
            {
                title = (hb_title_t *) hb_list_item( list, i );
                
                currentSource = [NSString stringWithUTF8String: title->name];
                /*Set DVD Name at top of window with the browsedSourceDisplayName grokked right before -performScan */
                [fSrcDVD2Field setStringValue:browsedSourceDisplayName];
                
                /* Use the dvd name in the default output field here
                 May want to add code to remove blank spaces for some dvd names*/
                /* Check to see if the last destination has been set,use if so, if not, use Desktop */
                if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastDestinationDirectory"])
                {
                    [fDstFile2Field setStringValue: [NSString stringWithFormat:
                                                     @"%@/%@.mp4", [[NSUserDefaults standardUserDefaults] stringForKey:@"LastDestinationDirectory"],[browsedSourceDisplayName stringByDeletingPathExtension]]];
                }
                else
                {
                    [fDstFile2Field setStringValue: [NSString stringWithFormat:
                                                     @"%@/Desktop/%@.mp4", NSHomeDirectory(),[browsedSourceDisplayName stringByDeletingPathExtension]]];
                }
                
                
                if (longuestpri < title->hours*60*60 + title->minutes *60 + title->seconds)
                {
                    longuestpri=title->hours*60*60 + title->minutes *60 + title->seconds;
                    indxpri=i;
                }
                
                [fSrcTitlePopUp addItemWithTitle: [NSString
                                                   stringWithFormat: @"%d - %02dh%02dm%02ds",
                                                   title->index, title->hours, title->minutes,
                                                   title->seconds]];
            }
            
            // Select the longuest title
            [fSrcTitlePopUp selectItemAtIndex: indxpri];
            [self titlePopUpChanged:nil];
            
            SuccessfulScan = YES;
            [self enableUI: YES];

            /* if its the initial successful scan after awakeFromNib */
            if (currentSuccessfulScanCount == 1)
            {
                [self selectDefaultPreset:nil];
                
                // Open preview window now if it was visible when HB was closed
                if ([[NSUserDefaults standardUserDefaults] boolForKey:@"PreviewWindowIsOpen"])
                    [self showPreviewWindow:nil];
                
                // Open picture sizing window now if it was visible when HB was closed
                if ([[NSUserDefaults standardUserDefaults] boolForKey:@"PictureSizeWindowIsOpen"])
                    [self showPicturePanel:nil];
                
            }

            
        }

}


#pragma mark -
#pragma mark New Output Destination

- (IBAction) browseFile: (id) sender
{
    /* Open a panel to let the user choose and update the text field */
    NSSavePanel * panel = [NSSavePanel savePanel];
	/* We get the current file name and path from the destination field here */
	[panel beginSheetForDirectory: [[fDstFile2Field stringValue] stringByDeletingLastPathComponent] file: [[fDstFile2Field stringValue] lastPathComponent]
				   modalForWindow: fWindow modalDelegate: self
				   didEndSelector: @selector( browseFileDone:returnCode:contextInfo: )
					  contextInfo: NULL];
}

- (void) browseFileDone: (NSSavePanel *) sheet
             returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        [fDstFile2Field setStringValue: [sheet filename]];
        /* Save this path to the prefs so that on next browse destination window it opens there */
        NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];   
    }
}


#pragma mark -
#pragma mark Main Window Control

- (IBAction) openMainWindow: (id) sender
{
    [fWindow  makeKeyAndOrderFront:nil];
}

- (BOOL) windowShouldClose: (id) sender
{
    return YES;
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag
{
    if( !flag ) {
        [fWindow  makeKeyAndOrderFront:nil];
                
        return YES;
    }
    
    return NO;
}

- (NSSize) drawerWillResizeContents:(NSDrawer *) drawer toSize:(NSSize) contentSize {
	[[NSUserDefaults standardUserDefaults] setObject:NSStringFromSize( contentSize ) forKey:@"Drawer Size"];
	return contentSize;
}

#pragma mark -
#pragma mark Queue File

- (void) loadQueueFile {
	/* We declare the default NSFileManager into fileManager */
	NSFileManager * fileManager = [NSFileManager defaultManager];
	/*We define the location of the user presets file */
    QueueFile = @"~/Library/Application Support/HandBrake/Queue.plist";
	QueueFile = [[QueueFile stringByExpandingTildeInPath]retain];
    /* We check for the presets.plist */
	if ([fileManager fileExistsAtPath:QueueFile] == 0)
	{
		[fileManager createFileAtPath:QueueFile contents:nil attributes:nil];
	}

	QueueFileArray = [[NSMutableArray alloc] initWithContentsOfFile:QueueFile];
	/* lets check to see if there is anything in the queue file .plist */
    if (nil == QueueFileArray)
	{
        /* if not, then lets initialize an empty array */
		QueueFileArray = [[NSMutableArray alloc] init];
        
     /* Initialize our curQueueEncodeIndex to 0
     * so we can use it to track which queue
     * item is to be used to track our encodes */
     /* NOTE: this should be changed if and when we
      * are able to get the last unfinished encode
      * in the case of a crash or shutdown */
    
	}
    else
    {
    [self clearQueueEncodedItems];
    }
    currentQueueEncodeIndex = 0;
}

- (void)addQueueFileItem
{
        [QueueFileArray addObject:[self createQueueFileItem]];
        [self saveQueueFileItem];

}

- (void) removeQueueFileItem:(int) queueItemToRemove
{
   
   /* Find out if the item we are removing is a cancelled (3) or a finished (0) item*/
   if ([[[QueueFileArray objectAtIndex:queueItemToRemove] objectForKey:@"Status"] intValue] == 3 || [[[QueueFileArray objectAtIndex:queueItemToRemove] objectForKey:@"Status"] intValue] == 0)
    {
    /* Since we are removing a cancelled or finished item, WE need to decrement the currentQueueEncodeIndex
     * by one to keep in sync with the queue array
     */
    currentQueueEncodeIndex--;
    [self writeToActivityLog: "removeQueueFileItem: Removing a cancelled/finished encode, decrement currentQueueEncodeIndex to %d", currentQueueEncodeIndex];
    }
    [QueueFileArray removeObjectAtIndex:queueItemToRemove];
    [self saveQueueFileItem];

}

- (void)saveQueueFileItem
{
    [QueueFileArray writeToFile:QueueFile atomically:YES];
    [fQueueController setQueueArray: QueueFileArray];
    [self getQueueStats];
}

- (void)getQueueStats
{
/* lets get the stats on the status of the queue array */

fEncodingQueueItem = 0;
fPendingCount = 0;
fCompletedCount = 0;
fCanceledCount = 0;
fWorkingCount = 0;

    /* We use a number system to set the encode status of the queue item
     * in controller.mm
     * 0 == already encoded
     * 1 == is being encoded
     * 2 == is yet to be encoded
     * 3 == cancelled
     */

	int i = 0;
    NSEnumerator *enumerator = [QueueFileArray objectEnumerator];
	id tempObject;
	while (tempObject = [enumerator nextObject])
	{
		NSDictionary *thisQueueDict = tempObject;
		if ([[thisQueueDict objectForKey:@"Status"] intValue] == 0) // Completed
		{
			fCompletedCount++;	
		}
		if ([[thisQueueDict objectForKey:@"Status"] intValue] == 1) // being encoded
		{
			fWorkingCount++;
            fEncodingQueueItem = i;	
		}
        if ([[thisQueueDict objectForKey:@"Status"] intValue] == 2) // pending		
        {
			fPendingCount++;
		}
        if ([[thisQueueDict objectForKey:@"Status"] intValue] == 3) // cancelled		
        {
			fCanceledCount++;
		}
		i++;
	}

    /* Set the queue status field in the main window */
    NSMutableString * string;
    if (fPendingCount == 1)
    {
        string = [NSMutableString stringWithFormat: NSLocalizedString( @"%d encode pending in the queue", @"" ), fPendingCount];
    }
    else
    {
        string = [NSMutableString stringWithFormat: NSLocalizedString( @"%d encode(s) pending in the queue", @"" ), fPendingCount];
    }
    [fQueueStatus setStringValue:string];
}

/* This method will set any item marked as encoding back to pending
 * currently used right after a queue reload
 */
- (void) setQueueEncodingItemsAsPending
{
    NSEnumerator *enumerator = [QueueFileArray objectEnumerator];
	id tempObject;
    NSMutableArray *tempArray;
    tempArray = [NSMutableArray array];
    /* we look here to see if the preset is we move on to the next one */
    while ( tempObject = [enumerator nextObject] )  
    {
        /* If the queue item is marked as "encoding" (1)
         * then change its status back to pending (2) which effectively
         * puts it back into the queue to be encoded
         */
        if ([[tempObject objectForKey:@"Status"] intValue] == 1)
        {
            [tempObject setObject:[NSNumber numberWithInt: 2] forKey:@"Status"];
        }
        [tempArray addObject:tempObject];
    }
    
    [QueueFileArray setArray:tempArray];
    [self saveQueueFileItem];
}


/* This method will clear the queue of any encodes that are not still pending
 * this includes both successfully completed encodes as well as cancelled encodes */
- (void) clearQueueEncodedItems
{
    NSEnumerator *enumerator = [QueueFileArray objectEnumerator];
	id tempObject;
    NSMutableArray *tempArray;
    tempArray = [NSMutableArray array];
    /* we look here to see if the preset is we move on to the next one */
    while ( tempObject = [enumerator nextObject] )  
    {
        /* If the queue item is either completed (0) or cancelled (3) from the
         * last session, then we put it in tempArray to be deleted from QueueFileArray.
         * NOTE: this means we retain pending (2) and also an item that is marked as
         * still encoding (1). If the queue has an item that is still marked as encoding
         * from a previous session, we can conlude that HB was either shutdown, or crashed
         * during the encodes so we keep it and tell the user in the "Load Queue Alert"
         */
        if ([[tempObject objectForKey:@"Status"] intValue] == 0 || [[tempObject objectForKey:@"Status"] intValue] == 3)
        {
            [tempArray addObject:tempObject];
        }
    }
    
    [QueueFileArray removeObjectsInArray:tempArray];
    [self saveQueueFileItem];
}

/* This method will clear the queue of all encodes. effectively creating an empty queue */
- (void) clearQueueAllItems
{
    NSEnumerator *enumerator = [QueueFileArray objectEnumerator];
	id tempObject;
    NSMutableArray *tempArray;
    tempArray = [NSMutableArray array];
    /* we look here to see if the preset is we move on to the next one */
    while ( tempObject = [enumerator nextObject] )  
    {
        [tempArray addObject:tempObject];
    }
    
    [QueueFileArray removeObjectsInArray:tempArray];
    [self saveQueueFileItem];
}

/* This method will duplicate prepareJob however into the
 * queue .plist instead of into the job structure so it can
 * be recalled later */
- (NSDictionary *)createQueueFileItem
{
    NSMutableDictionary *queueFileJob = [[NSMutableDictionary alloc] init];
    
       hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;
    
    
    
    /* We use a number system to set the encode status of the queue item
     * 0 == already encoded
     * 1 == is being encoded
     * 2 == is yet to be encoded
     * 3 == cancelled
     */
    [queueFileJob setObject:[NSNumber numberWithInt:2] forKey:@"Status"];
    /* Source and Destination Information */
    
    [queueFileJob setObject:[NSString stringWithUTF8String: title->dvd] forKey:@"SourcePath"];
    [queueFileJob setObject:[fSrcDVD2Field stringValue] forKey:@"SourceName"];
    [queueFileJob setObject:[NSNumber numberWithInt:title->index] forKey:@"TitleNumber"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcAnglePopUp indexOfSelectedItem] + 1] forKey:@"TitleAngle"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcChapterStartPopUp indexOfSelectedItem] + 1] forKey:@"ChapterStart"];
    
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcChapterEndPopUp indexOfSelectedItem] + 1] forKey:@"ChapterEnd"];
    
    [queueFileJob setObject:[fDstFile2Field stringValue] forKey:@"DestinationPath"];
    
    /* Lets get the preset info if there is any */
    [queueFileJob setObject:[fPresetSelectedDisplay stringValue] forKey:@"PresetName"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fPresetsOutlineView selectedRow]] forKey:@"PresetIndexNum"];
    
    [queueFileJob setObject:[fDstFormatPopUp titleOfSelectedItem] forKey:@"FileFormat"];
    /* Chapter Markers*/
    /* If we have only one chapter or a title without chapters, set chapter markers to off */
    if ([fSrcChapterStartPopUp indexOfSelectedItem] ==  [fSrcChapterEndPopUp indexOfSelectedItem])
    {
        [queueFileJob setObject:[NSNumber numberWithInt:0] forKey:@"ChapterMarkers"];
    }
    else
    {
        [queueFileJob setObject:[NSNumber numberWithInt:[fCreateChapterMarkers state]] forKey:@"ChapterMarkers"];
    }
	
    /* We need to get the list of chapter names to put into an array and store 
     * in our queue, so they can be reapplied in prepareJob when this queue
     * item comes up if Chapter Markers is set to on.
     */
     int i;
     NSMutableArray *ChapterNamesArray = [[NSMutableArray alloc] init];
     int chaptercount = hb_list_count( fTitle->list_chapter );
     for( i = 0; i < chaptercount; i++ )
    {
        hb_chapter_t *chapter = (hb_chapter_t *) hb_list_item( fTitle->list_chapter, i );
        if( chapter != NULL )
        {
          [ChapterNamesArray addObject:[NSString stringWithCString:chapter->title encoding:NSUTF8StringEncoding]];
        }
    }
    [queueFileJob setObject:[NSMutableArray arrayWithArray: ChapterNamesArray] forKey:@"ChapterNames"];
    [ChapterNamesArray autorelease];
    
    /* Allow Mpeg4 64 bit formatting +4GB file sizes */
	[queueFileJob setObject:[NSNumber numberWithInt:[fDstMp4LargeFileCheck state]] forKey:@"Mp4LargeFile"];
    /* Mux mp4 with http optimization */
    [queueFileJob setObject:[NSNumber numberWithInt:[fDstMp4HttpOptFileCheck state]] forKey:@"Mp4HttpOptimize"];
    /* Add iPod uuid atom */
    [queueFileJob setObject:[NSNumber numberWithInt:[fDstMp4iPodFileCheck state]] forKey:@"Mp4iPodCompatible"];
    
    /* Codecs */
	/* Video encoder */
	[queueFileJob setObject:[fVidEncoderPopUp titleOfSelectedItem] forKey:@"VideoEncoder"];
	/* x264 Option String */
	[queueFileJob setObject:[fAdvancedOptions optionsString] forKey:@"x264Option"];

	[queueFileJob setObject:[NSNumber numberWithInt:[fVidQualityMatrix selectedRow]] forKey:@"VideoQualityType"];
	[queueFileJob setObject:[fVidTargetSizeField stringValue] forKey:@"VideoTargetSize"];
	[queueFileJob setObject:[fVidBitrateField stringValue] forKey:@"VideoAvgBitrate"];
	[queueFileJob setObject:[NSNumber numberWithFloat:[fVidQualityRFField floatValue]] forKey:@"VideoQualitySlider"];
    /* Framerate */
    [queueFileJob setObject:[fVidRatePopUp titleOfSelectedItem] forKey:@"VideoFramerate"];
    
	/* 2 Pass Encoding */
	[queueFileJob setObject:[NSNumber numberWithInt:[fVidTwoPassCheck state]] forKey:@"VideoTwoPass"];
	/* Turbo 2 pass Encoding fVidTurboPassCheck*/
	[queueFileJob setObject:[NSNumber numberWithInt:[fVidTurboPassCheck state]] forKey:@"VideoTurboTwoPass"];
    
	/* Picture Sizing */
	/* Use Max Picture settings for whatever the dvd is.*/
	[queueFileJob setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->width] forKey:@"PictureWidth"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->height] forKey:@"PictureHeight"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->keep_ratio] forKey:@"PictureKeepRatio"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.mode] forKey:@"PicturePAR"];
    /* if we are custom anamorphic, store the exact storage, par and display dims */
    if (fTitle->job->anamorphic.mode == 3)
    {
        [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->width] forKey:@"PicturePARStorageWidth"];
        [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->height] forKey:@"PicturePARStorageHeight"];
        
        [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.par_width] forKey:@"PicturePARPixelWidth"];
        [queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.par_height] forKey:@"PicturePARPixelHeight"];
        
        [queueFileJob setObject:[NSNumber numberWithFloat:fTitle->job->anamorphic.dar_width] forKey:@"PicturePARDisplayWidth"];
        [queueFileJob setObject:[NSNumber numberWithFloat:fTitle->job->anamorphic.dar_height] forKey:@"PicturePARDisplayHeight"];

    }
    NSString * pictureSummary;
    pictureSummary = [fPictureSizeField stringValue];
    [queueFileJob setObject:pictureSummary forKey:@"PictureSizingSummary"];                 
    /* Set crop settings here */
	[queueFileJob setObject:[NSNumber numberWithInt:[fPictureController autoCrop]] forKey:@"PictureAutoCrop"];
    [queueFileJob setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
    [queueFileJob setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
	[queueFileJob setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
	[queueFileJob setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
    
    /* Picture Filters */
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController detelecine]] forKey:@"PictureDetelecine"];
    [queueFileJob setObject:[fPictureController detelecineCustomString] forKey:@"PictureDetelecineCustom"];
    
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController useDecomb]] forKey:@"PictureDecombDeinterlace"];
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController decomb]] forKey:@"PictureDecomb"];
    [queueFileJob setObject:[fPictureController decombCustomString] forKey:@"PictureDecombCustom"];
    
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController deinterlace]] forKey:@"PictureDeinterlace"];
    [queueFileJob setObject:[fPictureController deinterlaceCustomString] forKey:@"PictureDeinterlaceCustom"];
    
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController denoise]] forKey:@"PictureDenoise"];
    [queueFileJob setObject:[fPictureController denoiseCustomString] forKey:@"PictureDenoiseCustom"];
    
    [queueFileJob setObject:[NSString stringWithFormat:@"%d",[fPictureController deblock]] forKey:@"PictureDeblock"];
    
    [queueFileJob setObject:[NSNumber numberWithInt:[fPictureController grayscale]] forKey:@"VideoGrayScale"];
    
    /*Audio*/
    if ([fAudLang1PopUp indexOfSelectedItem] > 0)
    {
        [queueFileJob setObject:[NSNumber numberWithInt:[fAudLang1PopUp indexOfSelectedItem]] forKey:@"Audio1Track"];
        [queueFileJob setObject:[fAudLang1PopUp titleOfSelectedItem] forKey:@"Audio1TrackDescription"];
        [queueFileJob setObject:[fAudTrack1CodecPopUp titleOfSelectedItem] forKey:@"Audio1Encoder"];
        [queueFileJob setObject:[fAudTrack1MixPopUp titleOfSelectedItem] forKey:@"Audio1Mixdown"];
        [queueFileJob setObject:[fAudTrack1RatePopUp titleOfSelectedItem] forKey:@"Audio1Samplerate"];
        [queueFileJob setObject:[fAudTrack1BitratePopUp titleOfSelectedItem] forKey:@"Audio1Bitrate"];
        [queueFileJob setObject:[NSNumber numberWithFloat:[fAudTrack1DrcSlider floatValue]] forKey:@"Audio1TrackDRCSlider"];
    }
    if ([fAudLang2PopUp indexOfSelectedItem] > 0)
    {
        [queueFileJob setObject:[NSNumber numberWithInt:[fAudLang2PopUp indexOfSelectedItem]] forKey:@"Audio2Track"];
        [queueFileJob setObject:[fAudLang2PopUp titleOfSelectedItem] forKey:@"Audio2TrackDescription"];
        [queueFileJob setObject:[fAudTrack2CodecPopUp titleOfSelectedItem] forKey:@"Audio2Encoder"];
        [queueFileJob setObject:[fAudTrack2MixPopUp titleOfSelectedItem] forKey:@"Audio2Mixdown"];
        [queueFileJob setObject:[fAudTrack2RatePopUp titleOfSelectedItem] forKey:@"Audio2Samplerate"];
        [queueFileJob setObject:[fAudTrack2BitratePopUp titleOfSelectedItem] forKey:@"Audio2Bitrate"];
        [queueFileJob setObject:[NSNumber numberWithFloat:[fAudTrack2DrcSlider floatValue]] forKey:@"Audio2TrackDRCSlider"];
    }
    if ([fAudLang3PopUp indexOfSelectedItem] > 0)
    {
        [queueFileJob setObject:[NSNumber numberWithInt:[fAudLang3PopUp indexOfSelectedItem]] forKey:@"Audio3Track"];
        [queueFileJob setObject:[fAudLang3PopUp titleOfSelectedItem] forKey:@"Audio3TrackDescription"];
        [queueFileJob setObject:[fAudTrack3CodecPopUp titleOfSelectedItem] forKey:@"Audio3Encoder"];
        [queueFileJob setObject:[fAudTrack3MixPopUp titleOfSelectedItem] forKey:@"Audio3Mixdown"];
        [queueFileJob setObject:[fAudTrack3RatePopUp titleOfSelectedItem] forKey:@"Audio3Samplerate"];
        [queueFileJob setObject:[fAudTrack3BitratePopUp titleOfSelectedItem] forKey:@"Audio3Bitrate"];
        [queueFileJob setObject:[NSNumber numberWithFloat:[fAudTrack3DrcSlider floatValue]] forKey:@"Audio3TrackDRCSlider"];
    }
    if ([fAudLang4PopUp indexOfSelectedItem] > 0)
    {
        [queueFileJob setObject:[NSNumber numberWithInt:[fAudLang4PopUp indexOfSelectedItem]] forKey:@"Audio4Track"];
        [queueFileJob setObject:[fAudLang4PopUp titleOfSelectedItem] forKey:@"Audio4TrackDescription"];
        [queueFileJob setObject:[fAudTrack4CodecPopUp titleOfSelectedItem] forKey:@"Audio4Encoder"];
        [queueFileJob setObject:[fAudTrack4MixPopUp titleOfSelectedItem] forKey:@"Audio4Mixdown"];
        [queueFileJob setObject:[fAudTrack4RatePopUp titleOfSelectedItem] forKey:@"Audio4Samplerate"];
        [queueFileJob setObject:[fAudTrack4BitratePopUp titleOfSelectedItem] forKey:@"Audio4Bitrate"];
        [queueFileJob setObject:[NSNumber numberWithFloat:[fAudTrack4DrcSlider floatValue]] forKey:@"Audio4TrackDRCSlider"];
    }
    
	/* Subtitles*/
    NSMutableArray *subtitlesArray = [[NSMutableArray alloc] init];
    [queueFileJob setObject:[NSArray arrayWithArray: [fSubtitlesDelegate getSubtitleArray: subtitlesArray]] forKey:@"SubtitleList"];
    [subtitlesArray autorelease];

    /* Now we go ahead and set the "job->values in the plist for passing right to fQueueEncodeLibhb */
     
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcChapterStartPopUp indexOfSelectedItem] + 1] forKey:@"JobChapterStart"];
    
    [queueFileJob setObject:[NSNumber numberWithInt:[fSrcChapterEndPopUp indexOfSelectedItem] + 1] forKey:@"JobChapterEnd"];
    
    
    [queueFileJob setObject:[NSNumber numberWithInt:[[fDstFormatPopUp selectedItem] tag]] forKey:@"JobFileFormatMux"];
    
    /* Codecs */
	/* Video encoder */
	[queueFileJob setObject:[NSNumber numberWithInt:[[fVidEncoderPopUp selectedItem] tag]] forKey:@"JobVideoEncoderVcodec"];
	
    /* Framerate */
    [queueFileJob setObject:[NSNumber numberWithInt:[fVidRatePopUp indexOfSelectedItem]] forKey:@"JobIndexVideoFramerate"];
    [queueFileJob setObject:[NSNumber numberWithInt:title->rate] forKey:@"JobVrate"];
    [queueFileJob setObject:[NSNumber numberWithInt:title->rate_base] forKey:@"JobVrateBase"];
	
    /* Picture Sizing */
	/* Use Max Picture settings for whatever the dvd is.*/
	[queueFileJob setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->width] forKey:@"PictureWidth"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->height] forKey:@"PictureHeight"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->keep_ratio] forKey:@"PictureKeepRatio"];
	[queueFileJob setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.mode] forKey:@"PicturePAR"];
    
    /* Set crop settings here */
	[queueFileJob setObject:[NSNumber numberWithInt:[fPictureController autoCrop]] forKey:@"PictureAutoCrop"];
    [queueFileJob setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
    [queueFileJob setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
	[queueFileJob setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
	[queueFileJob setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
    
    
    /*Audio*/
    if ([fAudLang1PopUp indexOfSelectedItem] > 0)
    {
        //[queueFileJob setObject:[fAudTrack1CodecPopUp indexOfSelectedItem] forKey:@"JobAudio1Encoder"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack1CodecPopUp selectedItem] tag]] forKey:@"JobAudio1Encoder"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack1MixPopUp selectedItem] tag]] forKey:@"JobAudio1Mixdown"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack1RatePopUp selectedItem] tag]] forKey:@"JobAudio1Samplerate"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack1BitratePopUp selectedItem] tag]] forKey:@"JobAudio1Bitrate"];
     }
    if ([fAudLang2PopUp indexOfSelectedItem] > 0)
    {
        //[queueFileJob setObject:[fAudTrack1CodecPopUp indexOfSelectedItem] forKey:@"JobAudio2Encoder"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack2CodecPopUp selectedItem] tag]] forKey:@"JobAudio2Encoder"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack2MixPopUp selectedItem] tag]] forKey:@"JobAudio2Mixdown"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack2RatePopUp selectedItem] tag]] forKey:@"JobAudio2Samplerate"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack2BitratePopUp selectedItem] tag]] forKey:@"JobAudio2Bitrate"];
    }
    if ([fAudLang3PopUp indexOfSelectedItem] > 0)
    {
        //[queueFileJob setObject:[fAudTrack1CodecPopUp indexOfSelectedItem] forKey:@"JobAudio3Encoder"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack3CodecPopUp selectedItem] tag]] forKey:@"JobAudio3Encoder"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack3MixPopUp selectedItem] tag]] forKey:@"JobAudio3Mixdown"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack3RatePopUp selectedItem] tag]] forKey:@"JobAudio3Samplerate"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack3BitratePopUp selectedItem] tag]] forKey:@"JobAudio3Bitrate"];
    }
    if ([fAudLang4PopUp indexOfSelectedItem] > 0)
    {
        //[queueFileJob setObject:[fAudTrack1CodecPopUp indexOfSelectedItem] forKey:@"JobAudio4Encoder"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack4CodecPopUp selectedItem] tag]] forKey:@"JobAudio4Encoder"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack4MixPopUp selectedItem] tag]] forKey:@"JobAudio4Mixdown"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack4RatePopUp selectedItem] tag]] forKey:@"JobAudio4Samplerate"];
        [queueFileJob setObject:[NSNumber numberWithInt:[[fAudTrack4BitratePopUp selectedItem] tag]] forKey:@"JobAudio4Bitrate"];
    }

 
    /* we need to auto relase the queueFileJob and return it */
    [queueFileJob autorelease];
    return queueFileJob;

}

/* this is actually called from the queue controller to modify the queue array and return it back to the queue controller */
- (void)moveObjectsInQueueArray:(NSMutableArray *)array fromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex
{
    NSUInteger index = [indexSet lastIndex];
    NSUInteger aboveInsertIndexCount = 0;
    
    
    NSUInteger removeIndex;
        
    if (index >= insertIndex)
    {
        removeIndex = index + aboveInsertIndexCount;
        aboveInsertIndexCount++;
    }
    else
    {
        removeIndex = index;
        insertIndex--;
    }

    id object = [[QueueFileArray objectAtIndex:removeIndex] retain];
    [QueueFileArray removeObjectAtIndex:removeIndex];
    [QueueFileArray insertObject:object atIndex:insertIndex];
    [object release];
        
    index = [indexSet indexLessThanIndex:index];

   /* We save all of the Queue data here 
    * and it also gets sent back to the queue controller*/
    [self saveQueueFileItem]; 
    
}


#pragma mark -
#pragma mark Queue Job Processing

- (void) incrementQueueItemDone:(int) queueItemDoneIndexNum
{
    int i = currentQueueEncodeIndex;
    [[QueueFileArray objectAtIndex:i] setObject:[NSNumber numberWithInt:0] forKey:@"Status"];
	
    /* We save all of the Queue data here */
    [self saveQueueFileItem];
	/* We Reload the New Table data for presets */
    //[fPresetsOutlineView reloadData];

    /* Since we have now marked a queue item as done
     * we can go ahead and increment currentQueueEncodeIndex 
     * so that if there is anything left in the queue we can
     * go ahead and move to the next item if we want to */
    currentQueueEncodeIndex++ ;
    [self writeToActivityLog: "incrementQueueItemDone currentQueueEncodeIndex is incremented to: %d", currentQueueEncodeIndex];
    int queueItems = [QueueFileArray count];
    /* If we still have more items in our queue, lets go to the next one */
    if (currentQueueEncodeIndex < queueItems)
    {
    [self writeToActivityLog: "incrementQueueItemDone currentQueueEncodeIndex is incremented to: %d", currentQueueEncodeIndex];
    [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]];
    }
    else
    {
        [self writeToActivityLog: "incrementQueueItemDone the %d item queue is complete", currentQueueEncodeIndex - 1];
    }
}

/* Here we actually tell hb_scan to perform the source scan, using the path to source and title number*/
- (void) performNewQueueScan:(NSString *) scanPath scanTitleNum: (int) scanTitleNum
{
   /* Tell HB to output a new activity log file for this encode */
    [outputPanel startEncodeLog:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"DestinationPath"]];
    
    
     /* use a bool to determine whether or not we can decrypt using vlc */
    BOOL cancelScanDecrypt = 0;
    /* set the bool so that showNewScan knows to apply the appropriate queue
    * settings as this is a queue rescan
    */
    applyQueueToScan = YES;
    NSString *path = scanPath;
    HBDVDDetector *detector = [HBDVDDetector detectorForPath:path];

        /*On Screen Notification*/
        //int status;
        //status = NSRunAlertPanel(@"HandBrake is now loading up a new queue item...",@"Would You Like to wait until you add another encode?", @"Cancel", @"Okay", nil);
        //[NSApp requestUserAttention:NSCriticalRequest];

    if( [detector isVideoDVD] )
    {
        // The chosen path was actually on a DVD, so use the raw block
        // device path instead.
        path = [detector devicePath];
        [self writeToActivityLog: "trying to open a physical dvd at: %s", [scanPath UTF8String]];

        /* lets check for vlc here to make sure we have a dylib available to use for decrypting */
        NSString *vlcPath = @"/Applications/VLC.app";
        NSFileManager * fileManager = [NSFileManager defaultManager];
	    if ([fileManager fileExistsAtPath:vlcPath] == 0) 
	    {
            /*vlc not found in /Applications so we set the bool to cancel scanning to 1 */
            cancelScanDecrypt = 1;
            [self writeToActivityLog: "VLC app not found for decrypting physical dvd"];
            int status;
            status = NSRunAlertPanel(@"HandBrake could not find VLC.",@"Please download and install VLC media player in your /Applications folder if you wish to read encrypted DVDs.", @"Get VLC", @"Cancel Scan", @"Attempt Scan Anyway");
            [NSApp requestUserAttention:NSCriticalRequest];
            
            if (status == NSAlertDefaultReturn)
            {
                /* User chose to go download vlc (as they rightfully should) so we send them to the vlc site */
                [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://www.videolan.org/"]];
            }
            else if (status == NSAlertAlternateReturn)
            {
            /* User chose to cancel the scan */
            [self writeToActivityLog: "cannot open physical dvd , scan cancelled"];
            }
            else
            {
            /* User chose to override our warning and scan the physical dvd anyway, at their own peril. on an encrypted dvd this produces massive log files and fails */
            cancelScanDecrypt = 0;
            [self writeToActivityLog: "user overrode vlc warning -trying to open physical dvd without decryption"];
            }

        }
        else
        {
            /* VLC was found in /Applications so all is well, we can carry on using vlc's libdvdcss.dylib for decrypting if needed */
            [self writeToActivityLog: "VLC app found for decrypting physical dvd"];
        }
    }

    if (cancelScanDecrypt == 0)
    {
        /* we actually pass the scan off to libhb here */
        /* If there is no title number passed to scan, we use "0"
         * which causes the default behavior of a full source scan
         */
        if (!scanTitleNum)
        {
            scanTitleNum = 0;
        }
        if (scanTitleNum > 0)
        {
            [self writeToActivityLog: "scanning specifically for title: %d", scanTitleNum];
        }
        
        [self writeToActivityLog: "performNewQueueScan currentQueueEncodeIndex is: %d", currentQueueEncodeIndex];
        /* We use our advance pref to determine how many previews to scan */
        int hb_num_previews = [[[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewsNumber"] intValue];
        hb_scan( fQueueEncodeLibhb, [path UTF8String], scanTitleNum, hb_num_previews, 0 );
    }
}

/* This method was originally used to load up a new queue item in the gui and
 * then start processing it. However we now have modified -prepareJob and use a second
 * instance of libhb to do our actual encoding, therefor right now it is not required. 
 * Nonetheless I want to leave this in here
 * because basically its everything we need to be able to actually modify a pending queue
 * item in the gui and resave it. At least for now - dynaflash
 */

- (IBAction)applyQueueSettings:(id)sender
{
    NSMutableDictionary * queueToApply = [QueueFileArray objectAtIndex:currentQueueEncodeIndex];
    hb_job_t * job = fTitle->job;
    
    /* Set title number and chapters */
    /* since the queue only scans a single title, we really don't need to pick a title */
    //[fSrcTitlePopUp selectItemAtIndex: [[queueToApply objectForKey:@"TitleNumber"] intValue] - 1];
    
    [fSrcChapterStartPopUp selectItemAtIndex: [[queueToApply objectForKey:@"ChapterStart"] intValue] - 1];
    [fSrcChapterEndPopUp selectItemAtIndex: [[queueToApply objectForKey:@"ChapterEnd"] intValue] - 1];
    
    /* File Format */
    [fDstFormatPopUp selectItemWithTitle:[queueToApply objectForKey:@"FileFormat"]];
    [self formatPopUpChanged:nil];
    
    /* Chapter Markers*/
    [fCreateChapterMarkers setState:[[queueToApply objectForKey:@"ChapterMarkers"] intValue]];
    /* Allow Mpeg4 64 bit formatting +4GB file sizes */
    [fDstMp4LargeFileCheck setState:[[queueToApply objectForKey:@"Mp4LargeFile"] intValue]];
    /* Mux mp4 with http optimization */
    [fDstMp4HttpOptFileCheck setState:[[queueToApply objectForKey:@"Mp4HttpOptimize"] intValue]];
    
    /* Video encoder */
    /* We set the advanced opt string here if applicable*/
    [fVidEncoderPopUp selectItemWithTitle:[queueToApply objectForKey:@"VideoEncoder"]];
    [fAdvancedOptions setOptions:[queueToApply objectForKey:@"x264Option"]];
    
    /* Lets run through the following functions to get variables set there */
    [self videoEncoderPopUpChanged:nil];
    /* Set the state of ipod compatible with Mp4iPodCompatible. Only for x264*/
    [fDstMp4iPodFileCheck setState:[[queueToApply objectForKey:@"Mp4iPodCompatible"] intValue]];
    [self calculateBitrate:nil];
    
    /* Video quality */
    [fVidQualityMatrix selectCellAtRow:[[queueToApply objectForKey:@"VideoQualityType"] intValue] column:0];
    
    [fVidTargetSizeField setStringValue:[queueToApply objectForKey:@"VideoTargetSize"]];
    [fVidBitrateField setStringValue:[queueToApply objectForKey:@"VideoAvgBitrate"]];
    [fVidQualitySlider setFloatValue:[[queueToApply objectForKey:@"VideoQualitySlider"] floatValue]];
    
    [self videoMatrixChanged:nil];
    
    /* Video framerate */
    /* For video preset video framerate, we want to make sure that Same as source does not conflict with the
     detected framerate in the fVidRatePopUp so we use index 0*/
    if ([[queueToApply objectForKey:@"VideoFramerate"] isEqualToString:@"Same as source"])
    {
        [fVidRatePopUp selectItemAtIndex: 0];
    }
    else
    {
        [fVidRatePopUp selectItemWithTitle:[queueToApply objectForKey:@"VideoFramerate"]];
    }
    
    /* 2 Pass Encoding */
    [fVidTwoPassCheck setState:[[queueToApply objectForKey:@"VideoTwoPass"] intValue]];
    [self twoPassCheckboxChanged:nil];
    /* Turbo 1st pass for 2 Pass Encoding */
    [fVidTurboPassCheck setState:[[queueToApply objectForKey:@"VideoTurboTwoPass"] intValue]];
    
    /*Audio*/
    if ([queueToApply objectForKey:@"Audio1Track"] > 0)
    {
        if ([fAudLang1PopUp indexOfSelectedItem] == 0)
        {
            [fAudLang1PopUp selectItemAtIndex: 1];
        }
        [self audioTrackPopUpChanged: fAudLang1PopUp];
        [fAudTrack1CodecPopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio1Encoder"]];
        [self audioTrackPopUpChanged: fAudTrack1CodecPopUp];
        [fAudTrack1MixPopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio1Mixdown"]];
        /* check to see if the selections was available, if not, rerun audioTrackPopUpChanged using the codec to just set the default
         * mixdown*/
        if  ([fAudTrack1MixPopUp selectedItem] == nil)
        {
            [self audioTrackPopUpChanged: fAudTrack1CodecPopUp];
        }
        [fAudTrack1RatePopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio1Samplerate"]];
        /* We set the presets bitrate if it is *not* an AC3 track since that uses the input bitrate */
        if (![[queueToApply objectForKey:@"Audio1Encoder"] isEqualToString:@"AC3 Passthru"])
        {
            [fAudTrack1BitratePopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio1Bitrate"]];
        }
        [fAudTrack1DrcSlider setFloatValue:[[queueToApply objectForKey:@"Audio1TrackDRCSlider"] floatValue]];
        [self audioDRCSliderChanged: fAudTrack1DrcSlider];
    }
    if ([queueToApply objectForKey:@"Audio2Track"] > 0)
    {
        if ([fAudLang2PopUp indexOfSelectedItem] == 0)
        {
            [fAudLang2PopUp selectItemAtIndex: 1];
        }
        [self audioTrackPopUpChanged: fAudLang2PopUp];
        [fAudTrack2CodecPopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio2Encoder"]];
        [self audioTrackPopUpChanged: fAudTrack2CodecPopUp];
        [fAudTrack2MixPopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio2Mixdown"]];
        /* check to see if the selections was available, if not, rerun audioTrackPopUpChanged using the codec to just set the default
         * mixdown*/
        if  ([fAudTrack2MixPopUp selectedItem] == nil)
        {
            [self audioTrackPopUpChanged: fAudTrack2CodecPopUp];
        }
        [fAudTrack2RatePopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio2Samplerate"]];
        /* We set the presets bitrate if it is *not* an AC3 track since that uses the input bitrate */
        if (![[queueToApply objectForKey:@"Audio2Encoder"] isEqualToString:@"AC3 Passthru"])
        {
            [fAudTrack2BitratePopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio2Bitrate"]];
        }
        [fAudTrack2DrcSlider setFloatValue:[[queueToApply objectForKey:@"Audio2TrackDRCSlider"] floatValue]];
        [self audioDRCSliderChanged: fAudTrack2DrcSlider];
    }
    if ([queueToApply objectForKey:@"Audio3Track"] > 0)
    {
        if ([fAudLang3PopUp indexOfSelectedItem] == 0)
        {
            [fAudLang3PopUp selectItemAtIndex: 1];
        }
        [self audioTrackPopUpChanged: fAudLang3PopUp];
        [fAudTrack3CodecPopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio3Encoder"]];
        [self audioTrackPopUpChanged: fAudTrack3CodecPopUp];
        [fAudTrack3MixPopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio3Mixdown"]];
        /* check to see if the selections was available, if not, rerun audioTrackPopUpChanged using the codec to just set the default
         * mixdown*/
        if  ([fAudTrack3MixPopUp selectedItem] == nil)
        {
            [self audioTrackPopUpChanged: fAudTrack3CodecPopUp];
        }
        [fAudTrack3RatePopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio3Samplerate"]];
        /* We set the presets bitrate if it is *not* an AC3 track since that uses the input bitrate */
        if (![[queueToApply objectForKey:@"Audio3Encoder"] isEqualToString: @"AC3 Passthru"])
        {
            [fAudTrack3BitratePopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio3Bitrate"]];
        }
        [fAudTrack3DrcSlider setFloatValue:[[queueToApply objectForKey:@"Audio3TrackDRCSlider"] floatValue]];
        [self audioDRCSliderChanged: fAudTrack3DrcSlider];
    }
    if ([queueToApply objectForKey:@"Audio4Track"] > 0)
    {
        if ([fAudLang4PopUp indexOfSelectedItem] == 0)
        {
            [fAudLang4PopUp selectItemAtIndex: 1];
        }
        [self audioTrackPopUpChanged: fAudLang4PopUp];
        [fAudTrack4CodecPopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio4Encoder"]];
        [self audioTrackPopUpChanged: fAudTrack4CodecPopUp];
        [fAudTrack4MixPopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio4Mixdown"]];
        /* check to see if the selections was available, if not, rerun audioTrackPopUpChanged using the codec to just set the default
         * mixdown*/
        if  ([fAudTrack4MixPopUp selectedItem] == nil)
        {
            [self audioTrackPopUpChanged: fAudTrack4CodecPopUp];
        }
        [fAudTrack4RatePopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio4Samplerate"]];
        /* We set the presets bitrate if it is *not* an AC3 track since that uses the input bitrate */
        if (![[chosenPreset objectForKey:@"Audio4Encoder"] isEqualToString:@"AC3 Passthru"])
        {
            [fAudTrack4BitratePopUp selectItemWithTitle:[queueToApply objectForKey:@"Audio4Bitrate"]];
        }
        [fAudTrack4DrcSlider setFloatValue:[[queueToApply objectForKey:@"Audio4TrackDRCSlider"] floatValue]];
        [self audioDRCSliderChanged: fAudTrack4DrcSlider];
    }
    
    
    /*Subtitles*/
    [fSubPopUp selectItemWithTitle:[queueToApply objectForKey:@"Subtitles"]];
    /* Forced Subtitles */
    [fSubForcedCheck setState:[[queueToApply objectForKey:@"SubtitlesForced"] intValue]];
    
    /* Picture Settings */
    /* we check to make sure the presets width/height does not exceed the sources width/height */
    if (fTitle->width < [[queueToApply objectForKey:@"PictureWidth"]  intValue] || fTitle->height < [[queueToApply objectForKey:@"PictureHeight"]  intValue])
    {
        /* if so, then we use the sources height and width to avoid scaling up */
        job->width = fTitle->width;
        job->height = fTitle->height;
    }
    else // source width/height is >= the preset height/width
    {
        /* we can go ahead and use the presets values for height and width */
        job->width = [[queueToApply objectForKey:@"PictureWidth"]  intValue];
        job->height = [[queueToApply objectForKey:@"PictureHeight"]  intValue];
    }
    job->keep_ratio = [[queueToApply objectForKey:@"PictureKeepRatio"]  intValue];
    if (job->keep_ratio == 1)
    {
        hb_fix_aspect( job, HB_KEEP_WIDTH );
        if( job->height > fTitle->height )
        {
            job->height = fTitle->height;
            hb_fix_aspect( job, HB_KEEP_HEIGHT );
        }
    }
    job->anamorphic.mode = [[queueToApply objectForKey:@"PicturePAR"]  intValue];
    
    
    /* If Cropping is set to custom, then recall all four crop values from
     when the preset was created and apply them */
    if ([[queueToApply objectForKey:@"PictureAutoCrop"]  intValue] == 0)
    {
        [fPictureController setAutoCrop:NO];
        
        /* Here we use the custom crop values saved at the time the preset was saved */
        job->crop[0] = [[queueToApply objectForKey:@"PictureTopCrop"]  intValue];
        job->crop[1] = [[queueToApply objectForKey:@"PictureBottomCrop"]  intValue];
        job->crop[2] = [[queueToApply objectForKey:@"PictureLeftCrop"]  intValue];
        job->crop[3] = [[queueToApply objectForKey:@"PictureRightCrop"]  intValue];
        
    }
    else /* if auto crop has been saved in preset, set to auto and use post scan auto crop */
    {
        [fPictureController setAutoCrop:YES];
        /* Here we use the auto crop values determined right after scan */
        job->crop[0] = AutoCropTop;
        job->crop[1] = AutoCropBottom;
        job->crop[2] = AutoCropLeft;
        job->crop[3] = AutoCropRight;
        
    }
    
    /* Filters */
    /* Deinterlace */
    [fPictureController setDeinterlace:[[queueToApply objectForKey:@"PictureDeinterlace"] intValue]];
    
    /* Detelecine */
    [fPictureController setDetelecine:[[queueToApply objectForKey:@"PictureDetelecine"] intValue]];
    /* Denoise */
    [fPictureController setDenoise:[[queueToApply objectForKey:@"PictureDenoise"] intValue]];
    /* Deblock */
    [fPictureController setDeblock:[[queueToApply objectForKey:@"PictureDeblock"] intValue]];
    /* Decomb */
    [fPictureController setDecomb:[[queueToApply objectForKey:@"PictureDecomb"] intValue]];
    /* Grayscale */
    [fPictureController setGrayscale:[[queueToApply objectForKey:@"VideoGrayScale"] intValue]];
    
    [self calculatePictureSizing:nil];
    
    
    /* somehow we need to figure out a way to tie the queue item to a preset if it used one */
    //[queueFileJob setObject:[fPresetSelectedDisplay stringValue] forKey:@"PresetName"];
    //    [queueFileJob setObject:[NSNumber numberWithInt:[fPresetsOutlineView selectedRow]] forKey:@"PresetIndexNum"];
    if ([queueToApply objectForKey:@"PresetIndexNum"]) // This item used a preset so insert that info
	{
		/* Deselect the currently selected Preset if there is one*/
        //[fPresetsOutlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:[[queueToApply objectForKey:@"PresetIndexNum"] intValue]] byExtendingSelection:NO];
        //[self selectPreset:nil];
		
        //[fPresetsOutlineView selectRow:[[queueToApply objectForKey:@"PresetIndexNum"] intValue]];
		/* Change UI to show "Custom" settings are being used */
		//[fPresetSelectedDisplay setStringValue: [[queueToApply objectForKey:@"PresetName"] stringValue]];
        
		curUserPresetChosenNum = nil;
	}
    else
    {
        /* Deselect the currently selected Preset if there is one*/
		[fPresetsOutlineView deselectRow:[fPresetsOutlineView selectedRow]];
		/* Change UI to show "Custom" settings are being used */
		[fPresetSelectedDisplay setStringValue: @"Custom"];
        
		//curUserPresetChosenNum = nil;
    }
    
    /* We need to set this bool back to NO, in case the user wants to do a scan */
    //applyQueueToScan = NO;
    
    /* so now we go ahead and process the new settings */
    [self processNewQueueEncode];
}



/* This assumes that we have re-scanned and loaded up a new queue item to send to libhb as fQueueEncodeLibhb */
- (void) processNewQueueEncode
{
    hb_list_t  * list  = hb_get_titles( fQueueEncodeLibhb );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,0 ); // is always zero since now its a single title scan
    hb_job_t * job = title->job;
    
    if( !hb_list_count( list ) )
    {
        [self writeToActivityLog: "processNewQueueEncode WARNING nothing found in the title list"];
    }
    
    NSMutableDictionary * queueToApply = [QueueFileArray objectAtIndex:currentQueueEncodeIndex];
    [self writeToActivityLog: "Preset: %s", [[queueToApply objectForKey:@"PresetName"] UTF8String]];
    [self writeToActivityLog: "processNewQueueEncode number of passes expected is: %d", ([[queueToApply objectForKey:@"VideoTwoPass"] intValue] + 1)];
    job->file = [[queueToApply objectForKey:@"DestinationPath"] UTF8String];
    //[self writeToActivityLog: "processNewQueueEncode sending to prepareJob"];
    [self prepareJob];
    
    /*
     * If scanning we need to do some extra setup of the job.
     */
    if( job->indepth_scan == 1 )
    {
        char *x264opts_tmp;
        
        /*
         * When subtitle scan is enabled do a fast pre-scan job
         * which will determine which subtitles to enable, if any.
         */
        job->pass = -1;
        x264opts_tmp = job->x264opts;
        
        job->x264opts = NULL;
        
        job->indepth_scan = 1;  

        
        /*
         * Add the pre-scan job
         */
        hb_add( fQueueEncodeLibhb, job );
        job->x264opts = x264opts_tmp;
    }

    
    if( [[queueToApply objectForKey:@"VideoTwoPass"] intValue] == 1 )
    {
        job->indepth_scan = 0;
        

        
        job->pass = 1;
        
        hb_add( fQueueEncodeLibhb, job );
        
        job->pass = 2;
        
        job->x264opts = (char *)calloc(1024, 1); /* Fixme, this just leaks */  
        strcpy(job->x264opts, [[queueToApply objectForKey:@"x264Option"] UTF8String]);
        
        hb_add( fQueueEncodeLibhb, job );
        
    }
    else
    {
        job->indepth_scan = 0;
        job->pass = 0;
        
        hb_add( fQueueEncodeLibhb, job );
    }
	
    NSString *destinationDirectory = [[queueToApply objectForKey:@"DestinationPath"] stringByDeletingLastPathComponent];
	[[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];
	/* Lets mark our new encode as 1 or "Encoding" */
    [queueToApply setObject:[NSNumber numberWithInt:1] forKey:@"Status"];
    [self saveQueueFileItem];
    
    /* we need to clean up the subtitle tracks after the job(s) have been set  */
    int num_subtitle_tracks = hb_list_count(job->list_subtitle);
    int ii;
    for(ii = 0; ii < num_subtitle_tracks; ii++)
    {
        hb_subtitle_t * subtitle;
        subtitle = (hb_subtitle_t *)hb_list_item(job->list_subtitle, 0);
        

        hb_list_rem(job->list_subtitle, subtitle);
        free(subtitle);
    }
    
    
    /* We should be all setup so let 'er rip */   
    [self doRip];
}

#pragma mark -
#pragma mark Live Preview
/* Note,this is much like prepareJob, but directly sets the job vars so Picture Preview
 * can encode to its temp preview directory and playback. This is *not* used for any actual user
 * encodes
 */
- (void) prepareJobForPreview
{
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;
    hb_audio_config_t * audio;
    /* set job->angle for libdvdnav */
    job->angle = [fSrcAnglePopUp indexOfSelectedItem] + 1;
    /* Chapter selection */
    job->chapter_start = [fSrcChapterStartPopUp indexOfSelectedItem] + 1;
    job->chapter_end   = [fSrcChapterEndPopUp   indexOfSelectedItem] + 1;
	
    /* Format (Muxer) and Video Encoder */
    job->mux = [[fDstFormatPopUp selectedItem] tag];
    job->vcodec = [[fVidEncoderPopUp selectedItem] tag];

    job->chapter_markers = 0;
    
	if( job->vcodec & HB_VCODEC_X264 )
    {
		/* Set this flag to switch from Constant Quantizer(default) to Constant Rate Factor Thanks jbrjake
         Currently only used with Constant Quality setting*/
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultCrf"] > 0 && [fVidQualityMatrix selectedRow] == 2)
		{
	        job->crf = 1;
		}
		
		/* Below Sends x264 options to the core library if x264 is selected*/
		/* Lets use this as per Nyx, Thanks Nyx!*/
		job->x264opts = (char *)calloc(1024, 1); /* Fixme, this just leaks */
		/* For previews we ignore the turbo option for the first pass of two since we only use 1 pass */
		strcpy(job->x264opts, [[fAdvancedOptions optionsString] UTF8String]);

        
    }

    /* Video settings */
   /* Set vfr to 0 as it's only on if using same as source in the framerate popup
     * and detelecine is on, so we handle that in the logic below
     */
    job->vfr = 0;
    if( [fVidRatePopUp indexOfSelectedItem] > 0 )
    {
        /* a specific framerate has been chosen */
        job->vrate      = 27000000;
        job->vrate_base = hb_video_rates[[fVidRatePopUp indexOfSelectedItem]-1].rate;
        /* We are not same as source so we set job->cfr to 1 
         * to enable constant frame rate since user has specified
         * a specific framerate*/
        job->cfr = 1;
    }
    else
    {
        /* We are same as source (variable) */
        job->vrate      = title->rate;
        job->vrate_base = title->rate_base;
        /* We are same as source so we set job->cfr to 0 
         * to enable true same as source framerate */
        job->cfr = 0;
        /* If we are same as source and we have detelecine on, we need to turn on
         * job->vfr
         */
        if ([fPictureController detelecine] == 1)
        {
            job->vfr = 1;
        }
    }

    switch( [fVidQualityMatrix selectedRow] )
    {
        case 0:
            /* Target size.
               Bitrate should already have been calculated and displayed
               in fVidBitrateField, so let's just use it */
        case 1:
            job->vquality = -1.0;
            job->vbitrate = [fVidBitrateField intValue];
            break;
        case 2:
            job->vquality = [fVidQualityRFField floatValue];
            job->vbitrate = 0;
            break;
    }

    /* Subtitle settings */
    NSMutableArray *subtitlesArray = nil;
    subtitlesArray = [[NSMutableArray alloc] initWithArray:[fSubtitlesDelegate getSubtitleArray: subtitlesArray]];
    
    
    
 int subtitle = nil;
int force;
int burned;
int def;
bool one_burned = FALSE;

    int i = 0;
    NSEnumerator *enumerator = [subtitlesArray objectEnumerator];
    id tempObject;
    while (tempObject = [enumerator nextObject])
    {
        
        subtitle = [[tempObject objectForKey:@"subtitleSourceTrackNum"] intValue];
        force = [[tempObject objectForKey:@"subtitleTrackForced"] intValue];
        burned = [[tempObject objectForKey:@"subtitleTrackBurned"] intValue];
        def = [[tempObject objectForKey:@"subtitleTrackDefault"] intValue];
        
        /* since the subtitleSourceTrackNum 0 is "None" in our array of the subtitle popups,
         * we want to ignore it for display as well as encoding.
         */
        if (subtitle > 0)
        {
            /* if i is 0, then we are in the first item of the subtitles which we need to 
             * check for the "Foreign Audio Search" which would be subtitleSourceTrackNum of 1
             * bearing in mind that for all tracks subtitleSourceTrackNum of 0 is None.
             */
            
            /* if we are on the first track and using "Foreign Audio Search" */ 
            if (i == 0 && subtitle == 1)
            {
                /* NOTE: Currently foreign language search is borked for preview.
                 * Commented out but left in for initial commit. */
                
                
                [self writeToActivityLog: "Foreign Language Search: %d", 1];
                
                job->indepth_scan = 1;
                if (burned == 1 || job->mux != HB_MUX_MP4)
                {
                    if (burned != 1 && job->mux == HB_MUX_MKV)
                    {
                        job->select_subtitle_config.dest = PASSTHRUSUB;
                    }
                    else
                    {
                        job->select_subtitle_config.dest = RENDERSUB;
                    }
                    
                    job->select_subtitle_config.force = force;
                    job->select_subtitle_config.default_track = def;
                    
                }
                
                
            }
            else
            {
                
                /* for the actual source tracks, we must subtract the non source entries so 
                 * that the menu index matches the source subtitle_list index for convenience */
                if (i == 0)
                {
                    /* for the first track, the source tracks start at menu index 2 ( None is 0,
                     * Foreign Language Search is 1) so subtract 2 */
                    subtitle = subtitle - 2;
                }
                else
                {
                    /* for all other tracks, the source tracks start at menu index 1 (None is 0)
                     * so subtract 1. */
                    
                    subtitle = subtitle - 1;
                }
                
                /* We are setting a source subtitle so access the source subtitle info */  
                hb_subtitle_t * subt;
                
                subt = (hb_subtitle_t *)hb_list_item(title->list_subtitle, subtitle);
                
                /* if we are getting the subtitles from an external srt file */
                if ([[tempObject objectForKey:@"subtitleSourceTrackType"] isEqualToString:@"SRT"])
                {
                    hb_subtitle_config_t sub_config;
                    
                    sub_config.offset = [[tempObject objectForKey:@"subtitleTrackSrtOffset"] intValue];
                    
                    /* we need to srncpy file path and char code */
                    strncpy(sub_config.src_filename, [[tempObject objectForKey:@"subtitleSourceSrtFilePath"] UTF8String], 128);
                    strncpy(sub_config.src_codeset, [[tempObject objectForKey:@"subtitleTrackSrtCharCode"] UTF8String], 40);
                    
                    sub_config.force = 0;
                    sub_config.dest = PASSTHRUSUB;
                    sub_config.default_track = def;
                    
                    hb_srt_add( job, &sub_config, [[tempObject objectForKey:@"subtitleTrackSrtLanguageIso3"] UTF8String]);
                }
                
                if (subt != NULL)
                {
                    [self writeToActivityLog: "Setting Subtitle: %s", subt];

                    hb_subtitle_config_t sub_config = subt->config;
                    
                    if (!burned && job->mux == HB_MUX_MKV && 
                        subt->format == PICTURESUB)
                    {
                        sub_config.dest = PASSTHRUSUB;
                    }
                    else if (!burned && job->mux == HB_MUX_MP4 && 
                             subt->format == PICTURESUB)
                    {
                        // Skip any non-burned vobsubs when output is mp4
                        continue;
                    }
                    else if ( burned && subt->format == PICTURESUB )
                    {
                        // Only allow one subtitle to be burned into the video
                        if (one_burned)
                            continue;
                        one_burned = TRUE;
                    }
                    sub_config.force = force;
                    sub_config.default_track = def;
                    hb_subtitle_add( job, &sub_config, subtitle );
                }   
                
            }
        }
        i++;
    }
   
    
    
[subtitlesArray autorelease];    
    
    
    /* Audio tracks and mixdowns */
    /* Lets make sure there arent any erroneous audio tracks in the job list, so lets make sure its empty*/
    int audiotrack_count = hb_list_count(job->list_audio);
    for( int i = 0; i < audiotrack_count;i++)
    {
        hb_audio_t * temp_audio = (hb_audio_t*) hb_list_item( job->list_audio, 0 );
        hb_list_rem(job->list_audio, temp_audio);
    }
    /* Now lets add our new tracks to the audio list here */
    if ([fAudLang1PopUp indexOfSelectedItem] > 0)
    {
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [fAudLang1PopUp indexOfSelectedItem] - 1;
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [fAudLang1PopUp indexOfSelectedItem] - 1;
        audio->out.codec = [[fAudTrack1CodecPopUp selectedItem] tag];
        audio->out.mixdown = [[fAudTrack1MixPopUp selectedItem] tag];
        audio->out.bitrate = [[fAudTrack1BitratePopUp selectedItem] tag];
        audio->out.samplerate = [[fAudTrack1RatePopUp selectedItem] tag];
        audio->out.dynamic_range_compression = [fAudTrack1DrcField floatValue];
        
        hb_audio_add( job, audio );
        free(audio);
    }  
    if ([fAudLang2PopUp indexOfSelectedItem] > 0)
    {
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [fAudLang2PopUp indexOfSelectedItem] - 1;
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [fAudLang2PopUp indexOfSelectedItem] - 1;
        audio->out.codec = [[fAudTrack2CodecPopUp selectedItem] tag];
        audio->out.mixdown = [[fAudTrack2MixPopUp selectedItem] tag];
        audio->out.bitrate = [[fAudTrack2BitratePopUp selectedItem] tag];
        audio->out.samplerate = [[fAudTrack2RatePopUp selectedItem] tag];
        audio->out.dynamic_range_compression = [fAudTrack2DrcField floatValue];
        
        hb_audio_add( job, audio );
        free(audio);
        
    }
    
    if ([fAudLang3PopUp indexOfSelectedItem] > 0)
    {
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [fAudLang3PopUp indexOfSelectedItem] - 1;
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [fAudLang3PopUp indexOfSelectedItem] - 1;
        audio->out.codec = [[fAudTrack3CodecPopUp selectedItem] tag];
        audio->out.mixdown = [[fAudTrack3MixPopUp selectedItem] tag];
        audio->out.bitrate = [[fAudTrack3BitratePopUp selectedItem] tag];
        audio->out.samplerate = [[fAudTrack3RatePopUp selectedItem] tag];
        audio->out.dynamic_range_compression = [fAudTrack3DrcField floatValue];
        
        hb_audio_add( job, audio );
        free(audio);
        
    }

    if ([fAudLang4PopUp indexOfSelectedItem] > 0)
    {
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [fAudLang4PopUp indexOfSelectedItem] - 1;
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [fAudLang4PopUp indexOfSelectedItem] - 1;
        audio->out.codec = [[fAudTrack4CodecPopUp selectedItem] tag];
        audio->out.mixdown = [[fAudTrack4MixPopUp selectedItem] tag];
        audio->out.bitrate = [[fAudTrack4BitratePopUp selectedItem] tag];
        audio->out.samplerate = [[fAudTrack4RatePopUp selectedItem] tag];
        audio->out.dynamic_range_compression = [fAudTrack4DrcField floatValue];
        
        hb_audio_add( job, audio );
        free(audio);
        
    }

    
    
    /* Filters */
    
    /* Though Grayscale is not really a filter, per se
     * we put it here since its in the filters panel
     */
     
    if ([fPictureController grayscale])
    {
        job->grayscale = 1;
    }
    else
    {
        job->grayscale = 0;
    }
    
    /* Initialize the filters list */
    job->filters = hb_list_init();
    
    /* Now lets call the filters if applicable.
    * The order of the filters is critical
    */
    
	/* Detelecine */
    if ([fPictureController detelecine] == 1)
    {
        /* use a custom detelecine string */
        hb_filter_detelecine.settings = (char *) [[fPictureController detelecineCustomString] UTF8String];
        hb_list_add( job->filters, &hb_filter_detelecine );
    }
    if ([fPictureController detelecine] == 2)
    {
        /* Default */
        hb_list_add( job->filters, &hb_filter_detelecine );
    }
    
    
    
    if ([fPictureController useDecomb] == 1)
    {
        /* Decomb */
        /* we add the custom string if present */
        if ([fPictureController decomb] == 1)
        {
            /* use a custom decomb string */
            hb_filter_decomb.settings = (char *) [[fPictureController decombCustomString] UTF8String];
            hb_list_add( job->filters, &hb_filter_decomb );
        }
        if ([fPictureController decomb] == 2)
        {
            /* Run old deinterlacer fd by default */
            //hb_filter_decomb.settings = (char *) [[fPicSettingDecomb stringValue] UTF8String];
            hb_list_add( job->filters, &hb_filter_decomb );
        }
    }
    else
    {
        
        /* Deinterlace */
        if ([fPictureController deinterlace] == 1)
        {
            /* we add the custom string if present */
            hb_filter_deinterlace.settings = (char *) [[fPictureController deinterlaceCustomString] UTF8String];
            hb_list_add( job->filters, &hb_filter_deinterlace );            
        }
        else if ([fPictureController deinterlace] == 2)
        {
            /* Run old deinterlacer fd by default */
            hb_filter_deinterlace.settings = "-1"; 
            hb_list_add( job->filters, &hb_filter_deinterlace );
        }
        else if ([fPictureController deinterlace] == 3)
        {
            /* Yadif mode 0 (without spatial deinterlacing.) */
            hb_filter_deinterlace.settings = "2"; 
            hb_list_add( job->filters, &hb_filter_deinterlace );            
        }
        else if ([fPictureController deinterlace] == 4)
        {
            /* Yadif (with spatial deinterlacing) */
            hb_filter_deinterlace.settings = "0"; 
            hb_list_add( job->filters, &hb_filter_deinterlace );            
        }
        
	}
    
    /* Denoise */
	if ([fPictureController denoise] == 1) // custom in popup
	{
		/* we add the custom string if present */
        hb_filter_denoise.settings = (char *) [[fPictureController denoiseCustomString] UTF8String]; 
        hb_list_add( job->filters, &hb_filter_denoise );	
	}
    else if ([fPictureController denoise] == 2) // Weak in popup
	{
		hb_filter_denoise.settings = "2:1:2:3"; 
        hb_list_add( job->filters, &hb_filter_denoise );	
	}
	else if ([fPictureController denoise] == 3) // Medium in popup
	{
		hb_filter_denoise.settings = "3:2:2:3"; 
        hb_list_add( job->filters, &hb_filter_denoise );	
	}
	else if ([fPictureController denoise] == 4) // Strong in popup
	{
		hb_filter_denoise.settings = "7:7:5:5"; 
        hb_list_add( job->filters, &hb_filter_denoise );	
	}
    
    
    /* Deblock  (uses pp7 default) */
    /* NOTE: even though there is a valid deblock setting of 0 for the filter, for 
     * the macgui's purposes a value of 0 actually means to not even use the filter
     * current hb_filter_deblock.settings valid ranges are from 5 - 15 
     */
    if ([fPictureController deblock] != 0)
    {
        NSString *deblockStringValue = [NSString stringWithFormat: @"%d",[fPictureController deblock]];
        hb_filter_deblock.settings = (char *) [deblockStringValue UTF8String];
        hb_list_add( job->filters, &hb_filter_deblock );
    }

}


#pragma mark -
#pragma mark Job Handling


- (void) prepareJob
{
    
    NSMutableDictionary * queueToApply = [QueueFileArray objectAtIndex:currentQueueEncodeIndex];
    hb_list_t  * list  = hb_get_titles( fQueueEncodeLibhb );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,0 ); // is always zero since now its a single title scan
    hb_job_t * job = title->job;
    hb_audio_config_t * audio;
    /* Title Angle for dvdnav */
    job->angle = [[queueToApply objectForKey:@"TitleAngle"] intValue];
    /* Chapter selection */
    job->chapter_start = [[queueToApply objectForKey:@"JobChapterStart"] intValue];
    job->chapter_end   = [[queueToApply objectForKey:@"JobChapterEnd"] intValue];
	
    /* Format (Muxer) and Video Encoder */
    job->mux = [[queueToApply objectForKey:@"JobFileFormatMux"] intValue];
    job->vcodec = [[queueToApply objectForKey:@"JobVideoEncoderVcodec"] intValue];
    
    
    /* If mpeg-4, then set mpeg-4 specific options like chapters and > 4gb file sizes */
    if( [[queueToApply objectForKey:@"Mp4LargeFile"] intValue] == 1)
    {
        job->largeFileSize = 1;
    }
    else
    {
        job->largeFileSize = 0;
    }
    /* We set http optimized mp4 here */
    if( [[queueToApply objectForKey:@"Mp4HttpOptimize"] intValue] == 1 )
    {
        job->mp4_optimize = 1;
    }
    else
    {
        job->mp4_optimize = 0;
    }

	
    /* We set the chapter marker extraction here based on the format being
     mpeg4 or mkv and the checkbox being checked */
    if ([[queueToApply objectForKey:@"ChapterMarkers"] intValue] == 1)
    {
        job->chapter_markers = 1;
        
        /* now lets get our saved chapter names out the array in the queue file
         * and insert them back into the title chapter list. We have it here,
         * because unless we are inserting chapter markers there is no need to
         * spend the overhead of iterating through the chapter names array imo
         * Also, note that if for some reason we don't apply chapter names, the
         * chapters just come out 001, 002, etc. etc.
         */
         
        NSMutableArray *ChapterNamesArray = [queueToApply objectForKey:@"ChapterNames"];
        int i = 0;
        NSEnumerator *enumerator = [ChapterNamesArray objectEnumerator];
        id tempObject;
        while (tempObject = [enumerator nextObject])
        {
            hb_chapter_t *chapter = (hb_chapter_t *) hb_list_item( title->list_chapter, i );
            if( chapter != NULL )
            {
                strncpy( chapter->title, [tempObject UTF8String], 1023);
                chapter->title[1023] = '\0';
            }
            i++;
        }
    }
    else
    {
        job->chapter_markers = 0;
    }
    
    if( job->vcodec & HB_VCODEC_X264 )
    {
		if ([[queueToApply objectForKey:@"Mp4iPodCompatible"] intValue] == 1)
	    {
            job->ipod_atom = 1;
		}
        else
        {
            job->ipod_atom = 0;
        }
		
		/* Set this flag to switch from Constant Quantizer(default) to Constant Rate Factor Thanks jbrjake
         Currently only used with Constant Quality setting*/
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultCrf"] > 0 && [[queueToApply objectForKey:@"VideoQualityType"] intValue] == 2)
		{
	        job->crf = 1;
		}
		/* Below Sends x264 options to the core library if x264 is selected*/
		/* Lets use this as per Nyx, Thanks Nyx!*/
		job->x264opts = (char *)calloc(1024, 1); /* Fixme, this just leaks */
		/* Turbo first pass if two pass and Turbo First pass is selected */
		if( [[queueToApply objectForKey:@"VideoTwoPass"] intValue] == 1 && [[queueToApply objectForKey:@"VideoTurboTwoPass"] intValue] == 1 )
		{
			/* pass the "Turbo" string to be appended to the existing x264 opts string into a variable for the first pass */
			NSString *firstPassOptStringTurbo = @":ref=1:subme=1:me=dia:analyse=none:trellis=0:no-fast-pskip=0:8x8dct=0:weightb=0";
			/* append the "Turbo" string variable to the existing opts string.
             Note: the "Turbo" string must be appended, not prepended to work properly*/
			NSString *firstPassOptStringCombined = [[queueToApply objectForKey:@"x264Option"] stringByAppendingString:firstPassOptStringTurbo];
			strcpy(job->x264opts, [firstPassOptStringCombined UTF8String]);
		}
		else
		{
			strcpy(job->x264opts, [[queueToApply objectForKey:@"x264Option"] UTF8String]);
		}
        
    }
    
    
    /* Picture Size Settings */
    job->width = [[queueToApply objectForKey:@"PictureWidth"]  intValue];
    job->height = [[queueToApply objectForKey:@"PictureHeight"]  intValue];
    
    job->keep_ratio = [[queueToApply objectForKey:@"PictureKeepRatio"]  intValue];
    job->anamorphic.mode = [[queueToApply objectForKey:@"PicturePAR"]  intValue];
    if ([[queueToApply objectForKey:@"PicturePAR"]  intValue] == 3)
    {
        /* insert our custom values here for capuj */
        job->width = [[queueToApply objectForKey:@"PicturePARStorageWidth"]  intValue];
        job->height = [[queueToApply objectForKey:@"PicturePARStorageHeight"]  intValue];
        
        job->anamorphic.par_width = [[queueToApply objectForKey:@"PicturePARPixelWidth"]  intValue];
        job->anamorphic.par_height = [[queueToApply objectForKey:@"PicturePARPixelHeight"]  intValue];
        
        job->anamorphic.dar_width = [[queueToApply objectForKey:@"PicturePARDisplayWidth"]  floatValue];
        job->anamorphic.dar_height = [[queueToApply objectForKey:@"PicturePARDisplayHeight"]  floatValue];
    }
    
    /* Here we use the crop values saved at the time the preset was saved */
    job->crop[0] = [[queueToApply objectForKey:@"PictureTopCrop"]  intValue];
    job->crop[1] = [[queueToApply objectForKey:@"PictureBottomCrop"]  intValue];
    job->crop[2] = [[queueToApply objectForKey:@"PictureLeftCrop"]  intValue];
    job->crop[3] = [[queueToApply objectForKey:@"PictureRightCrop"]  intValue];
    
    /* Video settings */
    /* Framerate */
    
    /* Set vfr to 0 as it's only on if using same as source in the framerate popup
     * and detelecine is on, so we handle that in the logic below
     */
    job->vfr = 0;
    if( [[queueToApply objectForKey:@"JobIndexVideoFramerate"] intValue] > 0 )
    {
        /* a specific framerate has been chosen */
        job->vrate      = 27000000;
        job->vrate_base = hb_video_rates[[[queueToApply objectForKey:@"JobIndexVideoFramerate"] intValue]-1].rate;
        /* We are not same as source so we set job->cfr to 1 
         * to enable constant frame rate since user has specified
         * a specific framerate*/
        job->cfr = 1;
    }
    else
    {
        /* We are same as source (variable) */
        job->vrate      = [[queueToApply objectForKey:@"JobVrate"] intValue];
        job->vrate_base = [[queueToApply objectForKey:@"JobVrateBase"] intValue];
        /* We are same as source so we set job->cfr to 0 
         * to enable true same as source framerate */
        job->cfr = 0;
        /* If we are same as source and we have detelecine on, we need to turn on
         * job->vfr
         */
        if ([[queueToApply objectForKey:@"PictureDetelecine"] intValue] == 1)
        {
            job->vfr = 1;
        }
    }
    
    if ( [[queueToApply objectForKey:@"VideoQualityType"] intValue] != 2 )
    {
        /* Target size.
         Bitrate should already have been calculated and displayed
         in fVidBitrateField, so let's just use it same as abr*/
        job->vquality = -1.0;
        job->vbitrate = [[queueToApply objectForKey:@"VideoAvgBitrate"] intValue];
    }
    if ( [[queueToApply objectForKey:@"VideoQualityType"] intValue] == 2 )
    {
        job->vquality = [[queueToApply objectForKey:@"VideoQualitySlider"] floatValue];
        job->vbitrate = 0;
        
    }
    
    job->grayscale = [[queueToApply objectForKey:@"VideoGrayScale"] intValue];
    


#pragma mark -
#pragma mark Process Subtitles to libhb

/* Map the settings in the dictionaries for the SubtitleList array to match title->list_subtitle
 * which means that we need to account for the offset of non source language settings in from
 * the NSPopUpCell menu. For all of the objects in the SubtitleList array this means 0 is "None"
 * from the popup menu, additionally the first track has "Foreign Audio Search" at 1. So we use
 * an int to offset the index number for the objectForKey:@"subtitleSourceTrackNum" to map that
 * to the source tracks position in title->list_subtitle.
 */

int subtitle = nil;
int force;
int burned;
int def;
bool one_burned = FALSE;

    int i = 0;
    NSEnumerator *enumerator = [[queueToApply objectForKey:@"SubtitleList"] objectEnumerator];
    id tempObject;
    while (tempObject = [enumerator nextObject])
    {
        
        subtitle = [[tempObject objectForKey:@"subtitleSourceTrackNum"] intValue];
        force = [[tempObject objectForKey:@"subtitleTrackForced"] intValue];
        burned = [[tempObject objectForKey:@"subtitleTrackBurned"] intValue];
        def = [[tempObject objectForKey:@"subtitleTrackDefault"] intValue];
        
        /* since the subtitleSourceTrackNum 0 is "None" in our array of the subtitle popups,
         * we want to ignore it for display as well as encoding.
         */
        if (subtitle > 0)
        {
            /* if i is 0, then we are in the first item of the subtitles which we need to 
             * check for the "Foreign Audio Search" which would be subtitleSourceTrackNum of 1
             * bearing in mind that for all tracks subtitleSourceTrackNum of 0 is None.
             */
            
            /* if we are on the first track and using "Foreign Audio Search" */ 
            if (i == 0 && subtitle == 1)
            {
                [self writeToActivityLog: "Foreign Language Search: %d", 1];
                
                job->indepth_scan = 1;
                if (burned == 1 || job->mux != HB_MUX_MP4)
                {
                    if (burned != 1 && job->mux == HB_MUX_MKV)
                    {
                        job->select_subtitle_config.dest = PASSTHRUSUB;
                    }
                    else
                    {
                        job->select_subtitle_config.dest = RENDERSUB;
                    }
                    
                    job->select_subtitle_config.force = force;
                    job->select_subtitle_config.default_track = def;
                }
                
                
            }
            else
            {
                
                /* for the actual source tracks, we must subtract the non source entries so 
                 * that the menu index matches the source subtitle_list index for convenience */
                if (i == 0)
                {
                    /* for the first track, the source tracks start at menu index 2 ( None is 0,
                     * Foreign Language Search is 1) so subtract 2 */
                    subtitle = subtitle - 2;
                }
                else
                {
                    /* for all other tracks, the source tracks start at menu index 1 (None is 0)
                     * so subtract 1. */
                    
                    subtitle = subtitle - 1;
                }
                
                /* We are setting a source subtitle so access the source subtitle info */  
                hb_subtitle_t * subt;
                
                subt = (hb_subtitle_t *)hb_list_item(title->list_subtitle, subtitle);
                
                /* if we are getting the subtitles from an external srt file */
                if ([[tempObject objectForKey:@"subtitleSourceTrackType"] isEqualToString:@"SRT"])
                {
                    hb_subtitle_config_t sub_config;
                    
                    sub_config.offset = [[tempObject objectForKey:@"subtitleTrackSrtOffset"] intValue];
                    
                    /* we need to srncpy file name and codeset */
                    //sub_config.src_filename = [[tempObject objectForKey:@"subtitleSourceSrtFilePath"] UTF8String];
                    strncpy(sub_config.src_filename, [[tempObject objectForKey:@"subtitleSourceSrtFilePath"] UTF8String], 128);
                    //sub_config.src_codeset = [[tempObject objectForKey:@"subtitleTrackSrtCharCode"] UTF8String];
                    strncpy(sub_config.src_codeset, [[tempObject objectForKey:@"subtitleTrackSrtCharCode"] UTF8String], 40);
                    
                    sub_config.force = 0;
                    sub_config.dest = PASSTHRUSUB;
                    sub_config.default_track = def;
                    
                    hb_srt_add( job, &sub_config, [[tempObject objectForKey:@"subtitleTrackSrtLanguageIso3"] UTF8String]);
                }
                
                
                if (subt != NULL)
                {
                    [self writeToActivityLog: "Setting Subtitle: %s", subt];

                    hb_subtitle_config_t sub_config = subt->config;
                    
                    if (!burned && job->mux == HB_MUX_MKV && 
                        subt->format == PICTURESUB)
                    {
                        sub_config.dest = PASSTHRUSUB;
                    }
                    else if (!burned && job->mux == HB_MUX_MP4 && 
                             subt->format == PICTURESUB)
                    {
                        // Skip any non-burned vobsubs when output is mp4
                        continue;
                    }
                    else if ( burned && subt->format == PICTURESUB )
                    {
                        // Only allow one subtitle to be burned into the video
                        if (one_burned)
                            continue;
                        one_burned = TRUE;
                    }
                    sub_config.force = force;
                    sub_config.default_track = def;
                    hb_subtitle_add( job, &sub_config, subtitle );
                }   
                
            }
        }
        i++;
    }

#pragma mark -

   
    /* Audio tracks and mixdowns */
    /* Lets make sure there arent any erroneous audio tracks in the job list, so lets make sure its empty*/
    int audiotrack_count = hb_list_count(job->list_audio);
    for( int i = 0; i < audiotrack_count;i++)
    {
        hb_audio_t * temp_audio = (hb_audio_t*) hb_list_item( job->list_audio, 0 );
        hb_list_rem(job->list_audio, temp_audio);
    }
    /* Now lets add our new tracks to the audio list here */
    if ([[queueToApply objectForKey:@"Audio1Track"] intValue] > 0)
    {
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [[queueToApply objectForKey:@"Audio1Track"] intValue] - 1;
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [[queueToApply objectForKey:@"Audio1Track"] intValue] - 1;
        audio->out.codec = [[queueToApply objectForKey:@"JobAudio1Encoder"] intValue];
        audio->out.mixdown = [[queueToApply objectForKey:@"JobAudio1Mixdown"] intValue];
        audio->out.bitrate = [[queueToApply objectForKey:@"JobAudio1Bitrate"] intValue];
        audio->out.samplerate = [[queueToApply objectForKey:@"JobAudio1Samplerate"] intValue];
        audio->out.dynamic_range_compression = [[queueToApply objectForKey:@"Audio1TrackDRCSlider"] floatValue];
        
        hb_audio_add( job, audio );
        free(audio);
    }  
    if ([[queueToApply objectForKey:@"Audio2Track"] intValue] > 0)
    {
        
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [[queueToApply objectForKey:@"Audio2Track"] intValue] - 1;
        [self writeToActivityLog: "prepareJob audiotrack 2 is: %d", audio->in.track];
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [[queueToApply objectForKey:@"Audio2Track"] intValue] - 1;
        audio->out.codec = [[queueToApply objectForKey:@"JobAudio2Encoder"] intValue];
        audio->out.mixdown = [[queueToApply objectForKey:@"JobAudio2Mixdown"] intValue];
        audio->out.bitrate = [[queueToApply objectForKey:@"JobAudio2Bitrate"] intValue];
        audio->out.samplerate = [[queueToApply objectForKey:@"JobAudio2Samplerate"] intValue];
        audio->out.dynamic_range_compression = [[queueToApply objectForKey:@"Audio2TrackDRCSlider"] floatValue];
        
        hb_audio_add( job, audio );
        free(audio);
    }
    
    if ([[queueToApply objectForKey:@"Audio3Track"] intValue] > 0)
    {
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [[queueToApply objectForKey:@"Audio3Track"] intValue] - 1;
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [[queueToApply objectForKey:@"Audio3Track"] intValue] - 1;
        audio->out.codec = [[queueToApply objectForKey:@"JobAudio3Encoder"] intValue];
        audio->out.mixdown = [[queueToApply objectForKey:@"JobAudio3Mixdown"] intValue];
        audio->out.bitrate = [[queueToApply objectForKey:@"JobAudio3Bitrate"] intValue];
        audio->out.samplerate = [[queueToApply objectForKey:@"JobAudio3Samplerate"] intValue];
        audio->out.dynamic_range_compression = [[queueToApply objectForKey:@"Audio3TrackDRCSlider"] floatValue];
        
        hb_audio_add( job, audio );
        free(audio);        
    }
    
    if ([[queueToApply objectForKey:@"Audio4Track"] intValue] > 0)
    {
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [[queueToApply objectForKey:@"Audio4Track"] intValue] - 1;
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [[queueToApply objectForKey:@"Audio4Track"] intValue] - 1;
        audio->out.codec = [[queueToApply objectForKey:@"JobAudio4Encoder"] intValue];
        audio->out.mixdown = [[queueToApply objectForKey:@"JobAudio4Mixdown"] intValue];
        audio->out.bitrate = [[queueToApply objectForKey:@"JobAudio4Bitrate"] intValue];
        audio->out.samplerate = [[queueToApply objectForKey:@"JobAudio4Samplerate"] intValue];
        audio->out.dynamic_range_compression = [[queueToApply objectForKey:@"Audio4TrackDRCSlider"] floatValue];
        
        hb_audio_add( job, audio );
        

    }
    
    /* Filters */ 
    job->filters = hb_list_init();
    
    /* Now lets call the filters if applicable.
     * The order of the filters is critical
     */
    /* Detelecine */
    if ([[queueToApply objectForKey:@"PictureDetelecine"] intValue] == 1)
    {
        /* use a custom detelecine string */
        hb_filter_detelecine.settings = (char *) [[queueToApply objectForKey:@"PictureDetelecineCustom"] UTF8String];
        hb_list_add( job->filters, &hb_filter_detelecine );
    }
    if ([[queueToApply objectForKey:@"PictureDetelecine"] intValue] == 2)
    {
        /* Use libhb's default values */
        hb_list_add( job->filters, &hb_filter_detelecine );
    }
    
    if ([[queueToApply objectForKey:@"PictureDecombDeinterlace"] intValue] == 1)
    {
        /* Decomb */
        /* we add the custom string if present */
        if ([[queueToApply objectForKey:@"PictureDecomb"] intValue] == 1)
        {
            /* use a custom decomb string */
            hb_filter_decomb.settings = (char *) [[queueToApply objectForKey:@"PictureDecombCustom"] UTF8String];
            hb_list_add( job->filters, &hb_filter_decomb );
        }
        if ([[queueToApply objectForKey:@"PictureDecomb"] intValue] == 2)
        {
            /* Use libhb default */
            hb_list_add( job->filters, &hb_filter_decomb );
        }
        
    }
    else
    {
        
        /* Deinterlace */
        if ([[queueToApply objectForKey:@"PictureDeinterlace"] intValue] == 1)
        {
            /* we add the custom string if present */
            hb_filter_deinterlace.settings = (char *) [[queueToApply objectForKey:@"PictureDeinterlaceCustom"] UTF8String];
            hb_list_add( job->filters, &hb_filter_deinterlace );            
        }
        else if ([[queueToApply objectForKey:@"PictureDeinterlace"] intValue] == 2)
        {
            /* Run old deinterlacer fd by default */
            hb_filter_deinterlace.settings = "-1"; 
            hb_list_add( job->filters, &hb_filter_deinterlace );
        }
        else if ([[queueToApply objectForKey:@"PictureDeinterlace"] intValue] == 3)
        {
            /* Yadif mode 0 (without spatial deinterlacing.) */
            hb_filter_deinterlace.settings = "2"; 
            hb_list_add( job->filters, &hb_filter_deinterlace );            
        }
        else if ([[queueToApply objectForKey:@"PictureDeinterlace"] intValue] == 4)
        {
            /* Yadif (with spatial deinterlacing) */
            hb_filter_deinterlace.settings = "0"; 
            hb_list_add( job->filters, &hb_filter_deinterlace );            
        }
        
        
    }
    /* Denoise */
	if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] == 1) // Custom in popup
	{
		/* we add the custom string if present */
        hb_filter_denoise.settings = (char *) [[queueToApply objectForKey:@"PictureDenoiseCustom"] UTF8String];
        hb_list_add( job->filters, &hb_filter_denoise );	
	}
    else if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] == 2) // Weak in popup
	{
		hb_filter_denoise.settings = "2:1:2:3"; 
        hb_list_add( job->filters, &hb_filter_denoise );	
	}
	else if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] == 3) // Medium in popup
	{
		hb_filter_denoise.settings = "3:2:2:3"; 
        hb_list_add( job->filters, &hb_filter_denoise );	
	}
	else if ([[queueToApply objectForKey:@"PictureDenoise"] intValue] == 4) // Strong in popup
	{
		hb_filter_denoise.settings = "7:7:5:5"; 
        hb_list_add( job->filters, &hb_filter_denoise );	
	}
    
    
    /* Deblock  (uses pp7 default) */
    /* NOTE: even though there is a valid deblock setting of 0 for the filter, for 
     * the macgui's purposes a value of 0 actually means to not even use the filter
     * current hb_filter_deblock.settings valid ranges are from 5 - 15 
     */
    if ([[queueToApply objectForKey:@"PictureDeblock"] intValue] != 0)
    {
        hb_filter_deblock.settings = (char *) [[queueToApply objectForKey:@"PictureDeblock"] UTF8String];
        hb_list_add( job->filters, &hb_filter_deblock );
    }
[self writeToActivityLog: "prepareJob exiting"];    
}



/* addToQueue: puts up an alert before ultimately calling doAddToQueue
*/
- (IBAction) addToQueue: (id) sender
{
	/* We get the destination directory from the destination field here */
	NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
	/* We check for a valid destination here */
	if ([[NSFileManager defaultManager] fileExistsAtPath:destinationDirectory] == 0) 
	{
		NSRunAlertPanel(@"Warning!", @"This is not a valid destination directory!", @"OK", nil, nil);
        return;
	}
    
    BOOL fileExists;
    fileExists = NO;
    
    BOOL fileExistsInQueue;
    fileExistsInQueue = NO;
    
    /* We check for and existing file here */
    if([[NSFileManager defaultManager] fileExistsAtPath: [fDstFile2Field stringValue]])
    {
        fileExists = YES;
    }
    
    /* We now run through the queue and make sure we are not overwriting an exisiting queue item */
    int i = 0;
    NSEnumerator *enumerator = [QueueFileArray objectEnumerator];
	id tempObject;
	while (tempObject = [enumerator nextObject])
	{
		NSDictionary *thisQueueDict = tempObject;
		if ([[thisQueueDict objectForKey:@"DestinationPath"] isEqualToString: [fDstFile2Field stringValue]])
		{
			fileExistsInQueue = YES;	
		}
        i++;
	}
    
    
	if(fileExists == YES)
    {
        NSBeginCriticalAlertSheet( NSLocalizedString( @"File already exists.", @"" ),
                                  NSLocalizedString( @"Cancel", @"" ), NSLocalizedString( @"Overwrite", @"" ), nil, fWindow, self,
                                  @selector( overwriteAddToQueueAlertDone:returnCode:contextInfo: ),
                                  NULL, NULL, [NSString stringWithFormat:
                                               NSLocalizedString( @"Do you want to overwrite %@?", @"" ),
                                               [fDstFile2Field stringValue]] );
    }
    else if (fileExistsInQueue == YES)
    {
    NSBeginCriticalAlertSheet( NSLocalizedString( @"There is already a queue item for this destination.", @"" ),
                                  NSLocalizedString( @"Cancel", @"" ), NSLocalizedString( @"Overwrite", @"" ), nil, fWindow, self,
                                  @selector( overwriteAddToQueueAlertDone:returnCode:contextInfo: ),
                                  NULL, NULL, [NSString stringWithFormat:
                                               NSLocalizedString( @"Do you want to overwrite %@?", @"" ),
                                               [fDstFile2Field stringValue]] );
    }
    else
    {
        [self doAddToQueue];
    }
}

/* overwriteAddToQueueAlertDone: called from the alert posted by addToQueue that asks
   the user if they want to overwrite an exiting movie file.
*/
- (void) overwriteAddToQueueAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSAlertAlternateReturn )
        [self doAddToQueue];
}

- (void) doAddToQueue
{
    [self addQueueFileItem ];
}



/* Rip: puts up an alert before ultimately calling doRip
*/
- (IBAction) Rip: (id) sender
{
    [self writeToActivityLog: "Rip: Pending queue count is %d", fPendingCount];
    /* Rip or Cancel ? */
    hb_state_t s;
    hb_get_state2( fQueueEncodeLibhb, &s );
    
    if(s.state == HB_STATE_WORKING || s.state == HB_STATE_PAUSED)
	{
        [self Cancel: sender];
        return;
    }
    
    /* We check to see if we need to warn the user that the computer will go to sleep
                 or shut down when encoding is finished */
                [self remindUserOfSleepOrShutdown];
    
    // If there are pending jobs in the queue, then this is a rip the queue
    if (fPendingCount > 0)
    {
        /* here lets start the queue with the first pending item */
        [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]]; 
        
        return;
    }
    
    // Before adding jobs to the queue, check for a valid destination.
    
    NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
    if ([[NSFileManager defaultManager] fileExistsAtPath:destinationDirectory] == 0) 
    {
        NSRunAlertPanel(@"Warning!", @"This is not a valid destination directory!", @"OK", nil, nil);
        return;
    }
    
    /* We check for duplicate name here */
    if( [[NSFileManager defaultManager] fileExistsAtPath:[fDstFile2Field stringValue]] )
    {
        NSBeginCriticalAlertSheet( NSLocalizedString( @"File already exists", @"" ),
                                  NSLocalizedString( @"Cancel", "" ), NSLocalizedString( @"Overwrite", @"" ), nil, fWindow, self,
                                  @selector( overWriteAlertDone:returnCode:contextInfo: ),
                                  NULL, NULL, [NSString stringWithFormat:
                                               NSLocalizedString( @"Do you want to overwrite %@?", @"" ),
                                               [fDstFile2Field stringValue]] );
        
        // overWriteAlertDone: will be called when the alert is dismissed. It will call doRip.
    }
    else
    {
        /* if there are no pending jobs in the queue, then add this one to the queue and rip
         otherwise, just rip the queue */
        if(fPendingCount == 0)
        {
            [self doAddToQueue];
        }
        
        /* go right to processing the new queue encode */
        [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]]; 
        
    }
}

/* overWriteAlertDone: called from the alert posted by Rip: that asks the user if they
   want to overwrite an exiting movie file.
*/
- (void) overWriteAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSAlertAlternateReturn )
    {
        /* if there are no jobs in the queue, then add this one to the queue and rip 
        otherwise, just rip the queue */
        if( fPendingCount == 0 )
        {
            [self doAddToQueue];
        }

        NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];
        [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]]; 
      
    }
}

- (void) remindUserOfSleepOrShutdown
{
       if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Put Computer To Sleep"])
       {
               /*Warn that computer will sleep after encoding*/
               int reminduser;
               NSBeep();
               reminduser = NSRunAlertPanel(@"The computer will sleep after encoding is done.",@"You have selected to sleep the computer after encoding. To turn off sleeping, go to the HandBrake preferences.", @"OK", @"Preferences...", nil);
               [NSApp requestUserAttention:NSCriticalRequest];
               if ( reminduser == NSAlertAlternateReturn )
               {
                       [self showPreferencesWindow:nil];
               }
       }
       else if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Shut Down Computer"])
       {
               /*Warn that computer will shut down after encoding*/
               int reminduser;
               NSBeep();
               reminduser = NSRunAlertPanel(@"The computer will shut down after encoding is done.",@"You have selected to shut down the computer after encoding. To turn off shut down, go to the HandBrake preferences.", @"OK", @"Preferences...", nil);
               [NSApp requestUserAttention:NSCriticalRequest];
               if ( reminduser == NSAlertAlternateReturn )
               {
                       [self showPreferencesWindow:nil];
               }
       }

}


- (void) doRip
{
    /* Let libhb do the job */
    hb_start( fQueueEncodeLibhb );
    /*set the fEncodeState State */
	fEncodeState = 1;
}


//------------------------------------------------------------------------------------
// Displays an alert asking user if the want to cancel encoding of current job.
// Cancel: returns immediately after posting the alert. Later, when the user
// acknowledges the alert, doCancelCurrentJob is called.
//------------------------------------------------------------------------------------
- (IBAction)Cancel: (id)sender
{
    if (!fQueueController) return;
    
  hb_pause( fQueueEncodeLibhb );
    NSString * alertTitle = [NSString stringWithFormat:NSLocalizedString(@"You are currently encoding. What would you like to do ?", nil)];
   
    // Which window to attach the sheet to?
    NSWindow * docWindow;
    if ([sender respondsToSelector: @selector(window)])
        docWindow = [sender window];
    else
        docWindow = fWindow;
        
    NSBeginCriticalAlertSheet(
            alertTitle,
            NSLocalizedString(@"Continue Encoding", nil),
            NSLocalizedString(@"Cancel Current and Stop", nil),
            NSLocalizedString(@"Cancel Current and Continue", nil),
            docWindow, self,
            nil, @selector(didDimissCancel:returnCode:contextInfo:), nil,
            NSLocalizedString(@"Your encode will be cancelled if you don't continue encoding.", nil));
    
    // didDimissCancelCurrentJob:returnCode:contextInfo: will be called when the dialog is dismissed
}

- (void) didDimissCancel: (NSWindow *)sheet returnCode: (int)returnCode contextInfo: (void *)contextInfo
{
   hb_resume( fQueueEncodeLibhb );
     if (returnCode == NSAlertOtherReturn)
    {
        [self doCancelCurrentJob];  // <- this also stops libhb
    }
    if (returnCode == NSAlertAlternateReturn)
    {
    [self doCancelCurrentJobAndStop];
    }
}

//------------------------------------------------------------------------------------
// Cancels and deletes the current job and stops libhb from processing the remaining
// encodes.
//------------------------------------------------------------------------------------
- (void) doCancelCurrentJob
{
    // Stop the current job. hb_stop will only cancel the current pass and then set
    // its state to HB_STATE_WORKDONE. It also does this asynchronously. So when we
    // see the state has changed to HB_STATE_WORKDONE (in updateUI), we'll delete the
    // remaining passes of the job and then start the queue back up if there are any
    // remaining jobs.
     
    
    hb_stop( fQueueEncodeLibhb );
    
    // Delete all remaining jobs since libhb doesn't do this on its own.
            hb_job_t * job;
            while( ( job = hb_job(fQueueEncodeLibhb, 0) ) )
                hb_rem( fQueueEncodeLibhb, job );
                
    fEncodeState = 2;   // don't alert at end of processing since this was a cancel
    
    // now that we've stopped the currently encoding job, lets mark it as cancelled
    [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] setObject:[NSNumber numberWithInt:3] forKey:@"Status"];
    // and as always, save it in the queue .plist...
    /* We save all of the Queue data here */
    [self saveQueueFileItem];
    // so now lets move to 
    currentQueueEncodeIndex++ ;
    // ... and see if there are more items left in our queue
    int queueItems = [QueueFileArray count];
    /* If we still have more items in our queue, lets go to the next one */
    if (currentQueueEncodeIndex < queueItems)
    {
    [self writeToActivityLog: "doCancelCurrentJob currentQueueEncodeIndex is incremented to: %d", currentQueueEncodeIndex];
    [self writeToActivityLog: "doCancelCurrentJob moving to the next job"];
    
    [self performNewQueueScan:[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"SourcePath"] scanTitleNum:[[[QueueFileArray objectAtIndex:currentQueueEncodeIndex] objectForKey:@"TitleNumber"]intValue]];
    }
    else
    {
        [self writeToActivityLog: "doCancelCurrentJob the item queue is complete"];
    }

}

- (void) doCancelCurrentJobAndStop
{
    hb_stop( fQueueEncodeLibhb );
    
    // Delete all remaining jobs since libhb doesn't do this on its own.
            hb_job_t * job;
            while( ( job = hb_job(fQueueEncodeLibhb, 0) ) )
                hb_rem( fQueueEncodeLibhb, job );
                
                
    fEncodeState = 2;   // don't alert at end of processing since this was a cancel
    
    // now that we've stopped the currently encoding job, lets mark it as cancelled
    [[QueueFileArray objectAtIndex:currentQueueEncodeIndex] setObject:[NSNumber numberWithInt:3] forKey:@"Status"];
    // and as always, save it in the queue .plist...
    /* We save all of the Queue data here */
    [self saveQueueFileItem];
    // so now lets move to 
    currentQueueEncodeIndex++ ;
    [self writeToActivityLog: "cancelling current job and stopping the queue"];
}
- (IBAction) Pause: (id) sender
{
    hb_state_t s;
    hb_get_state2( fQueueEncodeLibhb, &s );

    if( s.state == HB_STATE_PAUSED )
    {
        hb_resume( fQueueEncodeLibhb );
    }
    else
    {
        hb_pause( fQueueEncodeLibhb );
    }
}

#pragma mark -
#pragma mark GUI Controls Changed Methods

- (IBAction) titlePopUpChanged: (id) sender
{
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t*)
        hb_list_item( list, [fSrcTitlePopUp indexOfSelectedItem] );

    /* If Auto Naming is on. We create an output filename of dvd name - title number */
    if( [[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"] > 0 && ( hb_list_count( list ) > 1 ) )
	{
		[fDstFile2Field setStringValue: [NSString stringWithFormat:
			@"%@/%@-%d.%@", [[fDstFile2Field stringValue] stringByDeletingLastPathComponent],
			[browsedSourceDisplayName stringByDeletingPathExtension],
            title->index,
			[[fDstFile2Field stringValue] pathExtension]]];	
	}

    /* Update chapter popups */
    [fSrcChapterStartPopUp removeAllItems];
    [fSrcChapterEndPopUp   removeAllItems];
    for( int i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        [fSrcChapterStartPopUp addItemWithTitle: [NSString
            stringWithFormat: @"%d", i + 1]];
        [fSrcChapterEndPopUp addItemWithTitle: [NSString
            stringWithFormat: @"%d", i + 1]];
    }

    [fSrcChapterStartPopUp selectItemAtIndex: 0];
    [fSrcChapterEndPopUp   selectItemAtIndex:
        hb_list_count( title->list_chapter ) - 1];
    [self chapterPopUpChanged:nil];
    
    /* if using dvd nav, show the angle widget */
    if ([[[NSUserDefaults standardUserDefaults] objectForKey:@"UseDvdNav"] boolValue])
    {
        [fSrcAngleLabel setHidden:NO];
        [fSrcAnglePopUp setHidden:NO];
        
        [fSrcAnglePopUp removeAllItems];
        for( int i = 0; i < title->angle_count; i++ )
        {
            [fSrcAnglePopUp addItemWithTitle: [NSString stringWithFormat: @"%d", i + 1]];
        }
        [fSrcAnglePopUp selectItemAtIndex: 0];
    }
    else
    {
        [fSrcAngleLabel setHidden:YES];
        [fSrcAnglePopUp setHidden:YES];
    }
    
    /* Start Get and set the initial pic size for display */
	hb_job_t * job = title->job;
	fTitle = title;
    
    /* Set Auto Crop to on upon selecting a new title  */
    [fPictureController setAutoCrop:YES];
    
	/* We get the originial output picture width and height and put them
	in variables for use with some presets later on */
	PicOrigOutputWidth = job->width;
	PicOrigOutputHeight = job->height;
	AutoCropTop = job->crop[0];
	AutoCropBottom = job->crop[1];
	AutoCropLeft = job->crop[2];
	AutoCropRight = job->crop[3];

	/* Reset the new title in fPictureController &&  fPreviewController*/
    [fPictureController SetTitle:title];

        
    /* Update Subtitle Table */
    [fSubtitlesDelegate resetWithTitle:title];
    [fSubtitlesTable reloadData];
    

    /* Update chapter table */
    [fChapterTitlesDelegate resetWithTitle:title];
    [fChapterTable reloadData];

   /* Lets make sure there arent any erroneous audio tracks in the job list, so lets make sure its empty*/
    int audiotrack_count = hb_list_count(job->list_audio);
    for( int i = 0; i < audiotrack_count;i++)
    {
        hb_audio_t * temp_audio = (hb_audio_t*) hb_list_item( job->list_audio, 0 );
        hb_list_rem(job->list_audio, temp_audio);
    }

    /* Update audio popups */
    [self addAllAudioTracksToPopUp: fAudLang1PopUp];
    [self addAllAudioTracksToPopUp: fAudLang2PopUp];
    [self addAllAudioTracksToPopUp: fAudLang3PopUp];
    [self addAllAudioTracksToPopUp: fAudLang4PopUp];
    /* search for the first instance of our prefs default language for track 1, and set track 2 to "none" */
	NSString * audioSearchPrefix = [[NSUserDefaults standardUserDefaults] stringForKey:@"DefaultLanguage"];
        [self selectAudioTrackInPopUp: fAudLang1PopUp searchPrefixString: audioSearchPrefix selectIndexIfNotFound: 1];
    [self selectAudioTrackInPopUp:fAudLang2PopUp searchPrefixString:nil selectIndexIfNotFound:0];
    [self selectAudioTrackInPopUp:fAudLang3PopUp searchPrefixString:nil selectIndexIfNotFound:0];
    [self selectAudioTrackInPopUp:fAudLang4PopUp searchPrefixString:nil selectIndexIfNotFound:0];

	/* changing the title may have changed the audio channels on offer, */
	/* so call audioTrackPopUpChanged for both audio tracks to update the mixdown popups */
	[self audioTrackPopUpChanged: fAudLang1PopUp];
	[self audioTrackPopUpChanged: fAudLang2PopUp];
    [self audioTrackPopUpChanged: fAudLang3PopUp];
    [self audioTrackPopUpChanged: fAudLang4PopUp];

    [fVidRatePopUp selectItemAtIndex: 0];

    /* we run the picture size values through calculatePictureSizing to get all picture setting	information*/
	[self calculatePictureSizing:nil];

   /* lets call tableViewSelected to make sure that any preset we have selected is enforced after a title change */
    [self selectPreset:nil];
}

- (IBAction) chapterPopUpChanged: (id) sender
{

	/* If start chapter popup is greater than end chapter popup,
	we set the end chapter popup to the same as start chapter popup */
	if ([fSrcChapterStartPopUp indexOfSelectedItem] > [fSrcChapterEndPopUp indexOfSelectedItem])
	{
		[fSrcChapterEndPopUp selectItemAtIndex: [fSrcChapterStartPopUp indexOfSelectedItem]];
    }

		
	hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *)
        hb_list_item( list, [fSrcTitlePopUp indexOfSelectedItem] );

    hb_chapter_t * chapter;
    int64_t        duration = 0;
    for( int i = [fSrcChapterStartPopUp indexOfSelectedItem];
         i <= [fSrcChapterEndPopUp indexOfSelectedItem]; i++ )
    {
        chapter = (hb_chapter_t *) hb_list_item( title->list_chapter, i );
        duration += chapter->duration;
    }
    
    duration /= 90000; /* pts -> seconds */
    [fSrcDuration2Field setStringValue: [NSString stringWithFormat:
        @"%02lld:%02lld:%02lld", duration / 3600, ( duration / 60 ) % 60,
        duration % 60]];

    [self calculateBitrate: sender];
    
    if ( [fSrcChapterStartPopUp indexOfSelectedItem] ==  [fSrcChapterEndPopUp indexOfSelectedItem] )
    {
    /* Disable chapter markers for any source with less than two chapters as it makes no sense. */
    [fCreateChapterMarkers setEnabled: NO];
    [fCreateChapterMarkers setState: NSOffState];
    }
    else
    {
    [fCreateChapterMarkers setEnabled: YES];
    }
}

- (IBAction) formatPopUpChanged: (id) sender
{
    NSString * string = [fDstFile2Field stringValue];
    int format = [fDstFormatPopUp indexOfSelectedItem];
    char * ext = NULL;
	/* Initially set the large file (64 bit formatting) output checkbox to hidden */
    [fDstMp4LargeFileCheck setHidden: YES];
    [fDstMp4HttpOptFileCheck setHidden: YES];
    [fDstMp4iPodFileCheck setHidden: YES];
    
    /* Update the Video Codec PopUp */
    /* lets get the tag of the currently selected item first so we might reset it later */
    int selectedVidEncoderTag;
    selectedVidEncoderTag = [[fVidEncoderPopUp selectedItem] tag];
    
    /* Note: we now store the video encoder int values from common.c in the tags of each popup for easy retrieval later */
    [fVidEncoderPopUp removeAllItems];
    NSMenuItem *menuItem;
    /* These video encoders are available to all of our current muxers, so lets list them once here */
    menuItem = [[fVidEncoderPopUp menu] addItemWithTitle:@"MPEG-4 (FFmpeg)" action: NULL keyEquivalent: @""];
    [menuItem setTag: HB_VCODEC_FFMPEG];
    
    switch( format )
    {
        case 0:
			/*Get Default MP4 File Extension*/
			if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultMpegName"] > 0)
			{
				ext = "m4v";
			}
			else
			{
				ext = "mp4";
			}
            /* Add additional video encoders here */
            menuItem = [[fVidEncoderPopUp menu] addItemWithTitle:@"H.264 (x264)" action: NULL keyEquivalent: @""];
            [menuItem setTag: HB_VCODEC_X264];
            /* We show the mp4 option checkboxes here since we are mp4 */
            [fCreateChapterMarkers setEnabled: YES];
			[fDstMp4LargeFileCheck setHidden: NO];
			[fDstMp4HttpOptFileCheck setHidden: NO];
            [fDstMp4iPodFileCheck setHidden: NO];
            break;
            
            case 1:
            ext = "mkv";
            /* Add additional video encoders here */
            menuItem = [[fVidEncoderPopUp menu] addItemWithTitle:@"H.264 (x264)" action: NULL keyEquivalent: @""];
            [menuItem setTag: HB_VCODEC_X264];
            menuItem = [[fVidEncoderPopUp menu] addItemWithTitle:@"VP3 (Theora)" action: NULL keyEquivalent: @""];
            [menuItem setTag: HB_VCODEC_THEORA];
            /* We enable the create chapters checkbox here */
			[fCreateChapterMarkers setEnabled: YES];
			break;
            

    }
    /* tell fSubtitlesDelegate we have a new video container */
    
    [fSubtitlesDelegate containerChanged:[[fDstFormatPopUp selectedItem] tag]];
    [fSubtitlesTable reloadData];
    /* if we have a previously selected vid encoder tag, then try to select it */
    if (selectedVidEncoderTag)
    {
        [fVidEncoderPopUp selectItemWithTag: selectedVidEncoderTag];
    }
    else
    {
        [fVidEncoderPopUp selectItemAtIndex: 0];
    }

    [self audioAddAudioTrackCodecs: fAudTrack1CodecPopUp];
    [self audioAddAudioTrackCodecs: fAudTrack2CodecPopUp];
    [self audioAddAudioTrackCodecs: fAudTrack3CodecPopUp];
    [self audioAddAudioTrackCodecs: fAudTrack4CodecPopUp];

    if( format == 0 )
        [self autoSetM4vExtension: sender];
    else
        [fDstFile2Field setStringValue: [NSString stringWithFormat:@"%@.%s", [string stringByDeletingPathExtension], ext]];

    if( SuccessfulScan )
    {
        /* Add/replace to the correct extension */
        [self audioTrackPopUpChanged: fAudLang1PopUp];
        [self audioTrackPopUpChanged: fAudLang2PopUp];
        [self audioTrackPopUpChanged: fAudLang3PopUp];
        [self audioTrackPopUpChanged: fAudLang4PopUp];

        if( [fVidEncoderPopUp selectedItem] == nil )
        {

            [fVidEncoderPopUp selectItemAtIndex:0];
            [self videoEncoderPopUpChanged:nil];

            /* changing the format may mean that we can / can't offer mono or 6ch, */
            /* so call audioTrackPopUpChanged for both audio tracks to update the mixdown popups */

            /* We call the method to properly enable/disable turbo 2 pass */
            [self twoPassCheckboxChanged: sender];
            /* We call method method to change UI to reflect whether a preset is used or not*/
        }
    }
	[self customSettingUsed: sender];
}

- (IBAction) autoSetM4vExtension: (id) sender
{
    if ( [fDstFormatPopUp indexOfSelectedItem] )
        return;

    NSString * extension = @"mp4";

    if( [[fAudTrack1CodecPopUp selectedItem] tag] == HB_ACODEC_AC3 || [[fAudTrack2CodecPopUp selectedItem] tag] == HB_ACODEC_AC3 ||
                                                        [[fAudTrack3CodecPopUp selectedItem] tag] == HB_ACODEC_AC3 ||
                                                        [[fAudTrack4CodecPopUp selectedItem] tag] == HB_ACODEC_AC3 ||
                                                        [fCreateChapterMarkers state] == NSOnState ||
                                                        [[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultMpegName"] > 0 )
    {
        extension = @"m4v";
    }

    if( [extension isEqualTo: [[fDstFile2Field stringValue] pathExtension]] )
        return;
    else
        [fDstFile2Field setStringValue: [NSString stringWithFormat:@"%@.%@",
                                    [[fDstFile2Field stringValue] stringByDeletingPathExtension], extension]];
}

/* Method to determine if we should change the UI
To reflect whether or not a Preset is being used or if
the user is using "Custom" settings by determining the sender*/
- (IBAction) customSettingUsed: (id) sender
{
	if ([sender stringValue])
	{
		/* Deselect the currently selected Preset if there is one*/
		[fPresetsOutlineView deselectRow:[fPresetsOutlineView selectedRow]];
		/* Change UI to show "Custom" settings are being used */
		[fPresetSelectedDisplay setStringValue: @"Custom"];

		curUserPresetChosenNum = nil;
	}
[self calculateBitrate:nil];
}


#pragma mark -
#pragma mark - Video

- (IBAction) videoEncoderPopUpChanged: (id) sender
{
    hb_job_t * job = fTitle->job;
    int videoEncoder = [[fVidEncoderPopUp selectedItem] tag];
    
    [fAdvancedOptions setHidden:YES];
    /* If we are using x264 then show the x264 advanced panel*/
    if (videoEncoder == HB_VCODEC_X264)
    {
        [fAdvancedOptions setHidden:NO];
        [self autoSetM4vExtension: sender];
    }
    
    /* We need to set loose anamorphic as available depending on whether or not the ffmpeg encoder
    is being used as it borks up loose anamorphic .
    For convenience lets use the titleOfSelected index. Probably should revisit whether or not we want
    to use the index itself but this is easier */
    if (videoEncoder == HB_VCODEC_FFMPEG)
    {
        if (job->anamorphic.mode == 2)
        {
            job->anamorphic.mode = 0;
        }
        [fPictureController setAllowLooseAnamorphic:NO];
        /* We set the iPod atom checkbox to disabled and uncheck it as its only for x264 in the mp4
         container. Format is taken care of in formatPopUpChanged method by hiding and unchecking
         anything other than MP4.
         */ 
        [fDstMp4iPodFileCheck setEnabled: NO];
        [fDstMp4iPodFileCheck setState: NSOffState];
    }
    else
    {
        [fPictureController setAllowLooseAnamorphic:YES];
        [fDstMp4iPodFileCheck setEnabled: YES];
    }
    [self setupQualitySlider];
	[self calculatePictureSizing: sender];
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
	[self customSettingUsed: sender];
}

- (IBAction ) videoFrameRateChanged: (id) sender
{
    /* We call method method to calculatePictureSizing to error check detelecine*/
    [self calculatePictureSizing: sender];

    /* We call method method to change UI to reflect whether a preset is used or not*/
	[self customSettingUsed: sender];
}
- (IBAction) videoMatrixChanged: (id) sender;
{
    bool target, bitrate, quality;

    target = bitrate = quality = false;
    if( [fVidQualityMatrix isEnabled] )
    {
        switch( [fVidQualityMatrix selectedRow] )
        {
            case 0:
                target = true;
                break;
            case 1:
                bitrate = true;
                break;
            case 2:
                quality = true;
                break;
        }
    }
    [fVidTargetSizeField  setEnabled: target];
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
    [self calculateBitrate: sender];
	[self customSettingUsed: sender];
}

/* Use this method to setup the quality slider for cq/rf values depending on
 * the video encoder selected.
 */
- (void) setupQualitySlider
{
    /* Get the current slider maxValue to check for a change in slider scale later
     * so that we can choose a new similar value on the new slider scale */
    float previousMaxValue = [fVidQualitySlider maxValue];
    float previousPercentOfSliderScale = [fVidQualitySlider floatValue] / ([fVidQualitySlider maxValue] - [fVidQualitySlider minValue] + 1);
    NSString * qpRFLabelString = @"QP:";
    /* x264 0-51 */
    if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_X264)
    {
        [fVidQualitySlider setMinValue:0.0];
        [fVidQualitySlider setMaxValue:51.0];
        /* As x264 allows for qp/rf values that are fractional, we get the value from the preferences */
        int fractionalGranularity = 1 / [[NSUserDefaults standardUserDefaults] floatForKey:@"x264CqSliderFractional"];
        [fVidQualitySlider setNumberOfTickMarks:(([fVidQualitySlider maxValue] - [fVidQualitySlider minValue]) * fractionalGranularity) + 1];
        if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultCrf"] > 0)
        {
            qpRFLabelString = @"RF:";
        }
    }
    /* ffmpeg  1-31 */
    if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_FFMPEG )
    {
        [fVidQualitySlider setMinValue:1.0];
        [fVidQualitySlider setMaxValue:31.0];
        [fVidQualitySlider setNumberOfTickMarks:31];
    }
    /* Theora 0-63 */
    if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_THEORA)
    {
        [fVidQualitySlider setMinValue:0.0];
        [fVidQualitySlider setMaxValue:63.0];
        [fVidQualitySlider setNumberOfTickMarks:64];
    }
    [fVidQualityRFLabel setStringValue:qpRFLabelString];
    
    /* check to see if we have changed slider scales */
    if (previousMaxValue != [fVidQualitySlider maxValue])
    {
        /* if so, convert the old setting to the new scale as close as possible based on percentages */
        float rf =  ([fVidQualitySlider maxValue] - [fVidQualitySlider minValue] + 1) * previousPercentOfSliderScale;
        [fVidQualitySlider setFloatValue:rf];
    }
    
    [self qualitySliderChanged:nil];
}

- (IBAction) qualitySliderChanged: (id) sender
{
    /* Our constant quality slider is in a range based
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
    
    float sliderRfInverse = ([fVidQualitySlider maxValue] - [fVidQualitySlider floatValue]) + [fVidQualitySlider minValue];
    /* If the encoder is theora, use the float, otherwise use the inverse float*/
    float sliderRfToPercent;
    if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_THEORA)
    {
        [fVidQualityRFField setStringValue: [NSString stringWithFormat: @"%.2f", [fVidQualitySlider floatValue]]];
        sliderRfToPercent = [fVidQualityRFField floatValue] / ([fVidQualitySlider maxValue] - [fVidQualitySlider minValue]);   
    }
    else
    {
        [fVidQualityRFField setStringValue: [NSString stringWithFormat: @"%.2f", sliderRfInverse]];
        sliderRfToPercent = ( ([fVidQualitySlider maxValue] - [fVidQualitySlider minValue])  - ([fVidQualityRFField floatValue] - [fVidQualitySlider minValue])) / ([fVidQualitySlider maxValue] - [fVidQualitySlider minValue]);
    }
    [fVidConstantCell setTitle: [NSString stringWithFormat:
                                 NSLocalizedString( @"Constant quality: %.2f %%", @"" ), 100 * sliderRfToPercent]];
    
    [self customSettingUsed: sender];
}

- (void) controlTextDidChange: (NSNotification *) notification
{
    [self calculateBitrate:nil];
}

- (IBAction) calculateBitrate: (id) sender
{
    if( !fHandle || [fVidQualityMatrix selectedRow] != 0 || !SuccessfulScan )
    {
        return;
    }

    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;
    hb_audio_config_t * audio;
    /* For  hb_calc_bitrate in addition to the Target Size in MB out of the
     * Target Size Field, we also need the job info for the Muxer, the Chapters
     * as well as all of the audio track info.
     * This used to be accomplished by simply calling prepareJob here, however
     * since the resilient queue sets the queue array values instead of the job
     * values directly, we duplicate the old prepareJob code here for the variables
     * needed
     */
    job->chapter_start = [fSrcChapterStartPopUp indexOfSelectedItem] + 1;
    job->chapter_end = [fSrcChapterEndPopUp indexOfSelectedItem] + 1; 
    job->mux = [[fDstFormatPopUp selectedItem] tag];
    
    /* Audio goes here */
    int audiotrack_count = hb_list_count(job->list_audio);
    for( int i = 0; i < audiotrack_count;i++)
    {
        hb_audio_t * temp_audio = (hb_audio_t*) hb_list_item( job->list_audio, 0 );
        hb_list_rem(job->list_audio, temp_audio);
    }
    /* Now we need our audio info here for each track if applicable */
    if ([fAudLang1PopUp indexOfSelectedItem] > 0)
    {
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [fAudLang1PopUp indexOfSelectedItem] - 1;
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [fAudLang1PopUp indexOfSelectedItem] - 1;
        audio->out.codec = [[fAudTrack1CodecPopUp selectedItem] tag];
        audio->out.mixdown = [[fAudTrack1MixPopUp selectedItem] tag];
        audio->out.bitrate = [[fAudTrack1BitratePopUp selectedItem] tag];
        audio->out.samplerate = [[fAudTrack1RatePopUp selectedItem] tag];
        audio->out.dynamic_range_compression = [fAudTrack1DrcField floatValue];
        
        hb_audio_add( job, audio );
        free(audio);
    }  
    if ([fAudLang2PopUp indexOfSelectedItem] > 0)
    {
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [fAudLang2PopUp indexOfSelectedItem] - 1;
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [fAudLang2PopUp indexOfSelectedItem] - 1;
        audio->out.codec = [[fAudTrack2CodecPopUp selectedItem] tag];
        audio->out.mixdown = [[fAudTrack2MixPopUp selectedItem] tag];
        audio->out.bitrate = [[fAudTrack2BitratePopUp selectedItem] tag];
        audio->out.samplerate = [[fAudTrack2RatePopUp selectedItem] tag];
        audio->out.dynamic_range_compression = [fAudTrack2DrcField floatValue];
        
        hb_audio_add( job, audio );
        free(audio);
        
    }
    
    if ([fAudLang3PopUp indexOfSelectedItem] > 0)
    {
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [fAudLang3PopUp indexOfSelectedItem] - 1;
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [fAudLang3PopUp indexOfSelectedItem] - 1;
        audio->out.codec = [[fAudTrack3CodecPopUp selectedItem] tag];
        audio->out.mixdown = [[fAudTrack3MixPopUp selectedItem] tag];
        audio->out.bitrate = [[fAudTrack3BitratePopUp selectedItem] tag];
        audio->out.samplerate = [[fAudTrack3RatePopUp selectedItem] tag];
        audio->out.dynamic_range_compression = [fAudTrack3DrcField floatValue];
        
        hb_audio_add( job, audio );
        free(audio);
        
    }

    if ([fAudLang4PopUp indexOfSelectedItem] > 0)
    {
        audio = (hb_audio_config_t *) calloc(1, sizeof(*audio));
        hb_audio_config_init(audio);
        audio->in.track = [fAudLang4PopUp indexOfSelectedItem] - 1;
        /* We go ahead and assign values to our audio->out.<properties> */
        audio->out.track = [fAudLang4PopUp indexOfSelectedItem] - 1;
        audio->out.codec = [[fAudTrack4CodecPopUp selectedItem] tag];
        audio->out.mixdown = [[fAudTrack4MixPopUp selectedItem] tag];
        audio->out.bitrate = [[fAudTrack4BitratePopUp selectedItem] tag];
        audio->out.samplerate = [[fAudTrack4RatePopUp selectedItem] tag];
        audio->out.dynamic_range_compression = [fAudTrack4DrcField floatValue];
        
        hb_audio_add( job, audio );
        free(audio);
        
    }
       
[fVidBitrateField setIntValue: hb_calc_bitrate( job, [fVidTargetSizeField intValue] )];
}

#pragma mark -
#pragma mark - Picture

/* lets set the picture size back to the max from right after title scan
   Lets use an IBAction here as down the road we could always use a checkbox
   in the gui to easily take the user back to max. Remember, the compiler
   resolves IBActions down to -(void) during compile anyway */
- (IBAction) revertPictureSizeToMax: (id) sender
{
	hb_job_t * job = fTitle->job;
	/* Here we apply the title source and height */
    job->width = fTitle->width;
    job->height = fTitle->height;
    
    [self calculatePictureSizing: sender];
    /* We call method to change UI to reflect whether a preset is used or not*/    
    [self customSettingUsed: sender];
}

/**
 * Registers changes made in the Picture Settings Window.
 */

- (void)pictureSettingsDidChange 
{
	[self calculatePictureSizing:nil];
}

/* Get and Display Current Pic Settings in main window */
- (IBAction) calculatePictureSizing: (id) sender
{
	if (fTitle->job->anamorphic.mode > 0)
	{
        fTitle->job->keep_ratio = 0;
	}
    
    [fPictureSizeField setStringValue: [NSString stringWithFormat:@"Picture Size: %@", [fPictureController getPictureSizeInfoString]]];
    
    NSString *picCropping;
    /* Set the display field for crop as per boolean */
	if (![fPictureController autoCrop])
	{
        picCropping =  @"Custom";
	}
	else
	{
		picCropping =  @"Auto";
	}
    picCropping = [picCropping stringByAppendingString:[NSString stringWithFormat:@" %d/%d/%d/%d",fTitle->job->crop[0],fTitle->job->crop[1],fTitle->job->crop[2],fTitle->job->crop[3]]];
    
    [fPictureCroppingField setStringValue: [NSString stringWithFormat:@"Picture Cropping: %@",picCropping]];
    
    NSString *videoFilters;
    videoFilters = @"";
    /* Detelecine */
    if ([fPictureController detelecine] == 2) 
    {
        videoFilters = [videoFilters stringByAppendingString:@" - Detelecine (Default)"];
    }
    else if ([fPictureController detelecine] == 1) 
    {
        videoFilters = [videoFilters stringByAppendingString:[NSString stringWithFormat:@" - Detelecine (%@)",[fPictureController detelecineCustomString]]];
    }
    
    
    if ([fPictureController useDecomb] == 1)
    {
        /* Decomb */
        if ([fPictureController decomb] == 2)
        {
            videoFilters = [videoFilters stringByAppendingString:@" - Decomb (Default)"];
        }
        else if ([fPictureController decomb] == 1)
        {
            videoFilters = [videoFilters stringByAppendingString:[NSString stringWithFormat:@" - Decomb (%@)",[fPictureController decombCustomString]]];
        }
    }
    else
    {
        /* Deinterlace */
        if ([fPictureController deinterlace] > 0)
        {
            fTitle->job->deinterlace  = 1;
        }
        else
        {
            fTitle->job->deinterlace  = 0;
        }
        
        if ([fPictureController deinterlace] == 2)
        {
            videoFilters = [videoFilters stringByAppendingString:@" - Deinterlace (Fast)"];
        }
        else if ([fPictureController deinterlace] == 3)
        {
            videoFilters = [videoFilters stringByAppendingString:@" - Deinterlace (Slow)"];
        }
        else if ([fPictureController deinterlace] == 4)
        {
            videoFilters = [videoFilters stringByAppendingString:@" - Deinterlace (Slower)"];
        }
        else if ([fPictureController deinterlace] == 1)
        {
            videoFilters = [videoFilters stringByAppendingString:[NSString stringWithFormat:@" - Deinterlace (%@)",[fPictureController deinterlaceCustomString]]];
        }
	}
    
    
    /* Denoise */
	if ([fPictureController denoise] == 2)
	{
		videoFilters = [videoFilters stringByAppendingString:@" - Denoise (Weak)"];
    }
	else if ([fPictureController denoise] == 3)
	{
		videoFilters = [videoFilters stringByAppendingString:@" - Denoise (Medium)"];
    }
	else if ([fPictureController denoise] == 4)
	{
		videoFilters = [videoFilters stringByAppendingString:@" - Denoise (Strong)"];
	}
    else if ([fPictureController denoise] == 1)
	{
		videoFilters = [videoFilters stringByAppendingString:[NSString stringWithFormat:@" - Denoise (%@)",[fPictureController denoiseCustomString]]];
	}
    
    /* Deblock */
    if ([fPictureController deblock] > 0) 
    {
        videoFilters = [videoFilters stringByAppendingString:[NSString stringWithFormat:@" - Deblock (%d)",[fPictureController deblock]]];
    }
	
    /* Grayscale */
    if ([fPictureController grayscale]) 
    {
        videoFilters = [videoFilters stringByAppendingString:@" - Grayscale"];
    }
    [fVideoFiltersField setStringValue: [NSString stringWithFormat:@"Video Filters: %@", videoFilters]];
    
    //[fPictureController reloadStillPreview]; 
}


#pragma mark -
#pragma mark - Audio and Subtitles
- (IBAction) audioCodecsPopUpChanged: (id) sender
{
    
    NSPopUpButton * audiotrackPopUp;
    NSPopUpButton * sampleratePopUp;
    NSPopUpButton * bitratePopUp;
    NSPopUpButton * audiocodecPopUp;
    if (sender == fAudTrack1CodecPopUp)
    {
        audiotrackPopUp = fAudLang1PopUp;
        audiocodecPopUp = fAudTrack1CodecPopUp;
        sampleratePopUp = fAudTrack1RatePopUp;
        bitratePopUp = fAudTrack1BitratePopUp;
    }
    else if (sender == fAudTrack2CodecPopUp)
    {
        audiotrackPopUp = fAudLang2PopUp;
        audiocodecPopUp = fAudTrack2CodecPopUp;
        sampleratePopUp = fAudTrack2RatePopUp;
        bitratePopUp = fAudTrack2BitratePopUp;
    }
    else if (sender == fAudTrack3CodecPopUp)
    {
        audiotrackPopUp = fAudLang3PopUp;
        audiocodecPopUp = fAudTrack3CodecPopUp;
        sampleratePopUp = fAudTrack3RatePopUp;
        bitratePopUp = fAudTrack3BitratePopUp;
    }
    else
    {
        audiotrackPopUp = fAudLang4PopUp;
        audiocodecPopUp = fAudTrack4CodecPopUp;
        sampleratePopUp = fAudTrack4RatePopUp;
        bitratePopUp = fAudTrack4BitratePopUp;
    }
	
    /* changing the codecs on offer may mean that we can / can't offer mono or 6ch, */
	/* so call audioTrackPopUpChanged for both audio tracks to update the mixdown popups */
    [self audioTrackPopUpChanged: audiotrackPopUp];
    
}

- (IBAction) setEnabledStateOfAudioMixdownControls: (id) sender
{
    /* We will be setting the enabled/disabled state of each tracks audio controls based on
     * the settings of the source audio for that track. We leave the samplerate and bitrate
     * to audiotrackMixdownChanged
     */
    
    /* We will first verify that a lower track number has been selected before enabling each track
     * for example, make sure a track is selected for track 1 before enabling track 2, etc.
     */
    if ([fAudLang1PopUp indexOfSelectedItem] == 0)
    {
        [fAudLang2PopUp setEnabled: NO];
        [fAudLang2PopUp selectItemAtIndex: 0];
    }
    else
    {
        [fAudLang2PopUp setEnabled: YES];
    }
    
    if ([fAudLang2PopUp indexOfSelectedItem] == 0)
    {
        [fAudLang3PopUp setEnabled: NO];
        [fAudLang3PopUp selectItemAtIndex: 0];
    }
    else
    {
        [fAudLang3PopUp setEnabled: YES];
    }
    if ([fAudLang3PopUp indexOfSelectedItem] == 0)
    {
        [fAudLang4PopUp setEnabled: NO];
        [fAudLang4PopUp selectItemAtIndex: 0];
    }
    else
    {
        [fAudLang4PopUp setEnabled: YES];
    }
    /* enable/disable the mixdown text and popupbutton for audio track 1 */
    [fAudTrack1CodecPopUp setEnabled: ([fAudLang1PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack1MixPopUp setEnabled: ([fAudLang1PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack1RatePopUp setEnabled: ([fAudLang1PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack1BitratePopUp setEnabled: ([fAudLang1PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack1DrcSlider setEnabled: ([fAudLang1PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack1DrcField setEnabled: ([fAudLang1PopUp indexOfSelectedItem] == 0) ? NO : YES];
    if ([fAudLang1PopUp indexOfSelectedItem] == 0)
    {
        [fAudTrack1CodecPopUp removeAllItems];
        [fAudTrack1MixPopUp removeAllItems];
        [fAudTrack1RatePopUp removeAllItems];
        [fAudTrack1BitratePopUp removeAllItems];
        [fAudTrack1DrcSlider setFloatValue: 1.00];
        [self audioDRCSliderChanged: fAudTrack1DrcSlider];
    }
    else if ([[fAudTrack1MixPopUp selectedItem] tag] == HB_ACODEC_AC3 || [[fAudTrack1MixPopUp selectedItem] tag] == HB_ACODEC_DCA)
    {
        [fAudTrack1RatePopUp setEnabled: NO];
        [fAudTrack1BitratePopUp setEnabled: NO];
        [fAudTrack1DrcSlider setEnabled: NO];
        [fAudTrack1DrcField setEnabled: NO];
    }
    
    /* enable/disable the mixdown text and popupbutton for audio track 2 */
    [fAudTrack2CodecPopUp setEnabled: ([fAudLang2PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack2MixPopUp setEnabled: ([fAudLang2PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack2RatePopUp setEnabled: ([fAudLang2PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack2BitratePopUp setEnabled: ([fAudLang2PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack2DrcSlider setEnabled: ([fAudLang2PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack2DrcField setEnabled: ([fAudLang2PopUp indexOfSelectedItem] == 0) ? NO : YES];
    if ([fAudLang2PopUp indexOfSelectedItem] == 0)
    {
        [fAudTrack2CodecPopUp removeAllItems];
        [fAudTrack2MixPopUp removeAllItems];
        [fAudTrack2RatePopUp removeAllItems];
        [fAudTrack2BitratePopUp removeAllItems];
        [fAudTrack2DrcSlider setFloatValue: 1.00];
        [self audioDRCSliderChanged: fAudTrack2DrcSlider];
    }
    else if ([[fAudTrack2MixPopUp selectedItem] tag] == HB_ACODEC_AC3 || [[fAudTrack2MixPopUp selectedItem] tag] == HB_ACODEC_DCA)
    {
        [fAudTrack2RatePopUp setEnabled: NO];
        [fAudTrack2BitratePopUp setEnabled: NO];
        [fAudTrack2DrcSlider setEnabled: NO];
        [fAudTrack2DrcField setEnabled: NO];
    }
    
    /* enable/disable the mixdown text and popupbutton for audio track 3 */
    [fAudTrack3CodecPopUp setEnabled: ([fAudLang3PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack3MixPopUp setEnabled: ([fAudLang3PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack3RatePopUp setEnabled: ([fAudLang3PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack3BitratePopUp setEnabled: ([fAudLang3PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack3DrcSlider setEnabled: ([fAudLang3PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack3DrcField setEnabled: ([fAudLang3PopUp indexOfSelectedItem] == 0) ? NO : YES];
    if ([fAudLang3PopUp indexOfSelectedItem] == 0)
    {
        [fAudTrack3CodecPopUp removeAllItems];
        [fAudTrack3MixPopUp removeAllItems];
        [fAudTrack3RatePopUp removeAllItems];
        [fAudTrack3BitratePopUp removeAllItems];
        [fAudTrack3DrcSlider setFloatValue: 1.00];
        [self audioDRCSliderChanged: fAudTrack3DrcSlider];
    }
    else if ([[fAudTrack3MixPopUp selectedItem] tag] == HB_ACODEC_AC3 || [[fAudTrack3MixPopUp selectedItem] tag] == HB_ACODEC_DCA)
    {
        [fAudTrack3RatePopUp setEnabled: NO];
        [fAudTrack3BitratePopUp setEnabled: NO];
        [fAudTrack3DrcSlider setEnabled: NO];
        [fAudTrack3DrcField setEnabled: NO];
    }
    
    /* enable/disable the mixdown text and popupbutton for audio track 4 */
    [fAudTrack4CodecPopUp setEnabled: ([fAudLang4PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack4MixPopUp setEnabled: ([fAudLang4PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack4RatePopUp setEnabled: ([fAudLang4PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack4BitratePopUp setEnabled: ([fAudLang4PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack4DrcSlider setEnabled: ([fAudLang4PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack4DrcField setEnabled: ([fAudLang4PopUp indexOfSelectedItem] == 0) ? NO : YES];
    if ([fAudLang4PopUp indexOfSelectedItem] == 0)
    {
        [fAudTrack4CodecPopUp removeAllItems];
        [fAudTrack4MixPopUp removeAllItems];
        [fAudTrack4RatePopUp removeAllItems];
        [fAudTrack4BitratePopUp removeAllItems];
        [fAudTrack4DrcSlider setFloatValue: 1.00];
        [self audioDRCSliderChanged: fAudTrack4DrcSlider];
    }
    else if ([[fAudTrack4MixPopUp selectedItem] tag] == HB_ACODEC_AC3 || [[fAudTrack4MixPopUp selectedItem] tag] == HB_ACODEC_DCA)
    {
        [fAudTrack4RatePopUp setEnabled: NO];
        [fAudTrack4BitratePopUp setEnabled: NO];
        [fAudTrack4DrcSlider setEnabled: NO];
        [fAudTrack4DrcField setEnabled: NO];
    }
    
}

- (IBAction) addAllAudioTracksToPopUp: (id) sender
{

    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t*)
        hb_list_item( list, [fSrcTitlePopUp indexOfSelectedItem] );

	hb_audio_config_t * audio;

    [sender removeAllItems];
    [sender addItemWithTitle: NSLocalizedString( @"None", @"" )];
    for( int i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = (hb_audio_config_t *) hb_list_audio_config_item( title->list_audio, i );
        [[sender menu] addItemWithTitle:
            [NSString stringWithCString: audio->lang.description]
            action: NULL keyEquivalent: @""];
    }
    [sender selectItemAtIndex: 0];

}

- (IBAction) selectAudioTrackInPopUp: (id) sender searchPrefixString: (NSString *) searchPrefixString selectIndexIfNotFound: (int) selectIndexIfNotFound
{

    /* this method can be used to find a language, or a language-and-source-format combination, by passing in the appropriate string */
    /* e.g. to find the first French track, pass in an NSString * of "Francais" */
    /* e.g. to find the first English 5.1 AC3 track, pass in an NSString * of "English (AC3) (5.1 ch)" */
    /* if no matching track is found, then selectIndexIfNotFound is used to choose which track to select instead */

	if (searchPrefixString)
	{

        for( int i = 0; i < [sender numberOfItems]; i++ )
        {
            /* Try to find the desired search string */
            if ([[[sender itemAtIndex: i] title] hasPrefix:searchPrefixString])
            {
                [sender selectItemAtIndex: i];
                return;
            }
        }
        /* couldn't find the string, so select the requested "search string not found" item */
        /* index of 0 means select the "none" item */
        /* index of 1 means select the first audio track */
        [sender selectItemAtIndex: selectIndexIfNotFound];
	}
    else
    {
        /* if no search string is provided, then select the selectIndexIfNotFound item */
        [sender selectItemAtIndex: selectIndexIfNotFound];
    }

}
- (IBAction) audioAddAudioTrackCodecs: (id)sender
{
    int format = [fDstFormatPopUp indexOfSelectedItem];
    
    /* setup pointers to the appropriate popups for the correct track */
    NSPopUpButton * audiocodecPopUp;
    NSPopUpButton * audiotrackPopUp;
    if (sender == fAudTrack1CodecPopUp)
    {
        audiotrackPopUp = fAudLang1PopUp;
        audiocodecPopUp = fAudTrack1CodecPopUp;
    }
    else if (sender == fAudTrack2CodecPopUp)
    {
        audiotrackPopUp = fAudLang2PopUp;
        audiocodecPopUp = fAudTrack2CodecPopUp;
    }
    else if (sender == fAudTrack3CodecPopUp)
    {
        audiotrackPopUp = fAudLang3PopUp;
        audiocodecPopUp = fAudTrack3CodecPopUp;
    }
    else
    {
        audiotrackPopUp = fAudLang4PopUp;
        audiocodecPopUp = fAudTrack4CodecPopUp;
    }
    
    [audiocodecPopUp removeAllItems];
    /* Make sure "None" isnt selected in the source track */
    if ([audiotrackPopUp indexOfSelectedItem] > 0)
    {
        [audiocodecPopUp setEnabled:YES];
        NSMenuItem *menuItem;
        /* We setup our appropriate popups for codecs and put the int value in the popup tag for easy retrieval */
        switch( format )
        {
            case 0:
                /* MP4 */
                // FAAC
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"AAC (faac)" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_FAAC];

                // CA_AAC
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"AAC (CoreAudio)" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_CA_AAC];

                // AC3 Passthru
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"AC3 Passthru" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_AC3];
                break;
                
            case 1:
                /* MKV */
                // FAAC
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"AAC (faac)" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_FAAC];
                // CA_AAC
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"AAC (CoreAudio)" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_CA_AAC];
                // AC3 Passthru
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"AC3 Passthru" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_AC3];
                // DTS Passthru
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"DTS Passthru" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_DCA];
                // MP3
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"MP3 (lame)" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_LAME];
                // Vorbis
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"Vorbis (vorbis)" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_VORBIS];
                break;
                
            case 2: 
                /* AVI */
                // MP3
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"MP3 (lame)" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_LAME];
                // AC3 Passthru
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"AC3 Passthru" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_AC3];
                break;
                
            case 3:
                /* OGM */
                // Vorbis
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"Vorbis (vorbis)" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_VORBIS];
                // MP3
                menuItem = [[audiocodecPopUp menu] addItemWithTitle:@"MP3 (lame)" action: NULL keyEquivalent: @""];
                [menuItem setTag: HB_ACODEC_LAME];
                break;
        }
        [audiocodecPopUp selectItemAtIndex:0];
    }
    else
    {
        [audiocodecPopUp setEnabled:NO];
    }
}

- (IBAction) audioTrackPopUpChanged: (id) sender
{
    /* utility function to call audioTrackPopUpChanged without passing in a mixdown-to-use */
    [self audioTrackPopUpChanged: sender mixdownToUse: 0];
}

- (IBAction) audioTrackPopUpChanged: (id) sender mixdownToUse: (int) mixdownToUse
{
    
    /* make sure we have a selected title before continuing */
    if (fTitle == NULL) return;
    /* if the sender is the lanaguage popup and there is nothing in the codec popup, lets call
    * audioAddAudioTrackCodecs on the codec popup to populate it properly before moving on
    */
    if (sender == fAudLang1PopUp && [[fAudTrack1CodecPopUp menu] numberOfItems] == 0)
    {
        [self audioAddAudioTrackCodecs: fAudTrack1CodecPopUp];
    }
    if (sender == fAudLang2PopUp && [[fAudTrack2CodecPopUp menu] numberOfItems] == 0)
    {
        [self audioAddAudioTrackCodecs: fAudTrack2CodecPopUp];
    }
    if (sender == fAudLang3PopUp && [[fAudTrack3CodecPopUp menu] numberOfItems] == 0)
    {
        [self audioAddAudioTrackCodecs: fAudTrack3CodecPopUp];
    }
    if (sender == fAudLang4PopUp && [[fAudTrack4CodecPopUp menu] numberOfItems] == 0)
    {
        [self audioAddAudioTrackCodecs: fAudTrack4CodecPopUp];
    }
    
    /* Now lets make the sender the appropriate Audio Track popup from this point on */
    if (sender == fAudTrack1CodecPopUp || sender == fAudTrack1MixPopUp)
    {
        sender = fAudLang1PopUp;
    }
    if (sender == fAudTrack2CodecPopUp || sender == fAudTrack2MixPopUp)
    {
        sender = fAudLang2PopUp;
    }
    if (sender == fAudTrack3CodecPopUp || sender == fAudTrack3MixPopUp)
    {
        sender = fAudLang3PopUp;
    }
    if (sender == fAudTrack4CodecPopUp || sender == fAudTrack4MixPopUp)
    {
        sender = fAudLang4PopUp;
    }
    
    /* pointer to this track's mixdown, codec, sample rate and bitrate NSPopUpButton's */
    NSPopUpButton * mixdownPopUp;
    NSPopUpButton * audiocodecPopUp;
    NSPopUpButton * sampleratePopUp;
    NSPopUpButton * bitratePopUp;
    if (sender == fAudLang1PopUp)
    {
        mixdownPopUp = fAudTrack1MixPopUp;
        audiocodecPopUp = fAudTrack1CodecPopUp;
        sampleratePopUp = fAudTrack1RatePopUp;
        bitratePopUp = fAudTrack1BitratePopUp;
    }
    else if (sender == fAudLang2PopUp)
    {
        mixdownPopUp = fAudTrack2MixPopUp;
        audiocodecPopUp = fAudTrack2CodecPopUp;
        sampleratePopUp = fAudTrack2RatePopUp;
        bitratePopUp = fAudTrack2BitratePopUp;
    }
    else if (sender == fAudLang3PopUp)
    {
        mixdownPopUp = fAudTrack3MixPopUp;
        audiocodecPopUp = fAudTrack3CodecPopUp;
        sampleratePopUp = fAudTrack3RatePopUp;
        bitratePopUp = fAudTrack3BitratePopUp;
    }
    else
    {
        mixdownPopUp = fAudTrack4MixPopUp;
        audiocodecPopUp = fAudTrack4CodecPopUp;
        sampleratePopUp = fAudTrack4RatePopUp;
        bitratePopUp = fAudTrack4BitratePopUp;
    }

    /* get the index of the selected audio Track*/
    int thisAudioIndex = [sender indexOfSelectedItem] - 1;

    /* pointer for the hb_audio_s struct we will use later on */
    hb_audio_config_t * audio;

    int acodec;
    /* check if the audio mixdown controls need their enabled state changing */
    [self setEnabledStateOfAudioMixdownControls:nil];

    if (thisAudioIndex != -1)
    {

        /* get the audio */
        audio = (hb_audio_config_t *) hb_list_audio_config_item( fTitle->list_audio, thisAudioIndex );// Should "fTitle" be title and be setup ?

        /* actually manipulate the proper mixdowns here */
        /* delete the previous audio mixdown options */
        [mixdownPopUp removeAllItems];

        acodec = [[audiocodecPopUp selectedItem] tag];

        if (audio != NULL)
        {

            /* find out if our selected output audio codec supports mono and / or 6ch */
            /* we also check for an input codec of AC3 or DCA,
             as they are the only libraries able to do the mixdown to mono / conversion to 6-ch */
            /* audioCodecsSupportMono and audioCodecsSupport6Ch are the same for now,
             but this may change in the future, so they are separated for flexibility */
            int audioCodecsSupportMono =
                    (audio->in.codec & (HB_ACODEC_AC3|HB_ACODEC_DCA)) &&
                    (acodec != HB_ACODEC_LAME);
            int audioCodecsSupport6Ch =
                    (audio->in.codec & (HB_ACODEC_AC3|HB_ACODEC_DCA)) &&
                    (acodec != HB_ACODEC_LAME);
            
            /* check for AC-3 passthru */
            if (audio->in.codec == HB_ACODEC_AC3 && acodec == HB_ACODEC_AC3)
            {
                
            NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                 [NSString stringWithCString: "AC3 Passthru"]
                                               action: NULL keyEquivalent: @""];
             [menuItem setTag: HB_ACODEC_AC3];   
            }
            else if (audio->in.codec == HB_ACODEC_DCA && acodec == HB_ACODEC_DCA)
            {
            NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                 [NSString stringWithCString: "DTS Passthru"]
                                               action: NULL keyEquivalent: @""];
             [menuItem setTag: HB_ACODEC_DCA]; 
            }
            else
            {
                
                /* add the appropriate audio mixdown menuitems to the popupbutton */
                /* in each case, we set the new menuitem's tag to be the amixdown value for that mixdown,
                 so that we can reference the mixdown later */
                
                /* keep a track of the min and max mixdowns we used, so we can select the best match later */
                int minMixdownUsed = 0;
                int maxMixdownUsed = 0;
                
                /* get the input channel layout without any lfe channels */
                int layout = audio->in.channel_layout & HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK;
                
                /* do we want to add a mono option? */
                if (audioCodecsSupportMono == 1)
                {
                    NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                                            [NSString stringWithCString: hb_audio_mixdowns[0].human_readable_name]
                                                                          action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[0].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[0].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[0].amixdown);
                }
                
                /* do we want to add a stereo option? */
                /* offer stereo if we have a mono source and non-mono-supporting codecs, as otherwise we won't have a mixdown at all */
                /* also offer stereo if we have a stereo-or-better source */
                if ((layout == HB_INPUT_CH_LAYOUT_MONO && audioCodecsSupportMono == 0) || layout >= HB_INPUT_CH_LAYOUT_STEREO)
                {
                    NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                                            [NSString stringWithCString: hb_audio_mixdowns[1].human_readable_name]
                                                                          action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[1].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[1].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[1].amixdown);
                }
                
                /* do we want to add a dolby surround (DPL1) option? */
                if (layout == HB_INPUT_CH_LAYOUT_3F1R || layout == HB_INPUT_CH_LAYOUT_3F2R || layout == HB_INPUT_CH_LAYOUT_DOLBY)
                {
                    NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                                            [NSString stringWithCString: hb_audio_mixdowns[2].human_readable_name]
                                                                          action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[2].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[2].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[2].amixdown);
                }
                
                /* do we want to add a dolby pro logic 2 (DPL2) option? */
                if (layout == HB_INPUT_CH_LAYOUT_3F2R)
                {
                    NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                                            [NSString stringWithCString: hb_audio_mixdowns[3].human_readable_name]
                                                                          action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[3].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[3].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[3].amixdown);
                }
                
                /* do we want to add a 6-channel discrete option? */
                if (audioCodecsSupport6Ch == 1 && layout == HB_INPUT_CH_LAYOUT_3F2R && (audio->in.channel_layout & HB_INPUT_CH_LAYOUT_HAS_LFE))
                {
                    NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                                            [NSString stringWithCString: hb_audio_mixdowns[4].human_readable_name]
                                                                          action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[4].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[4].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[4].amixdown);
                }
                
                /* do we want to add an AC-3 passthrough option? */
                if (audio->in.codec == HB_ACODEC_AC3 && acodec == HB_ACODEC_AC3) 
                {
                    NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                                            [NSString stringWithCString: hb_audio_mixdowns[5].human_readable_name]
                                                                          action: NULL keyEquivalent: @""];
                    [menuItem setTag: HB_ACODEC_AC3];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[5].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[5].amixdown);
                }
                
                /* do we want to add a DTS Passthru option ? HB_ACODEC_DCA*/
                if (audio->in.codec == HB_ACODEC_DCA && acodec == HB_ACODEC_DCA) 
                {
                    NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                                            [NSString stringWithCString: hb_audio_mixdowns[5].human_readable_name]
                                                                          action: NULL keyEquivalent: @""];
                    [menuItem setTag: HB_ACODEC_DCA];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[5].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[5].amixdown);
                }
                
                /* auto-select the best mixdown based on our saved mixdown preference */
                
                /* for now, this is hard-coded to a "best" mixdown of HB_AMIXDOWN_DOLBYPLII */
                /* ultimately this should be a prefs option */
                int useMixdown;
                
                /* if we passed in a mixdown to use - in order to load a preset - then try and use it */
                if (mixdownToUse > 0)
                {
                    useMixdown = mixdownToUse;
                }
                else
                {
                    useMixdown = HB_AMIXDOWN_DOLBYPLII;
                }
                
                /* if useMixdown > maxMixdownUsed, then use maxMixdownUsed */
                if (useMixdown > maxMixdownUsed)
                { 
                    useMixdown = maxMixdownUsed;
                }
                
                /* if useMixdown < minMixdownUsed, then use minMixdownUsed */
                if (useMixdown < minMixdownUsed)
                { 
                    useMixdown = minMixdownUsed;
                }
                
                /* select the (possibly-amended) preferred mixdown */
                [mixdownPopUp selectItemWithTag: useMixdown];

            }
            /* In the case of a source track that is not AC3 and the user tries to use AC3 Passthru (which does not work)
             * we force the Audio Codec choice back to a workable codec. We use MP3 for avi and aac for all
             * other containers.
             */
            if (audio->in.codec != HB_ACODEC_AC3 && [[audiocodecPopUp selectedItem] tag] == HB_ACODEC_AC3)
            {
                /* If we are using the avi container, we select MP3 as there is no aac available*/
                if ([[fDstFormatPopUp selectedItem] tag] == HB_MUX_AVI)
                {
                    [audiocodecPopUp selectItemWithTag: HB_ACODEC_LAME];
                }
                else
                {
                    [audiocodecPopUp selectItemWithTag: HB_ACODEC_FAAC];
                }
            }
            
            /* In the case of a source track that is not DTS and the user tries to use DTS Passthru (which does not work)
             * we force the Audio Codec choice back to a workable codec. We use MP3 for avi and aac for all
             * other containers.
             */
            if (audio->in.codec != HB_ACODEC_DCA && [[audiocodecPopUp selectedItem] tag] == HB_ACODEC_DCA)
            {
                /* If we are using the avi container, we select MP3 as there is no aac available*/
                if ([[fDstFormatPopUp selectedItem] tag] == HB_MUX_AVI)
                {
                    [audiocodecPopUp selectItemWithTag: HB_ACODEC_LAME];
                }
                else
                {
                    [audiocodecPopUp selectItemWithTag: HB_ACODEC_FAAC];
                }
            }
            
            /* Setup our samplerate and bitrate popups we will need based on mixdown */
            [self audioTrackMixdownChanged: mixdownPopUp];	       
        }
    
    }
    if( [fDstFormatPopUp indexOfSelectedItem] == 0 )
    {
        [self autoSetM4vExtension: sender];
    }
}

- (IBAction) audioTrackMixdownChanged: (id) sender
{
    
    int acodec;
    /* setup pointers to all of the other audio track controls
    * we will need later
    */
    NSPopUpButton * mixdownPopUp;
    NSPopUpButton * sampleratePopUp;
    NSPopUpButton * bitratePopUp;
    NSPopUpButton * audiocodecPopUp;
    NSPopUpButton * audiotrackPopUp;
    NSSlider * drcSlider;
    NSTextField * drcField;
    if (sender == fAudTrack1MixPopUp)
    {
        audiotrackPopUp = fAudLang1PopUp;
        audiocodecPopUp = fAudTrack1CodecPopUp;
        mixdownPopUp = fAudTrack1MixPopUp;
        sampleratePopUp = fAudTrack1RatePopUp;
        bitratePopUp = fAudTrack1BitratePopUp;
        drcSlider = fAudTrack1DrcSlider;
        drcField = fAudTrack1DrcField;
    }
    else if (sender == fAudTrack2MixPopUp)
    {
        audiotrackPopUp = fAudLang2PopUp;
        audiocodecPopUp = fAudTrack2CodecPopUp;
        mixdownPopUp = fAudTrack2MixPopUp;
        sampleratePopUp = fAudTrack2RatePopUp;
        bitratePopUp = fAudTrack2BitratePopUp;
        drcSlider = fAudTrack2DrcSlider;
        drcField = fAudTrack2DrcField;
    }
    else if (sender == fAudTrack3MixPopUp)
    {
        audiotrackPopUp = fAudLang3PopUp;
        audiocodecPopUp = fAudTrack3CodecPopUp;
        mixdownPopUp = fAudTrack3MixPopUp;
        sampleratePopUp = fAudTrack3RatePopUp;
        bitratePopUp = fAudTrack3BitratePopUp;
        drcSlider = fAudTrack3DrcSlider;
        drcField = fAudTrack3DrcField;
    }
    else
    {
        audiotrackPopUp = fAudLang4PopUp;
        audiocodecPopUp = fAudTrack4CodecPopUp;
        mixdownPopUp = fAudTrack4MixPopUp;
        sampleratePopUp = fAudTrack4RatePopUp;
        bitratePopUp = fAudTrack4BitratePopUp;
        drcSlider = fAudTrack4DrcSlider;
        drcField = fAudTrack4DrcField;
    }
    acodec = [[audiocodecPopUp selectedItem] tag];
    /* storage variable for the min and max bitrate allowed for this codec */
    int minbitrate;
    int maxbitrate;
    
    switch( acodec )
    {
        case HB_ACODEC_FAAC:
            /* check if we have a 6ch discrete conversion in either audio track */
            if ([[mixdownPopUp selectedItem] tag] == HB_AMIXDOWN_6CH)
            {
                /* FAAC is happy using our min bitrate of 32 kbps, even for 6ch */
                minbitrate = 32;
                /* If either mixdown popup includes 6-channel discrete, then allow up to 384 kbps */
                maxbitrate = 384;
                break;
            }
            else
            {
                /* FAAC is happy using our min bitrate of 32 kbps for stereo or mono */
                minbitrate = 32;
                /* FAAC won't honour anything more than 160 for stereo, so let's not offer it */
                /* note: haven't dealt with mono separately here, FAAC will just use the max it can */
                maxbitrate = 160;
                break;
            }

        case HB_ACODEC_CA_AAC:
            /* check if we have a 6ch discrete conversion in either audio track */
            if ([[mixdownPopUp selectedItem] tag] == HB_AMIXDOWN_6CH)
            {
                minbitrate = 128;
                maxbitrate = 768;
                break;
            }
            else
            {
                minbitrate = 64;
                maxbitrate = 320;
                break;
            }

            case HB_ACODEC_LAME:
            /* Lame is happy using our min bitrate of 32 kbps */
            minbitrate = 32;
            /* Lame won't encode if the bitrate is higher than 320 kbps */
            maxbitrate = 320;
            break;
            
            case HB_ACODEC_VORBIS:
            if ([[mixdownPopUp selectedItem] tag] == HB_AMIXDOWN_6CH)
            {
                /* Vorbis causes a crash if we use a bitrate below 192 kbps with 6 channel */
                minbitrate = 192;
                /* If either mixdown popup includes 6-channel discrete, then allow up to 384 kbps */
                maxbitrate = 384;
                break;
            }
            else
            {
                /* Vorbis causes a crash if we use a bitrate below 48 kbps */
                minbitrate = 48;
                /* Vorbis can cope with 384 kbps quite happily, even for stereo */
                maxbitrate = 384;
                break;
            }
            
            default:
            /* AC3 passthru disables the bitrate dropdown anyway, so we might as well just use the min and max bitrate */
            minbitrate = 32;
            maxbitrate = 384;
            
    }
    
    /* make sure we have a selected title before continuing */
    if (fTitle == NULL) return;
    /* get the audio so we can find out what input rates are*/
    hb_audio_config_t * audio;
    audio = (hb_audio_config_t *) hb_list_audio_config_item( fTitle->list_audio, [audiotrackPopUp indexOfSelectedItem] - 1 );
    int inputbitrate = audio->in.bitrate / 1000;
    int inputsamplerate = audio->in.samplerate;
    
    if ([[mixdownPopUp selectedItem] tag] != HB_ACODEC_AC3 && [[mixdownPopUp selectedItem] tag] != HB_ACODEC_DCA)
    {
        [bitratePopUp removeAllItems];
        
        for( int i = 0; i < hb_audio_bitrates_count; i++ )
        {
            if (hb_audio_bitrates[i].rate >= minbitrate && hb_audio_bitrates[i].rate <= maxbitrate)
            {
                /* add a new menuitem for this bitrate */
                NSMenuItem *menuItem = [[bitratePopUp menu] addItemWithTitle:
                                        [NSString stringWithCString: hb_audio_bitrates[i].string]
                                                                      action: NULL keyEquivalent: @""];
                /* set its tag to be the actual bitrate as an integer, so we can retrieve it later */
                [menuItem setTag: hb_audio_bitrates[i].rate];
            }
        }
        
        /* select the default bitrate (but use 384 for 6-ch AAC) */
        if ([[mixdownPopUp selectedItem] tag] == HB_AMIXDOWN_6CH)
        {
            [bitratePopUp selectItemWithTag: 384];
        }
        else
        {
            [bitratePopUp selectItemWithTag: hb_audio_bitrates[hb_audio_bitrates_default].rate];
        }
    }
    /* populate and set the sample rate popup */
    /* Audio samplerate */
    [sampleratePopUp removeAllItems];
    /* we create a same as source selection (Auto) so that we can choose to use the input sample rate */
    NSMenuItem *menuItem = [[sampleratePopUp menu] addItemWithTitle: @"Auto" action: NULL keyEquivalent: @""];
    [menuItem setTag: inputsamplerate];
    
    for( int i = 0; i < hb_audio_rates_count; i++ )
    {
        NSMenuItem *menuItem = [[sampleratePopUp menu] addItemWithTitle:
                                [NSString stringWithCString: hb_audio_rates[i].string]
                                                                 action: NULL keyEquivalent: @""];
        [menuItem setTag: hb_audio_rates[i].rate];
    }
    /* We use the input sample rate as the default sample rate as downsampling just makes audio worse
    * and there is no compelling reason to use anything else as default, though the users default
    * preset will likely override any setting chosen here.
    */
    [sampleratePopUp selectItemWithTag: inputsamplerate];
    
    
    /* Since AC3 Pass Thru and DTS Pass Thru uses the input bitrate and sample rate, we get the input tracks
    * bitrate and display it in the bitrate popup even though libhb happily ignores any bitrate input from
    * the gui. We do this for better user feedback in the audio tab as well as the queue for the most part
    */
    if ([[mixdownPopUp selectedItem] tag] == HB_ACODEC_AC3 || [[mixdownPopUp selectedItem] tag] == HB_ACODEC_DCA)
    {
        
        /* lets also set the bitrate popup to the input bitrate as thats what passthru will use */
        [bitratePopUp removeAllItems];
        NSMenuItem *menuItem = [[bitratePopUp menu] addItemWithTitle:
                                [NSString stringWithFormat:@"%d", inputbitrate]
                                                              action: NULL keyEquivalent: @""];
        [menuItem setTag: inputbitrate];
        /* For ac3 passthru we disable the sample rate and bitrate popups as well as the drc slider*/
        [bitratePopUp setEnabled: NO];
        [sampleratePopUp setEnabled: NO];
        
        [drcSlider setFloatValue: 1.00];
        [self audioDRCSliderChanged: drcSlider];
        [drcSlider setEnabled: NO];
        [drcField setEnabled: NO];
    }
    else
    {
        [sampleratePopUp setEnabled: YES];
        [bitratePopUp setEnabled: YES];
        [drcSlider setEnabled: YES];
        [drcField setEnabled: YES];
    }
[self calculateBitrate:nil];    
}

- (IBAction) audioDRCSliderChanged: (id) sender
{
    NSSlider * drcSlider;
    NSTextField * drcField;
    if (sender == fAudTrack1DrcSlider)
    {
        drcSlider = fAudTrack1DrcSlider;
        drcField = fAudTrack1DrcField;
    }
    else if (sender == fAudTrack2DrcSlider)
    {
        drcSlider = fAudTrack2DrcSlider;
        drcField = fAudTrack2DrcField;
    }
    else if (sender == fAudTrack3DrcSlider)
    {
        drcSlider = fAudTrack3DrcSlider;
        drcField = fAudTrack3DrcField;
    }
    else
    {
        drcSlider = fAudTrack4DrcSlider;
        drcField = fAudTrack4DrcField;
    }
    
    /* If we are between 0.0 and 1.0 on the slider, snap it to 1.0 */
    if ([drcSlider floatValue] > 0.0 && [drcSlider floatValue] < 1.0)
    {
        [drcSlider setFloatValue:1.0];
    }
    
    
    [drcField setStringValue: [NSString stringWithFormat: @"%.2f", [drcSlider floatValue]]];
    /* For now, do not call this until we have an intelligent way to determine audio track selections
    * compared to presets
    */
    //[self customSettingUsed: sender];
}

#pragma mark -

- (IBAction) browseImportSrtFile: (id) sender
{

    NSOpenPanel * panel;
	
    panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: NO ];
    NSString * sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastSrtImportDirectory"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastSrtImportDirectory"];
	}
	else
	{
		sourceDirectory = @"~/Desktop";
		sourceDirectory = [sourceDirectory stringByExpandingTildeInPath];
	}
    /* we open up the browse srt sheet here and call for browseImportSrtFileDone after the sheet is closed */
    NSArray *fileTypes = [NSArray arrayWithObjects:@"plist", @"srt", nil];
    [panel beginSheetForDirectory: sourceDirectory file: nil types: fileTypes
                   modalForWindow: fWindow modalDelegate: self
                   didEndSelector: @selector( browseImportSrtFileDone:returnCode:contextInfo: )
                      contextInfo: sender];
}

- (void) browseImportSrtFileDone: (NSSavePanel *) sheet
                     returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        NSString *importSrtDirectory = [[sheet filename] stringByDeletingLastPathComponent];
        NSString *importSrtFilePath = [sheet filename];
        [[NSUserDefaults standardUserDefaults] setObject:importSrtDirectory forKey:@"LastSrtImportDirectory"];
        
        /* now pass the string off to fSubtitlesDelegate to add the srt file to the dropdown */
        [fSubtitlesDelegate createSubtitleSrtTrack:importSrtFilePath];
        
        [fSubtitlesTable reloadData];
        
    }
}                                           

#pragma mark -
#pragma mark Open New Windows

- (IBAction) openHomepage: (id) sender
{
    [[NSWorkspace sharedWorkspace] openURL: [NSURL
        URLWithString:@"http://handbrake.fr/"]];
}

- (IBAction) openForums: (id) sender
{
    [[NSWorkspace sharedWorkspace] openURL: [NSURL
        URLWithString:@"http://handbrake.fr/forum/"]];
}
- (IBAction) openUserGuide: (id) sender
{
    [[NSWorkspace sharedWorkspace] openURL: [NSURL
        URLWithString:@"http://handbrake.fr/trac/wiki/HandBrakeGuide"]];
}

/**
 * Shows debug output window.
 */
- (IBAction)showDebugOutputPanel:(id)sender
{
    [outputPanel showOutputPanel:sender];
}

/**
 * Shows preferences window.
 */
- (IBAction) showPreferencesWindow: (id) sender
{
    NSWindow * window = [fPreferencesController window];
    if (![window isVisible])
        [window center];

    [window makeKeyAndOrderFront: nil];
}

/**
 * Shows queue window.
 */
- (IBAction) showQueueWindow:(id)sender
{
    [fQueueController showQueueWindow:sender];
}


- (IBAction) toggleDrawer:(id)sender {
    [fPresetDrawer toggle:self];
}

/**
 * Shows Picture Settings Window.
 */

- (IBAction) showPicturePanel: (id) sender
{
	[fPictureController showPictureWindow:sender];
}

- (void) picturePanelFullScreen
{
	[fPictureController setToFullScreenMode];
}

- (void) picturePanelWindowed
{
	[fPictureController setToWindowedMode];
}

- (IBAction) showPreviewWindow: (id) sender
{
	[fPictureController showPreviewWindow:sender];
}

#pragma mark -
#pragma mark Preset Outline View Methods
#pragma mark - Required
/* These are required by the NSOutlineView Datasource Delegate */


/* used to specify the number of levels to show for each item */
- (int)outlineView:(NSOutlineView *)fPresetsOutlineView numberOfChildrenOfItem:(id)item
{
    /* currently use no levels to test outline view viability */
    if (item == nil) // for an outline view the root level of the hierarchy is always nil
    {
        return [UserPresets count];
    }
    else
    {
        /* we need to return the count of the array in ChildrenArray for this folder */
        NSArray *children = nil;
        children = [item objectForKey:@"ChildrenArray"];
        if ([children count] > 0)
        {
            return [children count];
        }
        else
        {
            return 0;
        }
    }
}

/* We use this to deterimine children of an item */
- (id)outlineView:(NSOutlineView *)fPresetsOutlineView child:(NSInteger)index ofItem:(id)item
{
    
    /* we need to return the count of the array in ChildrenArray for this folder */
    NSArray *children = nil;
    if (item == nil)
    {
        children = UserPresets;
    }
    else
    {
        if ([item objectForKey:@"ChildrenArray"])
        {
            children = [item objectForKey:@"ChildrenArray"];
        }
    }   
    if ((children == nil) || ( [children count] <= (NSUInteger) index))
    {
        return nil;
    }
    else
    {
        return [children objectAtIndex:index];
    }
    
    
    // We are only one level deep, so we can't be asked about children
    //NSAssert (NO, @"Presets View outlineView:child:ofItem: currently can't handle nested items.");
    //return nil;
}

/* We use this to determine if an item should be expandable */
- (BOOL)outlineView:(NSOutlineView *)fPresetsOutlineView isItemExpandable:(id)item
{
    
    /* we need to return the count of the array in ChildrenArray for this folder */
    NSArray *children= nil;
    if (item == nil)
    {
        children = UserPresets;
    }
    else
    {
        if ([item objectForKey:@"ChildrenArray"])
        {
            children = [item objectForKey:@"ChildrenArray"];
        }
    }   
    
    /* To deterimine if an item should show a disclosure triangle
     * we could do it by the children count as so:
     * if ([children count] < 1)
     * However, lets leave the triangle show even if there are no
     * children to help indicate a folder, just like folder in the
     * finder can show a disclosure triangle even when empty
     */
    
    /* We need to determine if the item is a folder */
   if ([[item objectForKey:@"Folder"] intValue] == 1)
   {
        return YES;
    }
    else
    {
        return NO;
    }
    
}

- (BOOL)outlineView:(NSOutlineView *)outlineView shouldExpandItem:(id)item
{
    // Our outline view has no levels, but we can still expand every item. Doing so
    // just makes the row taller. See heightOfRowByItem below.
//return ![(HBQueueOutlineView*)outlineView isDragging];

return YES;
}


/* Used to tell the outline view which information is to be displayed per item */
- (id)outlineView:(NSOutlineView *)fPresetsOutlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
	/* We have two columns right now, icon and PresetName */
	
    if ([[tableColumn identifier] isEqualToString:@"PresetName"])
    {
        return [item objectForKey:@"PresetName"];
    }
    else
    {
        //return @"";
        return nil;
    }
}

- (id)outlineView:(NSOutlineView *)outlineView itemForPersistentObject:(id)object
{
    return [NSKeyedUnarchiver unarchiveObjectWithData:object];
}
- (id)outlineView:(NSOutlineView *)outlineView persistentObjectForItem:(id)item
{
    return [NSKeyedArchiver archivedDataWithRootObject:item];
}

#pragma mark - Added Functionality (optional)
/* Use to customize the font and display characteristics of the title cell */
- (void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    if ([[tableColumn identifier] isEqualToString:@"PresetName"])
    {
        NSFont *txtFont;
        NSColor *fontColor;
        NSColor *shadowColor;
        txtFont = [NSFont systemFontOfSize: [NSFont smallSystemFontSize]];
        /*check to see if its a selected row */
        if ([fPresetsOutlineView selectedRow] == [fPresetsOutlineView rowForItem:item])
        {
            
            fontColor = [NSColor blackColor];
            shadowColor = [NSColor colorWithDeviceRed:(127.0/255.0) green:(140.0/255.0) blue:(160.0/255.0) alpha:1.0];
        }
        else
        {
            if ([[item objectForKey:@"Type"] intValue] == 0)
            {
                fontColor = [NSColor blueColor];
            }
            else // User created preset, use a black font
            {
                fontColor = [NSColor blackColor];
            }
            /* check to see if its a folder */
            //if ([[item objectForKey:@"Folder"] intValue] == 1)
            //{
            //fontColor = [NSColor greenColor];
            //}
            
            
        }
        /* We use Bold Text for the HB Default */
        if ([[item objectForKey:@"Default"] intValue] == 1)// 1 is HB default
        {
            txtFont = [NSFont boldSystemFontOfSize: [NSFont smallSystemFontSize]];
        }
        /* We use Bold Text for the User Specified Default */
        if ([[item objectForKey:@"Default"] intValue] == 2)// 2 is User default
        {
            txtFont = [NSFont boldSystemFontOfSize: [NSFont smallSystemFontSize]];
        }
        
        
        [cell setTextColor:fontColor];
        [cell setFont:txtFont];
        
    }
}

/* We use this to edit the name field in the outline view */
- (void)outlineView:(NSOutlineView *)outlineView setObjectValue:(id)object forTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
    if ([[tableColumn identifier] isEqualToString:@"PresetName"])
    {
        id theRecord;
        
        theRecord = item;
        [theRecord setObject:object forKey:@"PresetName"];
        
        [self sortPresets];
        
        [fPresetsOutlineView reloadData];
        /* We save all of the preset data here */
        [self savePreset];
    }
}
/* We use this to provide tooltips for the items in the presets outline view */
- (NSString *)outlineView:(NSOutlineView *)fPresetsOutlineView toolTipForCell:(NSCell *)cell rect:(NSRectPointer)rect tableColumn:(NSTableColumn *)tc item:(id)item mouseLocation:(NSPoint)mouseLocation
{
    //if ([[tc identifier] isEqualToString:@"PresetName"])
    //{
        /* initialize the tooltip contents variable */
        NSString *loc_tip;
        /* if there is a description for the preset, we show it in the tooltip */
        if ([item objectForKey:@"PresetDescription"])
        {
            loc_tip = [item objectForKey:@"PresetDescription"];
            return (loc_tip);
        }
        else
        {
            loc_tip = @"No description available";
        }
        return (loc_tip);
    //}
}

#pragma mark -
#pragma mark Preset Outline View Methods (dragging related)


- (BOOL)outlineView:(NSOutlineView *)outlineView writeItems:(NSArray *)items toPasteboard:(NSPasteboard *)pboard
{
	// Dragging is only allowed for custom presets.
    //[[[fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]] objectForKey:@"Default"] intValue] != 1
	if ([[[fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]] objectForKey:@"Type"] intValue] == 0) // 0 is built in preset
    {
        return NO;
    }
    // Don't retain since this is just holding temporaral drag information, and it is
    //only used during a drag!  We could put this in the pboard actually.
    fDraggedNodes = items;
    // Provide data for our custom type, and simple NSStrings.
    [pboard declareTypes:[NSArray arrayWithObjects: DragDropSimplePboardType, nil] owner:self];
    
    // the actual data doesn't matter since DragDropSimplePboardType drags aren't recognized by anyone but us!.
    [pboard setData:[NSData data] forType:DragDropSimplePboardType]; 
    
    return YES;
}

- (NSDragOperation)outlineView:(NSOutlineView *)outlineView validateDrop:(id <NSDraggingInfo>)info proposedItem:(id)item proposedChildIndex:(NSInteger)index
{
	
	// Don't allow dropping ONTO an item since they can't really contain any children.
    
    BOOL isOnDropTypeProposal = index == NSOutlineViewDropOnItemIndex;
    if (isOnDropTypeProposal)
        return NSDragOperationNone;
    
    // Don't allow dropping INTO an item since they can't really contain any children as of yet.
	if (item != nil)
	{
		index = [fPresetsOutlineView rowForItem: item] + 1;
		item = nil;
	}
    
    // Don't allow dropping into the Built In Presets.
    if (index < presetCurrentBuiltInCount)
    {
        return NSDragOperationNone;
        index = MAX (index, presetCurrentBuiltInCount);
	}    
	
    [outlineView setDropItem:item dropChildIndex:index];
    return NSDragOperationGeneric;
}



- (BOOL)outlineView:(NSOutlineView *)outlineView acceptDrop:(id <NSDraggingInfo>)info item:(id)item childIndex:(NSInteger)index
{
    /* first, lets see if we are dropping into a folder */
    if ([[fPresetsOutlineView itemAtRow:index] objectForKey:@"Folder"] && [[[fPresetsOutlineView itemAtRow:index] objectForKey:@"Folder"] intValue] == 1) // if its a folder
	{
    NSMutableArray *childrenArray = [[NSMutableArray alloc] init];
    childrenArray = [[fPresetsOutlineView itemAtRow:index] objectForKey:@"ChildrenArray"];
    [childrenArray addObject:item];
    [[fPresetsOutlineView itemAtRow:index] setObject:[NSMutableArray arrayWithArray: childrenArray] forKey:@"ChildrenArray"];
    [childrenArray autorelease];
    }
    else // We are not, so we just move the preset into the existing array 
    {
        NSMutableIndexSet *moveItems = [NSMutableIndexSet indexSet];
        id obj;
        NSEnumerator *enumerator = [fDraggedNodes objectEnumerator];
        while (obj = [enumerator nextObject])
        {
            [moveItems addIndex:[UserPresets indexOfObject:obj]];
        }
        // Successful drop, lets rearrange the view and save it all
        [self moveObjectsInPresetsArray:UserPresets fromIndexes:moveItems toIndex: index];
    }
    [fPresetsOutlineView reloadData];
    [self savePreset];
    return YES;
}

- (void)moveObjectsInPresetsArray:(NSMutableArray *)array fromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex
{
    NSUInteger index = [indexSet lastIndex];
    NSUInteger aboveInsertIndexCount = 0;
    
    NSUInteger removeIndex;

    if (index >= insertIndex)
    {
        removeIndex = index + aboveInsertIndexCount;
        aboveInsertIndexCount++;
    }
    else
    {
        removeIndex = index;
        insertIndex--;
    }

    id object = [[array objectAtIndex:removeIndex] retain];
    [array removeObjectAtIndex:removeIndex];
    [array insertObject:object atIndex:insertIndex];
    [object release];

    index = [indexSet indexLessThanIndex:index];
}



#pragma mark - Functional Preset NSOutlineView Methods

- (IBAction)selectPreset:(id)sender
{
    
    if ([fPresetsOutlineView selectedRow] >= 0 && [[[fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]] objectForKey:@"Folder"] intValue] != 1)
    {
        chosenPreset = [fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]];
        [fPresetSelectedDisplay setStringValue:[chosenPreset objectForKey:@"PresetName"]];
        
        if ([[chosenPreset objectForKey:@"Default"] intValue] == 1)
        {
            [fPresetSelectedDisplay setStringValue:[NSString stringWithFormat:@"%@ (Default)", [chosenPreset objectForKey:@"PresetName"]]];
        }
        else
        {
            [fPresetSelectedDisplay setStringValue:[chosenPreset objectForKey:@"PresetName"]];
        }
        
        /* File Format */
        [fDstFormatPopUp selectItemWithTitle:[chosenPreset objectForKey:@"FileFormat"]];
        [self formatPopUpChanged:nil];
        
        /* Chapter Markers*/
        [fCreateChapterMarkers setState:[[chosenPreset objectForKey:@"ChapterMarkers"] intValue]];
        /* check to see if we have only one chapter */
        [self chapterPopUpChanged:nil];
        
        /* Allow Mpeg4 64 bit formatting +4GB file sizes */
        [fDstMp4LargeFileCheck setState:[[chosenPreset objectForKey:@"Mp4LargeFile"] intValue]];
        /* Mux mp4 with http optimization */
        [fDstMp4HttpOptFileCheck setState:[[chosenPreset objectForKey:@"Mp4HttpOptimize"] intValue]];
        
        /* Video encoder */
        [fVidEncoderPopUp selectItemWithTitle:[chosenPreset objectForKey:@"VideoEncoder"]];
        /* We set the advanced opt string here if applicable*/
        [fAdvancedOptions setOptions:[chosenPreset objectForKey:@"x264Option"]];
        
        /* Lets run through the following functions to get variables set there */
        [self videoEncoderPopUpChanged:nil];
        /* Set the state of ipod compatible with Mp4iPodCompatible. Only for x264*/
        [fDstMp4iPodFileCheck setState:[[chosenPreset objectForKey:@"Mp4iPodCompatible"] intValue]];
        [self calculateBitrate:nil];
        
        /* Video quality */
        [fVidQualityMatrix selectCellAtRow:[[chosenPreset objectForKey:@"VideoQualityType"] intValue] column:0];
        
        [fVidTargetSizeField setStringValue:[chosenPreset objectForKey:@"VideoTargetSize"]];
        [fVidBitrateField setStringValue:[chosenPreset objectForKey:@"VideoAvgBitrate"]];
        
        /* Since we are now using RF Values for the slider, we detect if the preset uses an old quality float.
         * So, check to see if the quality value is less than 1.0 which should indicate the old ".062" type
         * quality preset. Caveat: in the case of x264, where the RF scale starts at 0, it would misinterpret
         * a preset that uses 0.0 - 0.99 for RF as an old style preset. Not sure how to get around that one yet,
         * though it should be a corner case since it would pretty much be a preset for lossless encoding. */
        if ([[chosenPreset objectForKey:@"VideoQualitySlider"] floatValue] < 1.0)
        {
            /* For the quality slider we need to convert the old percent's to the new rf scales */
            float rf =  (([fVidQualitySlider maxValue] - [fVidQualitySlider minValue]) * [[chosenPreset objectForKey:@"VideoQualitySlider"] floatValue]);
            [fVidQualitySlider setFloatValue:rf];
            
        }
        else
        {
            /* Since theora's qp value goes up from left to right, we can just set the slider float value */
            if ([[fVidEncoderPopUp selectedItem] tag] == HB_VCODEC_THEORA)
            {
                [fVidQualitySlider setFloatValue:[[chosenPreset objectForKey:@"VideoQualitySlider"] floatValue]];
            }
            else
            {
                /* since ffmpeg and x264 use an "inverted" slider (lower qp/rf values indicate a higher quality) we invert the value on the slider */
                [fVidQualitySlider setFloatValue:([fVidQualitySlider maxValue] + [fVidQualitySlider minValue]) - [[chosenPreset objectForKey:@"VideoQualitySlider"] floatValue]];
            }
        }
        
        [self videoMatrixChanged:nil];
        
        /* Video framerate */
        /* For video preset video framerate, we want to make sure that Same as source does not conflict with the
         detected framerate in the fVidRatePopUp so we use index 0*/
        if ([[chosenPreset objectForKey:@"VideoFramerate"] isEqualToString:@"Same as source"])
        {
            [fVidRatePopUp selectItemAtIndex: 0];
        }
        else
        {
            [fVidRatePopUp selectItemWithTitle:[chosenPreset objectForKey:@"VideoFramerate"]];
        }
        
        
        /* 2 Pass Encoding */
        [fVidTwoPassCheck setState:[[chosenPreset objectForKey:@"VideoTwoPass"] intValue]];
        [self twoPassCheckboxChanged:nil];
        
        /* Turbo 1st pass for 2 Pass Encoding */
        [fVidTurboPassCheck setState:[[chosenPreset objectForKey:@"VideoTurboTwoPass"] intValue]];
        
        /*Audio*/
        /* First we check to see if we are using the current audio track layout based on AudioList array */
        if ([chosenPreset objectForKey:@"AudioList"])
        {
            /* Populate the audio widgets based on the contents of the AudioList array */
            int i = 0;
            NSEnumerator *enumerator = [[chosenPreset objectForKey:@"AudioList"] objectEnumerator];
            id tempObject;
            while (tempObject = [enumerator nextObject])
            {
                i++;
                if( i == 1 )
                {
                    if ([fAudLang1PopUp indexOfSelectedItem] == 0)
                    {
                        [fAudLang1PopUp selectItemAtIndex: 1];
                    }
                    [self audioTrackPopUpChanged: fAudLang1PopUp];
                    [fAudTrack1CodecPopUp selectItemWithTitle:[tempObject objectForKey:@"AudioEncoder"]];
                    /* check our pref for core audio and use it in place of faac if applicable */
                    if ([[NSUserDefaults standardUserDefaults] boolForKey: @"UseCoreAudio"] == YES && 
                        [[tempObject objectForKey:@"AudioEncoder"] isEqualToString: @"AAC (faac)"])
                    {
                        [fAudTrack1CodecPopUp selectItemWithTitle:@"AAC (CoreAudio)"];
                    }                    
                    
                    [self audioTrackPopUpChanged: fAudTrack1CodecPopUp];
                    [fAudTrack1MixPopUp selectItemWithTitle:[tempObject objectForKey:@"AudioMixdown"]];
                    /* check to see if the selections was available, if not, rerun audioTrackPopUpChanged using the codec to just set the default
                     * mixdown*/
                    if  ([fAudTrack1MixPopUp selectedItem] == nil)
                    {
                        [self audioTrackPopUpChanged: fAudTrack1CodecPopUp];
                    }
                    [fAudTrack1RatePopUp selectItemWithTitle:[tempObject objectForKey:@"AudioSamplerate"]];
                    /* We set the presets bitrate if it is *not* an AC3 track since that uses the input bitrate */
                    if (![[tempObject objectForKey:@"AudioEncoder"] isEqualToString:@"AC3 Passthru"])
                    {
                        [fAudTrack1BitratePopUp selectItemWithTitle:[tempObject objectForKey:@"AudioBitrate"]];
                    }
                    [fAudTrack1DrcSlider setFloatValue:[[tempObject objectForKey:@"AudioTrackDRCSlider"] floatValue]];
                    [self audioDRCSliderChanged: fAudTrack1DrcSlider];
                }
                
                if( i == 2 )
                {
                    
                    if ([fAudLang2PopUp indexOfSelectedItem] == 0)
                    {
                        [fAudLang2PopUp selectItemAtIndex: 1];
                    }
                    [self audioTrackPopUpChanged: fAudLang2PopUp];
                    [fAudTrack2CodecPopUp selectItemWithTitle:[tempObject objectForKey:@"AudioEncoder"]];
                    /* check our pref for core audio and use it in place of faac if applicable */
                    if ([[NSUserDefaults standardUserDefaults] boolForKey: @"UseCoreAudio"] == YES && 
                        [[tempObject objectForKey:@"AudioEncoder"] isEqualToString: @"AAC (faac)"])
                    {
                        [fAudTrack2CodecPopUp selectItemWithTitle:@"AAC (CoreAudio)"];
                    }
                    [self audioTrackPopUpChanged: fAudTrack2CodecPopUp];
                    [fAudTrack2MixPopUp selectItemWithTitle:[tempObject objectForKey:@"AudioMixdown"]];
                    /* check to see if the selections was available, if not, rerun audioTrackPopUpChanged using the codec to just set the default
                     * mixdown*/
                    if  ([fAudTrack2MixPopUp selectedItem] == nil)
                    {
                        [self audioTrackPopUpChanged: fAudTrack2CodecPopUp];
                    }
                    [fAudTrack2RatePopUp selectItemWithTitle:[tempObject objectForKey:@"AudioSamplerate"]];
                    /* We set the presets bitrate if it is *not* an AC3 track since that uses the input bitrate */
                    if (![[tempObject objectForKey:@"AudioEncoder"] isEqualToString:@"AC3 Passthru"])
                    {
                        [fAudTrack2BitratePopUp selectItemWithTitle:[tempObject objectForKey:@"AudioBitrate"]];
                    }
                    [fAudTrack2DrcSlider setFloatValue:[[tempObject objectForKey:@"AudioTrackDRCSlider"] floatValue]];
                    [self audioDRCSliderChanged: fAudTrack2DrcSlider];
                    
                }
                
            }
            
             /* We now cleanup any extra audio tracks that may have been previously set if we need to */
            
            if (i < 4)
            {
                [fAudLang4PopUp selectItemAtIndex: 0];
                [self audioTrackPopUpChanged: fAudLang4PopUp];
                
                if (i < 3)
                {
                    [fAudLang3PopUp selectItemAtIndex: 0];
                    [self audioTrackPopUpChanged: fAudLang3PopUp];
                    
                    if (i < 2)
                    {
                        [fAudLang2PopUp selectItemAtIndex: 0];
                        [self audioTrackPopUpChanged: fAudLang2PopUp];
                    }
                }
            }
            
        }
        else
        {
            if ([chosenPreset objectForKey:@"Audio1Track"] > 0)
            {
                if ([fAudLang1PopUp indexOfSelectedItem] == 0)
                {
                    [fAudLang1PopUp selectItemAtIndex: 1];
                }
                [self audioTrackPopUpChanged: fAudLang1PopUp];
                [fAudTrack1CodecPopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio1Encoder"]];
                /* check our pref for core audio and use it in place of faac if applicable */
                if ([[NSUserDefaults standardUserDefaults] boolForKey: @"UseCoreAudio"] == YES && 
                    [[chosenPreset objectForKey:@"Audio1Encoder"] isEqualToString: @"AAC (faac)"])
                {
                    [fAudTrack1CodecPopUp selectItemWithTitle:@"AAC (CoreAudio)"];
                }
                [self audioTrackPopUpChanged: fAudTrack1CodecPopUp];
                [fAudTrack1MixPopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio1Mixdown"]];
                /* check to see if the selections was available, if not, rerun audioTrackPopUpChanged using the codec to just set the default
                 * mixdown*/
                if  ([fAudTrack1MixPopUp selectedItem] == nil)
                {
                    [self audioTrackPopUpChanged: fAudTrack1CodecPopUp];
                }
                [fAudTrack1RatePopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio1Samplerate"]];
                /* We set the presets bitrate if it is *not* an AC3 track since that uses the input bitrate */
                if (![[chosenPreset objectForKey:@"Audio1Encoder"] isEqualToString:@"AC3 Passthru"])
                {
                    [fAudTrack1BitratePopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio1Bitrate"]];
                }
                [fAudTrack1DrcSlider setFloatValue:[[chosenPreset objectForKey:@"Audio1TrackDRCSlider"] floatValue]];
                [self audioDRCSliderChanged: fAudTrack1DrcSlider];
            }
            if ([chosenPreset objectForKey:@"Audio2Track"] > 0)
            {
                if ([fAudLang2PopUp indexOfSelectedItem] == 0)
                {
                    [fAudLang2PopUp selectItemAtIndex: 1];
                }
                [self audioTrackPopUpChanged: fAudLang2PopUp];
                [fAudTrack2CodecPopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio2Encoder"]];
                /* check our pref for core audio and use it in place of faac if applicable */
                if ([[NSUserDefaults standardUserDefaults] boolForKey: @"UseCoreAudio"] == YES && 
                    [[chosenPreset objectForKey:@"Audio2Encoder"] isEqualToString: @"AAC (faac)"])
                {
                    [fAudTrack2CodecPopUp selectItemWithTitle:@"AAC (CoreAudio)"];
                }
                [self audioTrackPopUpChanged: fAudTrack2CodecPopUp];
                [fAudTrack2MixPopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio2Mixdown"]];
                /* check to see if the selections was available, if not, rerun audioTrackPopUpChanged using the codec to just set the default
                 * mixdown*/
                if  ([fAudTrack2MixPopUp selectedItem] == nil)
                {
                    [self audioTrackPopUpChanged: fAudTrack2CodecPopUp];
                }
                [fAudTrack2RatePopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio2Samplerate"]];
                /* We set the presets bitrate if it is *not* an AC3 track since that uses the input bitrate */
                if (![[chosenPreset objectForKey:@"Audio2Encoder"] isEqualToString:@"AC3 Passthru"])
                {
                    [fAudTrack2BitratePopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio2Bitrate"]];
                }
                [fAudTrack2DrcSlider setFloatValue:[[chosenPreset objectForKey:@"Audio2TrackDRCSlider"] floatValue]];
                [self audioDRCSliderChanged: fAudTrack2DrcSlider];
            }
            if ([chosenPreset objectForKey:@"Audio3Track"] > 0)
            {
                if ([fAudLang3PopUp indexOfSelectedItem] == 0)
                {
                    [fAudLang3PopUp selectItemAtIndex: 1];
                }
                [self audioTrackPopUpChanged: fAudLang3PopUp];
                [fAudTrack3CodecPopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio3Encoder"]];
                /* check our pref for core audio and use it in place of faac if applicable */
                if ([[NSUserDefaults standardUserDefaults] boolForKey: @"UseCoreAudio"] == YES && 
                    [[chosenPreset objectForKey:@"Audio3Encoder"] isEqualToString: @"AAC (faac)"])
                {
                    [fAudTrack3CodecPopUp selectItemWithTitle:@"AAC (CoreAudio)"];
                }
                [self audioTrackPopUpChanged: fAudTrack3CodecPopUp];
                [fAudTrack3MixPopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio3Mixdown"]];
                /* check to see if the selections was available, if not, rerun audioTrackPopUpChanged using the codec to just set the default
                 * mixdown*/
                if  ([fAudTrack3MixPopUp selectedItem] == nil)
                {
                    [self audioTrackPopUpChanged: fAudTrack3CodecPopUp];
                }
                [fAudTrack3RatePopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio3Samplerate"]];
                /* We set the presets bitrate if it is *not* an AC3 track since that uses the input bitrate */
                if (![[chosenPreset objectForKey:@"Audio3Encoder"] isEqualToString: @"AC3 Passthru"])
                {
                    [fAudTrack3BitratePopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio3Bitrate"]];
                }
                [fAudTrack3DrcSlider setFloatValue:[[chosenPreset objectForKey:@"Audio3TrackDRCSlider"] floatValue]];
                [self audioDRCSliderChanged: fAudTrack3DrcSlider];
            }
            if ([chosenPreset objectForKey:@"Audio4Track"] > 0)
            {
                if ([fAudLang4PopUp indexOfSelectedItem] == 0)
                {
                    [fAudLang4PopUp selectItemAtIndex: 1];
                }
                [self audioTrackPopUpChanged: fAudLang4PopUp];
                [fAudTrack4CodecPopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio4Encoder"]];
                /* check our pref for core audio and use it in place of faac if applicable */
                if ([[NSUserDefaults standardUserDefaults] boolForKey: @"UseCoreAudio"] == YES && 
                    [[chosenPreset objectForKey:@"Audio4Encoder"] isEqualToString: @"AAC (faac)"])
                {
                    [fAudTrack4CodecPopUp selectItemWithTitle:@"AAC (CoreAudio)"];
                }
                [self audioTrackPopUpChanged: fAudTrack4CodecPopUp];
                [fAudTrack4MixPopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio4Mixdown"]];
                /* check to see if the selections was available, if not, rerun audioTrackPopUpChanged using the codec to just set the default
                 * mixdown*/
                if  ([fAudTrack4MixPopUp selectedItem] == nil)
                {
                    [self audioTrackPopUpChanged: fAudTrack4CodecPopUp];
                }
                [fAudTrack4RatePopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio4Samplerate"]];
                /* We set the presets bitrate if it is *not* an AC3 track since that uses the input bitrate */
                if (![[chosenPreset objectForKey:@"Audio4Encoder"] isEqualToString:@"AC3 Passthru"])
                {
                    [fAudTrack4BitratePopUp selectItemWithTitle:[chosenPreset objectForKey:@"Audio4Bitrate"]];
                }
                [fAudTrack4DrcSlider setFloatValue:[[chosenPreset objectForKey:@"Audio4TrackDRCSlider"] floatValue]];
                [self audioDRCSliderChanged: fAudTrack4DrcSlider];
            }
            
            /* We now cleanup any extra audio tracks that may have been previously set if we need to */
            
            if (![chosenPreset objectForKey:@"Audio2Track"] || [chosenPreset objectForKey:@"Audio2Track"] == 0)
            {
                [fAudLang2PopUp selectItemAtIndex: 0];
                [self audioTrackPopUpChanged: fAudLang2PopUp];
            }
            if (![chosenPreset objectForKey:@"Audio3Track"] || [chosenPreset objectForKey:@"Audio3Track"] > 0)
            {
                [fAudLang3PopUp selectItemAtIndex: 0];
                [self audioTrackPopUpChanged: fAudLang3PopUp];
            }
            if (![chosenPreset objectForKey:@"Audio4Track"] || [chosenPreset objectForKey:@"Audio4Track"] > 0)
            {
                [fAudLang4PopUp selectItemAtIndex: 0];
                [self audioTrackPopUpChanged: fAudLang4PopUp];
            }
        }
        
        /*Subtitles*/
        [fSubPopUp selectItemWithTitle:[chosenPreset objectForKey:@"Subtitles"]];
        /* Forced Subtitles */
        [fSubForcedCheck setState:[[chosenPreset objectForKey:@"SubtitlesForced"] intValue]];
        
        /* Picture Settings */
        /* Note: objectForKey:@"UsesPictureSettings" refers to picture size, which encompasses:
         * height, width, keep ar, anamorphic and crop settings.
         * picture filters are handled separately below.
         */
        /* Check to see if the objectForKey:@"UsesPictureSettings is greater than 0, as 0 means use picture sizing "None" 
         * ( 2 is use max for source and 1 is use exact size when the preset was created ) and the 
         * preset completely ignores any picture sizing values in the preset.
         */
        if ([[chosenPreset objectForKey:@"UsesPictureSettings"]  intValue] > 0)
        {
            hb_job_t * job = fTitle->job;
            
            /* If Cropping is set to custom, then recall all four crop values from
             when the preset was created and apply them */
            if ([[chosenPreset objectForKey:@"PictureAutoCrop"]  intValue] == 0)
            {
                [fPictureController setAutoCrop:NO];
                
                /* Here we use the custom crop values saved at the time the preset was saved */
                job->crop[0] = [[chosenPreset objectForKey:@"PictureTopCrop"]  intValue];
                job->crop[1] = [[chosenPreset objectForKey:@"PictureBottomCrop"]  intValue];
                job->crop[2] = [[chosenPreset objectForKey:@"PictureLeftCrop"]  intValue];
                job->crop[3] = [[chosenPreset objectForKey:@"PictureRightCrop"]  intValue];
                
            }
            else /* if auto crop has been saved in preset, set to auto and use post scan auto crop */
            {
                [fPictureController setAutoCrop:YES];
                /* Here we use the auto crop values determined right after scan */
                job->crop[0] = AutoCropTop;
                job->crop[1] = AutoCropBottom;
                job->crop[2] = AutoCropLeft;
                job->crop[3] = AutoCropRight;
                
            }
            
            
            /* Check to see if the objectForKey:@"UsesPictureSettings is 2 which is "Use Max for the source */
            if ([[chosenPreset objectForKey:@"UsesPictureSettings"]  intValue] == 2 || [[chosenPreset objectForKey:@"UsesMaxPictureSettings"]  intValue] == 1)
            {
                /* Use Max Picture settings for whatever the dvd is.*/
                [self revertPictureSizeToMax:nil];
                job->keep_ratio = [[chosenPreset objectForKey:@"PictureKeepRatio"]  intValue];
                if (job->keep_ratio == 1)
                {
                    hb_fix_aspect( job, HB_KEEP_WIDTH );
                    if( job->height > fTitle->height )
                    {
                        job->height = fTitle->height;
                        hb_fix_aspect( job, HB_KEEP_HEIGHT );
                    }
                }
                job->anamorphic.mode = [[chosenPreset objectForKey:@"PicturePAR"]  intValue];
            }
            else // /* If not 0 or 2 we assume objectForKey:@"UsesPictureSettings is 1 which is "Use picture sizing from when the preset was set" */
            {
                /* we check to make sure the presets width/height does not exceed the sources width/height */
                if (fTitle->width < [[chosenPreset objectForKey:@"PictureWidth"]  intValue] || fTitle->height < [[chosenPreset objectForKey:@"PictureHeight"]  intValue])
                {
                    /* if so, then we use the sources height and width to avoid scaling up */
                    //job->width = fTitle->width;
                    //job->height = fTitle->height;
                    [self revertPictureSizeToMax:nil];
                }
                else // source width/height is >= the preset height/width
                {
                    /* we can go ahead and use the presets values for height and width */
                    job->width = [[chosenPreset objectForKey:@"PictureWidth"]  intValue];
                    job->height = [[chosenPreset objectForKey:@"PictureHeight"]  intValue];
                }
                job->keep_ratio = [[chosenPreset objectForKey:@"PictureKeepRatio"]  intValue];
                if (job->keep_ratio == 1)
                {
                    hb_fix_aspect( job, HB_KEEP_WIDTH );
                    if( job->height > fTitle->height )
                    {
                        job->height = fTitle->height;
                        hb_fix_aspect( job, HB_KEEP_HEIGHT );
                    }
                }
                job->anamorphic.mode = [[chosenPreset objectForKey:@"PicturePAR"]  intValue];
                
            }
            
            
        }
        /* If the preset has an objectForKey:@"UsesPictureFilters", and handle the filters here */
        if ([chosenPreset objectForKey:@"UsesPictureFilters"] && [[chosenPreset objectForKey:@"UsesPictureFilters"]  intValue] > 0)
        {
            /* Filters */
            
            /* We only allow *either* Decomb or Deinterlace. So check for the PictureDecombDeinterlace key.
             * also, older presets may not have this key, in which case we also check to see if that preset had  PictureDecomb
             * specified, in which case we use decomb and ignore any possible Deinterlace settings as using both was less than
             * sane.
             */
            [fPictureController setUseDecomb:1];
            [fPictureController setDecomb:0];
            [fPictureController setDeinterlace:0];
            if ([[chosenPreset objectForKey:@"PictureDecombDeinterlace"] intValue] == 1 || [[chosenPreset objectForKey:@"PictureDecomb"] intValue] > 0)
            {
                /* we are using decomb */
                /* Decomb */
                if ([[chosenPreset objectForKey:@"PictureDecomb"] intValue] > 0)
                {
                    [fPictureController setDecomb:[[chosenPreset objectForKey:@"PictureDecomb"] intValue]];
                    
                    /* if we are using "Custom" in the decomb setting, also set the custom string*/
                    if ([[chosenPreset objectForKey:@"PictureDecomb"] intValue] == 1)
                    {
                        [fPictureController setDecombCustomString:[chosenPreset objectForKey:@"PictureDecombCustom"]];    
                    }
                }
             }
            else
            {
                /* We are using Deinterlace */
                /* Deinterlace */
                if ([[chosenPreset objectForKey:@"PictureDeinterlace"] intValue] > 0)
                {
                    [fPictureController setUseDecomb:0];
                    [fPictureController setDeinterlace:[[chosenPreset objectForKey:@"PictureDeinterlace"] intValue]];
                    /* if we are using "Custom" in the deinterlace setting, also set the custom string*/
                    if ([[chosenPreset objectForKey:@"PictureDeinterlace"] intValue] == 1)
                    {
                        [fPictureController setDeinterlaceCustomString:[chosenPreset objectForKey:@"PictureDeinterlaceCustom"]];    
                    }
                }
            }
            
            
            /* Detelecine */
            if ([[chosenPreset objectForKey:@"PictureDetelecine"] intValue] > 0)
            {
                [fPictureController setDetelecine:[[chosenPreset objectForKey:@"PictureDetelecine"] intValue]];
                /* if we are using "Custom" in the detelecine setting, also set the custom string*/
                if ([[chosenPreset objectForKey:@"PictureDetelecine"] intValue] == 1)
                {
                    [fPictureController setDetelecineCustomString:[chosenPreset objectForKey:@"PictureDetelecineCustom"]];    
                }
            }
            else
            {
                [fPictureController setDetelecine:0];
            }
            
            /* Denoise */
            if ([[chosenPreset objectForKey:@"PictureDenoise"] intValue] > 0)
            {
                [fPictureController setDenoise:[[chosenPreset objectForKey:@"PictureDenoise"] intValue]];
                /* if we are using "Custom" in the denoise setting, also set the custom string*/
                if ([[chosenPreset objectForKey:@"PictureDenoise"] intValue] == 1)
                {
                    [fPictureController setDenoiseCustomString:[chosenPreset objectForKey:@"PictureDenoiseCustom"]];    
                }
            }
            else
            {
                [fPictureController setDenoise:0];
            }   
            
            /* Deblock */
            if ([[chosenPreset objectForKey:@"PictureDeblock"] intValue] == 1)
            {
                /* if its a one, then its the old on/off deblock, set on to 5*/
                [fPictureController setDeblock:5];
            }
            else
            {
                /* use the settings intValue */
                [fPictureController setDeblock:[[chosenPreset objectForKey:@"PictureDeblock"] intValue]];
            }
            
            if ([[chosenPreset objectForKey:@"VideoGrayScale"] intValue] == 1)
            {
                [fPictureController setGrayscale:1];
            }
            else
            {
                [fPictureController setGrayscale:0];
            }
        }
        /* we call SetTitle: in fPictureController so we get an instant update in the Picture Settings window */
        [fPictureController SetTitle:fTitle];
        [fPictureController SetTitle:fTitle];
        [self calculatePictureSizing:nil];
    }
}


#pragma mark -
#pragma mark Manage Presets

- (void) loadPresets {
	/* We declare the default NSFileManager into fileManager */
	NSFileManager * fileManager = [NSFileManager defaultManager];
	/*We define the location of the user presets file */
    UserPresetsFile = @"~/Library/Application Support/HandBrake/UserPresets.plist";
	UserPresetsFile = [[UserPresetsFile stringByExpandingTildeInPath]retain];
    /* We check for the presets.plist */
	if ([fileManager fileExistsAtPath:UserPresetsFile] == 0)
	{
		[fileManager createFileAtPath:UserPresetsFile contents:nil attributes:nil];
	}

	UserPresets = [[NSMutableArray alloc] initWithContentsOfFile:UserPresetsFile];
	if (nil == UserPresets)
	{
		UserPresets = [[NSMutableArray alloc] init];
		[self addFactoryPresets:nil];
	}
	[fPresetsOutlineView reloadData];
    
    [self checkBuiltInsForUpdates];
}

- (void) checkBuiltInsForUpdates {
    
	BOOL updateBuiltInPresets = NO;
    int i = 0;
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
    id tempObject;
    while (tempObject = [enumerator nextObject])
    {
        /* iterate through the built in presets to see if any have an old build number */
        NSMutableDictionary *thisPresetDict = tempObject;
        /*Key Type == 0 is built in, and key PresetBuildNumber is the build number it was created with */
        if ([[thisPresetDict objectForKey:@"Type"] intValue] == 0)		
        {
			if (![thisPresetDict objectForKey:@"PresetBuildNumber"] || [[thisPresetDict objectForKey:@"PresetBuildNumber"] intValue] < [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue])
            {
                updateBuiltInPresets = YES;
            }	
		}
        i++;
    }
    /* if we have built in presets to update, then do so AlertBuiltInPresetUpdate*/
    if ( updateBuiltInPresets == YES)
    {
        if( [[NSUserDefaults standardUserDefaults] boolForKey:@"AlertBuiltInPresetUpdate"] == YES)
        {
            /* Show an alert window that built in presets will be updated */
            /*On Screen Notification*/
            int status;
            NSBeep();
            status = NSRunAlertPanel(@"HandBrake has determined your built in presets are out of date...",@"HandBrake will now update your built-in presets.", @"OK", nil, nil);
            [NSApp requestUserAttention:NSCriticalRequest];
        }
        /* when alert is dismissed, go ahead and update the built in presets */
        [self addFactoryPresets:nil];
    }
    
}


- (IBAction) showAddPresetPanel: (id) sender
{
    /* Deselect the currently selected Preset if there is one*/
    [fPresetsOutlineView deselectRow:[fPresetsOutlineView selectedRow]];

    /* Populate the preset picture settings popup here */
    [fPresetNewPicSettingsPopUp removeAllItems];
    [fPresetNewPicSettingsPopUp addItemWithTitle:@"None"];
    [fPresetNewPicSettingsPopUp addItemWithTitle:@"Current"];
    [fPresetNewPicSettingsPopUp addItemWithTitle:@"Source Maximum (post source scan)"];
    [fPresetNewPicSettingsPopUp selectItemAtIndex: 0];	
    /* Uncheck the preset use filters checkbox */
    [fPresetNewPicFiltersCheck setState:NSOffState];
    // fPresetNewFolderCheck
    [fPresetNewFolderCheck setState:NSOffState];
    /* Erase info from the input fields*/
	[fPresetNewName setStringValue: @""];
	[fPresetNewDesc setStringValue: @""];
	/* Show the panel */
	[NSApp beginSheet:fAddPresetPanel modalForWindow:fWindow modalDelegate:nil didEndSelector:NULL contextInfo:NULL];
}

- (IBAction) closeAddPresetPanel: (id) sender
{
    [NSApp endSheet: fAddPresetPanel];
    [fAddPresetPanel orderOut: self];
}

- (IBAction)addUserPreset:(id)sender
{
    if (![[fPresetNewName stringValue] length])
            NSRunAlertPanel(@"Warning!", @"You need to insert a name for the preset.", @"OK", nil , nil);
    else
    {
        /* Here we create a custom user preset */
        [UserPresets addObject:[self createPreset]];
        [self addPreset];

        [self closeAddPresetPanel:nil];
    }
}
- (void)addPreset
{

	
	/* We Reload the New Table data for presets */
    [fPresetsOutlineView reloadData];
   /* We save all of the preset data here */
    [self savePreset];
}

- (void)sortPresets
{

	
	/* We Sort the Presets By Factory or Custom */
	NSSortDescriptor * presetTypeDescriptor=[[[NSSortDescriptor alloc] initWithKey:@"Type" 
                                                    ascending:YES] autorelease];
	/* We Sort the Presets Alphabetically by name  We do not use this now as we have drag and drop*/
	/*
    NSSortDescriptor * presetNameDescriptor=[[[NSSortDescriptor alloc] initWithKey:@"PresetName" 
                                                    ascending:YES selector:@selector(caseInsensitiveCompare:)] autorelease];
	//NSArray *sortDescriptors=[NSArray arrayWithObjects:presetTypeDescriptor,presetNameDescriptor,nil];
    
    */
    /* Since we can drag and drop our custom presets, lets just sort by type and not name */
    NSArray *sortDescriptors=[NSArray arrayWithObjects:presetTypeDescriptor,nil];
	NSArray *sortedArray=[UserPresets sortedArrayUsingDescriptors:sortDescriptors];
	[UserPresets setArray:sortedArray];
	

}

- (IBAction)insertPreset:(id)sender
{
    int index = [fPresetsOutlineView selectedRow];
    [UserPresets insertObject:[self createPreset] atIndex:index];
    [fPresetsOutlineView reloadData];
    [self savePreset];
}

- (NSDictionary *)createPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
    /* Preset build number */
    [preset setObject:[NSString stringWithFormat: @"%d", [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue]] forKey:@"PresetBuildNumber"];
    [preset setObject:[fPresetNewName stringValue] forKey:@"PresetName"];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:[fPresetNewName stringValue] forKey:@"PresetName"];
    /* Set whether or not this is to be a folder fPresetNewFolderCheck*/
    [preset setObject:[NSNumber numberWithBool:[fPresetNewFolderCheck state]] forKey:@"Folder"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
    if ([fPresetNewFolderCheck state] == YES)
    {
        /* initialize and set an empty array for children here since we are a new folder */
        NSMutableArray *childrenArray = [[NSMutableArray alloc] init];
        [preset setObject:[NSMutableArray arrayWithArray: childrenArray] forKey:@"ChildrenArray"];
        [childrenArray autorelease];
    }
    else // we are not creating a preset folder, so we go ahead with the rest of the preset info
    {
        /*Get the whether or not to apply pic Size and Cropping (includes Anamorphic)*/
        [preset setObject:[NSNumber numberWithInt:[fPresetNewPicSettingsPopUp indexOfSelectedItem]] forKey:@"UsesPictureSettings"];
        /* Get whether or not to use the current Picture Filter settings for the preset */
        [preset setObject:[NSNumber numberWithInt:[fPresetNewPicFiltersCheck state]] forKey:@"UsesPictureFilters"];
        
        /* Get New Preset Description from the field in the AddPresetPanel*/
        [preset setObject:[fPresetNewDesc stringValue] forKey:@"PresetDescription"];
        /* File Format */
        [preset setObject:[fDstFormatPopUp titleOfSelectedItem] forKey:@"FileFormat"];
        /* Chapter Markers fCreateChapterMarkers*/
        [preset setObject:[NSNumber numberWithInt:[fCreateChapterMarkers state]] forKey:@"ChapterMarkers"];
        /* Allow Mpeg4 64 bit formatting +4GB file sizes */
        [preset setObject:[NSNumber numberWithInt:[fDstMp4LargeFileCheck state]] forKey:@"Mp4LargeFile"];
        /* Mux mp4 with http optimization */
        [preset setObject:[NSNumber numberWithInt:[fDstMp4HttpOptFileCheck state]] forKey:@"Mp4HttpOptimize"];
        /* Add iPod uuid atom */
        [preset setObject:[NSNumber numberWithInt:[fDstMp4iPodFileCheck state]] forKey:@"Mp4iPodCompatible"];
        
        /* Codecs */
        /* Video encoder */
        [preset setObject:[fVidEncoderPopUp titleOfSelectedItem] forKey:@"VideoEncoder"];
        /* x264 Option String */
        [preset setObject:[fAdvancedOptions optionsString] forKey:@"x264Option"];
        
        [preset setObject:[NSNumber numberWithInt:[fVidQualityMatrix selectedRow]] forKey:@"VideoQualityType"];
        [preset setObject:[fVidTargetSizeField stringValue] forKey:@"VideoTargetSize"];
        [preset setObject:[fVidBitrateField stringValue] forKey:@"VideoAvgBitrate"];
        [preset setObject:[NSNumber numberWithFloat:[fVidQualityRFField floatValue]] forKey:@"VideoQualitySlider"];
        
        /* Video framerate */
        if ([fVidRatePopUp indexOfSelectedItem] == 0) // Same as source is selected
        {
            [preset setObject:@"Same as source" forKey:@"VideoFramerate"];
        }
        else // we can record the actual titleOfSelectedItem
        {
            [preset setObject:[fVidRatePopUp titleOfSelectedItem] forKey:@"VideoFramerate"];
        }
        
        /* 2 Pass Encoding */
        [preset setObject:[NSNumber numberWithInt:[fVidTwoPassCheck state]] forKey:@"VideoTwoPass"];
        /* Turbo 2 pass Encoding fVidTurboPassCheck*/
        [preset setObject:[NSNumber numberWithInt:[fVidTurboPassCheck state]] forKey:@"VideoTurboTwoPass"];
        /*Picture Settings*/
        hb_job_t * job = fTitle->job;
        /* Picture Sizing */
        /* Use Max Picture settings for whatever the dvd is.*/
        [preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
        [preset setObject:[NSNumber numberWithInt:fTitle->job->width] forKey:@"PictureWidth"];
        [preset setObject:[NSNumber numberWithInt:fTitle->job->height] forKey:@"PictureHeight"];
        [preset setObject:[NSNumber numberWithInt:fTitle->job->keep_ratio] forKey:@"PictureKeepRatio"];
        [preset setObject:[NSNumber numberWithInt:fTitle->job->anamorphic.mode] forKey:@"PicturePAR"];
        
        /* Set crop settings here */
        [preset setObject:[NSNumber numberWithInt:[fPictureController autoCrop]] forKey:@"PictureAutoCrop"];
        [preset setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
        [preset setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
        [preset setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
        [preset setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
        
        /* Picture Filters */
        [preset setObject:[NSNumber numberWithInt:[fPictureController useDecomb]] forKey:@"PictureDecombDeinterlace"];
        [preset setObject:[NSNumber numberWithInt:[fPictureController deinterlace]] forKey:@"PictureDeinterlace"];
        [preset setObject:[fPictureController deinterlaceCustomString] forKey:@"PictureDeinterlaceCustom"];
        [preset setObject:[NSNumber numberWithInt:[fPictureController detelecine]] forKey:@"PictureDetelecine"];
        [preset setObject:[fPictureController detelecineCustomString] forKey:@"PictureDetelecineCustom"];
        [preset setObject:[NSNumber numberWithInt:[fPictureController denoise]] forKey:@"PictureDenoise"];
        [preset setObject:[fPictureController denoiseCustomString] forKey:@"PictureDenoiseCustom"];
        [preset setObject:[NSNumber numberWithInt:[fPictureController deblock]] forKey:@"PictureDeblock"]; 
        [preset setObject:[NSNumber numberWithInt:[fPictureController decomb]] forKey:@"PictureDecomb"];
        [preset setObject:[fPictureController decombCustomString] forKey:@"PictureDecombCustom"];
        [preset setObject:[NSNumber numberWithInt:[fPictureController grayscale]] forKey:@"VideoGrayScale"];
        
        /*Audio*/
        NSMutableArray *audioListArray = [[NSMutableArray alloc] init];
        /* we actually call the methods for the nests here */
        if ([fAudLang1PopUp indexOfSelectedItem] > 0)
        {
            NSMutableDictionary *audioTrack1Array = [[NSMutableDictionary alloc] init];
            [audioTrack1Array setObject:[NSNumber numberWithInt:[fAudLang1PopUp indexOfSelectedItem]] forKey:@"AudioTrack"];
            [audioTrack1Array setObject:[fAudLang1PopUp titleOfSelectedItem] forKey:@"AudioTrackDescription"];
            [audioTrack1Array setObject:[fAudTrack1CodecPopUp titleOfSelectedItem] forKey:@"AudioEncoder"];
            [audioTrack1Array setObject:[fAudTrack1MixPopUp titleOfSelectedItem] forKey:@"AudioMixdown"];
            [audioTrack1Array setObject:[fAudTrack1RatePopUp titleOfSelectedItem] forKey:@"AudioSamplerate"];
            [audioTrack1Array setObject:[fAudTrack1BitratePopUp titleOfSelectedItem] forKey:@"AudioBitrate"];
            [audioTrack1Array setObject:[NSNumber numberWithFloat:[fAudTrack1DrcSlider floatValue]] forKey:@"AudioTrackDRCSlider"];
            [audioTrack1Array autorelease];
            [audioListArray addObject:audioTrack1Array];
        }
        
        if ([fAudLang2PopUp indexOfSelectedItem] > 0)
        {
            NSMutableDictionary *audioTrack2Array = [[NSMutableDictionary alloc] init];
            [audioTrack2Array setObject:[NSNumber numberWithInt:[fAudLang2PopUp indexOfSelectedItem]] forKey:@"AudioTrack"];
            [audioTrack2Array setObject:[fAudLang2PopUp titleOfSelectedItem] forKey:@"AudioTrackDescription"];
            [audioTrack2Array setObject:[fAudTrack2CodecPopUp titleOfSelectedItem] forKey:@"AudioEncoder"];
            [audioTrack2Array setObject:[fAudTrack2MixPopUp titleOfSelectedItem] forKey:@"AudioMixdown"];
            [audioTrack2Array setObject:[fAudTrack2RatePopUp titleOfSelectedItem] forKey:@"AudioSamplerate"];
            [audioTrack2Array setObject:[fAudTrack2BitratePopUp titleOfSelectedItem] forKey:@"AudioBitrate"];
            [audioTrack2Array setObject:[NSNumber numberWithFloat:[fAudTrack2DrcSlider floatValue]] forKey:@"AudioTrackDRCSlider"];
            [audioTrack2Array autorelease];
            [audioListArray addObject:audioTrack2Array];
        }
        
        if ([fAudLang3PopUp indexOfSelectedItem] > 0)
        {
            NSMutableDictionary *audioTrack3Array = [[NSMutableDictionary alloc] init];
            [audioTrack3Array setObject:[NSNumber numberWithInt:[fAudLang3PopUp indexOfSelectedItem]] forKey:@"AudioTrack"];
            [audioTrack3Array setObject:[fAudLang3PopUp titleOfSelectedItem] forKey:@"AudioTrackDescription"];
            [audioTrack3Array setObject:[fAudTrack3CodecPopUp titleOfSelectedItem] forKey:@"AudioEncoder"];
            [audioTrack3Array setObject:[fAudTrack3MixPopUp titleOfSelectedItem] forKey:@"AudioMixdown"];
            [audioTrack3Array setObject:[fAudTrack3RatePopUp titleOfSelectedItem] forKey:@"AudioSamplerate"];
            [audioTrack3Array setObject:[fAudTrack3BitratePopUp titleOfSelectedItem] forKey:@"AudioBitrate"];
            [audioTrack3Array setObject:[NSNumber numberWithFloat:[fAudTrack3DrcSlider floatValue]] forKey:@"AudioTrackDRCSlider"];
            [audioTrack3Array autorelease];
            [audioListArray addObject:audioTrack3Array];
        }
        
        if ([fAudLang4PopUp indexOfSelectedItem] > 0)
        {
            NSMutableDictionary *audioTrack4Array = [[NSMutableDictionary alloc] init];
            [audioTrack4Array setObject:[NSNumber numberWithInt:[fAudLang4PopUp indexOfSelectedItem]] forKey:@"AudioTrack"];
            [audioTrack4Array setObject:[fAudLang4PopUp titleOfSelectedItem] forKey:@"AudioTrackDescription"];
            [audioTrack4Array setObject:[fAudTrack4CodecPopUp titleOfSelectedItem] forKey:@"AudioEncoder"];
            [audioTrack4Array setObject:[fAudTrack4MixPopUp titleOfSelectedItem] forKey:@"AudioMixdown"];
            [audioTrack4Array setObject:[fAudTrack4RatePopUp titleOfSelectedItem] forKey:@"AudioSamplerate"];
            [audioTrack4Array setObject:[fAudTrack4BitratePopUp titleOfSelectedItem] forKey:@"AudioBitrate"];
            [audioTrack4Array setObject:[NSNumber numberWithFloat:[fAudTrack4DrcSlider floatValue]] forKey:@"AudioTrackDRCSlider"];
            [audioTrack4Array autorelease];
            [audioListArray addObject:audioTrack4Array];
        }
        
        
        [preset setObject:[NSMutableArray arrayWithArray: audioListArray] forKey:@"AudioList"];

        
        /* Temporarily remove subtitles from creating a new preset as it has to be converted over to use the new
         * subititle array code. */
        /* Subtitles*/
        //[preset setObject:[fSubPopUp titleOfSelectedItem] forKey:@"Subtitles"];
        /* Forced Subtitles */
        //[preset setObject:[NSNumber numberWithInt:[fSubForcedCheck state]] forKey:@"SubtitlesForced"];
    }
    [preset autorelease];
    return preset;
    
}

- (void)savePreset
{
    [UserPresets writeToFile:UserPresetsFile atomically:YES];
	/* We get the default preset in case it changed */
	[self getDefaultPresets:nil];

}

- (IBAction)deletePreset:(id)sender
{
    
    
    if ( [fPresetsOutlineView numberOfSelectedRows] == 0 )
    {
        return;
    }
    /* Alert user before deleting preset */
	int status;
    status = NSRunAlertPanel(@"Warning!", @"Are you sure that you want to delete the selected preset?", @"OK", @"Cancel", nil);
    
    if ( status == NSAlertDefaultReturn ) 
    {
        int presetToModLevel = [fPresetsOutlineView levelForItem: [fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]]];
        NSDictionary *presetToMod = [fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]];
        NSDictionary *presetToModParent = [fPresetsOutlineView parentForItem: presetToMod];
        
        NSEnumerator *enumerator;
        NSMutableArray *presetsArrayToMod;
        NSMutableArray *tempArray;
        id tempObject;
        /* If we are a root level preset, we are modding the UserPresets array */
        if (presetToModLevel == 0)
        {
            presetsArrayToMod = UserPresets;
        }
        else // We have a parent preset, so we modify the chidren array object for key
        {
            presetsArrayToMod = [presetToModParent objectForKey:@"ChildrenArray"]; 
        }
        
        enumerator = [presetsArrayToMod objectEnumerator];
        tempArray = [NSMutableArray array];
        
        while (tempObject = [enumerator nextObject]) 
        {
            NSDictionary *thisPresetDict = tempObject;
            if (thisPresetDict == presetToMod)
            {
                [tempArray addObject:tempObject];
            }
        }
        
        [presetsArrayToMod removeObjectsInArray:tempArray];
        [fPresetsOutlineView reloadData];
        [self savePreset];   
    }
}


#pragma mark -
#pragma mark Import Export Preset(s)

- (IBAction) browseExportPresetFile: (id) sender
{
    /* Open a panel to let the user choose where and how to save the export file */
    NSSavePanel * panel = [NSSavePanel savePanel];
	/* We get the current file name and path from the destination field here */
    NSString *defaultExportDirectory = [NSString stringWithFormat: @"%@/Desktop/", NSHomeDirectory()];

	[panel beginSheetForDirectory: defaultExportDirectory file: @"HB_Export.plist"
				   modalForWindow: fWindow modalDelegate: self
				   didEndSelector: @selector( browseExportPresetFileDone:returnCode:contextInfo: )
					  contextInfo: NULL];
}

- (void) browseExportPresetFileDone: (NSSavePanel *) sheet
                   returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        NSString *presetExportDirectory = [[sheet filename] stringByDeletingLastPathComponent];
        NSString *exportPresetsFile = [sheet filename];
        [[NSUserDefaults standardUserDefaults] setObject:presetExportDirectory forKey:@"LastPresetExportDirectory"];
        /* We check for the presets.plist */
        if ([[NSFileManager defaultManager] fileExistsAtPath:exportPresetsFile] == 0)
        {
            [[NSFileManager defaultManager] createFileAtPath:exportPresetsFile contents:nil attributes:nil];
        }
        NSMutableArray * presetsToExport = [[NSMutableArray alloc] initWithContentsOfFile:exportPresetsFile];
        if (nil == presetsToExport)
        {
            presetsToExport = [[NSMutableArray alloc] init];
            
            /* now get and add selected presets to export */
            
        }
        if ([fPresetsOutlineView selectedRow] >= 0 && [[[fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]] objectForKey:@"Folder"] intValue] != 1)
        {
            [presetsToExport addObject:[fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]]];
            [presetsToExport writeToFile:exportPresetsFile atomically:YES];
            
        }
        
    }
}


- (IBAction) browseImportPresetFile: (id) sender
{

    NSOpenPanel * panel;
	
    panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: NO ];
    NSString * sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastPresetImportDirectory"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastPresetImportDirectory"];
	}
	else
	{
		sourceDirectory = @"~/Desktop";
		sourceDirectory = [sourceDirectory stringByExpandingTildeInPath];
	}
    /* we open up the browse sources sheet here and call for browseSourcesDone after the sheet is closed
        * to evaluate whether we want to specify a title, we pass the sender in the contextInfo variable
        */
    /* set this for allowed file types, not sure if we should allow xml or not */
    NSArray *fileTypes = [NSArray arrayWithObjects:@"plist", @"xml", nil];
    [panel beginSheetForDirectory: sourceDirectory file: nil types: fileTypes
                   modalForWindow: fWindow modalDelegate: self
                   didEndSelector: @selector( browseImportPresetDone:returnCode:contextInfo: )
                      contextInfo: sender];
}

- (void) browseImportPresetDone: (NSSavePanel *) sheet
                     returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        NSString *importPresetsDirectory = [[sheet filename] stringByDeletingLastPathComponent];
        NSString *importPresetsFile = [sheet filename];
        [[NSUserDefaults standardUserDefaults] setObject:importPresetsDirectory forKey:@"LastPresetImportDirectory"];
        /* NOTE: here we need to do some sanity checking to verify we do not hose up our presets file   */
        NSMutableArray * presetsToImport = [[NSMutableArray alloc] initWithContentsOfFile:importPresetsFile];
        /* iterate though the new array of presets to import and add them to our presets array */
        int i = 0;
        NSEnumerator *enumerator = [presetsToImport objectEnumerator];
        id tempObject;
        while (tempObject = [enumerator nextObject])
        {
            /* make any changes to the incoming preset we see fit */
            /* make sure the incoming preset is not tagged as default */
            [tempObject setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
            /* prepend "(imported) to the name of the incoming preset for clarification since it can be changed */
            NSString * prependedName = [@"(import) " stringByAppendingString:[tempObject objectForKey:@"PresetName"]] ;
            [tempObject setObject:prependedName forKey:@"PresetName"];
            
            /* actually add the new preset to our presets array */
            [UserPresets addObject:tempObject];
            i++;
        }
        [presetsToImport autorelease];
        [self sortPresets];
        [self addPreset];
        
    }
}

#pragma mark -
#pragma mark Manage Default Preset

- (IBAction)getDefaultPresets:(id)sender
{
	presetHbDefault = nil;
    presetUserDefault = nil;
    presetUserDefaultParent = nil;
    presetUserDefaultParentParent = nil;
    NSMutableDictionary *presetHbDefaultParent = nil;
    NSMutableDictionary *presetHbDefaultParentParent = nil;
    
    int i = 0;
    BOOL userDefaultFound = NO;
    presetCurrentBuiltInCount = 0;
    /* First we iterate through the root UserPresets array to check for defaults */
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
	id tempObject;
	while (tempObject = [enumerator nextObject])
	{
		NSMutableDictionary *thisPresetDict = tempObject;
		if ([[thisPresetDict objectForKey:@"Default"] intValue] == 1) // 1 is HB default
		{
			presetHbDefault = thisPresetDict;	
		}
		if ([[thisPresetDict objectForKey:@"Default"] intValue] == 2) // 2 is User specified default
		{
			presetUserDefault = thisPresetDict;
            userDefaultFound = YES;
        }
        if ([[thisPresetDict objectForKey:@"Type"] intValue] == 0) // Type 0 is a built in preset		
        {
			presetCurrentBuiltInCount++; // <--increment the current number of built in presets	
		}
		i++;
        
        /* if we run into a folder, go to level 1 and iterate through the children arrays for the default */
        if ([thisPresetDict objectForKey:@"ChildrenArray"])
        {
            NSMutableDictionary *thisPresetDictParent = thisPresetDict;
            NSEnumerator *enumerator = [[thisPresetDict objectForKey:@"ChildrenArray"] objectEnumerator];
            id tempObject;
            while (tempObject = [enumerator nextObject])
            {
                NSMutableDictionary *thisPresetDict = tempObject;
                if ([[thisPresetDict objectForKey:@"Default"] intValue] == 1) // 1 is HB default
                {
                    presetHbDefault = thisPresetDict;
                    presetHbDefaultParent = thisPresetDictParent;
                }
                if ([[thisPresetDict objectForKey:@"Default"] intValue] == 2) // 2 is User specified default
                {
                    presetUserDefault = thisPresetDict;
                    presetUserDefaultParent = thisPresetDictParent;
                    userDefaultFound = YES;
                }
                
                /* if we run into a folder, go to level 2 and iterate through the children arrays for the default */
                if ([thisPresetDict objectForKey:@"ChildrenArray"])
                {
                    NSMutableDictionary *thisPresetDictParentParent = thisPresetDict;
                    NSEnumerator *enumerator = [[thisPresetDict objectForKey:@"ChildrenArray"] objectEnumerator];
                    id tempObject;
                    while (tempObject = [enumerator nextObject])
                    {
                        NSMutableDictionary *thisPresetDict = tempObject;
                        if ([[thisPresetDict objectForKey:@"Default"] intValue] == 1) // 1 is HB default
                        {
                            presetHbDefault = thisPresetDict;
                            presetHbDefaultParent = thisPresetDictParent;
                            presetHbDefaultParentParent = thisPresetDictParentParent;	
                        }
                        if ([[thisPresetDict objectForKey:@"Default"] intValue] == 2) // 2 is User specified default
                        {
                            presetUserDefault = thisPresetDict;
                            presetUserDefaultParent = thisPresetDictParent;
                            presetUserDefaultParentParent = thisPresetDictParentParent;
                            userDefaultFound = YES;	
                        }
                        
                    }
                }
            }
        }
        
	}
    /* check to see if a user specified preset was found, if not then assign the parents for
     * the presetHbDefault so that we can open the parents for the nested presets
     */
    if (userDefaultFound == NO)
    {
        presetUserDefaultParent = presetHbDefaultParent;
        presetUserDefaultParentParent = presetHbDefaultParentParent;
    }
}

- (IBAction)setDefaultPreset:(id)sender
{
/* We need to determine if the item is a folder */
   if ([[[fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]] objectForKey:@"Folder"] intValue] == 1)
   {
   return;
   }

    int i = 0;
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
	id tempObject;
	/* First make sure the old user specified default preset is removed */
    while (tempObject = [enumerator nextObject])
	{
		NSMutableDictionary *thisPresetDict = tempObject;
		if ([[tempObject objectForKey:@"Default"] intValue] != 1) // if not the default HB Preset, set to 0
		{
			[[UserPresets objectAtIndex:i] setObject:[NSNumber numberWithInt:0] forKey:@"Default"];	
		}
		
		/* if we run into a folder, go to level 1 and iterate through the children arrays for the default */
        if ([thisPresetDict objectForKey:@"ChildrenArray"])
        {
            NSEnumerator *enumerator = [[thisPresetDict objectForKey:@"ChildrenArray"] objectEnumerator];
            id tempObject;
            int ii = 0;
            while (tempObject = [enumerator nextObject])
            {
                NSMutableDictionary *thisPresetDict1 = tempObject;
                if ([[tempObject objectForKey:@"Default"] intValue] != 1) // if not the default HB Preset, set to 0
                {
                    [[[thisPresetDict objectForKey:@"ChildrenArray"] objectAtIndex:ii] setObject:[NSNumber numberWithInt:0] forKey:@"Default"];	
                }
                /* if we run into a folder, go to level 2 and iterate through the children arrays for the default */
                if ([thisPresetDict1 objectForKey:@"ChildrenArray"])
                {
                    NSEnumerator *enumerator = [[thisPresetDict1 objectForKey:@"ChildrenArray"] objectEnumerator];
                    id tempObject;
                    int iii = 0;
                    while (tempObject = [enumerator nextObject])
                    {
                        if ([[tempObject objectForKey:@"Default"] intValue] != 1) // if not the default HB Preset, set to 0
                        {
                            [[[thisPresetDict1 objectForKey:@"ChildrenArray"] objectAtIndex:iii] setObject:[NSNumber numberWithInt:0] forKey:@"Default"];	
                        }
                        iii++;
                    }
                }
                ii++;
            }
            
        }
        i++; 
	}
    
    
    int presetToModLevel = [fPresetsOutlineView levelForItem: [fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]]];
    NSDictionary *presetToMod = [fPresetsOutlineView itemAtRow:[fPresetsOutlineView selectedRow]];
    NSDictionary *presetToModParent = [fPresetsOutlineView parentForItem: presetToMod];
    
    
    NSMutableArray *presetsArrayToMod;
    NSMutableArray *tempArray;
    
    /* If we are a root level preset, we are modding the UserPresets array */
    if (presetToModLevel == 0)
    {
        presetsArrayToMod = UserPresets;
    }
    else // We have a parent preset, so we modify the chidren array object for key
    {
        presetsArrayToMod = [presetToModParent objectForKey:@"ChildrenArray"]; 
    }
    
    enumerator = [presetsArrayToMod objectEnumerator];
    tempArray = [NSMutableArray array];
    int iiii = 0;
    while (tempObject = [enumerator nextObject]) 
    {
        NSDictionary *thisPresetDict = tempObject;
        if (thisPresetDict == presetToMod)
        {
            if ([[tempObject objectForKey:@"Default"] intValue] != 1) // if not the default HB Preset, set to 2
            {
                [[presetsArrayToMod objectAtIndex:iiii] setObject:[NSNumber numberWithInt:2] forKey:@"Default"];	
            }
        }
     iiii++;
     }
    
    
    /* We save all of the preset data here */
    [self savePreset];
    /* We Reload the New Table data for presets */
    [fPresetsOutlineView reloadData];
}

- (IBAction)selectDefaultPreset:(id)sender
{
	NSMutableDictionary *presetToMod;
    /* if there is a user specified default, we use it */
	if (presetUserDefault)
	{
        presetToMod = presetUserDefault;
    }
	else if (presetHbDefault) //else we use the built in default presetHbDefault
	{
        presetToMod = presetHbDefault;
	}
    else
    {
    return;
    }
    
    if (presetUserDefaultParent != nil)
    {
        [fPresetsOutlineView expandItem:presetUserDefaultParent];
        
    }
    if (presetUserDefaultParentParent != nil)
    {
        [fPresetsOutlineView expandItem:presetUserDefaultParentParent];
        
    }
    
    [fPresetsOutlineView selectRowIndexes:[NSIndexSet indexSetWithIndex:[fPresetsOutlineView rowForItem: presetToMod]] byExtendingSelection:NO];
	[self selectPreset:nil];
}


#pragma mark -
#pragma mark Manage Built In Presets


- (IBAction)deleteFactoryPresets:(id)sender
{
    //int status;
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
	id tempObject;
    
	//NSNumber *index;
    NSMutableArray *tempArray;


        tempArray = [NSMutableArray array];
        /* we look here to see if the preset is we move on to the next one */
        while ( tempObject = [enumerator nextObject] )  
		{
			/* if the preset is "Factory" then we put it in the array of
			presets to delete */
			if ([[tempObject objectForKey:@"Type"] intValue] == 0)
			{
				[tempArray addObject:tempObject];
			}
        }
        
        [UserPresets removeObjectsInArray:tempArray];
        [fPresetsOutlineView reloadData];
        [self savePreset];   

}

   /* We use this method to recreate new, updated factory presets */
- (IBAction)addFactoryPresets:(id)sender
{
    
    /* First, we delete any existing built in presets */
    [self deleteFactoryPresets: sender];
    /* Then we generate new built in presets programmatically with fPresetsBuiltin
     * which is all setup in HBPresets.h and  HBPresets.m*/
    [fPresetsBuiltin generateBuiltinPresets:UserPresets];
    /* update build number for built in presets */
    /* iterate though the new array of presets to import and add them to our presets array */
    int i = 0;
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
    id tempObject;
    while (tempObject = [enumerator nextObject])
    {
        /* Record the apps current build number in the PresetBuildNumber key */
        if ([[tempObject objectForKey:@"Type"] intValue] == 0) // Type 0 is a built in preset		
        {
            /* Preset build number */
            [[UserPresets objectAtIndex:i] setObject:[NSNumber numberWithInt:[[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue]] forKey:@"PresetBuildNumber"];
        }
        i++;
    }
    /* report the built in preset updating to the activity log */
    [self writeToActivityLog: "built in presets updated to build number: %d", [[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"] intValue]];
    
    [self sortPresets];
    [self addPreset];
    
}


@end

/*******************************
 * Subclass of the HBPresetsOutlineView *
 *******************************/

@implementation HBPresetsOutlineView
- (NSImage *)dragImageForRowsWithIndexes:(NSIndexSet *)dragRows tableColumns:(NSArray *)tableColumns event:(NSEvent*)dragEvent offset:(NSPointPointer)dragImageOffset
{
    fIsDragging = YES;

    // By default, NSTableView only drags an image of the first column. Change this to
    // drag an image of the queue's icon and PresetName columns.
    NSArray * cols = [NSArray arrayWithObjects: [self tableColumnWithIdentifier:@"PresetName"], nil];
    return [super dragImageForRowsWithIndexes:dragRows tableColumns:cols event:dragEvent offset:dragImageOffset];
}



- (void) mouseDown:(NSEvent *)theEvent
{
    [super mouseDown:theEvent];
	fIsDragging = NO;
}



- (BOOL) isDragging;
{
    return fIsDragging;
}
@end



