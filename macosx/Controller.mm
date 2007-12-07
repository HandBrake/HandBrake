/* $Id: Controller.mm,v 1.79 2005/11/04 19:41:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "Controller.h"
#include "a52dec/a52.h"
#import "HBOutputPanelController.h"
#import "HBPreferencesController.h"
/* Added to integrate scanning into HBController */
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>
#include "HBDVDDetector.h"
#include "dvdread/dvd_reader.h"
#include "HBPresets.h"

#define _(a) NSLocalizedString(a,NULL)

static int FormatSettings[4][10] =
  { { HB_MUX_MP4 | HB_VCODEC_FFMPEG | HB_ACODEC_FAAC,
	  HB_MUX_MP4 | HB_VCODEC_X264   | HB_ACODEC_FAAC,
	  0,
	  0 },
    { HB_MUX_MKV | HB_VCODEC_FFMPEG | HB_ACODEC_FAAC,
	  HB_MUX_MKV | HB_VCODEC_FFMPEG | HB_ACODEC_AC3,
	  HB_MUX_MKV | HB_VCODEC_FFMPEG | HB_ACODEC_LAME,
	  HB_MUX_MKV | HB_VCODEC_FFMPEG | HB_ACODEC_VORBIS,
	  HB_MUX_MKV | HB_VCODEC_X264   | HB_ACODEC_FAAC,
	  HB_MUX_MKV | HB_VCODEC_X264   | HB_ACODEC_AC3,
	  HB_MUX_MKV | HB_VCODEC_X264   | HB_ACODEC_LAME,
	  HB_MUX_MKV | HB_VCODEC_X264   | HB_ACODEC_VORBIS,
	  0,
	  0 },
    { HB_MUX_AVI | HB_VCODEC_FFMPEG | HB_ACODEC_LAME,
	  HB_MUX_AVI | HB_VCODEC_FFMPEG | HB_ACODEC_AC3,
	  HB_MUX_AVI | HB_VCODEC_X264   | HB_ACODEC_LAME,
	  HB_MUX_AVI | HB_VCODEC_X264   | HB_ACODEC_AC3},
    { HB_MUX_OGM | HB_VCODEC_FFMPEG | HB_ACODEC_VORBIS,
	  HB_MUX_OGM | HB_VCODEC_FFMPEG | HB_ACODEC_LAME,
	  0,
	  0 } };

/* We setup the toolbar values here */
static NSString *        ToggleDrawerIdentifier             = @"Toggle Drawer Item Identifier";
static NSString *        StartEncodingIdentifier            = @"Start Encoding Item Identifier";
static NSString *        PauseEncodingIdentifier            = @"Pause Encoding Item Identifier";
static NSString *        ShowQueueIdentifier                = @"Show Queue Item Identifier";
static NSString *        AddToQueueIdentifier               = @"Add to Queue Item Identifier";
static NSString *        ShowActivityIdentifier             = @"Debug Output Item Identifier";
static NSString *        ChooseSourceIdentifier             = @"Choose Source Item Identifier";


/*******************************
 * HBController implementation *
 *******************************/
@implementation HBController

- init
{
    self = [super init];
    [HBPreferencesController registerUserDefaults];
    fHandle = NULL;
    /* Check for check for the app support directory here as
        * outputPanel needs it right away, as may other future methods
        */
    /* We declare the default NSFileManager into fileManager */
	NSFileManager * fileManager = [NSFileManager defaultManager];
	/* we set the files and support paths here */
	AppSupportDirectory = @"~/Library/Application Support/HandBrake";
    AppSupportDirectory = [AppSupportDirectory stringByExpandingTildeInPath];
    /* We check for the app support directory for handbrake */
	if ([fileManager fileExistsAtPath:AppSupportDirectory] == 0) 
	{
		// If it doesnt exist yet, we create it here 
		[fileManager createDirectoryAtPath:AppSupportDirectory attributes:nil];
	}
    
    outputPanel = [[HBOutputPanelController alloc] init];
    fPictureController = [[PictureController alloc] initWithDelegate:self];
    fQueueController = [[HBQueueController alloc] init];
    fAdvancedOptions = [[HBAdvancedController alloc] init];
    /* we init the HBPresets class which currently is only used
    * for updating built in presets, may move more functionality
    * there in the future
    */
    fPresetsBuiltin = [[HBPresets alloc] init];
    fPreferencesController = [[HBPreferencesController alloc] init];
    return self;
}


- (void) applicationDidFinishLaunching: (NSNotification *) notification
{
    int    build;
    char * version;
    
    // Init libhb
	int debugLevel = [[NSUserDefaults standardUserDefaults] boolForKey:@"ShowVerboseOutput"] ? HB_DEBUG_ALL : HB_DEBUG_NONE;
    fHandle = hb_init(debugLevel, [[NSUserDefaults standardUserDefaults] boolForKey:@"CheckForUpdates"]);
    
	// Set the Growl Delegate
    [GrowlApplicationBridge setGrowlDelegate: self];    
    /* Init others controllers */
    [fPictureController SetHandle: fHandle];
    [fQueueController   setHandle: fHandle];
    [fQueueController   setHBController: self];
	
    fChapterTitlesDelegate = [[ChapterTitles alloc] init];
    [fChapterTable setDataSource:fChapterTitlesDelegate];
    
    /* Call UpdateUI every 1/2 sec */
    [[NSRunLoop currentRunLoop] addTimer: [NSTimer
        scheduledTimerWithTimeInterval: 0.5 target: self
                              selector: @selector( updateUI: ) userInfo: NULL repeats: YES]
                                forMode: NSEventTrackingRunLoopMode];
    
    // Open debug output window now if it was visible when HB was closed
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"OutputPanelIsOpen"])
        [self showDebugOutputPanel:nil];
    
    // Open queue window now if it was visible when HB was closed
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"QueueWindowIsOpen"])
        [self showQueueWindow:nil];
    
	[self openMainWindow:nil];
    
    if( ( build = hb_check_update( fHandle, &version ) ) > -1 )
    {
        /* Update available - tell the user */
        
        NSBeginInformationalAlertSheet( _( @"Update is available" ),
                                        _( @"Go get it!" ), _( @"Discard" ), NULL, fWindow, self,
                                        @selector( updateAlertDone:returnCode:contextInfo: ),
                                        NULL, NULL, [NSString stringWithFormat:
                                            _( @"HandBrake %s (build %d) is now available for download." ),
                                            version, build] );
        return;
        
    }
	
    /* Show Browse Sources Window ASAP */
    [self performSelectorOnMainThread: @selector(browseSources:)
                           withObject: NULL waitUntilDone: NO];
                           }

- (void) updateAlertDone: (NSWindow *) sheet
              returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSAlertDefaultReturn )
    {
        /* Go to HandBrake homepage and exit */
        [self openHomepage: NULL];
        [NSApp terminate: self];
     
    }
    else
    {
         /* Show scan panel */
        [self performSelectorOnMainThread: @selector(showScanPanel:)
                               withObject: NULL waitUntilDone: NO];
        return;
               /* Go to HandBrake homepage and exit */
        [self openHomepage: NULL];
        [NSApp terminate: self];
    }
}

- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication *) app
{
    // Warn if encoding a movie
    hb_state_t s;
    hb_get_state( fHandle, &s );
    hb_job_t * job = hb_current_job( fHandle );
    if ( job && ( s.state != HB_STATE_IDLE ) )
    {
        hb_job_t * job = hb_current_job( fHandle );
        int result = NSRunCriticalAlertPanel(
                NSLocalizedString(@"Are you sure you want to quit HandBrake?", nil),
                NSLocalizedString(@"%@ is currently encoding. If you quit HandBrake, your movie will be lost. Do you want to quit anyway?", nil),
                NSLocalizedString(@"Quit", nil), NSLocalizedString(@"Don't Quit", nil), nil,
                job ? [NSString stringWithUTF8String:job->title->name] : @"A movie" );
        
        if (result == NSAlertDefaultReturn)
        {
            [self doCancelCurrentJob];
            return NSTerminateNow;
        }
        else
            return NSTerminateCancel;
    }
    
    // Warn if items still in the queue
    else if ( hb_count( fHandle ) > 0 )
    {
        int result = NSRunCriticalAlertPanel(
                NSLocalizedString(@"Are you sure you want to quit HandBrake?", nil),
                NSLocalizedString(@"One or more encodes are queued for encoding. Do you want to quit anyway?", nil),
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
	[outputPanel release];
	[fQueueController release];
	hb_close(&fHandle);
}


- (void) awakeFromNib
{
    [fWindow center];
    [fWindow setExcludedFromWindowsMenu:YES];
    [fAdvancedOptions setView:fAdvancedView];

    /* Initialize currentScanCount so HB can use it to
		evaluate successive scans */
	currentScanCount = 0;
	
    /* Init UserPresets .plist */
	[self loadPresets];
		
	fRipIndicatorShown = NO;  // initially out of view in the nib
	
	/* Show/Dont Show Presets drawer upon launch based
		on user preference DefaultPresetsDrawerShow*/
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultPresetsDrawerShow"] > 0)
	{
		[fPresetDrawer open];
	}
	
	
    /* Destination box*/
    [fDstFormatPopUp removeAllItems];
    [fDstFormatPopUp addItemWithTitle: _( @"MP4 file" )];
	[fDstFormatPopUp addItemWithTitle: _( @"MKV file" )];
    [fDstFormatPopUp addItemWithTitle: _( @"AVI file" )];
    [fDstFormatPopUp addItemWithTitle: _( @"OGM file" )];
    [fDstFormatPopUp selectItemAtIndex: 0];
	
    [self formatPopUpChanged: NULL];
    
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
    [fVidEncoderPopUp addItemWithTitle: @"XviD"];
	
    
	
    /* Video quality */
    [fVidTargetSizeField setIntValue: 700];
	[fVidBitrateField    setIntValue: 1000];
	
    [fVidQualityMatrix   selectCell: fVidBitrateCell];
    [self videoMatrixChanged: NULL];
	
    /* Video framerate */
    [fVidRatePopUp removeAllItems];
	[fVidRatePopUp addItemWithTitle: _( @"Same as source" )];
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
	
	/* Picture Settings */
	[fPicLabelPAROutputX setStringValue: @""];
	[fPicSettingPARWidth setStringValue: @""];
	[fPicSettingPARHeight setStringValue:  @""];
	
	/* Set Auto Crop to On at launch */
    [fPictureController setAutoCrop:YES];
	
	/* Audio bitrate */
    [fAudBitratePopUp removeAllItems];
    for( int i = 0; i < hb_audio_bitrates_count; i++ )
    {
        [fAudBitratePopUp addItemWithTitle:
				[NSString stringWithCString: hb_audio_bitrates[i].string]];

    }
    [fAudBitratePopUp selectItemAtIndex: hb_audio_bitrates_default];
	
    /* Audio samplerate */
    [fAudRatePopUp removeAllItems];
    for( int i = 0; i < hb_audio_rates_count; i++ )
    {
        [fAudRatePopUp addItemWithTitle:
            [NSString stringWithCString: hb_audio_rates[i].string]];
    }
    [fAudRatePopUp selectItemAtIndex: hb_audio_rates_default];
	
    /* Bottom */
    [fStatusField setStringValue: @""];
	
    [self enableUI: NO];
	[self setupToolbar];
	
	[fPresetsActionButton setMenu:fPresetsActionMenu];
	
	/* We disable the Turbo 1st pass checkbox since we are not x264 */
	[fVidTurboPassCheck setEnabled: NO];
	[fVidTurboPassCheck setState: NSOffState];
	
	
	/* lets get our default prefs here */
	[self getDefaultPresets: NULL];
	/* lets initialize the current successful scancount here to 0 */
	currentSuccessfulScanCount = 0;
    
}

- (void) TranslateStrings
{
    [fSrcTitleField     setStringValue: _( @"Title:" )];
    [fSrcChapterField   setStringValue: _( @"Chapters:" )];
    [fSrcChapterToField setStringValue: _( @"to" )];
    [fSrcDuration1Field setStringValue: _( @"Duration:" )];

    [fDstFormatField    setStringValue: _( @"Format:" )];
    [fDstCodecsField    setStringValue: _( @"Codecs:" )];
    [fDstFile1Field     setStringValue: _( @"File:" )];
    [fDstBrowseButton   setTitle:       _( @"Browse" )];

    [fVidRateField      setStringValue: _( @"Framerate (fps):" )];
    [fVidEncoderField   setStringValue: _( @"Encoder:" )];
    [fVidQualityField   setStringValue: _( @"Quality:" )];
}


- (void) enableUI: (bool) b
{
    NSControl * controls[] =
      { fSrcTitleField, fSrcTitlePopUp,
        fSrcChapterField, fSrcChapterStartPopUp, fSrcChapterToField,
        fSrcChapterEndPopUp, fSrcDuration1Field, fSrcDuration2Field,
        fDstFormatField, fDstFormatPopUp, fDstCodecsField,
        fDstCodecsPopUp, fDstFile1Field, fDstFile2Field,
        fDstBrowseButton, fVidRateField, fVidRatePopUp,
        fVidEncoderField, fVidEncoderPopUp, fVidQualityField,
        fVidQualityMatrix, fVidGrayscaleCheck, fSubField, fSubPopUp,
        fAudLang1Field, fAudLang1PopUp, fAudLang2Field, fAudLang2PopUp,
        fAudTrack1MixLabel, fAudTrack1MixPopUp, fAudTrack2MixLabel, fAudTrack2MixPopUp,
        fAudRateField, fAudRatePopUp, fAudBitrateField,
        fAudBitratePopUp, fPictureButton,fQueueStatus, 
		fPicSrcWidth,fPicSrcHeight,fPicSettingWidth,fPicSettingHeight,fPicSettingARkeep,
		fPicSettingDeinterlace,fPicLabelSettings,fPicLabelSrc,fPicLabelOutp,
		fPicLabelAr,fPicLabelDeinterlace,fPicLabelSrcX,fPicLabelOutputX,
		fPicLabelPAROutputX,fPicSettingPARWidth,fPicSettingPARHeight,
		fPicSettingPAR,fPicLabelAnamorphic,tableView,fPresetsAdd,fPresetsDelete,
		fCreateChapterMarkers,fVidTurboPassCheck,fDstMpgLargeFileCheck,fPicLabelAutoCrop,
		fPicSettingAutoCrop,fPicSettingDetelecine,fPicLabelDetelecine,fPicLabelDenoise,fPicSettingDenoise,
        fSubForcedCheck,fPicSettingDeblock,fPicLabelDeblock,};

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
        [self setEnabledStateOfAudioMixdownControls: NULL];
	
	} else {

		[tableView setEnabled: NO];
	
	}

    [self videoMatrixChanged: NULL];
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
    NSImage * icon;
    NSData * tiff;
    NSBitmapImageRep * bmp;
    uint32_t * pen;
    uint32_t black = htonl( 0x000000FF );
    uint32_t red   = htonl( 0xFF0000FF );
    uint32_t white = htonl( 0xFFFFFFFF );
    int row_start, row_end;
    int i, j;

    /* Get application original icon */
    icon = [NSImage imageNamed: @"NSApplicationIcon"];

    if( progress < 0.0 || progress > 1.0 )
    {
        [NSApp setApplicationIconImage: icon];
        return;
    }

    /* Get it in a raw bitmap form */
    tiff = [icon TIFFRepresentationUsingCompression:
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
    icon = [[NSImage alloc] initWithData: tiff];
    [NSApp setApplicationIconImage: icon];
    [icon release];
}

- (void) updateUI: (NSTimer *) timer
{

    hb_list_t  * list;
    list = hb_get_titles( fHandle );	
    /* check to see if there has been a new scan done
	this bypasses the constraints of HB_STATE_WORKING
	not allowing setting a newly scanned source */
	int checkScanCount = hb_get_scancount( fHandle );
	if (checkScanCount > currentScanCount)
	{
		
		currentScanCount = checkScanCount;
		[self showNewScan: NULL];
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
                                            _( @"Scanning title %d of %d..." ),
                                            p.title_cur, p.title_count]];
            float scanprogress_total;
            scanprogress_total = ( p.title_cur - 1 ) / p.title_count;
            /* FIX ME: currently having an issue showing progress on the scan
             * indicator ( fScanIndicator ), for now just set to barber pole (indeterminate) in -performScan
             * and stop indeterminate in -showNewScan .
             */
            //[fScanIndicator setHidden: NO];
            //[fScanIndicator setDoubleValue: 100.0 * scanprogress_total];
            break;
		}
#undef p
	
#define p s.param.scandone
        case HB_STATE_SCANDONE:
        {
			[self showNewScan: NULL];
            [toolbar validateVisibleItems];
			break;
        }
#undef p
			
#define p s.param.working
        case HB_STATE_WORKING:
        {
            float progress_total;
            NSMutableString * string;
			/* Currently, p.job_cur and p.job_count get screwed up when adding
				jobs during encoding, if they cannot be fixed in libhb, will implement a
				nasty but working cocoa solution */
			/* Update text field */
			string = [NSMutableString stringWithFormat: _( @"Encoding: task %d of %d, %.2f %%" ), p.job_cur, p.job_count, 100.0 * p.progress];
            
			if( p.seconds > -1 )
            {
                [string appendFormat:
                    _( @" (%.2f fps, avg %.2f fps, ETA %02dh%02dm%02ds)" ),
                    p.rate_cur, p.rate_avg, p.hours, p.minutes, p.seconds];
            }
            [fStatusField setStringValue: string];
			
            /* Update slider */
			progress_total = ( p.progress + p.job_cur - 1 ) / p.job_count;
            [fRipIndicator setIndeterminate: NO];
            [fRipIndicator setDoubleValue: 100.0 * progress_total];
			
            // If progress bar hasn't been revealed at the bottom of the window, do
            // that now. This code used to be in doRip. I moved it to here to handle
            // the case where hb_start is called by HBQueueController and not from
            // HBController.
            if (!fRipIndicatorShown)
            {
                NSRect frame = [fWindow frame];
                if (frame.size.width <= 591)
                    frame.size.width = 591;
                frame.size.height += 36;
                frame.origin.y -= 36;
                [fWindow setFrame:frame display:YES animate:YES];
                fRipIndicatorShown = YES;
                /* We check to see if we need to warn the user that the computer will go to sleep
                   or shut down when encoding is finished */
                [self remindUserOfSleepOrShutdown];
            }

            /* Update dock icon */
            [self UpdateDockIcon: progress_total];
			
            // Has current job changed? That means the queue has probably changed as
			// well so update it
            [fQueueController hblibStateChanged: s];
            
            break;
        }
#undef p
			
#define p s.param.muxing
        case HB_STATE_MUXING:
        {
            NSMutableString * string;
			
            /* Update text field */
            string = [NSMutableString stringWithFormat:
                _( @"Muxing..." )];
            [fStatusField setStringValue: string];
			
            /* Update slider */
            [fRipIndicator setIndeterminate: YES];
            [fRipIndicator startAnimation: nil];
			
            /* Update dock icon */
            [self UpdateDockIcon: 1.0];
			
			// Pass along the info to HBQueueController
            [fQueueController hblibStateChanged: s];
			
            break;
        }
#undef p
			
        case HB_STATE_PAUSED:
		    [fStatusField setStringValue: _( @"Paused" )];
            
			// Pass along the info to HBQueueController
            [fQueueController hblibStateChanged: s];

            break;
			
        case HB_STATE_WORKDONE:
        {
            // HB_STATE_WORKDONE happpens as a result of hblib finishing all its jobs
            // or someone calling hb_stop. In the latter case, hb_stop does not clear
            // out the remaining passes/jobs in the queue. We'll do that here.
                        
            // Delete all remaining scans of this job, ie, delete whole encodes.
            hb_job_t * job;
            while( ( job = hb_job( fHandle, 0 ) ) && (job->sequence_id != 0) )
                hb_rem( fHandle, job );

            // Start processing back up if jobs still left in queue
            if (hb_count(fHandle) > 0)
            {
                hb_start(fHandle);
                fEncodeState = 1;
                // Validate the toolbar (hack). The toolbar will usually get autovalidated
                // before we had the chance to restart the queue, hence it will now be in
                // the wrong state.
                [toolbar validateVisibleItems];
                break;
            }

            [fStatusField setStringValue: _( @"Done." )];
            [fRipIndicator setIndeterminate: NO];
            [fRipIndicator setDoubleValue: 0.0];
            [toolbar validateVisibleItems];

            /* Restore dock icon */
            [self UpdateDockIcon: -1.0];

            if (fRipIndicatorShown)
            {
                NSRect frame = [fWindow frame];
                if (frame.size.width <= 591)
				    frame.size.width = 591;
                frame.size.height += -36;
                frame.origin.y -= -36;
                [fWindow setFrame:frame display:YES animate:YES];
				fRipIndicatorShown = NO;
			}
			
			// Pass along the info to HBQueueController
            [fQueueController hblibStateChanged: s];
			
            /* Check to see if the encode state has not been cancelled
				to determine if we should check for encode done notifications */
			if (fEncodeState != 2) 			{
				/* If Growl Notification or Window and Growl has been selected */
				if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Growl Notification"] || 
					[[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window And Growl"])
                {
					/*Growl Notification*/
					[self showGrowlDoneNotification: NULL];
                }
                /* If Alert Window or Window and Growl has been selected */
				if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window"] || 
					[[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Alert Window And Growl"])
                {
					/*On Screen Notification*/
					int status;
					NSBeep();
					status = NSRunAlertPanel(@"Put down that cocktail...",@"Your HandBrake encode is done!", @"OK", nil, nil);
					[NSApp requestUserAttention:NSCriticalRequest];
					if ( status == NSAlertDefaultReturn ) 
					{
						[self enableUI: YES];
					}
                }
				else
				{
					[self enableUI: YES];
				}
			           /* If sleep has been selected */ 
            if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Put Computer To Sleep"]) 
                { 
               /* Sleep */ 
               NSDictionary* errorDict; 
               NSAppleEventDescriptor* returnDescriptor = NULL; 
               NSAppleScript* scriptObject = [[NSAppleScript alloc] initWithSource: 
                        @"tell application \"Finder\" to sleep"]; 
               returnDescriptor = [scriptObject executeAndReturnError: &errorDict]; 
               [scriptObject release]; 
               [self enableUI: YES]; 
                } 
            /* If Shutdown has been selected */ 
            if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"AlertWhenDone"] isEqualToString: @"Shut Down Computer"]) 
                { 
               /* Shut Down */ 
               NSDictionary* errorDict; 
               NSAppleEventDescriptor* returnDescriptor = NULL; 
               NSAppleScript* scriptObject = [[NSAppleScript alloc] initWithSource: 
                        @"tell application \"Finder\" to shut down"]; 
               returnDescriptor = [scriptObject executeAndReturnError: &errorDict]; 
               [scriptObject release]; 
               [self enableUI: YES]; 
                }
			
						// MetaX insertion via AppleScript
			if([[NSUserDefaults standardUserDefaults] boolForKey: @"sendToMetaX"] == YES)
			{
			NSAppleScript *myScript = [[NSAppleScript alloc] initWithSource: [NSString stringWithFormat: @"%@%@%@", @"tell application \"MetaX\" to open (POSIX file \"", [fDstFile2Field stringValue], @"\")"]];
			[myScript executeAndReturnError: nil];
			[myScript release];
			}
			
			
			}
			else
			{
				[self enableUI: YES];
			}
            break;
        }
    }
	
    /* Lets show the queue status here in the main window */
	int queue_count = hb_count( fHandle );
	if( queue_count )
	{
		[fQueueStatus setStringValue: [NSString stringWithFormat:
			@"%d pass%s in the queue",
						 queue_count, ( queue_count > 1 ) ? "es" : ""]];
	}
	else
	{
		[fQueueStatus setStringValue: @""];
	}
}


#pragma mark -
#pragma mark Toolbar
// ============================================================
// NSToolbar Related Methods
// ============================================================

- (void) setupToolbar {
    toolbar = [[[NSToolbar alloc] initWithIdentifier: @"HandBrake Toolbar"] autorelease];
    
    [toolbar setAllowsUserCustomization: YES];
    [toolbar setAutosavesConfiguration: YES];
    [toolbar setDisplayMode: NSToolbarDisplayModeIconAndLabel];
    
    [toolbar setDelegate: self];
    
    [fWindow setToolbar: toolbar];
}

- (NSToolbarItem *) toolbar: (NSToolbar *)toolbar itemForItemIdentifier:
    (NSString *) itemIdent willBeInsertedIntoToolbar:(BOOL) willBeInserted {
    NSToolbarItem * item = [[NSToolbarItem alloc] initWithItemIdentifier: itemIdent];
    
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
    else if ([itemIdent isEqualToString: ShowActivityIdentifier]) {
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
        [item release];
        return nil;
    }

    return item;
}

- (NSArray *) toolbarDefaultItemIdentifiers: (NSToolbar *) toolbar
{
    return [NSArray arrayWithObjects: ChooseSourceIdentifier, NSToolbarSeparatorItemIdentifier, StartEncodingIdentifier,
        PauseEncodingIdentifier, AddToQueueIdentifier, ShowQueueIdentifier, NSToolbarFlexibleSpaceItemIdentifier, 
		NSToolbarSpaceItemIdentifier, ShowActivityIdentifier, ToggleDrawerIdentifier, nil];
}

- (NSArray *) toolbarAllowedItemIdentifiers: (NSToolbar *) toolbar
{
    return [NSArray arrayWithObjects:  StartEncodingIdentifier, PauseEncodingIdentifier, AddToQueueIdentifier,
        ChooseSourceIdentifier, ShowQueueIdentifier, ShowActivityIdentifier, ToggleDrawerIdentifier,
        NSToolbarCustomizeToolbarItemIdentifier, NSToolbarFlexibleSpaceItemIdentifier,
        NSToolbarSpaceItemIdentifier, NSToolbarSeparatorItemIdentifier, nil];
}

- (BOOL) validateToolbarItem: (NSToolbarItem *) toolbarItem
{
    NSString * ident = [toolbarItem itemIdentifier];
        
    if (fHandle)
    {
        hb_state_t s;
        hb_get_state2( fHandle, &s );
        
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
                if ([ident isEqualToString: AddToQueueIdentifier])
                    return YES;
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
        }

    }
    
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
            return [tableView selectedRow] >= 0 && [fWindow attachedSheet] == nil;
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
    
    return YES;
}

#pragma mark -
#pragma mark Growl
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

-(IBAction)showGrowlDoneNotification:(id)sender
{
  [GrowlApplicationBridge 
            notifyWithTitle:@"Put down that cocktail..." 
                description:@"your HandBrake encode is done!" 
           notificationName:SERVICE_NAME
                   iconData:nil 
                   priority:0 
                   isSticky:1 
               clickContext:nil];
}

#pragma mark -
#pragma mark Get New Source

/*Opens the source browse window, called from Open Source widgets */
- (IBAction) browseSources: (id) sender
{
    [self enableUI: NO];
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
            [fScanSrcTitlePathField setStringValue: [NSString stringWithFormat:@"%@", scanPath]];
            NSString *displayTitlescanSourceName;
            
            if ([[scanPath lastPathComponent] isEqualToString: @"VIDEO_TS"])
            {
                /* If VIDEO_TS Folder is chosen, choose its parent folder for the source display name 
                we have to use the title->dvd value so we get the proper name of the volume if a physical dvd is the source*/
                displayTitlescanSourceName = [NSString stringWithFormat:[[scanPath stringByDeletingLastPathComponent] lastPathComponent]];
            }
            else
            {
                /* if not the VIDEO_TS Folder, we can assume the chosen folder is the source name */
                displayTitlescanSourceName = [NSString stringWithFormat:[scanPath lastPathComponent]];
            }
            /* we set the source display name in the title selection dialogue */
            [fSrcDsplyNameTitleScan setStringValue: [NSString stringWithFormat:@"%@", displayTitlescanSourceName]];
            /* We show the actual sheet where the user specifies the title to be scanned 
                * as we are going to do a title specific scan
                */
            [self showSourceTitleScanPanel:NULL];
        }
        else
        {
            /* We are just doing a standard full source scan, so we specify "0" to libhb */
            NSString *path = [[sheet filenames] objectAtIndex: 0];
            [self performScan:path scanTitleNum:0];   
        }
        
    }
    else // User clicked Cancel in browse window
    {
        /* if we have a title loaded up */
        if ([[fSrcDVD2Field stringValue] length] > 0)
        {
            [self enableUI: YES];
        }
    }
}

/* Here we open the title selection sheet where we can specify an exact title to be scanned */
- (IBAction) showSourceTitleScanPanel: (id) sender
{
    /* We default the title number to be scanned to "0" which results in a full source scan, unless the
    * user changes it
    */
    [fScanSrcTitleNumField setStringValue: @"0"];
	/* Show the panel */
	[NSApp beginSheet: fScanSrcTitlePanel modalForWindow: fWindow modalDelegate: NULL didEndSelector: NULL contextInfo: NULL];
}

- (IBAction) closeSourceTitleScanPanel: (id) sender
{
    [NSApp endSheet: fScanSrcTitlePanel];
    [fScanSrcTitlePanel orderOut: self];
    
    
    
    if(sender == fScanSrcTitleOpenButton)
    {
        /* We setup the scan status in the main window to indicate a source title scan */
        [fSrcDVD2Field setStringValue: _( @"Opening a new source title ..." )];
		//[fScanIndicator setHidden: NO];
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
    NSString *path = scanPath;
    HBDVDDetector *detector = [HBDVDDetector detectorForPath:path];
    if( [detector isVideoDVD] )
    {
        // The chosen path was actually on a DVD, so use the raw block
        // device path instead.
        path = [detector devicePath];
    }
    /* If there is no title number passed to scan, we use "0"
        * which causes the default behavior of a full source scan
    */
    if (!scanTitleNum)
    {
        scanTitleNum = 0;
    }
    
    [fSrcDVD2Field setStringValue: [NSString stringWithFormat: @"Scanning new source ..."]];
    /* due to issue with progress in the scan indicator, we are starting the
    indeterminate animation here */
    [fScanIndicator startAnimation: self];
    /* we actually pass the scan off to libhb here */
    hb_scan( fHandle, [path UTF8String], scanTitleNum );
    
}

- (IBAction) showNewScan:(id)sender
{
	/* due to issue with progress in the scan indicator, we are stopping the
    indeterminate animation here */
    [fScanIndicator stopAnimation: self];
    
    hb_list_t  * list;
	hb_title_t * title;
	int indxpri=0; 	  // Used to search the longuest title (default in combobox)
	int longuestpri=0; // Used to search the longuest title (default in combobox)
	
	list = hb_get_titles( fHandle );
	
	if( !hb_list_count( list ) )
	{
		/* We display a message if a valid dvd source was not chosen */
		[fSrcDVD2Field setStringValue: @"No Valid Title Found"];
        SuccessfulScan = NO;
	}
	else
	{
        /* We increment the successful scancount here by one,
        which we use at the end of this function to tell the gui
        if this is the first successful scan since launch and whether
        or not we should set all settings to the defaults */
		
        currentSuccessfulScanCount++;
        
        [toolbar validateVisibleItems];
		
		[fSrcTitlePopUp removeAllItems];
		for( int i = 0; i < hb_list_count( list ); i++ )
		{
			title = (hb_title_t *) hb_list_item( list, i );
			
            currentSource = [NSString stringWithUTF8String: title->name];
            
            /* To get the source name as well as the default output name, first we check to see if
                the selected directory is the VIDEO_TS Directory */
            if ([[currentSource lastPathComponent] isEqualToString: @"VIDEO_TS"])
            {
                /* If VIDEO_TS Folder is chosen, choose its parent folder for the source display name 
                we have to use the title->dvd value so we get the proper name of the volume if a physical dvd is the source*/
                sourceDisplayName = [NSString stringWithFormat:[[[NSString stringWithUTF8String: title->dvd] stringByDeletingLastPathComponent] lastPathComponent]];
            }
            else
            {
                /* if not the VIDEO_TS Folder, we can assume the chosen folder is the source name */
                sourceDisplayName = [NSString stringWithFormat:[currentSource lastPathComponent]];
            }
			/*Set DVD Name at top of window*/
			[fSrcDVD2Field setStringValue:[NSString stringWithFormat: @"%@", sourceDisplayName]];
			
			/* Use the dvd name in the default output field here 
				May want to add code to remove blank spaces for some dvd names*/
			/* Check to see if the last destination has been set,use if so, if not, use Desktop */
			if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastDestinationDirectory"])
			{
				[fDstFile2Field setStringValue: [NSString stringWithFormat:
					@"%@/%@.mp4", [[NSUserDefaults standardUserDefaults] stringForKey:@"LastDestinationDirectory"],sourceDisplayName]];
			}
			else
			{
				[fDstFile2Field setStringValue: [NSString stringWithFormat:
					@"%@/Desktop/%@.mp4", NSHomeDirectory(),sourceDisplayName]];
			}
			
			if (longuestpri < title->hours*60*60 + title->minutes *60 + title->seconds)
			{
				longuestpri=title->hours*60*60 + title->minutes *60 + title->seconds;
				indxpri=i;
			}
			
			[self formatPopUpChanged:NULL];
			
            [fSrcTitlePopUp addItemWithTitle: [NSString
                stringWithFormat: @"%d - %02dh%02dm%02ds",
                title->index, title->hours, title->minutes,
                title->seconds]];
		}
        
		// Select the longuest title
		[fSrcTitlePopUp selectItemAtIndex: indxpri];
		[self titlePopUpChanged: NULL];
		
        SuccessfulScan = YES;
		[self enableUI: YES];
		
		/* if its the initial successful scan after awakeFromNib */
        if (currentSuccessfulScanCount == 1)
        {
            [self selectDefaultPreset: NULL];
            /* if Deinterlace upon launch is specified in the prefs, then set to 1 for "Fast",
            if not, then set to 0 for none */
            if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultDeinterlaceOn"] > 0)
                [fPictureController setDeinterlace:1];
            else
                [fPictureController setDeinterlace:0];
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

#pragma mark -
#pragma mark Job Handling


- (void) prepareJob
{
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;
    //int i;

    /* Chapter selection */
    job->chapter_start = [fSrcChapterStartPopUp indexOfSelectedItem] + 1;
    job->chapter_end   = [fSrcChapterEndPopUp   indexOfSelectedItem] + 1;
	
    /* Format and codecs */
    int format = [fDstFormatPopUp indexOfSelectedItem];
    int codecs = [fDstCodecsPopUp indexOfSelectedItem];
    job->mux    = FormatSettings[format][codecs] & HB_MUX_MASK;
    job->vcodec = FormatSettings[format][codecs] & HB_VCODEC_MASK;
    job->acodec = FormatSettings[format][codecs] & HB_ACODEC_MASK;
    /* If mpeg-4, then set mpeg-4 specific options like chapters and > 4gb file sizes */
	if ([fDstFormatPopUp indexOfSelectedItem] == 0)
	{
        /* We set the largeFileSize (64 bit formatting) variable here to allow for > 4gb files based on the format being
		mpeg4 and the checkbox being checked 
		*Note: this will break compatibility with some target devices like iPod, etc.!!!!*/
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"AllowLargeFiles"] > 0 && [fDstMpgLargeFileCheck state] == NSOnState)
		{
			job->largeFileSize = 1;
		}
		else
		{
			job->largeFileSize = 0;
		}
	}
	if ([fDstFormatPopUp indexOfSelectedItem] == 0 || [fDstFormatPopUp indexOfSelectedItem] == 3)
	{
	  /* We set the chapter marker extraction here based on the format being
		mpeg4 or mkv and the checkbox being checked */
		if ([fCreateChapterMarkers state] == NSOnState)
		{
			job->chapter_markers = 1;
		}
		else
		{
			job->chapter_markers = 0;
		}
	}
	if( ( job->vcodec & HB_VCODEC_FFMPEG ) &&
        [fVidEncoderPopUp indexOfSelectedItem] > 0 )
    {
        job->vcodec = HB_VCODEC_XVID;
    }
    if( job->vcodec & HB_VCODEC_X264 )
    {
		if ([fVidEncoderPopUp indexOfSelectedItem] > 0 )
	    {
			/* Just use new Baseline Level 3.0 
			Lets Deprecate Baseline Level 1.3h264_level*/
			job->h264_level = 30;
			job->mux = HB_MUX_IPOD;
			/* move sanity check for iPod Encoding here */
			job->pixel_ratio = 0 ;
			
		}
		
		/* Set this flag to switch from Constant Quantizer(default) to Constant Rate Factor Thanks jbrjake
		Currently only used with Constant Quality setting*/
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultCrf"] > 0 && [fVidQualityMatrix selectedRow] == 2)
		{
	        job->crf = 1;
		}
		
		/* Below Sends x264 options to the core library if x264 is selected*/
		/* Lets use this as per Nyx, Thanks Nyx!*/
		job->x264opts = (char *)calloc(1024, 1); /* Fixme, this just leaks */
		/* Turbo first pass if two pass and Turbo First pass is selected */
		if( [fVidTwoPassCheck state] == NSOnState && [fVidTurboPassCheck state] == NSOnState )
		{
			/* pass the "Turbo" string to be appended to the existing x264 opts string into a variable for the first pass */
			NSString *firstPassOptStringTurbo = @":ref=1:subme=1:me=dia:analyse=none:trellis=0:no-fast-pskip=0:8x8dct=0";
			/* append the "Turbo" string variable to the existing opts string.
			Note: the "Turbo" string must be appended, not prepended to work properly*/
			NSString *firstPassOptStringCombined = [[fAdvancedOptions optionsString] stringByAppendingString:firstPassOptStringTurbo];
			strcpy(job->x264opts, [firstPassOptStringCombined UTF8String]);
		}
		else
		{
			strcpy(job->x264opts, [[fAdvancedOptions optionsString] UTF8String]);
		}
		
        job->h264_13 = [fVidEncoderPopUp indexOfSelectedItem];
    }

    /* Video settings */
    if( [fVidRatePopUp indexOfSelectedItem] > 0 )
    {
        job->vrate      = 27000000;
        job->vrate_base = hb_video_rates[[fVidRatePopUp
            indexOfSelectedItem]-1].rate;
    }
    else
    {
        job->vrate      = title->rate;
        job->vrate_base = title->rate_base;
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
            job->vquality = [fVidQualitySlider floatValue];
            job->vbitrate = 0;
            break;
    }

    job->grayscale = ( [fVidGrayscaleCheck state] == NSOnState );

    /* Subtitle settings */
    job->subtitle = [fSubPopUp indexOfSelectedItem] - 2;

    /* Audio tracks and mixdowns */
    /* check for the condition where track 2 has an audio selected, but track 1 does not */
    /* we will use track 2 as track 1 in this scenario */
    if ([fAudLang1PopUp indexOfSelectedItem] > 0)
    {
        job->audios[0] = [fAudLang1PopUp indexOfSelectedItem] - 1;
        job->audios[1] = [fAudLang2PopUp indexOfSelectedItem] - 1; /* will be -1 if "none" is selected */
        job->audios[2] = -1;
        job->audio_mixdowns[0] = [[fAudTrack1MixPopUp selectedItem] tag];
        job->audio_mixdowns[1] = [[fAudTrack2MixPopUp selectedItem] tag];
    }
    else if ([fAudLang2PopUp indexOfSelectedItem] > 0)
    {
        job->audios[0] = [fAudLang2PopUp indexOfSelectedItem] - 1;
        job->audio_mixdowns[0] = [[fAudTrack2MixPopUp selectedItem] tag];
        job->audios[1] = -1;
    }
    else
    {
        job->audios[0] = -1;
    }

    /* Audio settings */
    job->arate = hb_audio_rates[[fAudRatePopUp
                     indexOfSelectedItem]].rate;
    job->abitrate = [[fAudBitratePopUp selectedItem] tag];
    
    /* set vfr according to the Picture Window */
    if ([fPictureController vfr])
    {
    job->vfr = 1;
    }
    else
    {
    job->vfr = 0;
    }
    
    /* Filters */ 
    job->filters = hb_list_init();
   
	/* Detelecine */
    if ([fPictureController detelecine])
    {
        hb_list_add( job->filters, &hb_filter_detelecine );
    }
   
    /* Deinterlace */
    if ([fPictureController deinterlace] == 1)
    {
        /* Run old deinterlacer by default */
        hb_filter_deinterlace.settings = "-1"; 
        hb_list_add( job->filters, &hb_filter_deinterlace );
    }
    else if ([fPictureController deinterlace] == 2)
    {
        /* Yadif mode 0 (1-pass with spatial deinterlacing.) */
        hb_filter_deinterlace.settings = "0"; 
        hb_list_add( job->filters, &hb_filter_deinterlace );            
    }
    else if ([fPictureController deinterlace] == 3)
    {
        /* Yadif (1-pass w/o spatial deinterlacing) and Mcdeint */
        hb_filter_deinterlace.settings = "2:-1:1"; 
        hb_list_add( job->filters, &hb_filter_deinterlace );            
    }
    else if ([fPictureController deinterlace] == 4)
    {
        /* Yadif (2-pass w/ spatial deinterlacing) and Mcdeint*/
        hb_filter_deinterlace.settings = "1:-1:1"; 
        hb_list_add( job->filters, &hb_filter_deinterlace );            
    }
	
	/* Denoise */
	
	if ([fPictureController denoise] == 1) // Weak in popup
	{
		hb_filter_denoise.settings = "2:1:2:3"; 
        hb_list_add( job->filters, &hb_filter_denoise );	
	}
	else if ([fPictureController denoise] == 2) // Medium in popup
	{
		hb_filter_denoise.settings = "3:2:2:3"; 
        hb_list_add( job->filters, &hb_filter_denoise );	
	}
	else if ([fPictureController denoise] == 3) // Strong in popup
	{
		hb_filter_denoise.settings = "7:7:5:5"; 
        hb_list_add( job->filters, &hb_filter_denoise );	
	}
    
    /* Deblock  (uses pp7 default) */
    if ([fPictureController deblock])
    {
        hb_list_add( job->filters, &hb_filter_deblock );
    }

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

    /* We check for duplicate name here */
	if( [[NSFileManager defaultManager] fileExistsAtPath:
            [fDstFile2Field stringValue]] )
    {
        NSBeginCriticalAlertSheet( _( @"File already exists" ),
            _( @"Cancel" ), _( @"Overwrite" ), NULL, fWindow, self,
            @selector( overwriteAddToQueueAlertDone:returnCode:contextInfo: ),
            NULL, NULL, [NSString stringWithFormat:
            _( @"Do you want to overwrite %@?" ),
            [fDstFile2Field stringValue]] );
        // overwriteAddToQueueAlertDone: will be called when the alert is dismissed.
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
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list, [fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;

    // Assign a sequence number, starting at zero, to each job added so they can
    // be lumped together in the UI.
    job->sequence_id = -1;

    [self prepareJob];

    /* Destination file */
    job->file = [[fDstFile2Field stringValue] UTF8String];

    if( [fSubForcedCheck state] == NSOnState )
        job->subtitle_force = 1;
    else
        job->subtitle_force = 0;

    /*
    * subtitle of -1 is a scan
    */
    if( job->subtitle == -1 )
    {
        char *x264opts_tmp;

        /*
        * When subtitle scan is enabled do a fast pre-scan job
        * which will determine which subtitles to enable, if any.
        */
        job->pass = -1;
        x264opts_tmp = job->x264opts;
        job->subtitle = -1;

        job->x264opts = NULL;

        job->indepth_scan = 1;  

        job->select_subtitle = (hb_subtitle_t**)malloc(sizeof(hb_subtitle_t*));
        *(job->select_subtitle) = NULL;

        /*
        * Add the pre-scan job
        */
        job->sequence_id++; // for job grouping
        hb_add( fHandle, job );

        job->x264opts = x264opts_tmp;
    }
    else
        job->select_subtitle = NULL;

    /* No subtitle were selected, so reset the subtitle to -1 (which before
    * this point meant we were scanning
    */
    if( job->subtitle == -2 )
        job->subtitle = -1;

    if( [fVidTwoPassCheck state] == NSOnState )
    {
        hb_subtitle_t **subtitle_tmp = job->select_subtitle;
        job->indepth_scan = 0;

        /*
         * Do not autoselect subtitles on the first pass of a two pass
         */
        job->select_subtitle = NULL;
        
        job->pass = 1;
        job->sequence_id++; // for job grouping
        hb_add( fHandle, job );

        job->pass = 2;
        job->sequence_id++; // for job grouping

        job->x264opts = (char *)calloc(1024, 1); /* Fixme, this just leaks */  
        strcpy(job->x264opts, [[fAdvancedOptions optionsString] UTF8String]);

        job->select_subtitle = subtitle_tmp;

        hb_add( fHandle, job );
    }
    else
    {
        job->indepth_scan = 0;
        job->pass = 0;
        job->sequence_id++; // for job grouping
        hb_add( fHandle, job );
    }
	
    NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
	[[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];
	
    // Notify the queue
	[fQueueController hblibJobListChanged];
}

/* Rip: puts up an alert before ultimately calling doRip
*/
- (IBAction) Rip: (id) sender
{
    /* Rip or Cancel ? */
    hb_state_t s;
    hb_get_state2( fHandle, &s );

    if(s.state == HB_STATE_WORKING || s.state == HB_STATE_PAUSED)
	{
        [self Cancel: sender];
        return;
    }
    
    // If there are jobs in the queue, then this is a rip the queue
    
    if (hb_count( fHandle ) > 0)
    {
        [self doRip];
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
        NSBeginCriticalAlertSheet( _( @"File already exists" ),
            _( @"Cancel" ), _( @"Overwrite" ), NULL, fWindow, self,
            @selector( overWriteAlertDone:returnCode:contextInfo: ),
            NULL, NULL, [NSString stringWithFormat:
            _( @"Do you want to overwrite %@?" ),
            [fDstFile2Field stringValue]] );
            
        // overWriteAlertDone: will be called when the alert is dismissed. It will call doRip.
    }
    else
    {
        /* if there are no jobs in the queue, then add this one to the queue and rip 
        otherwise, just rip the queue */
        if( hb_count( fHandle ) == 0)
        {
            [self doAddToQueue];
        }

        NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];
        [self doRip];
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
        if( hb_count( fHandle ) == 0 )
        {
            [self doAddToQueue];
        }

        NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
        [[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];
        [self doRip];
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
                       [self showPreferencesWindow:NULL];
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
                       [self showPreferencesWindow:NULL];
               }
       }

}


- (void) doRip
{
    /* Let libhb do the job */
    hb_start( fHandle );
	/*set the fEncodeState State */
	fEncodeState = 1;
}




//------------------------------------------------------------------------------------
// Removes all jobs from the queue. Does not cancel the current processing job.
//------------------------------------------------------------------------------------
- (void) doDeleteQueuedJobs
{
    hb_job_t * job;
    while( ( job = hb_job( fHandle, 0 ) ) )
        hb_rem( fHandle, job );
}

//------------------------------------------------------------------------------------
// Cancels the current job and proceeds with the next one in the queue.
//------------------------------------------------------------------------------------
- (void) doCancelCurrentJob
{
    // Stop the current job. hb_stop will only cancel the current pass and then set
    // its state to HB_STATE_WORKDONE. It also does this asynchronously. So when we
    // see the state has changed to HB_STATE_WORKDONE (in updateUI), we'll delete the
    // remaining passes of the job and then start the queue back up if there are any
    // remaining jobs.
     
    [fQueueController hblibWillStop];
    hb_stop( fHandle );
    fEncodeState = 2;   // don't alert at end of processing since this was a cancel
    
}

//------------------------------------------------------------------------------------
// Displays an alert asking user if the want to cancel encoding of current job.
// Cancel: returns immediately after posting the alert. Later, when the user
// acknowledges the alert, doCancelCurrentJob is called.
//------------------------------------------------------------------------------------
- (IBAction)Cancel: (id)sender
{
    if (!fHandle) return;
    
    hb_job_t * job = hb_current_job(fHandle);
    if (!job) return;

    NSString * alertTitle = [NSString stringWithFormat:NSLocalizedString(@"Do you want to stop encoding of %@?", nil),
            [NSString stringWithUTF8String:job->title->name]];
    
    // Which window to attach the sheet to?
    NSWindow * docWindow;
    if ([sender respondsToSelector: @selector(window)])
        docWindow = [sender window];
    else
        docWindow = fWindow;
        
    NSBeginCriticalAlertSheet(
            alertTitle,
            NSLocalizedString(@"Keep Encoding", nil),
            NSLocalizedString(@"Delete All", nil),
            NSLocalizedString(@"Stop Encoding", nil),
            docWindow, self,
            nil, @selector(didDimissCancelCurrentJob:returnCode:contextInfo:), nil,
            NSLocalizedString(@"Your movie will be lost if you don't continue encoding.", nil),
            [NSString stringWithUTF8String:job->title->name]);
    
    // didDimissCancelCurrentJob:returnCode:contextInfo: will be called when the dialog is dismissed

    // N.B.: didDimissCancelCurrentJob:returnCode:contextInfo: is designated as the dismiss
    // selector to prevent a crash. As a dismiss selector, the alert window will
    // have already be dismissed. If we don't do it this way, the dismissing of
    // the alert window will cause the table view to be redrawn at a point where
    // current job has been deleted by hblib but we don't know about it yet. This
    // is a prime example of wy we need to NOT be relying on hb_current_job!!!!
}

- (void) didDimissCancelCurrentJob: (NSWindow *)sheet returnCode: (int)returnCode contextInfo: (void *)contextInfo
{
    if (returnCode == NSAlertOtherReturn)
        [self doCancelCurrentJob];
    else if (returnCode == NSAlertAlternateReturn)
    {
        [self doDeleteQueuedJobs];
        [self doCancelCurrentJob];
    }
}





- (IBAction) Pause: (id) sender
{
    hb_state_t s;
    hb_get_state2( fHandle, &s );

    if( s.state == HB_STATE_PAUSED )
    {
        hb_resume( fHandle );
    }
    else
    {
        hb_pause( fHandle );
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
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultAutoNaming"] > 0)
	{
		[fDstFile2Field setStringValue: [NSString stringWithFormat:
			@"%@/%@-%d.%@", [[fDstFile2Field stringValue] stringByDeletingLastPathComponent],
			[NSString stringWithUTF8String: title->name],
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
    [self chapterPopUpChanged: NULL];

/* Start Get and set the initial pic size for display */
	hb_job_t * job = title->job;
	fTitle = title; 
	/* Turn Deinterlace on/off depending on the preference */
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultDeinterlaceOn"] > 0)
	{
		[fPictureController setDeinterlace:1];
	}
	else
	{
		[fPictureController setDeinterlace:0];
	}
	
	/* Pixel Ratio Setting */
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"PixelRatio"])
    {
		job->pixel_ratio = 1 ;
	}
	else
	{
		job->pixel_ratio = 0 ;
	}
	/*Set Source Size Fields Here */
	[fPicSrcWidth setStringValue: [NSString stringWithFormat:
							 @"%d", fTitle->width]];
	[fPicSrcHeight setStringValue: [NSString stringWithFormat:
							 @"%d", fTitle->height]];
							 
	/* Set Auto Crop to on upon selecting a new title */
    [fPictureController setAutoCrop:YES];
    
	/* We get the originial output picture width and height and put them
	in variables for use with some presets later on */
	PicOrigOutputWidth = job->width;
	PicOrigOutputHeight = job->height;
	AutoCropTop = job->crop[0];
	AutoCropBottom = job->crop[1];
	AutoCropLeft = job->crop[2];
	AutoCropRight = job->crop[3];
	/* we test getting the max output value for pic sizing here to be used later*/
	[fPicSettingWidth setStringValue: [NSString stringWithFormat:
		@"%d", PicOrigOutputWidth]];
	[fPicSettingHeight setStringValue: [NSString stringWithFormat:
		@"%d", PicOrigOutputHeight]];
	/* we run the picture size values through
	calculatePictureSizing to get all picture size
	information*/
	[self calculatePictureSizing: NULL];
	/* Run Through encoderPopUpChanged to see if there
		needs to be any pic value modifications based on encoder settings */
	//[self encoderPopUpChanged: NULL];
	/* END Get and set the initial pic size for display */ 

    /* Update subtitle popups */
    hb_subtitle_t * subtitle;
    [fSubPopUp removeAllItems];
    [fSubPopUp addItemWithTitle: @"None"];
    [fSubPopUp addItemWithTitle: @"Autoselect"];
    for( int i = 0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        subtitle = (hb_subtitle_t *) hb_list_item( title->list_subtitle, i );

        /* We cannot use NSPopUpButton's addItemWithTitle because
           it checks for duplicate entries */
        [[fSubPopUp menu] addItemWithTitle: [NSString stringWithCString:
            subtitle->lang] action: NULL keyEquivalent: @""];
    }
    [fSubPopUp selectItemAtIndex: 0];
	
	[self subtitleSelectionChanged: NULL];
    
    /* Update chapter table */
    [fChapterTitlesDelegate resetWithTitle:title];
    [fChapterTable reloadData];

    /* Update audio popups */
    [self addAllAudioTracksToPopUp: fAudLang1PopUp];
    [self addAllAudioTracksToPopUp: fAudLang2PopUp];
    /* search for the first instance of our prefs default language for track 1, and set track 2 to "none" */
	NSString * audioSearchPrefix = [[NSUserDefaults standardUserDefaults] stringForKey:@"DefaultLanguage"];
        [self selectAudioTrackInPopUp: fAudLang1PopUp searchPrefixString: audioSearchPrefix selectIndexIfNotFound: 1];
    [self selectAudioTrackInPopUp: fAudLang2PopUp searchPrefixString: NULL selectIndexIfNotFound: 0];
	
	/* changing the title may have changed the audio channels on offer, */
	/* so call audioTrackPopUpChanged for both audio tracks to update the mixdown popups */
	[self audioTrackPopUpChanged: fAudLang1PopUp];
	[self audioTrackPopUpChanged: fAudLang2PopUp];
    
    /* We repopulate the Video Framerate popup and show the detected framerate along with "Same as Source"*/
    [fVidRatePopUp removeAllItems];
    if (fTitle->rate_base == 1126125) // 23.976 NTSC Film
    {
        [fVidRatePopUp addItemWithTitle: @"Same as source (23.976)"];
    }
    else if (fTitle->rate_base == 1080000) // 25 PAL Film/Video
    {
        [fVidRatePopUp addItemWithTitle: @"Same as source (25)"];
    }
    else if (fTitle->rate_base == 900900) // 29.97 NTSC Video
    {
        [fVidRatePopUp addItemWithTitle: @"Same as source (29.97)"];
    }
    else
    {
        /* if none of the common dvd source framerates is detected, just use "Same as source" */
        [fVidRatePopUp addItemWithTitle: @"Same as source"];
    }
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
    
   /* lets call tableViewSelected to make sure that any preset we have selected is enforced after a title change */
	[self tableViewSelected:NULL]; 
	
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
}

- (IBAction) formatPopUpChanged: (id) sender
{
    NSString * string = [fDstFile2Field stringValue];
    NSString * selectedCodecs = [fDstCodecsPopUp titleOfSelectedItem];
    int format = [fDstFormatPopUp indexOfSelectedItem];
    char * ext = NULL;
	/* Initially set the large file (64 bit formatting) output checkbox to hidden */
    [fDstMpgLargeFileCheck setHidden: YES];
    /* Update the codecs popup */
    [fDstCodecsPopUp removeAllItems];
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
            
            [fDstCodecsPopUp addItemWithTitle:_( @"MPEG-4 Video / AAC Audio" )];
            [fDstCodecsPopUp addItemWithTitle:_( @"AVC/H.264 Video / AAC Audio" )];
            
			/* We enable the create chapters checkbox here since we are .mp4*/
			[fCreateChapterMarkers setEnabled: YES];
			/* We show the Large File (64 bit formatting) checkbox since we are .mp4 
			if we have enabled the option in the global preferences*/
			if ([[NSUserDefaults standardUserDefaults] boolForKey:@"AllowLargeFiles"] > 0)
			{
				[fDstMpgLargeFileCheck setHidden: NO];
			}
				else
				{
					/* if not enable in global preferences, we additionaly sanity check that the
					hidden checkbox is set to off. */
                    [fDstMpgLargeFileCheck setState: NSOffState];
				}
            break;
            
        case 1:
            ext = "mkv";
            [fDstCodecsPopUp addItemWithTitle:_( @"MPEG-4 Video / AAC Audio" )];
            [fDstCodecsPopUp addItemWithTitle:_( @"MPEG-4 Video / AC-3 Audio" )];
			[fDstCodecsPopUp addItemWithTitle:_( @"MPEG-4 Video / MP3 Audio" )];
			[fDstCodecsPopUp addItemWithTitle:_( @"MPEG-4 Video / Vorbis Audio" )];
            
			[fDstCodecsPopUp addItemWithTitle:_( @"AVC/H.264 Video / AAC Audio" )];
			[fDstCodecsPopUp addItemWithTitle:_( @"AVC/H.264 Video / AC-3 Audio" )];
			[fDstCodecsPopUp addItemWithTitle:_( @"AVC/H.264 Video / MP3 Audio" )];
			[fDstCodecsPopUp addItemWithTitle:_( @"AVC/H.264 Video / Vorbis Audio" )];
            /* We enable the create chapters checkbox here */
			[fCreateChapterMarkers setEnabled: YES];
			break;
            
        case 2: 
            ext = "avi";
            [fDstCodecsPopUp addItemWithTitle:_( @"MPEG-4 Video / MP3 Audio" )];
            [fDstCodecsPopUp addItemWithTitle:_( @"MPEG-4 Video / AC-3 Audio" )];
            [fDstCodecsPopUp addItemWithTitle:_( @"AVC/H.264 Video / MP3 Audio" )];
            [fDstCodecsPopUp addItemWithTitle:_( @"AVC/H.264 Video / AC-3 Audio" )];
			/* We disable the create chapters checkbox here and make sure it is unchecked*/
			[fCreateChapterMarkers setEnabled: NO];
			[fCreateChapterMarkers setState: NSOffState];
			break;
            
        case 3:
            ext = "ogm";
            [fDstCodecsPopUp addItemWithTitle:_( @"MPEG-4 Video / Vorbis Audio" )];
            [fDstCodecsPopUp addItemWithTitle:_( @"MPEG-4 Video / MP3 Audio" )];
            /* We disable the create chapters checkbox here and make sure it is unchecked*/
			[fCreateChapterMarkers setEnabled: NO];
			[fCreateChapterMarkers setState: NSOffState];
			break;
    }
    
    if ( SuccessfulScan ) {
        [fDstCodecsPopUp selectItemWithTitle:selectedCodecs];
        
        /* Add/replace to the correct extension */
        if( [string characterAtIndex: [string length] - 4] == '.' )
        {
            [fDstFile2Field setStringValue: [NSString stringWithFormat:
                @"%@.%s", [string substringToIndex: [string length] - 4],
                ext]];
        }
        else
        {
            [fDstFile2Field setStringValue: [NSString stringWithFormat:
                @"%@.%s", string, ext]];
        }
        
        if ( [fDstCodecsPopUp selectedItem] == NULL )
        {
            [fDstCodecsPopUp selectItemAtIndex:0];
            [self codecsPopUpChanged: NULL];
            
            /* changing the format may mean that we can / can't offer mono or 6ch, */
            /* so call audioTrackPopUpChanged for both audio tracks to update the mixdown popups */
            [self audioTrackPopUpChanged: fAudLang1PopUp];
            [self audioTrackPopUpChanged: fAudLang2PopUp];
            /* We call the method to properly enable/disable turbo 2 pass */
            [self twoPassCheckboxChanged: sender];
            /* We call method method to change UI to reflect whether a preset is used or not*/
        }
    }
    
	[self customSettingUsed: sender];	
}

- (IBAction) codecsPopUpChanged: (id) sender
{
    int format = [fDstFormatPopUp indexOfSelectedItem];
    int codecs = [fDstCodecsPopUp indexOfSelectedItem];
	
    [fAdvancedOptions setHidden:YES];

    /* Update the encoder popup*/
    if( ( FormatSettings[format][codecs] & HB_VCODEC_X264 ) )
    {
        /* MPEG-4 -> H.264 */
        [fVidEncoderPopUp removeAllItems];
		[fVidEncoderPopUp addItemWithTitle: @"x264 (h.264 Main)"];
		[fVidEncoderPopUp addItemWithTitle: @"x264 (h.264 iPod)"];
		[fVidEncoderPopUp selectItemAtIndex: 0];
        [fAdvancedOptions setHidden:NO];
    }
    
    else if( ( FormatSettings[format][codecs] & HB_VCODEC_FFMPEG ) )
    {
        /* H.264 -> MPEG-4 */
        [fVidEncoderPopUp removeAllItems];
        [fVidEncoderPopUp addItemWithTitle: @"FFmpeg"];
        [fVidEncoderPopUp addItemWithTitle: @"XviD"];
        [fVidEncoderPopUp selectItemAtIndex: 0];
				
    }

    if( FormatSettings[format][codecs] & HB_ACODEC_AC3 )
    {
        /* AC-3 pass-through: disable samplerate and bitrate */
        [fAudRatePopUp    setEnabled: NO];
        [fAudBitratePopUp setEnabled: NO];
    }
    else
    {
        [fAudRatePopUp    setEnabled: YES];
        [fAudBitratePopUp setEnabled: YES];
    }
    /* changing the codecs on offer may mean that we can / can't offer mono or 6ch, */
	/* so call audioTrackPopUpChanged for both audio tracks to update the mixdown popups */
	[self audioTrackPopUpChanged: fAudLang1PopUp];
	[self audioTrackPopUpChanged: fAudLang2PopUp];

    [self calculateBitrate: sender];
    [self twoPassCheckboxChanged: sender];
}

- (IBAction) encoderPopUpChanged: (id) sender
{
    
	/* Check to see if we need to modify the job pic values based on x264 (iPod) encoder selection */
    if ([fDstFormatPopUp indexOfSelectedItem] == 0 && [fDstCodecsPopUp indexOfSelectedItem] == 1 && [fVidEncoderPopUp indexOfSelectedItem] == 1)
    {
		hb_job_t * job = fTitle->job;
		job->pixel_ratio = 0 ;
		
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultPicSizeAutoiPod"] > 0)
		{
			
			if (job->width > 640)
			{
				job->width = 640;
			}
			job->keep_ratio = 1;
			hb_fix_aspect( job, HB_KEEP_WIDTH );
			
		}
		/* Make sure the 64bit formatting checkbox is off */
		[fDstMpgLargeFileCheck setState: NSOffState];
	}
    
	[self calculatePictureSizing: sender];
	[self twoPassCheckboxChanged: sender];
}

/* Method to determine if we should change the UI
To reflect whether or not a Preset is being used or if
the user is using "Custom" settings by determining the sender*/
- (IBAction) customSettingUsed: (id) sender
{
	if ([sender stringValue] != NULL)
	{
		/* Deselect the currently selected Preset if there is one*/
		[tableView deselectRow:[tableView selectedRow]];
		[[fPresetsActionMenu itemAtIndex:0] setEnabled: NO];
		/* Change UI to show "Custom" settings are being used */
		[fPresetSelectedDisplay setStringValue: @"Custom"];
		
		curUserPresetChosenNum = nil;
	}

}


#pragma mark -
#pragma mark - Video

- (IBAction) twoPassCheckboxChanged: (id) sender
{
	/* check to see if x264 is chosen */
	int format = [fDstFormatPopUp indexOfSelectedItem];
    int codecs = [fDstCodecsPopUp indexOfSelectedItem];
	if( ( FormatSettings[format][codecs] & HB_VCODEC_X264 ) )
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

- (IBAction) qualitySliderChanged: (id) sender
{
    [fVidConstantCell setTitle: [NSString stringWithFormat:
        _( @"Constant quality: %.0f %%" ), 100.0 *
        [fVidQualitySlider floatValue]]];
		[self customSettingUsed: sender];
}

- (void) controlTextDidChange: (NSNotification *) notification
{
    [self calculateBitrate: NULL];
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

    [self prepareJob];

    [fVidBitrateField setIntValue: hb_calc_bitrate( job,
            [fVidTargetSizeField intValue] )];
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
	/* We use the output picture width and height
     as calculated from libhb right after title is set
     in TitlePopUpChanged */
	job->width = PicOrigOutputWidth;
	job->height = PicOrigOutputHeight;
    [fPictureController setAutoCrop:YES];
	/* Here we use the auto crop values determined right after scan */
	job->crop[0] = AutoCropTop;
	job->crop[1] = AutoCropBottom;
	job->crop[2] = AutoCropLeft;
	job->crop[3] = AutoCropRight;
    
    
    [self calculatePictureSizing: sender];
    /* We call method to change UI to reflect whether a preset is used or not*/    
    [self customSettingUsed: sender];
}

/**
 * Registers changes made in the Picture Settings Window.
 */

- (void)pictureSettingsDidChange {
	[self calculatePictureSizing: NULL];
}

/* Get and Display Current Pic Settings in main window */
- (IBAction) calculatePictureSizing: (id) sender
{
	[fPicSettingWidth setStringValue: [NSString stringWithFormat:@"%d", fTitle->job->width]];
	[fPicSettingHeight setStringValue: [NSString stringWithFormat:@"%d", fTitle->job->height]];
		
	if (fTitle->job->pixel_ratio == 1)
	{
        int titlewidth = fTitle->width-fTitle->job->crop[2]-fTitle->job->crop[3];
        int arpwidth = fTitle->job->pixel_aspect_width;
        int arpheight = fTitle->job->pixel_aspect_height;
        int displayparwidth = titlewidth * arpwidth / arpheight;
        int displayparheight = fTitle->height-fTitle->job->crop[0]-fTitle->job->crop[1];
        
        [fPicSettingWidth setStringValue: [NSString stringWithFormat:@"%d", titlewidth]];
        [fPicSettingHeight setStringValue: [NSString stringWithFormat:@"%d", displayparheight]];
        [fPicLabelPAROutputX setStringValue: @"x"];
        [fPicSettingPARWidth setStringValue: [NSString stringWithFormat:@"%d", displayparwidth]];
        [fPicSettingPARHeight setStringValue: [NSString stringWithFormat:@"%d", displayparheight]];
        
        fTitle->job->keep_ratio = 0;
	}
	else
	{
        [fPicLabelPAROutputX setStringValue: @""];
        [fPicSettingPARWidth setStringValue: @""];
        [fPicSettingPARHeight setStringValue:  @""];
	}
				
	/* Set ON/Off values for the deinterlace/keep aspect ratio according to boolean */	
	if (fTitle->job->keep_ratio > 0)
	{
		[fPicSettingARkeep setStringValue: @"On"];
	}
	else
	{
		[fPicSettingARkeep setStringValue: @"Off"];
	}	
    /* Detelecine */
    if ([fPictureController detelecine]) {
        [fPicSettingDetelecine setStringValue: @"Yes"];
    }
    else {
        [fPicSettingDetelecine setStringValue: @"No"];
    }
    
    /* VFR (Variable Frame Rate) */
    if ([fPictureController vfr]) {
        /* vfr has to set the framerate to 29.97 (ntsc video)
        and disable the framerate popup */
        [fVidRatePopUp selectItemAtIndex: 8];
        [fVidRatePopUp setEnabled: NO];
        /* We change the string of the fps popup to warn that vfr is on Framerate (FPS): */
        [fVidRateField setStringValue: @"Framerate (VFR On):"];  
        
    }
    else {
        /* vfr is off, make sure the framerate popup is enabled */
        [fVidRatePopUp setEnabled: YES];
        /* and make sure the label for framerate is set to its default */  
        [fVidRateField setStringValue: @"Framerate (FPS):"];
    }

	/* Deinterlace */
	if ([fPictureController deinterlace] == 0)
	{
		[fPicSettingDeinterlace setStringValue: @"Off"];
	}
	else if ([fPictureController deinterlace] == 1)
	{
		[fPicSettingDeinterlace setStringValue: @"Fast"];
	}
	else if ([fPictureController deinterlace] == 2)
	{
		[fPicSettingDeinterlace setStringValue: @"Slow"];
	}
	else if ([fPictureController deinterlace] == 3)
	{
		[fPicSettingDeinterlace setStringValue: @"Slower"];
	}
	else if ([fPictureController deinterlace] ==4)
	{
		[fPicSettingDeinterlace setStringValue: @"Slowest"];
	}
	/* Denoise */
	if ([fPictureController denoise] == 0)
	{
		[fPicSettingDenoise setStringValue: @"Off"];
	}
	else if ([fPictureController denoise] == 1)
	{
		[fPicSettingDenoise setStringValue: @"Weak"];
	}
	else if ([fPictureController denoise] == 2)
	{
		[fPicSettingDenoise setStringValue: @"Medium"];
	}
	else if ([fPictureController denoise] == 3)
	{
		[fPicSettingDenoise setStringValue: @"Strong"];
	}

    /* Deblock */
    if ([fPictureController deblock]) {
        [fPicSettingDeblock setStringValue: @"Yes"];
    }
    else {
        [fPicSettingDeblock setStringValue: @"No"];
    }
	
	if (fTitle->job->pixel_ratio > 0)
	{
		[fPicSettingPAR setStringValue: @""];
	}
	else
	{
		[fPicSettingPAR setStringValue: @"Off"];
	}
	/* Set the display field for crop as per boolean */
	if (![fPictureController autoCrop])
	{
	    [fPicSettingAutoCrop setStringValue: @"Custom"];
	}
	else
	{
		[fPicSettingAutoCrop setStringValue: @"Auto"];
	}	
	

}


#pragma mark -
#pragma mark - Audio and Subtitles

- (IBAction) setEnabledStateOfAudioMixdownControls: (id) sender
{

    /* enable/disable the mixdown text and popupbutton for audio track 1 */
    [fAudTrack1MixPopUp setEnabled: ([fAudLang1PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack1MixLabel setTextColor: ([fAudLang1PopUp indexOfSelectedItem] == 0) ?
        [NSColor disabledControlTextColor] : [NSColor controlTextColor]];

    /* enable/disable the mixdown text and popupbutton for audio track 2 */
    [fAudTrack2MixPopUp setEnabled: ([fAudLang2PopUp indexOfSelectedItem] == 0) ? NO : YES];
    [fAudTrack2MixLabel setTextColor: ([fAudLang2PopUp indexOfSelectedItem] == 0) ?
        [NSColor disabledControlTextColor] : [NSColor controlTextColor]];

}

- (IBAction) addAllAudioTracksToPopUp: (id) sender
{

    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t*)
        hb_list_item( list, [fSrcTitlePopUp indexOfSelectedItem] );

	hb_audio_t * audio;

    [sender removeAllItems];
    [sender addItemWithTitle: _( @"None" )];
    for( int i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = (hb_audio_t *) hb_list_item( title->list_audio, i );
        [[sender menu] addItemWithTitle:
            [NSString stringWithCString: audio->lang]
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
    
	if (searchPrefixString != NULL) 
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

- (IBAction) audioTrackPopUpChanged: (id) sender
{
    /* utility function to call audioTrackPopUpChanged without passing in a mixdown-to-use */
    [self audioTrackPopUpChanged: sender mixdownToUse: 0];
}

- (IBAction) audioTrackPopUpChanged: (id) sender mixdownToUse: (int) mixdownToUse
{

    /* make sure we have a selected title before continuing */
    if (fTitle == NULL) return;

    /* find out if audio track 1 or 2 was changed - this is passed to us in the tag of the sender */
    /* the sender will have been either fAudLang1PopUp (tag = 0) or fAudLang2PopUp (tag = 1) */
    int thisAudio = [sender tag];

    /* get the index of the selected audio */
    int thisAudioIndex = [sender indexOfSelectedItem] - 1;

    /* Handbrake can't currently cope with ripping the same source track twice */
    /* So, if this audio is also selected in the other audio track popup, set that popup's selection to "none" */
    /* get a reference to the two audio track popups */
    NSPopUpButton * thisAudioPopUp  = (thisAudio == 1 ? fAudLang2PopUp : fAudLang1PopUp);
    NSPopUpButton * otherAudioPopUp = (thisAudio == 1 ? fAudLang1PopUp : fAudLang2PopUp);
    /* if the same track is selected in the other audio popup, then select "none" in that popup */
    /* unless, of course, both are selected as "none!" */
    if ([thisAudioPopUp indexOfSelectedItem] != 0 && [thisAudioPopUp indexOfSelectedItem] == [otherAudioPopUp indexOfSelectedItem]) {
        [otherAudioPopUp selectItemAtIndex: 0];
        [self audioTrackPopUpChanged: otherAudioPopUp];
    }

    /* pointer for the hb_audio_s struct we will use later on */
    hb_audio_t * audio;

    /* find out what the currently-selected output audio codec is */
    int format = [fDstFormatPopUp indexOfSelectedItem];
    int codecs = [fDstCodecsPopUp indexOfSelectedItem];
    int acodec = FormatSettings[format][codecs] & HB_ACODEC_MASK;

    /* pointer to this track's mixdown NSPopUpButton */
    NSTextField   * mixdownTextField;
    NSPopUpButton * mixdownPopUp;

    /* find our mixdown NSTextField and NSPopUpButton */
    if (thisAudio == 0)
    {
        mixdownTextField = fAudTrack1MixLabel;
        mixdownPopUp = fAudTrack1MixPopUp;
    }
    else
    {
        mixdownTextField = fAudTrack2MixLabel;
        mixdownPopUp = fAudTrack2MixPopUp;
    }

    /* delete the previous audio mixdown options */
    [mixdownPopUp removeAllItems];

    /* check if the audio mixdown controls need their enabled state changing */
    [self setEnabledStateOfAudioMixdownControls: NULL];

    if (thisAudioIndex != -1)
    {

        /* get the audio */
        audio = (hb_audio_t *) hb_list_item( fTitle->list_audio, thisAudioIndex );
        if (audio != NULL)
        {

            /* find out if our selected output audio codec supports mono and / or 6ch */
            /* we also check for an input codec of AC3 or DCA,
               as they are the only libraries able to do the mixdown to mono / conversion to 6-ch */
            /* audioCodecsSupportMono and audioCodecsSupport6Ch are the same for now,
               but this may change in the future, so they are separated for flexibility */
            int audioCodecsSupportMono = ((audio->codec == HB_ACODEC_AC3 ||
                audio->codec == HB_ACODEC_DCA) && acodec == HB_ACODEC_FAAC);
            int audioCodecsSupport6Ch =  ((audio->codec == HB_ACODEC_AC3 ||
                audio->codec == HB_ACODEC_DCA) && (acodec == HB_ACODEC_FAAC ||
                acodec == HB_ACODEC_VORBIS));

            /* check for AC-3 passthru */
            if (audio->codec == HB_ACODEC_AC3 && acodec == HB_ACODEC_AC3)
            {
                    [[mixdownPopUp menu] addItemWithTitle:
                        [NSString stringWithCString: "AC3 Passthru"]
                        action: NULL keyEquivalent: @""];
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
                int layout = audio->input_channel_layout & HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK;

                /* do we want to add a mono option? */
                if (audioCodecsSupportMono == 1) {
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
                if ((layout == HB_INPUT_CH_LAYOUT_MONO && audioCodecsSupportMono == 0) || layout >= HB_INPUT_CH_LAYOUT_STEREO) {
                    NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                        [NSString stringWithCString: hb_audio_mixdowns[1].human_readable_name]
                        action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[1].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[1].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[1].amixdown);
                }

                /* do we want to add a dolby surround (DPL1) option? */
                if (layout == HB_INPUT_CH_LAYOUT_3F1R || layout == HB_INPUT_CH_LAYOUT_3F2R || layout == HB_INPUT_CH_LAYOUT_DOLBY) {
                    NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                        [NSString stringWithCString: hb_audio_mixdowns[2].human_readable_name]
                        action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[2].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[2].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[2].amixdown);
                }

                /* do we want to add a dolby pro logic 2 (DPL2) option? */
                if (layout == HB_INPUT_CH_LAYOUT_3F2R) {
                    NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                        [NSString stringWithCString: hb_audio_mixdowns[3].human_readable_name]
                        action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[3].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[3].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[3].amixdown);
                }

                /* do we want to add a 6-channel discrete option? */
                if (audioCodecsSupport6Ch == 1 && layout == HB_INPUT_CH_LAYOUT_3F2R && (audio->input_channel_layout & HB_INPUT_CH_LAYOUT_HAS_LFE)) {
                    NSMenuItem *menuItem = [[mixdownPopUp menu] addItemWithTitle:
                        [NSString stringWithCString: hb_audio_mixdowns[4].human_readable_name]
                        action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[4].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[4].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[4].amixdown);
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
                if (useMixdown > maxMixdownUsed) useMixdown = maxMixdownUsed;

                /* if useMixdown < minMixdownUsed, then use minMixdownUsed */
                if (useMixdown < minMixdownUsed) useMixdown = minMixdownUsed;

                /* select the (possibly-amended) preferred mixdown */
                [mixdownPopUp selectItemWithTag: useMixdown];
				
				/* lets call the audioTrackMixdownChanged method here to determine appropriate bitrates, etc. */
                [self audioTrackMixdownChanged: NULL];
            }

        }
        
    }

	/* see if the new audio track choice will change the bitrate we need */
    [self calculateBitrate: sender];	

}
- (IBAction) audioTrackMixdownChanged: (id) sender
{

    /* find out what the currently-selected output audio codec is */
    int format = [fDstFormatPopUp indexOfSelectedItem];
    int codecs = [fDstCodecsPopUp indexOfSelectedItem];
    int acodec = FormatSettings[format][codecs] & HB_ACODEC_MASK;
    
    /* storage variable for the min and max bitrate allowed for this codec */
    int minbitrate;
    int maxbitrate;
    
    switch( acodec )
    {
        case HB_ACODEC_FAAC:
            /* check if we have a 6ch discrete conversion in either audio track */
            if ([[fAudTrack1MixPopUp selectedItem] tag] == HB_AMIXDOWN_6CH || [[fAudTrack2MixPopUp selectedItem] tag] == HB_AMIXDOWN_6CH)
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

        case HB_ACODEC_LAME:
            /* Lame is happy using our min bitrate of 32 kbps */
            minbitrate = 32;
            /* Lame won't encode if the bitrate is higher than 320 kbps */
            maxbitrate = 320;
            break;

        case HB_ACODEC_VORBIS:
        if ([[fAudTrack1MixPopUp selectedItem] tag] == HB_AMIXDOWN_6CH || [[fAudTrack2MixPopUp selectedItem] tag] == HB_AMIXDOWN_6CH)
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

    [fAudBitratePopUp removeAllItems];

    for( int i = 0; i < hb_audio_bitrates_count; i++ )
    {
        if (hb_audio_bitrates[i].rate >= minbitrate && hb_audio_bitrates[i].rate <= maxbitrate)
        {
            /* add a new menuitem for this bitrate */
            NSMenuItem *menuItem = [[fAudBitratePopUp menu] addItemWithTitle:
                [NSString stringWithCString: hb_audio_bitrates[i].string]
                action: NULL keyEquivalent: @""];
            /* set its tag to be the actual bitrate as an integer, so we can retrieve it later */
            [menuItem setTag: hb_audio_bitrates[i].rate];
        }
    }

    /* select the default bitrate (but use 384 for 6-ch AAC) */
    if ([[fAudTrack1MixPopUp selectedItem] tag] == HB_AMIXDOWN_6CH || [[fAudTrack2MixPopUp selectedItem] tag] == HB_AMIXDOWN_6CH)
    {
        [fAudBitratePopUp selectItemWithTag: 384];
    }
    else
    {
        [fAudBitratePopUp selectItemWithTag: hb_audio_bitrates[hb_audio_bitrates_default].rate];
    }

}

- (IBAction) subtitleSelectionChanged: (id) sender
{
	if ([fSubPopUp indexOfSelectedItem] == 0)
	{
        [fSubForcedCheck setState: NSOffState];
        [fSubForcedCheck setEnabled: NO];	
	}
	else
	{
        [fSubForcedCheck setEnabled: YES];	
	}
	
}




#pragma mark -
#pragma mark Open New Windows

- (IBAction) openHomepage: (id) sender
{
    [[NSWorkspace sharedWorkspace] openURL: [NSURL
        URLWithString:@"http://handbrake.m0k.org/"]];
}

- (IBAction) openForums: (id) sender
{
    [[NSWorkspace sharedWorkspace] openURL: [NSURL
        URLWithString:@"http://handbrake.m0k.org/forum/"]];
}
- (IBAction) openUserGuide: (id) sender
{
    [[NSWorkspace sharedWorkspace] openURL: [NSURL
        URLWithString:@"http://handbrake.m0k.org/trac/wiki/HandBrakeGuide"]];
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
	hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );
    [fPictureController showPanelInWindow:fWindow forTitle:title];
}

#pragma mark -
#pragma mark Preset Table View Methods

- (IBAction)tableViewSelected:(id)sender
{
    /* Since we cannot disable the presets tableView in terms of clickability
     we will use the enabled state of the add presets button to determine whether
     or not clicking on a preset will do anything */
	if ([fPresetsAdd isEnabled])
	{
		if ([tableView selectedRow] >= 0)
		{	
			/* we get the chosen preset from the UserPresets array */
			chosenPreset = [UserPresets objectAtIndex:[tableView selectedRow]];
			curUserPresetChosenNum = [sender selectedRow];
			/* we set the preset display field in main window here */
			[fPresetSelectedDisplay setStringValue: [NSString stringWithFormat: @"%@",[chosenPreset valueForKey:@"PresetName"]]];
			if ([[chosenPreset objectForKey:@"Default"] intValue] == 1)
			{
				[fPresetSelectedDisplay setStringValue: [NSString stringWithFormat: @"%@ (Default)",[chosenPreset valueForKey:@"PresetName"]]];
			}
			else
			{
				[fPresetSelectedDisplay setStringValue: [NSString stringWithFormat: @"%@",[chosenPreset valueForKey:@"PresetName"]]];
			}
			/* File Format */
			[fDstFormatPopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"FileFormat"]]];
			[self formatPopUpChanged: NULL];
			
			/* Chapter Markers*/
			[fCreateChapterMarkers setState:[[chosenPreset objectForKey:@"ChapterMarkers"] intValue]];
			/* Allow Mpeg4 64 bit formatting +4GB file sizes */
			[fDstMpgLargeFileCheck setState:[[chosenPreset objectForKey:@"Mp4LargeFile"] intValue]];
			/* Codecs */
			[fDstCodecsPopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"FileCodecs"]]];
			[self codecsPopUpChanged: NULL];
			/* Video encoder */
			[fVidEncoderPopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoEncoder"]]];
			
			/* We can show the preset options here in the gui if we want to
             so we check to see it the user has specified it in the prefs */
			[fAdvancedOptions setOptions: [NSString stringWithFormat:[chosenPreset valueForKey:@"x264Option"]]];
			
			/* Lets run through the following functions to get variables set there */
			[self encoderPopUpChanged: NULL];
			
			[self calculateBitrate: NULL];
			
			/* Video quality */
			[fVidQualityMatrix selectCellAtRow:[[chosenPreset objectForKey:@"VideoQualityType"] intValue] column:0];
			
			[fVidTargetSizeField setStringValue: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoTargetSize"]]];
			[fVidBitrateField setStringValue: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoAvgBitrate"]]];
			
			[fVidQualitySlider setFloatValue: [[chosenPreset valueForKey:@"VideoQualitySlider"] floatValue]];
			[self videoMatrixChanged: NULL];
			
			/* Video framerate */
			/* For video preset video framerate, we want to make sure that Same as source does not conflict with the
             detected framerate in the fVidRatePopUp so we use index 0*/
			if ([[NSString stringWithFormat:[chosenPreset valueForKey:@"VideoFramerate"]] isEqualToString: @"Same as source"])
            {
                [fVidRatePopUp selectItemAtIndex: 0];
            }
            else
            {
                [fVidRatePopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoFramerate"]]];
            }
			
			/* GrayScale */
			[fVidGrayscaleCheck setState:[[chosenPreset objectForKey:@"VideoGrayScale"] intValue]];
			
			/* 2 Pass Encoding */
			[fVidTwoPassCheck setState:[[chosenPreset objectForKey:@"VideoTwoPass"] intValue]];
			[self twoPassCheckboxChanged: NULL];
			/* Turbo 1st pass for 2 Pass Encoding */
			[fVidTurboPassCheck setState:[[chosenPreset objectForKey:@"VideoTurboTwoPass"] intValue]];
			
			/*Audio*/
			
			/* Audio Sample Rate*/
			[fAudRatePopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"AudioSampleRate"]]];
			/* Audio Bitrate Rate*/
			[fAudBitratePopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"AudioBitRate"]]];
			/*Subtitles*/
			[fSubPopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"Subtitles"]]];
			
			/* Picture Settings */
			/* Note: objectForKey:@"UsesPictureSettings" now refers to picture size, this encompasses:
             * height, width, keep ar, anamorphic and crop settings.
             * picture filters are now handled separately.
             * We will be able to actually change the key names for legacy preset keys when preset file
             * update code is done. But for now, lets hang onto the old legacy key name for backwards compatibility.
             */
			/* Check to see if the objectForKey:@"UsesPictureSettings is greater than 0, as 0 means use picture sizing "None" 
             * and the preset completely ignores any picture sizing values in the preset.
             */
            if ([[chosenPreset objectForKey:@"UsesPictureSettings"]  intValue] > 0)
			{
				hb_job_t * job = fTitle->job;
				/* Check to see if the objectForKey:@"UsesPictureSettings is 2 which is "Use Max for the source */
				if ([[chosenPreset objectForKey:@"UsesPictureSettings"]  intValue] == 2 || [[chosenPreset objectForKey:@"UsesMaxPictureSettings"]  intValue] == 1)
				{
					/* Use Max Picture settings for whatever the dvd is.*/
					[self revertPictureSizeToMax: NULL];
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
					job->pixel_ratio = [[chosenPreset objectForKey:@"PicturePAR"]  intValue];
				}
				else // /* If not 0 or 2 we assume objectForKey:@"UsesPictureSettings is 1 which is "Use picture sizing from when the preset was set" */
				{
					job->width = [[chosenPreset objectForKey:@"PictureWidth"]  intValue];
					job->height = [[chosenPreset objectForKey:@"PictureHeight"]  intValue];
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
					job->pixel_ratio = [[chosenPreset objectForKey:@"PicturePAR"]  intValue];
                    
                    
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
                    /* If the preset has no objectForKey:@"UsesPictureFilters", then we know it is a legacy preset
                     * and handle the filters here as before.
                     * NOTE: This should be removed when the update presets code is done as we can be assured that legacy
                     * presets are updated to work properly with new keys.
                     */
                    if (![chosenPreset objectForKey:@"UsesPictureFilters"])
                    {
                        /* Filters */
                        /* Deinterlace */
                        if ([chosenPreset objectForKey:@"PictureDeinterlace"])
                        {
                            [fPictureController setDeinterlace:[[chosenPreset objectForKey:@"PictureDeinterlace"] intValue]];
                        }
                        else
                        {
                            [fPictureController setDeinterlace:0];
                        }
                        /* VFR */
                        if ([[chosenPreset objectForKey:@"VFR"] intValue] == 1)
                        {
                            [fPictureController setVFR:[[chosenPreset objectForKey:@"VFR"] intValue]];
                        }
                        else
                        {
                            [fPictureController setVFR:0];
                        }
                        /* Detelecine */
                        if ([[chosenPreset objectForKey:@"PictureDetelecine"] intValue] == 1)
                        {
                            [fPictureController setDetelecine:[[chosenPreset objectForKey:@"PictureDetelecine"] intValue]];
                        }
                        else
                        {
                            [fPictureController setDetelecine:0];
                        }
                        /* Denoise */
                        if ([chosenPreset objectForKey:@"PictureDenoise"])
                        {
                            [fPictureController setDenoise:[[chosenPreset objectForKey:@"PictureDenoise"] intValue]];
                        }
                        else
                        {
                            [fPictureController setDenoise:0];
                        }   
                        /* Deblock */
                        if ([[chosenPreset objectForKey:@"PictureDeblock"] intValue] == 1)
                        {
                            [fPictureController setDeblock:[[chosenPreset objectForKey:@"PictureDeblock"] intValue]];
                        }
                        else
                        {
                            [fPictureController setDeblock:0];
                        }   
                        
                        [self calculatePictureSizing: NULL];
                    }
                    
				}
                
                
			}
			/* If the preset has an objectForKey:@"UsesPictureFilters", then we know it is a newer style filters preset
            * and handle the filters here depending on whether or not the preset specifies applying the filter.
            */
            if ([chosenPreset objectForKey:@"UsesPictureFilters"] && [[chosenPreset objectForKey:@"UsesPictureFilters"]  intValue] > 0)
            {
                /* Filters */
                /* Deinterlace */
                if ([chosenPreset objectForKey:@"PictureDeinterlace"])
                {
                    [fPictureController setDeinterlace:[[chosenPreset objectForKey:@"PictureDeinterlace"] intValue]];
                }
                else
                {
                    [fPictureController setDeinterlace:0];
                }
                /* VFR */
                if ([[chosenPreset objectForKey:@"VFR"] intValue] == 1)
                {
                    [fPictureController setVFR:[[chosenPreset objectForKey:@"VFR"] intValue]];
                }
                else
                {
                    [fPictureController setVFR:0];
                }
                /* Detelecine */
                if ([[chosenPreset objectForKey:@"PictureDetelecine"] intValue] == 1)
                {
                    [fPictureController setDetelecine:[[chosenPreset objectForKey:@"PictureDetelecine"] intValue]];
                }
                else
                {
                    [fPictureController setDetelecine:0];
                }
                /* Denoise */
                if ([chosenPreset objectForKey:@"PictureDenoise"])
                {
                    [fPictureController setDenoise:[[chosenPreset objectForKey:@"PictureDenoise"] intValue]];
                }
                else
                {
                    [fPictureController setDenoise:0];
                }   
                /* Deblock */
                if ([[chosenPreset objectForKey:@"PictureDeblock"] intValue] == 1)
                {
                    [fPictureController setDeblock:[[chosenPreset objectForKey:@"PictureDeblock"] intValue]];
                }
                else
                {
                    [fPictureController setDeblock:0];
                }             
                
            }
			[self calculatePictureSizing: NULL];
			[[fPresetsActionMenu itemAtIndex:0] setEnabled: YES];
        }
    }
}



- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return [UserPresets count];
}

/* we use this to determine display characteristics for
each table cell based on content currently only used to
show the built in presets in a blue font. */
- (void)tableView:(NSTableView *)aTableView
 willDisplayCell:(id)aCell 
 forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
    NSDictionary *userPresetDict = [UserPresets objectAtIndex:rowIndex];
	NSFont *txtFont;
	NSColor *fontColor;
	NSColor *shadowColor;
	txtFont = [NSFont systemFontOfSize: [NSFont smallSystemFontSize]];
	/* First, we check to see if its a selected row, if so, we use white since its highlighted in blue */
	if ([[aTableView selectedRowIndexes] containsIndex:rowIndex] && ([tableView editedRow] != rowIndex))
	{
		
		fontColor = [NSColor whiteColor];
		shadowColor = [NSColor colorWithDeviceRed:(127.0/255.0) green:(140.0/255.0) blue:(160.0/255.0) alpha:1.0];
	}
	else
	{
		/* We set the properties of unselected rows */
		/* if built-in preset (defined by "type" == 0) we use a blue font */
		if ([[userPresetDict objectForKey:@"Type"] intValue] == 0)
		{
			fontColor = [NSColor blueColor];
		}
		else // User created preset, use a black font
		{
			fontColor = [NSColor blackColor];
		}
		shadowColor = nil;
	}
	/* We check to see if this is the HB default, if so, color it appropriately */
	if (!presetUserDefault && presetHbDefault && rowIndex == presetHbDefault)
	{
	txtFont = [NSFont boldSystemFontOfSize: [NSFont smallSystemFontSize]];
	}
	/* We check to see if this is the User Specified default, if so, color it appropriately */
	if (presetUserDefault && rowIndex == presetUserDefault)
	{
	txtFont = [NSFont boldSystemFontOfSize: [NSFont smallSystemFontSize]];
	}
	
	[aCell setTextColor:fontColor];
	[aCell setFont:txtFont];
	/* this shadow stuff (like mail app) for some reason looks crappy, commented out
	temporarily in case we want to resurrect it */
	/*
	NSShadow *shadow = [[NSShadow alloc] init];
	NSSize shadowOffset = { width: 1.0, height: -1.5};
	[shadow setShadowOffset:shadowOffset];
	[shadow setShadowColor:shadowColor];
	[shadow set];
	*/
	
}
/* Method to display tooltip with the description for each preset, if available */
- (NSString *)tableView:(NSTableView *)aTableView toolTipForCell:(NSCell *)aCell 
                   rect:(NSRectPointer)aRect tableColumn:(NSTableColumn *)aTableColumn
                    row:(int)rowIndex mouseLocation:(NSPoint)aPos
{
     /* initialize the tooltip contents variable */
	 NSString *loc_tip;
     /* if there is a description for the preset, we show it in the tooltip */
	 if ([[UserPresets objectAtIndex:rowIndex] valueForKey:@"PresetDescription"])
	 {
        loc_tip = [NSString stringWithFormat: @"%@",[[UserPresets objectAtIndex:rowIndex] valueForKey:@"PresetDescription"]];
        return (loc_tip);
	 }
	 else
	 {
        loc_tip = @"No description available";
	 }
	 return (loc_tip);

}

- (id)tableView:(NSTableView *)aTableView
      objectValueForTableColumn:(NSTableColumn *)aTableColumn
      row:(int)rowIndex
{
id theRecord, theValue;
    
    theRecord = [UserPresets objectAtIndex:rowIndex];
    theValue = [theRecord objectForKey:[aTableColumn identifier]];
    return theValue;
}

// NSTableDataSource method that we implement to edit values directly in the table...
- (void)tableView:(NSTableView *)aTableView
        setObjectValue:(id)anObject
        forTableColumn:(NSTableColumn *)aTableColumn
        row:(int)rowIndex
{
    id theRecord;
    
    theRecord = [UserPresets objectAtIndex:rowIndex];
    [theRecord setObject:anObject forKey:@"PresetName"];
    /* We Sort the Presets By Factory or Custom */
	NSSortDescriptor * presetTypeDescriptor=[[[NSSortDescriptor alloc] initWithKey:@"Type" 
                                                    ascending:YES] autorelease];
		/* We Sort the Presets Alphabetically by name */
	NSSortDescriptor * presetNameDescriptor=[[[NSSortDescriptor alloc] initWithKey:@"PresetName" 
                                                    ascending:YES selector:@selector(caseInsensitiveCompare:)] autorelease];
	NSArray *sortDescriptors=[NSArray arrayWithObjects:presetTypeDescriptor,presetNameDescriptor,nil];
    NSArray *sortedArray=[UserPresets sortedArrayUsingDescriptors:sortDescriptors];
	[UserPresets setArray:sortedArray];
	/* We Reload the New Table data for presets */
    [tableView reloadData];
   /* We save all of the preset data here */
    [self savePreset];
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
		[self addFactoryPresets:NULL];
	}
	
}


- (IBAction) showAddPresetPanel: (id) sender
{
    /* Deselect the currently selected Preset if there is one*/
    [tableView deselectRow:[tableView selectedRow]];

    /* Populate the preset picture settings popup here */
    [fPresetNewPicSettingsPopUp removeAllItems];
    [fPresetNewPicSettingsPopUp addItemWithTitle:@"None"];
    [fPresetNewPicSettingsPopUp addItemWithTitle:@"Current"];
    [fPresetNewPicSettingsPopUp addItemWithTitle:@"Source Maximum (post source scan)"];
    [fPresetNewPicSettingsPopUp selectItemAtIndex: 0];	
    /* Uncheck the preset use filters checkbox */
    [fPresetNewPicFiltersCheck setState:NSOffState];
    /* Erase info from the input fields*/
	[fPresetNewName setStringValue: @""];
	[fPresetNewDesc setStringValue: @""];
	/* Show the panel */
	[NSApp beginSheet: fAddPresetPanel modalForWindow: fWindow modalDelegate: NULL didEndSelector: NULL contextInfo: NULL];
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
        
        [self closeAddPresetPanel:NULL];
    }
}
- (void)addPreset
{

	
	/* We Sort the Presets By Factory or Custom */
	NSSortDescriptor * presetTypeDescriptor=[[[NSSortDescriptor alloc] initWithKey:@"Type" 
                                                    ascending:YES] autorelease];
	/* We Sort the Presets Alphabetically by name */
	NSSortDescriptor * presetNameDescriptor=[[[NSSortDescriptor alloc] initWithKey:@"PresetName" 
                                                    ascending:YES selector:@selector(caseInsensitiveCompare:)] autorelease];
	NSArray *sortDescriptors=[NSArray arrayWithObjects:presetTypeDescriptor,presetNameDescriptor,nil];
	NSArray *sortedArray=[UserPresets sortedArrayUsingDescriptors:sortDescriptors];
	[UserPresets setArray:sortedArray];
	
	
	/* We Reload the New Table data for presets */
    [tableView reloadData];
   /* We save all of the preset data here */
    [self savePreset];
}

- (IBAction)insertPreset:(id)sender
{
    int index = [tableView selectedRow];
    [UserPresets insertObject:[self createPreset] atIndex:index];
    [tableView reloadData];
    [self savePreset];
}

- (NSDictionary *)createPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:[fPresetNewName stringValue] forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
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
	[preset setObject:[NSNumber numberWithInt:[fDstMpgLargeFileCheck state]] forKey:@"Mp4LargeFile"];
	/* Codecs */
	[preset setObject:[fDstCodecsPopUp titleOfSelectedItem] forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:[fVidEncoderPopUp titleOfSelectedItem] forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:[fAdvancedOptions optionsString] forKey:@"x264Option"];
	
	[preset setObject:[NSNumber numberWithInt:[fVidQualityMatrix selectedRow]] forKey:@"VideoQualityType"];
	[preset setObject:[fVidTargetSizeField stringValue] forKey:@"VideoTargetSize"];
	[preset setObject:[fVidBitrateField stringValue] forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:[fVidQualitySlider floatValue]] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
    if ([fVidRatePopUp indexOfSelectedItem] == 0) // Same as source is selected
	{
    [preset setObject:[NSString stringWithFormat: @"Same as source"] forKey:@"VideoFramerate"];
    }
    else // we can record the actual titleOfSelectedItem
    {
    [preset setObject:[fVidRatePopUp titleOfSelectedItem] forKey:@"VideoFramerate"];
    }
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:[fVidGrayscaleCheck state]] forKey:@"VideoGrayScale"];
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
	[preset setObject:[NSNumber numberWithInt:fTitle->job->pixel_ratio] forKey:@"PicturePAR"];
    
    /* Set crop settings here */
	[preset setObject:[NSNumber numberWithInt:[fPictureController autoCrop]] forKey:@"PictureAutoCrop"];
    [preset setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
    
    /* Picture Filters */
    [preset setObject:[NSNumber numberWithInt:[fPictureController deinterlace]] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:[fPictureController detelecine]] forKey:@"PictureDetelecine"];
    [preset setObject:[NSNumber numberWithInt:[fPictureController vfr]] forKey:@"VFR"];
	[preset setObject:[NSNumber numberWithInt:[fPictureController denoise]] forKey:@"PictureDenoise"];
    [preset setObject:[NSNumber numberWithInt:[fPictureController deblock]] forKey:@"PictureDeblock"];
    

	
	/*Audio*/
	/* Audio Sample Rate*/
	[preset setObject:[fAudRatePopUp titleOfSelectedItem] forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:[fAudBitratePopUp titleOfSelectedItem] forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:[fSubPopUp titleOfSelectedItem] forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (void)savePreset
{
    [UserPresets writeToFile:UserPresetsFile atomically:YES];
	/* We get the default preset in case it changed */
	[self getDefaultPresets: NULL];

}

- (IBAction)deletePreset:(id)sender
{
    int status;
    NSEnumerator *enumerator;
    NSNumber *index;
    NSMutableArray *tempArray;
    id tempObject;
    
    if ( [tableView numberOfSelectedRows] == 0 )
        return;
    /* Alert user before deleting preset */
	/* Comment out for now, tie to user pref eventually */

    //NSBeep();
    status = NSRunAlertPanel(@"Warning!", @"Are you sure that you want to delete the selected preset?", @"OK", @"Cancel", nil);
    
    if ( status == NSAlertDefaultReturn ) {
        enumerator = [tableView selectedRowEnumerator];
        tempArray = [NSMutableArray array];
        
        while ( (index = [enumerator nextObject]) ) {
            tempObject = [UserPresets objectAtIndex:[index intValue]];
            [tempArray addObject:tempObject];
        }
        
        [UserPresets removeObjectsInArray:tempArray];
        [tableView reloadData];
        [self savePreset];   
    }
}

#pragma mark -
#pragma mark Manage Default Preset

- (IBAction)getDefaultPresets:(id)sender
{
	int i = 0;
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
	id tempObject;
	while (tempObject = [enumerator nextObject])
	{
		NSDictionary *thisPresetDict = tempObject;
		if ([[thisPresetDict objectForKey:@"Default"] intValue] == 1) // 1 is HB default
		{
			presetHbDefault = i;	
		}
		if ([[thisPresetDict objectForKey:@"Default"] intValue] == 2) // 2 is User specified default
		{
			presetUserDefault = i;	
		}
		i++;
	}
}

- (IBAction)setDefaultPreset:(id)sender
{
    int i = 0;
    NSEnumerator *enumerator = [UserPresets objectEnumerator];
	id tempObject;
	/* First make sure the old user specified default preset is removed */
	while (tempObject = [enumerator nextObject])
	{
		/* make sure we are not removing the default HB preset */
		if ([[[UserPresets objectAtIndex:i] objectForKey:@"Default"] intValue] != 1) // 1 is HB default
		{
			[[UserPresets objectAtIndex:i] setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
		}
		i++;
	}
	/* Second, go ahead and set the appropriate user specfied preset */
	/* we get the chosen preset from the UserPresets array */
	if ([[[UserPresets objectAtIndex:[tableView selectedRow]] objectForKey:@"Default"] intValue] != 1) // 1 is HB default
	{
		[[UserPresets objectAtIndex:[tableView selectedRow]] setObject:[NSNumber numberWithInt:2] forKey:@"Default"];
	}
	presetUserDefault = [tableView selectedRow];
	
	/* We save all of the preset data here */
    [self savePreset];
	/* We Reload the New Table data for presets */
    [tableView reloadData];
}

- (IBAction)selectDefaultPreset:(id)sender
{
	/* if there is a user specified default, we use it */
	if (presetUserDefault)
	{
	[tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:presetUserDefault] byExtendingSelection:NO];
	[self tableViewSelected:NULL];
	}
	else if (presetHbDefault) //else we use the built in default presetHbDefault
	{
	[tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:presetHbDefault] byExtendingSelection:NO];
	[self tableViewSelected:NULL];
	}
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
        [tableView reloadData];
        [self savePreset];   

}

   /* We use this method to recreate new, updated factory
   presets */
- (IBAction)addFactoryPresets:(id)sender
{
   
   /* First, we delete any existing built in presets */
    [self deleteFactoryPresets: sender];
    /* Then we generate new built in presets programmatically with fPresetsBuiltin
    * which is all setup in HBPresets.h and  HBPresets.m*/
    [fPresetsBuiltin generateBuiltinPresets:UserPresets];

    [self addPreset];
}








@end
