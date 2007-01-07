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
		/* TEMPORARILY COMMENT OUT AS UPDATE CHECK IS NOT ACCURATE */
		 /*
        NSBeginInformationalAlertSheet( _( @"Update is available" ),
            _( @"Go get it!" ), _( @"Discard" ), NULL, fWindow, self,
            @selector( UpdateAlertDone:returnCode:contextInfo: ),
            NULL, NULL, [NSString stringWithFormat:
            _( @"HandBrake %s (build %d) is now available for download." ),
            version, build] );
        return;
		*/
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

    /* Destination box */
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
				[fDstFile2Field setStringValue: [NSString stringWithFormat:
                @"%@/Desktop/%@.mp4", NSHomeDirectory(),[NSString
                  stringWithUTF8String: title->name]]];
				
			    [fSrcTitlePopUp addItemWithTitle: [NSString
                    stringWithFormat: @"%d - %02dh%02dm%02ds",
                    title->index, title->hours, title->minutes,
                    title->seconds]];
					
					
            
			
			}

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
            float progress_total;
            NSMutableString * string;
			
            /* Update text field */
            string = [NSMutableString stringWithFormat:
                _( @"Muxing: %.2f %%" ), 100.0 * p.progress];
            [fStatusField setStringValue: string];
			
            /* Update slider */
            [fRipIndicator setDoubleValue: 100.0 * p.progress];
			
            /* Update dock icon */
            [self UpdateDockIcon: 100.0 * p.progress];
			
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
        fAudBitratePopUp, fPictureButton };

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

    [panel beginSheetForDirectory: NULL file: NULL
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
	 if ([fVidEncoderPopUp indexOfSelectedItem] < 1 )
	    {
		/* Just use new Baseline Level 3.0 
		Lets Deprecate Baseline Level 1.3*/
		job->h264_level = 30;
		job->mux = HB_MUX_IPOD;
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
    hb_list_t  * list  = hb_get_titles( fHandle );
    hb_title_t * title = (hb_title_t *) hb_list_item( list,
            [fSrcTitlePopUp indexOfSelectedItem] );
    hb_job_t * job = title->job;

    [self PrepareJob];

    /* Destination file */
    job->file = strdup( [[fDstFile2Field stringValue] UTF8String] );

    if( [fVidTwoPassCheck state] == NSOnState )
    {
        job->pass = 1;
        hb_add( fHandle, job );
        job->pass = 2;
        hb_add( fHandle, job );
    }
    else
    {
        job->pass = 0;
        hb_add( fHandle, job );
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

    [self _Rip];
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

    /* Update lang popups */
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
		audiotmppri=(NSString *) [NSString stringWithCString: audio->lang];
		// Try to find the desired default language
	   if ([audiotmppri hasPrefix:audiosearchpri] && indxpri==0)
		{
			indxpri=i+1;
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
            ext = "mp4";
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
		[fVidEncoderPopUp addItemWithTitle: @"x264 (h.264 Baseline iPod)"];
        [fVidEncoderPopUp addItemWithTitle: @"x264 (h.264 Main)"];
        
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

    [self CalculateBitrate: sender];
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
