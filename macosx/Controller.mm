/* $Id: Controller.mm,v 1.79 2005/11/04 19:41:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include "Controller.h"

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
        fAudRateField, fAudRatePopUp, fAudBitrateField,
        fAudBitratePopUp, fPictureButton, fQueueCheck, 
		fPicSrcWidth,fPicSrcHeight,fPicSettingWidth,fPicSettingHeight,
		fPicSettingARkeep,fPicSettingDeinterlace,fPicSettingARkeepDsply,
		fPicSettingDeinterlaceDsply,fPicLabelSettings,fPicLabelSrc,fPicLabelOutp,
		fPicLabelAr,fPicLabelDeinter,fPicLabelSrcX,fPicLabelOutputX,
		fPicLabelPAROutp,fPicLabelPAROutputX,fPicSettingPARWidth,fPicSettingPARHeight,
		fPicSettingPARDsply,fPicLabelAnamorphic,tableView,fPresetsAdd,fPresetsDelete};

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
		/* if we're enabling the interface, check if we should / should't offer 6-channel AAC extraction */
		[self Check6ChannelAACExtraction: NULL];
	
	} else {
		/* if we're disabling the interface, turn it off */
		[fAudLang1SurroundCheck setEnabled: NO];
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
}

- (IBAction) QualitySliderChanged: (id) sender
{
    [fVidConstantCell setTitle: [NSString stringWithFormat:
        _( @"Constant quality: %.0f %%" ), 100.0 *
        [fVidQualitySlider floatValue]]];
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
		[self FormatPopUpChanged: NULL];
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
		
		/* Sends x264 options to the core library*/
		job->x264opts = [[[NSUserDefaults standardUserDefaults] stringForKey:@"DefAdvancedx264Flags"] UTF8String];
		
		
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
	/* have we selected that 5.1 should be extracted as AAC? */
	if (job->acodec == HB_ACODEC_FAAC && [fAudLang1SurroundCheck isEnabled] && [fAudLang1SurroundCheck state] == NSOnState) {
		job->surround = 1;
	} else {
		job->surround = 0;
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
			job->x264opts = [[[NSUserDefaults standardUserDefaults] stringForKey:@"DefAdvancedx264Flags"] UTF8String];
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
	/* We get the destination directory from the destingation field here */
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
	[self EncoderPopUpChanged: NULL];
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

	/* changing the title may have changed the audio channels on offer, so */
	/* check if this change means we should / should't offer 6-channel AAC extraction */
	[self Check6ChannelAACExtraction: sender];

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
            break;
        case 2:
            ext = "ogm";
            [fDstCodecsPopUp addItemWithTitle:
                _( @"MPEG-4 Video / Vorbis Audio" )];
            [fDstCodecsPopUp addItemWithTitle:
                _( @"MPEG-4 Video / MP3 Audio" )];
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

	/* changing the codecs on offer may mean that we are/aren't now offering AAC, so */
	/* check if this change means we should / should't offer 6-channel AAC extraction */
	[self Check6ChannelAACExtraction: sender];

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

	/* check if this change means we should / should't offer 6-channel AAC extraction */
	[self Check6ChannelAACExtraction: sender];

    [self CalculateBitrate: sender];

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
		 
		 if (fTitle->job->width > 640)
				{
				fTitle->job->width = 640;
				}
		 fTitle->job->keep_ratio = 1;
		 hb_fix_aspect( job, HB_KEEP_WIDTH );
		 
		 }

		/* uncheck the "export 5.1 as 6-channel AAC" checkbox if it is checked */
		[fAudLang1SurroundCheck setState: NSOffState];

	}
    
	[self CalculatePictureSizing: sender];    
  
}

- (IBAction) Check6ChannelAACExtraction: (id) sender
{

	/* make sure we have a selected title before continuing */
	if (fTitle == NULL) return;

	/* get the current title's job into a convenience variable */
	hb_job_t * job = fTitle->job;
	
    /* get the current audio tracks */
	/* this is done in PrepareJob too, but we want them here to check against below */
    job->audios[0] = [fAudLang1PopUp indexOfSelectedItem] - 1;
    job->audios[1] = [fAudLang2PopUp indexOfSelectedItem] - 1;
    job->audios[2] = -1;

	/* now, let's check if any selected audio track has 5.1 sound */
	hb_audio_t * audio;
	bool foundfiveoneaudio = false;

	/* find out what the currently-selected audio codec is */
    int format = [fDstFormatPopUp indexOfSelectedItem];
    int codecs = [fDstCodecsPopUp indexOfSelectedItem];
	int acodec = FormatSettings[format][codecs] & HB_ACODEC_MASK;

	/* we only offer the option to extract 5.1 to 6-channel if the selected audio codec is AAC */
	if (acodec == HB_ACODEC_FAAC) {

		bool doneaudios = false;
		int thisaudio = 0;
		
		while (!doneaudios) {

			if (job->audios[thisaudio] == -1) {
				doneaudios = true;
			} else {
				audio = (hb_audio_t *) hb_list_item( fTitle->list_audio, job->audios[thisaudio] );
				if (audio != NULL) {
					if (audio->channels == 5 && audio->lfechannels == 1) {
						foundfiveoneaudio = true;
						doneaudios = true; /* as it doesn't matter if we find any more! */
					}
				}
			}

			thisaudio++;
		}
	}

    /* If we are extracting to AAC, and any of our soundtracks were 5.1, then enable the checkbox  */
	if (foundfiveoneaudio) {
		[fAudLang1SurroundCheck setEnabled: YES];
		/* Check default surround sound pref and if it is YES, lets also check the checkbox */
		if ([[NSUserDefaults standardUserDefaults] boolForKey:@"DefaultSurroundSound"] > 0)
		{
			[fAudLang1SurroundCheck setState: NSOnState];
		}
	} else {
		[fAudLang1SurroundCheck setEnabled: NO];
		/* as well as disabling the checkbox, let's uncheck it if it is checked */
		[fAudLang1SurroundCheck setState: NSOffState];
		
	}

}


- (IBAction) LanguagePopUpChanged: (id) sender
{
	
	/* selecting a different language may mean we have a different number of channels, so */
	/* check if this change means we should / should't offer 6-channel AAC extraction */
	[self Check6ChannelAACExtraction: sender];
	
	/* see if the new language setting will change the bitrate we need */
    [self CalculateBitrate: sender];	

}


/* Get and Display Current Pic Settings in main window */
- (IBAction) CalculatePictureSizing: (id) sender
{

	hb_job_t * job = fTitle->job;		

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
	[fPicSettingHeight setStringValue: [NSString stringWithFormat:
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

- (IBAction) ShowAddPresetPanel: (id) sender
{
    /* Show the panel */
    [NSApp beginSheet: fAddPresetPanel modalForWindow: fWindow
        modalDelegate: NULL didEndSelector: NULL contextInfo: NULL];
    [NSApp runModalForWindow: fAddPresetPanel];
    [NSApp endSheet: fAddPresetPanel];
    [fAddPresetPanel orderOut: self];
}
- (IBAction) CloseAddPresetPanel: (id) sender
{
    [NSApp stopModal];
}

- (IBAction)addPreset:(id)sender
{
    [UserPresets addObject:[self CreatePreset]];
	/* We Sort the Presets Alphabetically by name */
	NSSortDescriptor * lastNameDescriptor=[[[NSSortDescriptor alloc] initWithKey:@"PresetName" 
                                                    ascending:YES selector:@selector(caseInsensitiveCompare:)] autorelease];
	NSArray *sortDescriptors=[NSArray arrayWithObject:lastNameDescriptor];
	NSArray *sortedArray=[UserPresets sortedArrayUsingDescriptors:sortDescriptors];
	[UserPresets setArray:sortedArray];
	
	/* We stop the modal window for the new preset */
	[fPresetNewName    setStringValue: @""];
	[NSApp stopModal];
	/* We Reload the New Table data for presets */
    [tableView reloadData];
   /* We save all of the preset data here */
    [self savePreset];
}

- (IBAction)insertPreset:(id)sender
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
	/*Get the whether or not to apply pic settings in the AddPresetPanel*/
	[preset setObject:[NSNumber numberWithInt:[fPresetNewPicSettingsApply state]] forKey:@"UsesPictureSettings"];
	/* File Format */
    [preset setObject:[fDstFormatPopUp titleOfSelectedItem] forKey:@"FileFormat"];
	/* Codecs */
	[preset setObject:[fDstCodecsPopUp titleOfSelectedItem] forKey:@"FileCodecs"];
	/* Video encoder */
	[preset setObject:[fVidEncoderPopUp titleOfSelectedItem] forKey:@"VideoEncoder"];
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
	/* Audio Language One Surround Sound Checkbox*/
	[preset setObject:[NSNumber numberWithInt:[fAudLang1SurroundCheck state]] forKey:@"AudioLang1Surround"];
	/* Audio Sample Rate*/
	[preset setObject:[fAudRatePopUp titleOfSelectedItem] forKey:@"AudioSampleRate"];
	/* Audio Bitrate Rate*/
	[preset setObject:[fAudBitratePopUp titleOfSelectedItem] forKey:@"AudioBitRate"];
	/* Subtitles*/
	[preset setObject:[fSubPopUp titleOfSelectedItem] forKey:@"Subtitles"];
	

    [preset autorelease];
    return preset;

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
- (IBAction)tableViewSelected:(id)sender
{
    /* Since we cannot disable the presets tableView in terms of clickability
	   we will use the enabled state of the add presets button to determine whether
	   or not clicking on a preset will do anything */
	if ([fPresetsAdd isEnabled])
	{
		
		/* we get the chosen preset from the UserPresets array */
		chosenPreset = [UserPresets objectAtIndex:[sender selectedRow]];
		/* we set the preset display field in main window here */
		//[fPresetSelectedDisplay setStringValue: [NSString stringWithFormat: @"%@", [chosenPreset valueForKey:@"PresetName"]]];
		/* File Format */
		[fDstFormatPopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"FileFormat"]]];
		[self FormatPopUpChanged: NULL];
		/* Codecs */
		[fDstCodecsPopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"FileCodecs"]]];
		[self CodecsPopUpChanged: NULL];
		/* Video encoder */
		[fVidEncoderPopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoEncoder"]]];
		/* Lets run through the following functions to get variables set there */
		[self EncoderPopUpChanged: sender];
		[self Check6ChannelAACExtraction: sender];
		[self CalculateBitrate: sender];
		
		/* Video quality */
		[fVidQualityMatrix selectCellAtRow:[[chosenPreset objectForKey:@"VideoQualityType"] intValue] column:0];
		
		[fVidTargetSizeField setStringValue: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoTargetSize"]]];
		[fVidBitrateField setStringValue: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoAvgBitrate"]]];
		
		[fVidQualitySlider setFloatValue: [[chosenPreset valueForKey:@"VideoQualitySlider"] floatValue]];
		[self VideoMatrixChanged: sender];
		
		/* Video framerate */
		[fVidRatePopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"VideoFramerate"]]];
		
		/* GrayScale */
		[fVidGrayscaleCheck setState:[[chosenPreset objectForKey:@"VideoGrayScale"] intValue]];
		
		/* 2 Pass Encoding */
		[fVidTwoPassCheck setState:[[chosenPreset objectForKey:@"VideoTwoPass"] intValue]];
		
		
		/*Audio*/
		/* Audio Language One*/
		[fAudLang1PopUp selectItemWithTitle: [NSString stringWithFormat:[chosenPreset valueForKey:@"AudioLang1"]]];
		/* Audio Language One Surround Sound Checkbox*/
		[fAudLang1SurroundCheck setState:[[chosenPreset objectForKey:@"AudioLang1Surround"] intValue]];
		[self Check6ChannelAACExtraction: sender];
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
			[self CalculatePictureSizing: sender]; 
		}
		
		// Deselect the currently selected table //
		//[tableView deselectRow:[tableView selectedRow]];
}
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    return [UserPresets count];
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
    [theRecord setObject:anObject forKey:[aTableColumn identifier]];
    
		/* We Sort the Presets Alphabetically by name */
	NSSortDescriptor * lastNameDescriptor=[[[NSSortDescriptor alloc] initWithKey:@"PresetName" 
                                                    ascending:YES selector:@selector(caseInsensitiveCompare:)] autorelease];
	NSArray *sortDescriptors=[NSArray arrayWithObject:lastNameDescriptor];
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
