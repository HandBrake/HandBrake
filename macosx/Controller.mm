/* $Id: Controller.mm,v 1.79 2005/11/04 19:41:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "Controller.h"
#include "a52dec/a52.h"

#define _(a) NSLocalizedString(a,NULL)

static int FormatSettings[3][4] =
  { { HB_MUX_MP4 | HB_VCODEC_FFMPEG | HB_ACODEC_FAAC,
      HB_MUX_MP4 | HB_VCODEC_X264   | HB_ACODEC_FAAC,
      0,
      0 },
    { HB_MUX_AVI | HB_VCODEC_FFMPEG | HB_ACODEC_LAME,
      HB_MUX_AVI | HB_VCODEC_FFMPEG | HB_ACODEC_AC3,
      HB_MUX_AVI | HB_VCODEC_X264   | HB_ACODEC_LAME,
      HB_MUX_AVI | HB_VCODEC_X264   | HB_ACODEC_AC3 },
    { HB_MUX_OGM | HB_VCODEC_FFMPEG | HB_ACODEC_VORBIS,
      HB_MUX_OGM | HB_VCODEC_FFMPEG | HB_ACODEC_LAME,
      0,
      0 } };

/*******************************
 * HBController implementation *
 *******************************/
@implementation HBController

- init
{
    self    = [super init];
    fHandle = NULL;
    return self;
}

- (void) applicationDidFinishLaunching: (NSNotification *) notification
{
    int    build;
    char * version;

    /* Init libhb */
    fHandle = hb_init( HB_DEBUG_NONE, [[NSUserDefaults
        standardUserDefaults] boolForKey:@"CheckForUpdates"] );
    
    /* Init others controllers */
    [fScanController    SetHandle: fHandle];
    [fPictureController SetHandle: fHandle];
    [fQueueController   SetHandle: fHandle];
	

     /* Call UpdateUI every 2/10 sec */
    [[NSRunLoop currentRunLoop] addTimer: [NSTimer
        scheduledTimerWithTimeInterval: 0.2 target: self
        selector: @selector( UpdateUI: ) userInfo: NULL repeats: FALSE]
        forMode: NSModalPanelRunLoopMode];

    if( ( build = hb_check_update( fHandle, &version ) ) > -1 )
    {
        /* Update available - tell the user */
	
        NSBeginInformationalAlertSheet( _( @"Update is available" ),
            _( @"Go get it!" ), _( @"Discard" ), NULL, fWindow, self,
            @selector( UpdateAlertDone:returnCode:contextInfo: ),
            NULL, NULL, [NSString stringWithFormat:
            _( @"HandBrake %s (build %d) is now available for download." ),
            version, build] );
        return;

    }

    /* Show scan panel ASAP */
    [self performSelectorOnMainThread: @selector(ShowScanPanel:)
        withObject: NULL waitUntilDone: NO];
}

- (NSApplicationTerminateReply) applicationShouldTerminate:
    (NSApplication *) app
{
    if( [[fRipButton title] isEqualToString: _( @"Cancel" )] )
    {
        [self Cancel: NULL];
        return NSTerminateCancel;
    }
    
    /* Clean up */
    hb_close( &fHandle );
    return NSTerminateNow;
}

- (void) awakeFromNib
{
    [fWindow center];

    [self TranslateStrings];

	/* Init User Presets .plist */
	/* We declare the default NSFileManager into fileManager */
	NSFileManager * fileManager = [NSFileManager defaultManager];
	//presetPrefs = [[NSUserDefaults standardUserDefaults] retain];
	/* we set the files and support paths here */
	AppSupportDirectory = @"~/Library/Application Support/HandBrake";
    AppSupportDirectory = [AppSupportDirectory stringByExpandingTildeInPath];
    
	UserPresetsFile = @"~/Library/Application Support/HandBrake/UserPresets.plist";
    UserPresetsFile = [UserPresetsFile stringByExpandingTildeInPath];
	
	x264ProfilesFile = @"~/Library/Application Support/HandBrake/x264Profiles.plist";
    x264ProfilesFile = [x264ProfilesFile stringByExpandingTildeInPath];
	/* We check for the app support directory for media fork */
	if ([fileManager fileExistsAtPath:AppSupportDirectory] == 0) 
	{
		// If it doesnt exist yet, we create it here 
		[fileManager createDirectoryAtPath:AppSupportDirectory attributes:nil];
	}
	// We check for the presets.plist here
	
	if ([fileManager fileExistsAtPath:UserPresetsFile] == 0) 
	{

		[fileManager createFileAtPath:UserPresetsFile contents:nil attributes:nil];
		
	}
	// We check for the x264profiles.plist here
	 
	if ([fileManager fileExistsAtPath:x264ProfilesFile] == 0) 
	{
        
		[fileManager createFileAtPath:x264ProfilesFile contents:nil attributes:nil];
	}
    
	
  UserPresetsFile = @"~/Library/Application Support/HandBrake/UserPresets.plist";
  UserPresetsFile = [[UserPresetsFile stringByExpandingTildeInPath]retain];

  UserPresets = [[NSMutableArray alloc] initWithContentsOfFile:UserPresetsFile];
  if (nil == UserPresets) 
  {
    UserPresets = [[NSMutableArray alloc] init];
	[self AddFactoryPresets:NULL];
  }
  /* Show/Dont Show Presets drawer upon launch based
  on user preference DefaultPresetsDrawerShow*/
  if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultPresetsDrawerShow"] > 0)
		{
	  [fPresetDrawer open];
		}



    /* Destination box*/
    [fDstFormatPopUp removeAllItems];
    [fDstFormatPopUp addItemWithTitle: _( @"MP4 file" )];
    [fDstFormatPopUp addItemWithTitle: _( @"AVI file" )];
    [fDstFormatPopUp addItemWithTitle: _( @"OGM file" )];
    [fDstFormatPopUp selectItemAtIndex: 0];

    [self FormatPopUpChanged: NULL];
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
    [self VideoMatrixChanged: NULL];

    /* Video framerate */
    [fVidRatePopUp removeAllItems];
	[fVidRatePopUp addItemWithTitle: _( @"Same as source" )];
    for( int i = 0; i < hb_video_rates_count; i++ )
    {
        [fVidRatePopUp addItemWithTitle:
            [NSString stringWithCString: hb_video_rates[i].string]];
    }
    [fVidRatePopUp selectItemAtIndex: 0];
	
	/* Picture Settings */
	[fPicLabelPAROutp setStringValue: @""];
	[fPicLabelPAROutputX setStringValue: @""];
	[fPicSettingPARWidth setStringValue: @""];
	[fPicSettingPARHeight setStringValue:  @""];
	
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

    [self EnableUI: NO];
    [fPauseButton setEnabled: NO];
    [fRipButton setEnabled: NO];



}


- (void) TranslateStrings
{
    [fSrcDVD1Field      setStringValue: _( @"DVD:" )];
    [fSrcTitleField     setStringValue: _( @"Title:" )];
    [fSrcChapterField   setStringValue: _( @"Chapters:" )];
    [fSrcChapterToField setStringValue: _( @"to" )];
    [fSrcDuration1Field setStringValue: _( @"Duration:" )];

    [fDstFormatField    setStringValue: _( @"File format:" )];
    [fDstCodecsField    setStringValue: _( @"Codecs:" )];
    [fDstFile1Field     setStringValue: _( @"File:" )];
    [fDstBrowseButton   setTitle:       _( @"Browse" )];

    [fVidRateField      setStringValue: _( @"Framerate (fps):" )];
    [fVidEncoderField   setStringValue: _( @"Encoder:" )];
    [fVidQualityField   setStringValue: _( @"Quality:" )];
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

- (void) UpdateUI: (NSTimer *) timer
{

    hb_state_t s;
    hb_get_state( fHandle, &s );

    switch( s.state )
    {
        case HB_STATE_IDLE:
            break;

        case HB_STATE_SCANNING:
            [fScanController UpdateUI: &s];
            break;

#define p s.param.scandone
        case HB_STATE_SCANDONE:
        {
            hb_list_t  * list;
            hb_title_t * title;
			int indxpri=0; 	  // Used to search the longuest title (default in combobox)
			int longuestpri=0; // Used to search the longuest title (default in combobox)

            [fScanController UpdateUI: &s];

            list = hb_get_titles( fHandle );

            if( !hb_list_count( list ) )
            {
                break;
            }


            [fSrcTitlePopUp removeAllItems];
            for( int i = 0; i < hb_list_count( list ); i++ )
            {
                title = (hb_title_t *) hb_list_item( list, i );
                /*Set DVD Name at top of window*/
				[fSrcDVD2Field setStringValue: [NSString
                  stringWithUTF8String: title->name]];	
				
				/* Use the dvd name in the default output field here 
				May want to add code to remove blank spaces for some dvd names*/
				/* Check to see if the last destination has been set,use if so, if not, use Desktop */
				if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastDestinationDirectory"])
				{
				[fDstFile2Field setStringValue: [NSString stringWithFormat:
                @"%@/%@.mp4", [[NSUserDefaults standardUserDefaults] stringForKey:@"LastDestinationDirectory"],[NSString
                  stringWithUTF8String: title->name]]];
				}
				else
				{
				[fDstFile2Field setStringValue: [NSString stringWithFormat:
                @"%@/Desktop/%@.mp4", NSHomeDirectory(),[NSString
                  stringWithUTF8String: title->name]]];
				}

                  
                if (longuestpri < title->hours*60*60 + title->minutes *60 + title->seconds)
                {
                	longuestpri=title->hours*60*60 + title->minutes *60 + title->seconds;
                	indxpri=i;
                }
                
				
                int format = [fDstFormatPopUp indexOfSelectedItem];
				char * ext = NULL;
				switch( format )
                {
                 case 0:
					 
					 /*Get Default MP4 File Extension for mpeg4 (.mp4 or .m4v) from prefs*/
					 if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultMpegName"] > 0)
					 {
					 ext = "m4v";
					 }
				     else
				     {
					 ext = "mp4";
					 }
					break;
				case 1: 
                     ext = "avi";
				case 2:
				   break;
                     ext = "ogm";
			       break;
				   }
				
				
				NSString * string = [fDstFile2Field stringValue];
				/* Add/replace File Output name to the correct extension*/
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

				
			    [fSrcTitlePopUp addItemWithTitle: [NSString
                    stringWithFormat: @"%d - %02dh%02dm%02ds",
                    title->index, title->hours, title->minutes,
                    title->seconds]];
			
            }
            // Select the longuest title
			[fSrcTitlePopUp selectItemAtIndex: indxpri];
            /* We set the Settings Display to "Default" here
			until we get default presets implemented */
			[fPresetSelectedDisplay setStringValue: @"Default"];
			
            [self TitlePopUpChanged: NULL];
            [self EnableUI: YES];
            [fPauseButton setEnabled: NO];
            [fRipButton   setEnabled: YES];
            break;
        }
#undef p

#define p s.param.working
        case HB_STATE_WORKING:
        {
            float progress_total;
            NSMutableString * string;

            /* Update text field */
            string = [NSMutableString stringWithFormat:
                _( @"Encoding: task %d of %d, %.2f %%" ),
                p.job_cur, p.job_count, 100.0 * p.progress];
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

            /* Update dock icon */
            [self UpdateDockIcon: progress_total];

            [fPauseButton setEnabled: YES];
            [fPauseButton setTitle: _( @"Pause" )];
            [fRipButton setEnabled: YES];
            [fRipButton setTitle: _( @"Cancel" )];
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
			
            [fPauseButton setEnabled: YES];
            [fPauseButton setTitle: _( @"Pause" )];
            [fRipButton setEnabled: YES];
            [fRipButton setTitle: _( @"Cancel" )];
            break;
        }
#undef p
			
        case HB_STATE_PAUSED:
		    [fStatusField setStringValue: _( @"Paused" )];
            [fPauseButton setEnabled: YES];
            [fPauseButton setTitle: _( @"Resume" )];
            [fRipButton setEnabled: YES];
            [fRipButton setTitle: _( @"Cancel" )];
            break;

        case HB_STATE_WORKDONE:
        {
            [self EnableUI: YES];
            [fStatusField setStringValue: _( @"Done." )];
            [fRipIndicator setIndeterminate: NO];
            [fRipIndicator setDoubleValue: 0.0];
            [fRipButton setTitle: _( @"Rip" )];

            /* Restore dock icon */
            [self UpdateDockIcon: -1.0];

            [fPauseButton setEnabled: NO];
            [fPauseButton setTitle: _( @"Pause" )];
            [fRipButton setEnabled: YES];
            [fRipButton setTitle: _( @"Rip" )];

            /* FIXME */
            hb_job_t * job;
            while( ( job = hb_job( fHandle, 0 ) ) )
            {
                hb_rem( fHandle, job );
            }
            break;
        }
    }

    /* FIXME: we should only do that when necessary */
    if( [fQueueCheck state] == NSOnState )
    {
        int count = hb_count( fHandle );
        if( count )
        {
            [fQueueCheck setTitle: [NSString stringWithFormat:
                @"Enable queue (%d task%s in queue)",
                count, ( count > 1 ) ? "s" : ""]];
        }
        else
        {
            [fQueueCheck setTitle: @"Enable queue (no task in queue)"];
        }
    }

    [[NSRunLoop currentRunLoop] addTimer: [NSTimer
        scheduledTimerWithTimeInterval: 0.2 target: self
        selector: @selector( UpdateUI: ) userInfo: NULL repeats: FALSE]
        forMode: NSModalPanelRunLoopMode];
}

- (void) EnableUI: (bool) b
{
    NSControl * controls[] =
      { fSrcDVD1Field, fSrcDVD2Field, fSrcTitleField, fSrcTitlePopUp,
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
        fAudBitratePopUp, fPictureButton, fQueueCheck, 
		fPicSrcWidth,fPicSrcHeight,fPicSettingWidth,fPicSettingHeight,
		fPicSettingARkeep,fPicSettingDeinterlace,fPicSettingARkeepDsply,
		fPicSettingDeinterlaceDsply,fPicLabelSettings,fPicLabelSrc,fPicLabelOutp,
		fPicLabelAr,fPicLabelDeinter,fPicLabelSrcX,fPicLabelOutputX,
		fPicLabelPAROutp,fPicLabelPAROutputX,fPicSettingPARWidth,fPicSettingPARHeight,
		fPicSettingPARDsply,fPicLabelAnamorphic,tableView,fPresetsAdd,fPresetsDelete,
		fCreateChapterMarkers,fPresetNewX264OptLabel};

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
        [self SetEnabledStateOfAudioMixdownControls: NULL];
	
	} else {

		[tableView setEnabled: NO];
	
	}

    [self VideoMatrixChanged: NULL];
}

- (IBAction) ShowScanPanel: (id) sender
{
    [fScanController Show];
}

- (BOOL) windowShouldClose: (id) sender
{
    /* Stop the application when the user closes the window */
    [NSApp terminate: self];
    return YES;
}

- (IBAction) VideoMatrixChanged: (id) sender;
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
    }

    [self QualitySliderChanged: sender];
    [self CalculateBitrate:     sender];
	[self CustomSettingUsed: sender];
}

- (IBAction) QualitySliderChanged: (id) sender
{
    [fVidConstantCell setTitle: [NSString stringWithFormat:
        _( @"Constant quality: %.0f %%" ), 100.0 *
        [fVidQualitySlider floatValue]]];
		[self CustomSettingUsed: sender];
}

- (IBAction) BrowseFile: (id) sender
{
    /* Open a panel to let the user choose and update the text field */
    NSSavePanel * panel = [NSSavePanel savePanel];
	/* We get the current file name and path from the destination field here */
	[panel beginSheetForDirectory: [[fDstFile2Field stringValue] stringByDeletingLastPathComponent] file: [[fDstFile2Field stringValue] lastPathComponent]
				   modalForWindow: fWindow modalDelegate: self
				   didEndSelector: @selector( BrowseFileDone:returnCode:contextInfo: )
					  contextInfo: NULL];
}

- (void) BrowseFileDone: (NSSavePanel *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSOKButton )
    {
        [fDstFile2Field setStringValue: [sheet filename]];
		
    }
}

- (IBAction) ShowPicturePanel: (id) sender
{
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );

    /* Resize the panel */
    NSSize newSize;
    newSize.width  = 246 + title->width;
    newSize.height = 80 + title->height;
    [fPicturePanel setContentSize: newSize];

    [fPictureController SetTitle: title];

    [NSApp beginSheet: fPicturePanel modalForWindow: fWindow
        modalDelegate: NULL didEndSelector: NULL contextInfo: NULL];
    [NSApp runModalForWindow: fPicturePanel];
    [NSApp endSheet: fPicturePanel];
    [fPicturePanel orderOut: self];
	[self CalculatePictureSizing: sender];
}

- (IBAction) ShowQueuePanel: (id) sender
{
    /* Update the OutlineView */
    [fQueueController Update: sender];

    /* Show the panel */
    [NSApp beginSheet: fQueuePanel modalForWindow: fWindow
        modalDelegate: NULL didEndSelector: NULL contextInfo: NULL];
    [NSApp runModalForWindow: fQueuePanel];
    [NSApp endSheet: fQueuePanel];
    [fQueuePanel orderOut: self];
}

- (void) PrepareJob
{
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;

    /* Chapter selection */
    job->chapter_start = [fSrcChapterStartPopUp indexOfSelectedItem] + 1;
    job->chapter_end   = [fSrcChapterEndPopUp   indexOfSelectedItem] + 1;
	


    /* Format and codecs */
    int format = [fDstFormatPopUp indexOfSelectedItem];
    int codecs = [fDstCodecsPopUp indexOfSelectedItem];
    job->mux    = FormatSettings[format][codecs] & HB_MUX_MASK;
    job->vcodec = FormatSettings[format][codecs] & HB_VCODEC_MASK;
    job->acodec = FormatSettings[format][codecs] & HB_ACODEC_MASK;
    /* We set the chapter marker extraction here based on the format being
	mpeg4 and the checkbox being checked */
	if ([fDstFormatPopUp indexOfSelectedItem] == 0 && [fCreateChapterMarkers state] == NSOnState)
	{
	job->chapter_markers = 1;
	}
	else
	{
	job->chapter_markers = 0;
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
		Lets Deprecate Baseline Level 1.3*/
		job->h264_level = 30;
		job->mux = HB_MUX_IPOD;
        /* move sanity check for iPod Encoding here */
		job->pixel_ratio = 0 ;

		}
		
		/* Set this flag to switch from Constant Quantizer(default) to Constant Rate Factor Thanks jbrjake
		Currently only used with Constant Quality setting*/
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultCrf"] > 0 && [fVidQualityMatrix selectedRow] == 2)
		{
	        /* Can only be used with svn rev >= 89 */
			job->crf = 1;
		}
		
		/* Below Sends x264 options to the core library if x264 is selected*/
		/* First we look to see if a user preset has been selected that contains a x264 optional string CurUserPresetChosenNum = nil */
		if (curUserPresetChosenNum != nil)
		{
			
			/* Lets use this as per Nyx, Thanks Nyx! */
			job->x264opts = (char *)calloc(1024, 1); /* Fixme, this just leaks */  
			strcpy(job->x264opts, [[chosenPreset valueForKey:@"x264Option"] UTF8String]);
			//job->x264opts = [[chosenPreset valueForKey:@"x264Option"] cString];
		}
		else
		{
		    /* if not, then we check to see if there is a x264 opt in the preferences and use that if we want */
			//job->x264opts = [[[NSUserDefaults standardUserDefaults] stringForKey:@"DefAdvancedx264Flags"] UTF8String];
			/* Lets use this as per Nyx, Thanks Nyx! */
			job->x264opts = (char *)calloc(1024, 1); /* Fixme, this just leaks */  
			strcpy(job->x264opts, [[[NSUserDefaults standardUserDefaults] stringForKey:@"DefAdvancedx264Flags"] UTF8String]);
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
    job->subtitle = [fSubPopUp indexOfSelectedItem] - 1;

    /* Audio tracks */
    job->audios[0] = [fAudLang1PopUp indexOfSelectedItem] - 1;
    job->audios[1] = [fAudLang2PopUp indexOfSelectedItem] - 1;
    job->audios[2] = -1;

    /* Audio settings */
    job->arate = hb_audio_rates[[fAudRatePopUp
                     indexOfSelectedItem]].rate;
    job->abitrate = hb_audio_bitrates[[fAudBitratePopUp
                        indexOfSelectedItem]].rate;

    /* Audio mixdown(s) */
    if (job->audios[0] > -1)
    {
        job->audio_mixdowns[0] = [[fAudTrack1MixPopUp selectedItem] tag];
    }

    if (job->audios[1] > -1)
    {
        job->audio_mixdowns[1] = [[fAudTrack2MixPopUp selectedItem] tag];
    }

}

- (IBAction) EnableQueue: (id) sender
{
    bool e = ( [fQueueCheck state] == NSOnState );
    [fQueueAddButton  setHidden: !e];
    [fQueueShowButton setHidden: !e];
    [fRipButton       setTitle: e ? @"Start" : @"Rip"];
}

- (IBAction) AddToQueue: (id) sender
{
/* We get the destination directory from the destingation field here */
	NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
	/* We check for a valid destination here */
	if ([[NSFileManager defaultManager] fileExistsAtPath:destinationDirectory] == 0) 
	{
		NSRunAlertPanel(@"Warning!", @"This is not a valid destination directory!", @"OK", nil, nil);
	}
	else
	{
		
		hb_list_t  * list  = hb_get_titles( fHandle );
		hb_title_t * title = (hb_title_t *) hb_list_item( list,
														  [fSrcTitlePopUp indexOfSelectedItem] );
		hb_job_t * job = title->job;
		
		[self PrepareJob];
		
		/* Destination file */
		job->file = [[fDstFile2Field stringValue] UTF8String];
		
		if( [fVidTwoPassCheck state] == NSOnState )
		{
			job->pass = 1;
			hb_add( fHandle, job );
			job->pass = 2;
			/* First we look to see if a user preset has been selected that contains a x264 optional string CurUserPresetChosenNum = nil */
			if (curUserPresetChosenNum != nil)
			{
				
				/* Lets use this as per Nyx, Thanks Nyx! */
				job->x264opts = (char *)calloc(1024, 1); /* Fixme, this just leaks */  
				strcpy(job->x264opts, [[chosenPreset valueForKey:@"x264Option"] UTF8String]);
				//job->x264opts = [[chosenPreset valueForKey:@"x264Option"] cString];
			}
			else
			{
				//job->x264opts = [[[NSUserDefaults standardUserDefaults] stringForKey:@"DefAdvancedx264Flags"] UTF8String];
				/* Lets use this as per Nyx, Thanks Nyx! */
				job->x264opts = (char *)calloc(1024,1); /* Fixme, this just leaks */ 
				strcpy(job->x264opts, [[[NSUserDefaults standardUserDefaults]stringForKey:@"DefAdvancedx264Flags"] UTF8String]);
			} 
			hb_add( fHandle, job );
		}
		else
		{
			job->pass = 0;
			hb_add( fHandle, job );
		}
	
	[[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];
	}
}

- (IBAction) Rip: (id) sender
{
   
    
    /* Rip or Cancel ? */
    if( [[fRipButton title] isEqualToString: _( @"Cancel" )] )
    {
        [self Cancel: sender];
        return;
    }

    if( [fQueueCheck state] == NSOffState )
    {

			[self AddToQueue: sender];

		

    }

    /* We check for duplicate name here */
	if( [[NSFileManager defaultManager] fileExistsAtPath:
            [fDstFile2Field stringValue]] )
    {
        NSBeginCriticalAlertSheet( _( @"File already exists" ),
            _( @"Cancel" ), _( @"Overwrite" ), NULL, fWindow, self,
            @selector( OverwriteAlertDone:returnCode:contextInfo: ),
            NULL, NULL, [NSString stringWithFormat:
            _( @"Do you want to overwrite %@?" ),
            [fDstFile2Field stringValue]] );
        return;
    }
	/* We get the destination directory from the destination field here */
	NSString *destinationDirectory = [[fDstFile2Field stringValue] stringByDeletingLastPathComponent];
	/* We check for a valid destination here */
	if ([[NSFileManager defaultManager] fileExistsAtPath:destinationDirectory] == 0) 
	{
		NSRunAlertPanel(@"Warning!", @"This is not a valid destination directory!", @"OK", nil, nil);
	}
	else
	{
	[[NSUserDefaults standardUserDefaults] setObject:destinationDirectory forKey:@"LastDestinationDirectory"];
		[self _Rip];
	}
	


}

- (void) OverwriteAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSAlertAlternateReturn )
    {
        [self _Rip];
    }
}

- (void) UpdateAlertDone: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSAlertAlternateReturn )
    {
        /* Show scan panel */
        [self performSelectorOnMainThread: @selector(ShowScanPanel:)
            withObject: NULL waitUntilDone: NO];
        return;
    }

    /* Go to HandBrake homepage and exit */
    [self OpenHomepage: NULL];
    [NSApp terminate: self];
}

- (void) _Rip
{
    /* Let libhb do the job */
    hb_start( fHandle );

    /* Disable interface */
   [self EnableUI: NO];
    [fPauseButton setEnabled: NO];
    [fRipButton   setEnabled: NO];
}

- (IBAction) Cancel: (id) sender
{
    NSBeginCriticalAlertSheet( _( @"Cancel - Are you sure?" ),
        _( @"Keep working" ), _( @"Cancel encoding" ), NULL, fWindow, self,
        @selector( _Cancel:returnCode:contextInfo: ), NULL, NULL,
        _( @"Encoding won't be recoverable." ) );
}

- (void) _Cancel: (NSWindow *) sheet
    returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    if( returnCode == NSAlertAlternateReturn )
    {
        hb_stop( fHandle );
        [fPauseButton setEnabled: NO];
        [fRipButton   setEnabled: NO];
    }
}

- (IBAction) Pause: (id) sender
{
    [fPauseButton setEnabled: NO];
    [fRipButton   setEnabled: NO];

    if( [[fPauseButton title] isEqualToString: _( @"Resume" )] )
    {
        hb_resume( fHandle );
    }
    else
    {
        hb_pause( fHandle );
    }
}

- (IBAction) TitlePopUpChanged: (id) sender
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
			[fSrcTitlePopUp indexOfSelectedItem] + 1,
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
    [self ChapterPopUpChanged: NULL];

/* Start Get and set the initial pic size for display */
	hb_job_t * job = title->job;
	fTitle = title; 
	/*Set Source Size Fields Here */
	[fPicSrcWidth setStringValue: [NSString stringWithFormat:
							 @"%d", fTitle->width]];
	[fPicSrcHeight setStringValue: [NSString stringWithFormat:
							 @"%d", fTitle->height]];
	/* We get the originial output picture width and height and put them
	in variables for use with some presets later on */
	PicOrigOutputWidth = job->width;
	PicOrigOutputHeight = job->height;
	/* we test getting the max output value for pic sizing here to be used later*/
	[fPicSettingWidth setStringValue: [NSString stringWithFormat:
		@"%d", PicOrigOutputWidth]];
	[fPicSettingHeight setStringValue: [NSString stringWithFormat:
		@"%d", PicOrigOutputHeight]];
	/* Turn Deinterlace on/off depending on the preference */
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultDeinterlaceOn"] > 0)
	{
		job->deinterlace = 1;
	}
	else
	{
		job->deinterlace = 0;
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
	/* Run Through EncoderPopUpChanged to see if there
		needs to be any pic value modifications based on encoder settings */
	//[self EncoderPopUpChanged: NULL];
	/* END Get and set the initial pic size for display */ 


    /* Update subtitle popups */
    hb_subtitle_t * subtitle;
    [fSubPopUp removeAllItems];
    [fSubPopUp addItemWithTitle: @"None"];
    for( int i = 0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        subtitle = (hb_subtitle_t *) hb_list_item( title->list_subtitle, i );

        /* We cannot use NSPopUpButton's addItemWithTitle because
           it checks for duplicate entries */
        [[fSubPopUp menu] addItemWithTitle: [NSString stringWithCString:
            subtitle->lang] action: NULL keyEquivalent: @""];
    }
    [fSubPopUp selectItemAtIndex: 0];

    /* START pri */
	hb_audio_t * audio;

	// PRI CHANGES 02/12/06
	NSString * audiotmppri;
	NSString * audiosearchpri=[[NSUserDefaults standardUserDefaults] stringForKey:@"DefaultLanguage"];
	int indxpri=0;
	// End of pri changes 02/12/06
    [fAudLang1PopUp removeAllItems];
	[fAudLang2PopUp removeAllItems];
    [fAudLang1PopUp addItemWithTitle: _( @"None" )];
	[fAudLang2PopUp addItemWithTitle: _( @"None" )];
    for( int i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = (hb_audio_t *) hb_list_item( title->list_audio, i );
	// PRI CHANGES 02/12/06
		if (audiosearchpri!= NULL) 
		{
			audiotmppri=(NSString *) [NSString stringWithCString: audio->lang];
			// Try to find the desired default language
			if ([audiotmppri hasPrefix:audiosearchpri] && indxpri==0)
			{
				indxpri=i+1;
			}
		}
	// End of pri changes 02/12/06

        [[fAudLang1PopUp menu] addItemWithTitle:
            [NSString stringWithCString: audio->lang]
            action: NULL keyEquivalent: @""];
       
	   [[fAudLang2PopUp menu] addItemWithTitle:
            [NSString stringWithCString: audio->lang]
            action: NULL keyEquivalent: @""];
		
    }
	// PRI CHANGES 02/12/06
	if (indxpri==0) { indxpri=1; }
	  [fAudLang1PopUp selectItemAtIndex: indxpri];
	// End of pri changes 02/12/06
    [fAudLang2PopUp selectItemAtIndex: 0];
	
	/* END pri */

	/* changing the title may have changed the audio channels on offer, */
	/* so call AudioTrackPopUpChanged for both audio tracks to update the mixdown popups */
	[self AudioTrackPopUpChanged: fAudLang1PopUp];
	[self AudioTrackPopUpChanged: fAudLang2PopUp];

}

- (IBAction) ChapterPopUpChanged: (id) sender
{
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

    [self CalculateBitrate: sender];
}

- (IBAction) FormatPopUpChanged: (id) sender
{
    NSString * string = [fDstFile2Field stringValue];
    int format = [fDstFormatPopUp indexOfSelectedItem];
    char * ext = NULL;

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
            [fDstCodecsPopUp addItemWithTitle:
                _( @"MPEG-4 Video / AAC Audio" )];
            [fDstCodecsPopUp addItemWithTitle:
                _( @"AVC/H.264 Video / AAC Audio" )];
			/* We enable the create chapters checkbox here since we are .mp4*/
			[fCreateChapterMarkers setEnabled: YES];
			break;
        case 1: 
            ext = "avi";
            [fDstCodecsPopUp addItemWithTitle:
                _( @"MPEG-4 Video / MP3 Audio" )];
            [fDstCodecsPopUp addItemWithTitle:
                _( @"MPEG-4 Video / AC-3 Audio" )];
            [fDstCodecsPopUp addItemWithTitle:
                _( @"AVC/H.264 Video / MP3 Audio" )];
            [fDstCodecsPopUp addItemWithTitle:
                _( @"AVC/H.264 Video / AC-3 Audio" )];
			/* We disable the create chapters checkbox here since we are NOT .mp4 
			and make sure it is unchecked*/
			[fCreateChapterMarkers setEnabled: NO];
			[fCreateChapterMarkers setState: NSOffState];
            break;
        case 2:
            ext = "ogm";
            [fDstCodecsPopUp addItemWithTitle:
                _( @"MPEG-4 Video / Vorbis Audio" )];
            [fDstCodecsPopUp addItemWithTitle:
                _( @"MPEG-4 Video / MP3 Audio" )];
            /* We disable the create chapters checkbox here since we are NOT .mp4 
			and make sure it is unchecked*/
			[fCreateChapterMarkers setEnabled: NO];
			[fCreateChapterMarkers setState: NSOffState];
			break;
    }
    [self CodecsPopUpChanged: NULL];

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

	/* changing the format may mean that we can / can't offer mono or 6ch, */
	/* so call AudioTrackPopUpChanged for both audio tracks to update the mixdown popups */
	[self AudioTrackPopUpChanged: fAudLang1PopUp];
	[self AudioTrackPopUpChanged: fAudLang2PopUp];

	/* We call method method to change UI to reflect whether a preset is used or not*/
	[self CustomSettingUsed: sender];	
	
}

- (IBAction) CodecsPopUpChanged: (id) sender
{
    int format = [fDstFormatPopUp indexOfSelectedItem];
    int codecs = [fDstCodecsPopUp indexOfSelectedItem];

    /* Update the encoder popup */
    if( ( FormatSettings[format][codecs] & HB_VCODEC_X264 ) )
    {
        /* MPEG-4 -> H.264 */
        [fVidEncoderPopUp removeAllItems];
		[fVidEncoderPopUp addItemWithTitle: @"x264 (h.264 Main)"];
		[fVidEncoderPopUp addItemWithTitle: @"x264 (h.264 iPod)"];
        
        
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
	/* so call AudioTrackPopUpChanged for both audio tracks to update the mixdown popups */
	[self AudioTrackPopUpChanged: fAudLang1PopUp];
	[self AudioTrackPopUpChanged: fAudLang2PopUp];

    [self CalculateBitrate: sender];
    /* We call method method to change UI to reflect whether a preset is used or not*/
	[self CustomSettingUsed: sender];
}

- (IBAction) EncoderPopUpChanged: (id) sender
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

		/* uncheck the "export 5.1 as 6-channel AAC" checkbox if it is checked */
//		[fAudLang1SurroundCheck setState: NSOffState];

	}
    
	[self CalculatePictureSizing: sender];
	/* We call method method to change UI to reflect whether a preset is used or not*/    
    [self CustomSettingUsed: sender];
}

- (IBAction) SetEnabledStateOfAudioMixdownControls: (id) sender
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

- (IBAction) AudioTrackPopUpChanged: (id) sender
{
    /* utility function to call AudioTrackPopUpChanged without passing in a mixdown-to-use */
    [self AudioTrackPopUpChanged: sender mixdownToUse: 0];
}

- (IBAction) AudioTrackPopUpChanged: (id) sender mixdownToUse: (int) mixdownToUse
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
    if ([thisAudioPopUp indexOfSelectedItem] == [otherAudioPopUp indexOfSelectedItem]) {
        [otherAudioPopUp selectItemAtIndex: 0];
        [self AudioTrackPopUpChanged: otherAudioPopUp];
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
    [self SetEnabledStateOfAudioMixdownControls: NULL];

    if (thisAudioIndex != -1)
    {

        /* get the audio */
        audio = (hb_audio_t *) hb_list_item( fTitle->list_audio, thisAudioIndex );
        if (audio != NULL)
        {

            int audioCodecsSupportMono = (audio->codec == HB_ACODEC_AC3 && acodec == HB_ACODEC_FAAC);
            int audioCodecsSupport6Ch = (audio->codec == HB_ACODEC_AC3 && acodec == HB_ACODEC_FAAC);

            /* check for AC-3 passthru */
            if (audio->codec == HB_ACODEC_AC3 && acodec == HB_ACODEC_AC3)
            {
                    [[mixdownPopUp menu] addItemWithTitle:
                        [NSString stringWithCString: "AC3 Passthru"]
                        action: NULL keyEquivalent: @""];
            }
            else
            {
            
                /* find out if our selected output audio codec supports mono and / or 6ch */
                /* we also check for an input codec of AC3,
                   as it is the only library able to do the mixdown to mono / conversion to 6-ch */
                /* audioCodecsSupportMono and audioCodecsSupport6Ch are the same for now,
                   but this may change in the future, so they are separated for flexibility */

                /* find out the audio channel layout for our input audio */
                /* we'll cheat and use the liba52 layouts, even if the source isn't AC3 */
                int channel_layout;
                int audio_has_lfe;
                if (audio->codec == HB_ACODEC_AC3)
                {
                    channel_layout = (audio->ac3flags & A52_CHANNEL_MASK);
                    audio_has_lfe = (audio->ac3flags & A52_LFE);
                }
                else
                {
                    /* assume a stereo input for all other input codecs */
                    channel_layout = A52_STEREO;
                    audio_has_lfe = 0;
                }

                /* add the appropriate audio mixdown menuitems to the popupbutton */
                /* in each case, we set the new menuitem's tag to be the amixdown value for that mixdown,
                   so that we can reference the mixdown later */

                /* keep a track of the min and max mixdowns we used, so we can select the best match later */
                int minMixdownUsed = 0;
                int maxMixdownUsed = 0;

                /* do we want to add a mono option? */
                if (audioCodecsSupportMono == 1) {
                    id<NSMenuItem> menuItem = [[mixdownPopUp menu] addItemWithTitle:
                        [NSString stringWithCString: hb_audio_mixdowns[0].human_readable_name]
                        action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[0].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[0].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[0].amixdown);
                }

                /* do we want to add a stereo option? */
                if (channel_layout >= A52_STEREO && channel_layout != A52_CHANNEL1 && channel_layout != A52_CHANNEL2) {
                    id<NSMenuItem> menuItem = [[mixdownPopUp menu] addItemWithTitle:
                        [NSString stringWithCString: hb_audio_mixdowns[1].human_readable_name]
                        action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[1].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[1].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[1].amixdown);
                }

                /* do we want to add a dolby surround (DPL1) option? */
                if (channel_layout == A52_3F1R || channel_layout == A52_3F2R || channel_layout == A52_DOLBY) {
                    id<NSMenuItem> menuItem = [[mixdownPopUp menu] addItemWithTitle:
                        [NSString stringWithCString: hb_audio_mixdowns[2].human_readable_name]
                        action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[2].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[2].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[2].amixdown);
                }

                /* do we want to add a dolby pro logic 2 (DPL2) option? */
                if (channel_layout == A52_3F2R) {
                    id<NSMenuItem> menuItem = [[mixdownPopUp menu] addItemWithTitle:
                        [NSString stringWithCString: hb_audio_mixdowns[3].human_readable_name]
                        action: NULL keyEquivalent: @""];
                    [menuItem setTag: hb_audio_mixdowns[3].amixdown];
                    if (minMixdownUsed == 0) minMixdownUsed = hb_audio_mixdowns[3].amixdown;
                    maxMixdownUsed = MAX(maxMixdownUsed, hb_audio_mixdowns[3].amixdown);
                }

                /* do we want to add a 6-channel discrete option? */
                if (audioCodecsSupport6Ch == 1 && channel_layout == A52_3F2R && audio_has_lfe == A52_LFE) {
                    id<NSMenuItem> menuItem = [[mixdownPopUp menu] addItemWithTitle:
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
            
            }

        }
        
    }

	/* see if the new audio track choice will change the bitrate we need */
    [self CalculateBitrate: sender];	

}

/* lets set the picture size back to the max from right after title scan
   Lets use an IBAction here as down the road we could always use a checkbox
   in the gui to easily take the user back to max. Remember, the compiler
   resolves IBActions down to -(void) during compile anyway */
- (IBAction) RevertPictureSizeToMax: (id) sender
{
	 hb_job_t * job = fTitle->job;
	/* We use the output picture width and height
	as calculated from libhb right after title is set
	in TitlePopUpChanged */
	job->width = PicOrigOutputWidth;
	job->height = PicOrigOutputHeight;


    
	[self CalculatePictureSizing: sender];
	/* We call method method to change UI to reflect whether a preset is used or not*/    
    [self CustomSettingUsed: sender];
}


/* Get and Display Current Pic Settings in main window */
- (IBAction) CalculatePictureSizing: (id) sender
{

	//hb_job_t * job = fTitle->job;		

	[fPicSettingWidth setStringValue: [NSString stringWithFormat:
		@"%d", fTitle->job->width]];
	[fPicSettingHeight setStringValue: [NSString stringWithFormat:
		@"%d", fTitle->job->height]];
	[fPicSettingARkeep setStringValue: [NSString stringWithFormat:
		@"%d", fTitle->job->keep_ratio]];		 
	[fPicSettingDeinterlace setStringValue: [NSString stringWithFormat:
		@"%d", fTitle->job->deinterlace]];
	[fPicSettingPAR setStringValue: [NSString stringWithFormat:
		@"%d", fTitle->job->pixel_ratio]];
		
	if (fTitle->job->pixel_ratio == 1)
	{
	int titlewidth = fTitle->width-fTitle->job->crop[2]-fTitle->job->crop[3];
	int arpwidth = fTitle->job->pixel_aspect_width;
	int arpheight = fTitle->job->pixel_aspect_height;
	int displayparwidth = titlewidth * arpwidth / arpheight;
	int displayparheight = fTitle->height-fTitle->job->crop[0]-fTitle->job->crop[1];
	[fPicLabelPAROutp setStringValue: @"Anamorphic Output:"];
	[fPicLabelPAROutputX setStringValue: @"x"];
    [fPicSettingPARWidth setStringValue: [NSString stringWithFormat:
        @"%d", displayparwidth]];
	[fPicSettingPARHeight setStringValue: [NSString stringWithFormat:
        @"%d", displayparheight]];

	fTitle->job->keep_ratio = 0;
	}
	else
	{
	[fPicLabelPAROutp setStringValue: @""];
	[fPicLabelPAROutputX setStringValue: @""];
	[fPicSettingPARWidth setStringValue: @""];
	[fPicSettingPARHeight setStringValue:  @""];
	}
		
	/* Set ON/Off values for the deinterlace/keep aspect ratio according to boolean */	
	if (fTitle->job->keep_ratio > 0)
		{
		[fPicSettingARkeepDsply setStringValue: @"On"];
        }
		else
		{
		[fPicSettingARkeepDsply setStringValue: @"Off"];
		}	
	if (fTitle->job->deinterlace > 0)
		{
		[fPicSettingDeinterlaceDsply setStringValue: @"On"];
        }
		else
		{
		[fPicSettingDeinterlaceDsply setStringValue: @"Off"];
		}
	if (fTitle->job->pixel_ratio > 0)
		{
		[fPicSettingPARDsply setStringValue: @"On"];
        }
		else
		{
		[fPicSettingPARDsply setStringValue: @"Off"];
		}	
		
	[self CustomSettingUsed: sender];
}

- (IBAction) CalculateBitrate: (id) sender
{
    if( !fHandle || [fVidQualityMatrix selectedRow] != 0 )
    {
        return;
    }

    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;

    [self PrepareJob];

    [fVidBitrateField setIntValue: hb_calc_bitrate( job,
            [fVidTargetSizeField intValue] )];
			
			
}

/* Method to determine if we should change the UI
To reflect whether or not a Preset is being used or if
the user is using "Custom" settings by determining the sender*/
- (IBAction) CustomSettingUsed: (id) sender
{
	if ([sender stringValue] != NULL)
	{
		/* Deselect the currently selected Preset if there is one*/
		[tableView deselectRow:[tableView selectedRow]];
		/* Change UI to show "Custom" settings are being used */
		[fPresetSelectedDisplay setStringValue: @"Custom"];
		/* Empty the field to display custom x264 preset options*/
		[fDisplayX264Options setStringValue: @""];
		curUserPresetChosenNum = nil;
		/* If we have MP4, AVC H.264 and x264 Main then we look to see
		if there are any x264 options from the preferences to use */
		if ([fDstFormatPopUp indexOfSelectedItem] == 0 && [fDstCodecsPopUp indexOfSelectedItem] == 1)
		{
		//[fDisplayX264Options setStringValue: [NSString stringWithFormat:[chosenPreset valueForKey:@"x264Option"]]];
			[fDisplayX264Options setStringValue: [NSString stringWithFormat:[[NSUserDefaults standardUserDefaults] stringForKey:@"DefAdvancedx264Flags"]]];
		}
		
	}
}


- (IBAction) ShowAddPresetPanel: (id) sender
{
    /* Deselect the currently selected Preset if there is one*/
		[tableView deselectRow:[tableView selectedRow]];
    /* If we have MP4, AVC H.264 and Main then we enable the x264 Options field for the
	 Add Preset window we are about to open. We do this before we actually open the panel,
	 as doing it after causes it to stick from the last selection for some reason. */
	if ([fDstFormatPopUp indexOfSelectedItem] == 0 && [fDstCodecsPopUp indexOfSelectedItem] == 1)
	{
		[fPresetNewX264Opt setEditable: YES];
		[fPresetNewX264OptLabel setEnabled: YES];
	}
	else
	{
		[fPresetNewX264Opt setEditable: NO];
		[fPresetNewX264OptLabel setEnabled: NO];
	}
		/* Erase info from the input fields */
	[fPresetNewName setStringValue: @""];
	[fPresetNewX264Opt setString:@""];
	/* Show the panel */
	[NSApp beginSheet: fAddPresetPanel modalForWindow: fWindow
        modalDelegate: NULL didEndSelector: NULL contextInfo: NULL];
    [NSApp runModalForWindow: fAddPresetPanel];
    [NSApp endSheet: fAddPresetPanel];
    [fAddPresetPanel orderOut: self];
	
	
}
- (IBAction) CloseAddPresetPanel: (id) sender
{
	/* Erase info from the input fields */
	[fPresetNewName setStringValue: @""];
	[fPresetNewX264Opt setString:@""];
    [NSApp stopModal];
}
   /* We use this method to recreate new, updated factory
   presets */
- (IBAction)AddFactoryPresets:(id)sender
{
    /* First, we delete any existing built in presets */
    [self DeleteFactoryPresets: sender];
    /* Then, we re-create new built in presets programmatically CreatePS3Preset*/
	[UserPresets addObject:[self CreateIpodPreset]];
	[UserPresets addObject:[self CreateAppleTVPreset]];
	[UserPresets addObject:[self CreatePSThreePreset]];
    [self AddPreset];
}
- (IBAction)DeleteFactoryPresets:(id)sender
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

- (IBAction)AddUserPreset:(id)sender
{
    /* Here we create a custom user preset */
	[UserPresets addObject:[self CreatePreset]];

	/* We stop the modal window for the new preset */
	[NSApp stopModal];
    [self AddPreset];
	

}
- (void)AddPreset
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

- (IBAction)InsertPreset:(id)sender
{
    int index = [tableView selectedRow];
    [UserPresets insertObject:[self CreatePreset] atIndex:index];
    [tableView reloadData];
    [self savePreset];
}

- (NSDictionary *)CreatePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:[fPresetNewName stringValue] forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:[fPresetNewPicSettingsApply state]] forKey:@"UsesPictureSettings"];
	/* File Format */
    [preset setObject:[fDstFormatPopUp titleOfSelectedItem] forKey:@"FileFormat"];
	/* Chapter Markers fCreateChapterMarkers*/
	[preset setObject:[NSNumber numberWithInt:[fCreateChapterMarkers state]] forKey:@"ChapterMarkers"];
	/* Codecs */
	[preset setObject:[fDstCodecsPopUp titleOfSelectedItem] forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:[fVidEncoderPopUp titleOfSelectedItem] forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:[fPresetNewX264Opt string] forKey:@"x264Option"];
	//[fDisplayX264Options setStringValue: [NSString stringWithFormat: @"Using Option: %@",CurUserPresetx264Opt]];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:[fVidQualityMatrix selectedRow]] forKey:@"VideoQualityType"];
	[preset setObject:[fVidTargetSizeField stringValue] forKey:@"VideoTargetSize"];
	[preset setObject:[fVidBitrateField stringValue] forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:[fVidQualitySlider floatValue]] forKey:@"VideoQualitySlider"];
	
	/* Video framerate */
	[preset setObject:[fVidRatePopUp titleOfSelectedItem] forKey:@"VideoFramerate"];
	/* GrayScale */
	[preset setObject:[NSNumber numberWithInt:[fVidGrayscaleCheck state]] forKey:@"VideoGrayScale"];
	/* 2 Pass Encoding */
	[preset setObject:[NSNumber numberWithInt:[fVidTwoPassCheck state]] forKey:@"VideoTwoPass"];
	
	/*Picture Settings*/
	hb_job_t * job = fTitle->job;
	/* Basic Picture Settings */
	/* Use Max Picture settings for whatever the dvd is.*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesMaxPictureSettings"];
	[preset setObject:[NSNumber numberWithInt:fTitle->job->width] forKey:@"PictureWidth"];
	[preset setObject:[NSNumber numberWithInt:fTitle->job->height] forKey:@"PictureHeight"];
	[preset setObject:[NSNumber numberWithInt:fTitle->job->keep_ratio] forKey:@"PictureKeepRatio"];
	[preset setObject:[NSNumber numberWithInt:fTitle->job->deinterlace] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:fTitle->job->pixel_ratio] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	[preset setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
    [preset setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
	[preset setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
	[preset setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio Language One*/
	[preset setObject:[fAudLang1PopUp titleOfSelectedItem] forKey:@"AudioLang1"];
	/* Audio Track one mixdown */
    if ([fAudLang1PopUp indexOfSelectedItem] > 0) {
        [preset setObject:[NSString stringWithCString:hb_mixdown_get_short_name_from_mixdown([[fAudTrack1MixPopUp selectedItem] tag])] forKey:@"AudioLang1Mixdown"];
    }
    else
    {
        [preset setObject:[NSString stringWithCString:""] forKey:@"AudioLang1Mixdown"];
    }
	/* Audio Language Two*/
	[preset setObject:[fAudLang2PopUp titleOfSelectedItem] forKey:@"AudioLang2"];
	/* Audio Track Two mixdown */
    if ([fAudLang2PopUp indexOfSelectedItem] > 0) {
        [preset setObject:[NSString stringWithCString:hb_mixdown_get_short_name_from_mixdown([[fAudTrack2MixPopUp selectedItem] tag])] forKey:@"AudioLang2Mixdown"];
    }
    else
    {
        [preset setObject:[NSString stringWithCString:""] forKey:@"AudioLang2Mixdown"];
    }
	/* Audio Sample Rate*/
	[preset setObject:[fAudRatePopUp titleOfSelectedItem] forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:[fAudBitratePopUp titleOfSelectedItem] forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:[fSubPopUp titleOfSelectedItem] forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)CreateIpodPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"HB-iPod" forKey:@"PresetName"];
	/*Set whether or not this is a user preset or factory 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"UsesPictureSettings"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
    /* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 iPod)" forKey:@"VideoEncoder"];
	/* x264 Option String */
	[preset setObject:@"" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
	[preset setObject:[fVidTargetSizeField stringValue] forKey:@"VideoTargetSize"];
	[preset setObject:@"1400" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:[fVidQualitySlider floatValue]] forKey:@"VideoQualitySlider"];
	
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
	//[preset setObject:[NSNumber numberWithInt:fTitle->job->width] forKey:@"PictureWidth"];
	//[preset setObject:[NSNumber numberWithInt:fTitle->job->height] forKey:@"PictureHeight"];
	//[preset setObject:[NSNumber numberWithInt:fTitle->job->keep_ratio] forKey:@"PictureKeepRatio"];
	//[preset setObject:[NSNumber numberWithInt:fTitle->job->deinterlace] forKey:@"PictureDeinterlace"];
	//[preset setObject:[NSNumber numberWithInt:fTitle->job->pixel_ratio] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	//[preset setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
    //[preset setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
	//[preset setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
	//[preset setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio track one*/
	[preset setObject:[fAudLang1PopUp titleOfSelectedItem] forKey:@"AudioLang1"];
	/* Track one mixdown dpl2*/
	[preset setObject:[NSString stringWithCString:"stereo"] forKey:@"AudioLang1Mixdown"];
    /* Audio track two */
	[preset setObject:[NSString stringWithFormat:@"None"] forKey:@"AudioLang2"];
	/* Track two mixdown */
	[preset setObject:[NSString stringWithCString:""] forKey:@"AudioLang2Mixdown"];
	/* Audio Sample Rate*/
	[preset setObject:@"44.1" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"128" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)CreateAppleTVPreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"HB-AppleTV" forKey:@"PresetName"];
	/*Set whether or not this is a user preset where 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
	/* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String (We can use this to tweak the appleTV output)*/
	[preset setObject:@"" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
	[preset setObject:[fVidTargetSizeField stringValue] forKey:@"VideoTargetSize"];
	[preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:[fVidQualitySlider floatValue]] forKey:@"VideoQualitySlider"];
	
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
	//[preset setObject:[NSNumber numberWithInt:PicOrigOutputWidth] forKey:@"PictureWidth"];
	//[preset setObject:[NSNumber numberWithInt:PicOrigOutputHeight] forKey:@"PictureHeight"];
	//[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
	//[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	//[preset setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
    //[preset setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
	//[preset setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
	//[preset setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio track one*/
	[preset setObject:[fAudLang1PopUp titleOfSelectedItem] forKey:@"AudioLang1"];
	/* Track one mixdown dpl2*/
	[preset setObject:[NSString stringWithCString:"dpl2"] forKey:@"AudioLang1Mixdown"];
    /* Audio track two */
	[preset setObject:[NSString stringWithFormat:@"None"] forKey:@"AudioLang2"];
	/* Track two mixdown */
	[preset setObject:[NSString stringWithCString:""] forKey:@"AudioLang2Mixdown"];
	/* Audio Sample Rate*/
	[preset setObject:@"44.1" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}

- (NSDictionary *)CreatePSThreePreset
{
    NSMutableDictionary *preset = [[NSMutableDictionary alloc] init];
	/* Get the New Preset Name from the field in the AddPresetPanel */
    [preset setObject:@"HB-PS3" forKey:@"PresetName"];
	/*Set whether or not this is a user preset where 0 is factory, 1 is user*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Type"];
	/*Set whether or not this is default, at creation set to 0*/
	[preset setObject:[NSNumber numberWithInt:0] forKey:@"Default"];
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"UsesPictureSettings"];
	/* File Format */
    [preset setObject:@"MP4 file" forKey:@"FileFormat"];
	/* Chapter Markers*/
	 [preset setObject:[NSNumber numberWithInt:1] forKey:@"ChapterMarkers"];
	/* Codecs */
	[preset setObject:@"AVC/H.264 Video / AAC Audio" forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:@"x264 (h.264 Main)" forKey:@"VideoEncoder"];
	/* x264 Option String (We can use this to tweak the appleTV output)*/
	[preset setObject:@"level=41" forKey:@"x264Option"];
	/* Video quality */
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"VideoQualityType"];
	[preset setObject:[fVidTargetSizeField stringValue] forKey:@"VideoTargetSize"];
	[preset setObject:@"2500" forKey:@"VideoAvgBitrate"];
	[preset setObject:[NSNumber numberWithFloat:[fVidQualitySlider floatValue]] forKey:@"VideoQualitySlider"];
	
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
	//[preset setObject:[NSNumber numberWithInt:PicOrigOutputWidth] forKey:@"PictureWidth"];
	//[preset setObject:[NSNumber numberWithInt:PicOrigOutputHeight] forKey:@"PictureHeight"];
	//[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureKeepRatio"];
	//[preset setObject:[NSNumber numberWithInt:0] forKey:@"PictureDeinterlace"];
	[preset setObject:[NSNumber numberWithInt:1] forKey:@"PicturePAR"];
	/* Set crop settings here */
	/* The Auto Crop Matrix in the Picture Window autodetects differences in crop settings */
	//[preset setObject:[NSNumber numberWithInt:job->crop[0]] forKey:@"PictureTopCrop"];
    //[preset setObject:[NSNumber numberWithInt:job->crop[1]] forKey:@"PictureBottomCrop"];
	//[preset setObject:[NSNumber numberWithInt:job->crop[2]] forKey:@"PictureLeftCrop"];
	//[preset setObject:[NSNumber numberWithInt:job->crop[3]] forKey:@"PictureRightCrop"];
	
	/*Audio*/
	/* Audio track one*/
	[preset setObject:[fAudLang1PopUp titleOfSelectedItem] forKey:@"AudioLang1"];
	/* Track one mixdown dpl2*/
	[preset setObject:[NSString stringWithCString:"dpl2"] forKey:@"AudioLang1Mixdown"];
    /* Audio track two */
	[preset setObject:[NSString stringWithFormat:@"None"] forKey:@"AudioLang2"];
	/* Track two mixdown */
	[preset setObject:[NSString stringWithCString:""] forKey:@"AudioLang2Mixdown"];
	/* Audio Sample Rate*/
	[preset setObject:@"44.1" forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:@"160" forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:@"None" forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

}


- (IBAction)DeletePreset:(id)sender
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
- (IBAction)tableViewSelected:(id)sender
{
    /* Since we cannot disable the presets tableView in terms of clickability
	   we will use the enabled state of the add presets button to determine whether
	   or not clicking on a preset will do anything */
	if ([fPresetsAdd isEnabled])
	{
		
		/* we get the chosen preset from the UserPresets array */
		chosenPreset = [UserPresets objectAtIndex:[sender selectedRow]];
		curUserPresetChosenNum = [sender selectedRow];
		/* we set the preset display field in main window here */
		[fPresetSelectedDisplay setStringValue: [NSString stringWithFormat: @"%@",[chosenPreset valueForKey:@"PresetName"]]];
		/* File Format */
		[fDstFormatPopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"FileFormat"]]];
		[self FormatPopUpChanged: NULL];
		/* Chapter Markers*/
		[fCreateChapterMarkers setState:[[chosenPreset objectForKey:@"ChapterMarkers"] intValue]];
	    /* Codecs */
		[fDstCodecsPopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"FileCodecs"]]];
		[self CodecsPopUpChanged: NULL];
		/* Video encoder */
		[fVidEncoderPopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoEncoder"]]];
		
		/* We can show the preset options here in the gui if we want to
		Field is currently hidden from user, unhide it if we need to test */
		[fDisplayX264Options setStringValue: [NSString stringWithFormat:[chosenPreset valueForKey:@"x264Option"]]];
		/* Lets run through the following functions to get variables set there */
		[self EncoderPopUpChanged: NULL];
		[self CalculateBitrate: NULL];
		
		/* Video quality */
		[fVidQualityMatrix selectCellAtRow:[[chosenPreset objectForKey:@"VideoQualityType"] intValue] column:0];
		
		[fVidTargetSizeField setStringValue: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoTargetSize"]]];
		[fVidBitrateField setStringValue: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoAvgBitrate"]]];
		
		[fVidQualitySlider setFloatValue: [[chosenPreset valueForKey:@"VideoQualitySlider"] floatValue]];
		[self VideoMatrixChanged: NULL];
		
		/* Video framerate */
		[fVidRatePopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoFramerate"]]];

		/* GrayScale */
		[fVidGrayscaleCheck setState:[[chosenPreset objectForKey:@"VideoGrayScale"] intValue]];
		
		/* 2 Pass Encoding */
		[fVidTwoPassCheck setState:[[chosenPreset objectForKey:@"VideoTwoPass"] intValue]];
		
		
		/*Audio*/
		/* Audio Language One*/
		[fAudLang1PopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"AudioLang1"]]];
		/* We check to make sure something is selected for track 1 */
		if ([fAudLang1PopUp indexOfSelectedItem] == -1)
		{
			/* If not we choose the first source in the track 1 dropdown */
			[fAudLang1PopUp selectItemAtIndex: 0];
		}

        /* if the preset contains a mixdown value for track 1, then try and load it */
        /* if the preset contains the empty string for this value, then we'll get
           a mixdown of 0 from hb_mixdown_get_mixdown_from_short_name,
           which will be correctly ignored by AudioTrackPopUpChanged */
        /* if the mixdown is unavailable, AudioTrackPopUpChanged will choose the next best mixdown */
        char cBuffer1[32];
        NSString * short_name1 = [NSString stringWithFormat:[chosenPreset valueForKey:@"AudioLang1Mixdown"]];
        [short_name1 getCString:cBuffer1];
        int mixdown1 = hb_mixdown_get_mixdown_from_short_name(cBuffer1);
        [self AudioTrackPopUpChanged: fAudLang1PopUp mixdownToUse: mixdown1];

		/* Audio Language Two*/
		[fAudLang2PopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"AudioLang2"]]];
		/* We check to make sure something is selected for track 2 */
		if ([fAudLang2PopUp indexOfSelectedItem] == -1)
		{
			/* If not we choose "None" in the track 2 dropdown */
			[fAudLang2PopUp selectItemWithTitle: [NSString stringWithFormat:@"None"]];
			//[self SetEnabledStateOfAudioMixdownControls: sender];
		}
		/* if the preset contains a mixdown value for track 2, then try and load it */
        /* if the preset contains the empty string for this value, then we'll get
           a mixdown of 0 from hb_mixdown_get_mixdown_from_short_name,
           which will be correctly ignored by AudioTrackPopUpChanged */
        /* if the mixdown is unavailable, AudioTrackPopUpChanged will choose the next best mixdown */
        char cBuffer2[32];
        NSString * short_name2 = [NSString stringWithFormat:[chosenPreset valueForKey:@"AudioLang2Mixdown"]];
        [short_name2 getCString:cBuffer2];
        int mixdown2 = hb_mixdown_get_mixdown_from_short_name(cBuffer2);
        [self AudioTrackPopUpChanged: fAudLang2PopUp mixdownToUse: mixdown2];
	
		
		/* Audio Sample Rate*/
		[fAudRatePopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"AudioSampleRate"]]];
		/* Audio Bitrate Rate*/
		[fAudBitratePopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"AudioBitRate"]]];
		/*Subtitles*/
		[fSubPopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"Subtitles"]]];
		
		/* Picture Settings */
		/* Look to see if we apply these here in objectForKey:@"UsesPictureSettings"] */
		if ([[chosenPreset objectForKey:@"UsesPictureSettings"]  intValue] == 1)
		{
			hb_job_t * job = fTitle->job;
			/* Check to see if we should use the max picture setting for the current title*/
			if ([[chosenPreset objectForKey:@"UsesMaxPictureSettings"]  intValue] == 1)
			{
				/* Use Max Picture settings for whatever the dvd is.*/
				[self RevertPictureSizeToMax: NULL];
				job->keep_ratio = [[chosenPreset objectForKey:@"PictureKeepRatio"]  intValue];
				if (job->keep_ratio == 1)
				{
					hb_fix_aspect( job, HB_KEEP_WIDTH );
				}
				job->pixel_ratio = [[chosenPreset objectForKey:@"PicturePAR"]  intValue];
			}
			else
			{
				job->width = [[chosenPreset objectForKey:@"PictureWidth"]  intValue];
				job->height = [[chosenPreset objectForKey:@"PictureHeight"]  intValue];
				job->keep_ratio = [[chosenPreset objectForKey:@"PictureKeepRatio"]  intValue];
				if (job->keep_ratio == 1)
				{
					hb_fix_aspect( job, HB_KEEP_WIDTH );
				}
				job->pixel_ratio = [[chosenPreset objectForKey:@"PicturePAR"]  intValue];
				job->crop[0] = [[chosenPreset objectForKey:@"PictureTopCrop"]  intValue];
				job->crop[1] = [[chosenPreset objectForKey:@"PictureBottomCrop"]  intValue];
				job->crop[2] = [[chosenPreset objectForKey:@"PictureLeftCrop"]  intValue];
				job->crop[3] = [[chosenPreset objectForKey:@"PictureRightCrop"]  intValue];
			}
			[self CalculatePictureSizing: NULL]; 
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
   if ([[userPresetDict objectForKey:@"Type"] intValue] == 0)
	{
		[aCell setTextColor:[NSColor blueColor]];
	}
	else
	{
		[aCell setTextColor:[NSColor blackColor]];
	}

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


- (void)savePreset
{
    [UserPresets writeToFile:UserPresetsFile atomically:YES];

}



- (void) controlTextDidBeginEditing: (NSNotification *) notification
{
    [self CalculateBitrate: NULL];
}

- (void) controlTextDidEndEditing: (NSNotification *) notification
{
    [self CalculateBitrate: NULL];
}

- (void) controlTextDidChange: (NSNotification *) notification
{
    [self CalculateBitrate: NULL];
}

- (IBAction) OpenHomepage: (id) sender
{
    [[NSWorkspace sharedWorkspace] openURL: [NSURL
        URLWithString:@"http://handbrake.m0k.org/"]];
}

- (IBAction) OpenForums: (id) sender
{
    [[NSWorkspace sharedWorkspace] openURL: [NSURL
        URLWithString:@"http://handbrake.m0k.org/forum/"]];
}



@end
