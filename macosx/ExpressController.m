#import "ExpressController.h"
#import "DriveDetector.h"

#define INSERT_STRING @"Insert a DVD"

@interface ExpressController (Private)

- (void) openUpdateDrives: (NSDictionary *) drives;
- (void) openBrowseDidEnd: (NSOpenPanel *) sheet returnCode: (int)
    returnCode contextInfo: (void *) contextInfo;
- (void) openEnable: (BOOL) b;
- (void) openTimer: (NSTimer *) timer;

- (void) convertShow;
- (void) convertEnable: (BOOL) b;
- (void) convertTimer: (NSTimer *) timer;

@end

@implementation ExpressController

/***********************************************************************
 * Application delegate methods
 **********************************************************************/
- (void) awakeFromNib
{
    NSEnumerator * enumerator;

    /* Show the "Open DVD" interface */
    fDriveDetector = [[DriveDetector alloc] initWithCallback: self
        selector: @selector( openUpdateDrives: )];
    [fDriveDetector run];
    [self openEnable: YES];
    [fWindow setContentSize: [fOpenView frame].size];
    [fWindow setContentView: fOpenView];
    [fWindow center];
    [fWindow makeKeyAndOrderFront: nil];

    /* NSTableView initializations */
     NSButtonCell * buttonCell;
     NSTableColumn * tableColumn;
     enumerator = [[fConvertTableView tableColumns] objectEnumerator];
     while( ( tableColumn = [enumerator nextObject] ) )
     {
         [tableColumn setEditable: NO];
     }
     tableColumn = [fConvertTableView tableColumnWithIdentifier: @"Check"];
     buttonCell = [[[NSButtonCell alloc] initTextCell: @""] autorelease];
    [buttonCell setEditable: YES];
    [buttonCell setButtonType: NSSwitchButton];
    [tableColumn setDataCell: buttonCell];

    /* Preferences */
    fConvertFolderString = [@"~/Movies" stringByExpandingTildeInPath];
    [fConvertFolderString retain];
}

- (void) applicationWillFinishLaunching: (NSNotification *) n
{
    fHandle = hb_init_express( HB_DEBUG_ALL, 0);//HB_DEBUG_NONE, 0 );
    fList   = hb_get_titles( fHandle );
}

- (void) applicationWillTerminate: (NSNotification *) n
{
    hb_close( &fHandle );
}

/***********************************************************************
 * Tableview datasource methods
 **********************************************************************/
- (int) numberOfRowsInTableView: (NSTableView *) t
{
    if( !fHandle )
        return 0;

    return hb_list_count( fList );
}

- (id) tableView:(NSTableView *) t objectValueForTableColumn:
    (NSTableColumn *) col row: (int) row
{
    if( [[col identifier] isEqualToString: @"Check"] )
    {
        return [fConvertCheckArray objectAtIndex: row];
    }
    else
    {
        hb_title_t * title = hb_list_item( fList, row );
        if( [[col identifier] isEqualToString: @"Title"] )
        {
            return [@"Title " stringByAppendingFormat: @"%d",
                    title->index];
        }
        else if( [[col identifier] isEqualToString: @"Duration"] )
        {
            if( title->hours > 0 )
            {
                return [NSString stringWithFormat:
                    @"%d hour%s %d min%s", title->hours,
                    title->hours > 1 ? "s" : "", title->minutes,
                    title->minutes > 1 ? "s": ""];
            }
            else if( title->minutes > 0 )
            {
                return [NSString stringWithFormat:
                    @"%d min%s %d sec%s", title->minutes,
                    title->minutes > 1 ? "s" : "", title->seconds,
                    title->seconds > 1 ? "s": ""];
            }
            else
            {
                return [NSString stringWithFormat: @"%d seconds",
                        title->seconds];
            }
        }
    }
    return nil;
}

- (void) tableView: (NSTableView *) t setObjectValue: (id) object
    forTableColumn: (NSTableColumn *) col row: (int) row
{
    if( [[col identifier] isEqualToString: @"Check"] )
    {
        [fConvertCheckArray replaceObjectAtIndex: row withObject: object];
    }
}

/***********************************************************************
 * User events methods
 **********************************************************************/
- (void) openShow: (id) sender
{
    NSRect frame  = [fWindow frame];
    float  offset = [fConvertView frame].size.height -
                    [fOpenView frame].size.height;

    frame.origin.y    += offset;
    frame.size.height -= offset;
    [fWindow setContentView: fEmptyView];
    [fWindow setFrame: frame display: YES animate: YES];
    [fWindow setContentView: fOpenView];

    [fDriveDetector run];
}

- (void) openMatrixChanged: (id) sender
{
    [self openEnable: YES];
    if( [fOpenMatrix selectedRow] )
    {
        [self openBrowse: self];
    }
}

- (void) openBrowse: (id) sender
{
    NSOpenPanel * panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: YES ];
    [panel beginSheetForDirectory: nil file: nil types: nil
        modalForWindow: fWindow modalDelegate: self
        didEndSelector: @selector( openBrowseDidEnd:returnCode:contextInfo: )
        contextInfo: nil];                                                      
}

- (void) openGo: (id) sender
{
    [self openEnable: NO];
    [fOpenIndicator setIndeterminate: YES];
    [fOpenIndicator startAnimation: nil];
    [fOpenProgressField setStringValue: @"Opening..."];
    [fDriveDetector stop];

    if( [fOpenMatrix selectedRow] )
    {
        hb_scan( fHandle, [fOpenFolderString UTF8String], 0 );
    }
    else
    {
        hb_scan( fHandle, [[fDrives objectForKey: [fOpenPopUp
                 titleOfSelectedItem]] UTF8String], 0 );
    }

    NSTimer * timer = [NSTimer scheduledTimerWithTimeInterval: 2.0
        target: self selector: @selector( openTimer: ) userInfo: nil
        repeats: YES];
}

- (void) convertGo: (id) sender
{
    int i, j;

    for( i = 0; i < hb_list_count( fList ); i++ )
    {
        if( ![[fConvertCheckArray objectAtIndex: i] boolValue] )
            continue;

        hb_title_t * title = hb_list_item( fList, i );
        hb_job_t   * job   = title->job;

        int pixels = 307200;
		int aspect = title->aspect;
		if( [fConvertAspectPopUp indexOfSelectedItem] == 1)
		{
            aspect = 4 * HB_ASPECT_BASE / 3;
		}

		int maxwidth = 640;
		job->vbitrate = 1000;
		if( [fConvertMaxWidthPopUp indexOfSelectedItem] == 1)
		{
            maxwidth = 320;
			job->vbitrate = 500;
		}
		job->deinterlace = 1;
		
		do
		{
			hb_set_size( job, aspect, pixels );
			pixels -= 10;
		} while(job->width > maxwidth);
		
        if( [fConvertFormatPopUp indexOfSelectedItem] == 0 )
        {
            /* iPod / H.264 */
            job->mux      = HB_MUX_IPOD;
            job->vcodec   = HB_VCODEC_X264;
			job->h264_level = 30;
        }
        else if( [fConvertFormatPopUp indexOfSelectedItem] == 1 )
        {
            /* iPod / MPEG-4 */
            job->mux      = HB_MUX_MP4;
            job->vcodec   = HB_VCODEC_FFMPEG;
        }
        else
        {
            /* PSP / MPEG-4 */
            job->mux        = HB_MUX_PSP;
            job->vrate      = 27000000;
            job->vrate_base = 900900;   /* 29.97 fps */
            job->vcodec     = HB_VCODEC_FFMPEG;
            job->vbitrate   = 600;
            pixels          = 76800;
            job->arate      = 24000;
            job->abitrate   = 96;
            aspect          = 16 * HB_ASPECT_BASE / 9;

			if( [fConvertAspectPopUp indexOfSelectedItem] )
			{
				aspect = -1;
			}

			hb_set_size( job, aspect, pixels );
        }

        job->vquality = -1.0;

        const char * lang;

        /* Audio selection */
        hb_audio_t * audio;
        lang = [[fConvertAudioPopUp titleOfSelectedItem] UTF8String];
        job->audios[0] = -1;
        for( j = 0; j < hb_list_count( title->list_audio ); j++ )
        {
            /* Choose the first track that matches the language */
            audio = hb_list_item( title->list_audio, j );
            if( !strcmp( lang, audio->lang_simple ) )
            {
                job->audios[0] = j;
                break;
            }
        }
        if( job->audios[0] == -1 )
        {
            /* If the language isn't available in this title, choose
               the first track */
            job->audios[0] = 0;
        }
        job->audios[1] = -1;

        /* Subtitle selection */
        hb_subtitle_t * subtitle;
        lang = [[fConvertSubtitlePopUp titleOfSelectedItem] UTF8String];
        job->subtitle = -1;
        for( j = 0; j < hb_list_count( title->list_subtitle ); j++ )
        {
            /* Choose the first track that matches the language */
            subtitle = hb_list_item( title->list_subtitle, j );
            if( !strcmp( lang, subtitle->lang ) )
            {
                job->subtitle = j;
                break;
            }
        }
        
        job->file = strdup( [[NSString stringWithFormat:                 
                @"%@/%s - Title %d.m4v", fConvertFolderString,      
                title->name, title->index] UTF8String] );
        hb_add( fHandle, job );
    }

    hb_start( fHandle );

    NSTimer * timer = [NSTimer scheduledTimerWithTimeInterval: 2.0
        target: self selector: @selector( convertTimer: ) userInfo: nil
        repeats: YES];

    [self convertEnable: NO];
}

- (void) convertCancel: (id) sender
{
    hb_stop( fHandle );
    [self convertEnable: YES];
}

@end

/***********************************************************************
 * Private methods
 **********************************************************************/

@implementation ExpressController (Private)

- (void) openUpdateDrives: (NSDictionary *) drives
{
    if( fDrives )
    {
        [fDrives release];
    }
    fDrives = [[NSDictionary alloc] initWithDictionary: drives];

    NSString * device;
    NSEnumerator * enumerator = [fDrives keyEnumerator];
    [fOpenPopUp removeAllItems];
    while( ( device = [enumerator nextObject] ) )
    {
        [fOpenPopUp addItemWithTitle: device];
    }

    if( ![fOpenPopUp numberOfItems] )
    {
        [fOpenPopUp addItemWithTitle: INSERT_STRING];
    }
    [fOpenPopUp selectItemAtIndex: 0];
    if( [fOpenMatrix isEnabled] )
    {
        [self openEnable: YES];
    }
}

- (void) openBrowseDidEnd: (NSOpenPanel *) sheet returnCode: (int)
    returnCode contextInfo: (void *) contextInfo
{
    if( returnCode != NSOKButton )
        return;

    if( fOpenFolderString )
        [fOpenFolderString release];
    fOpenFolderString = [[[sheet filenames] objectAtIndex: 0] retain];
    [fOpenFolderField setStringValue: [fOpenFolderString lastPathComponent]];
    [self openGo: self];
}

- (void) openEnable: (BOOL) b
{
    [fOpenMatrix       setEnabled: b];
    [fOpenPopUp        setEnabled: b];
    [fOpenFolderField  setEnabled: b];
    [fOpenBrowseButton setEnabled: b];
    [fOpenGoButton     setEnabled: b];

    if( b )
    {
        if( [fOpenMatrix selectedRow] )
        {
            [fOpenPopUp setEnabled: NO];
        }
        else
        {
            [fOpenFolderField  setEnabled: NO];
            [fOpenBrowseButton setEnabled: NO];
            if( [[fOpenPopUp titleOfSelectedItem]
                    isEqualToString: INSERT_STRING] )
            {
                [fOpenGoButton setEnabled: NO];
            }
        }
    }
}

- (void) openTimer: (NSTimer *) timer
{
    hb_state_t s;
    hb_get_state( fHandle, &s );
    switch( s.state )
    {
#define p s.param.scanning
        case HB_STATE_SCANNING:
            [fOpenIndicator setIndeterminate: NO];
            [fOpenIndicator setDoubleValue: 100.0 *
                ( (float) p.title_cur - 0.5 ) / p.title_count];
            [fOpenProgressField setStringValue: [NSString
                stringWithFormat: @"Scanning title %d of %d...",
                p.title_cur, p.title_count]];
            break;
#undef p

        case HB_STATE_SCANDONE:
            [timer invalidate];

            [fOpenIndicator setIndeterminate: NO];
            [fOpenIndicator setDoubleValue: 0.0];
            [self openEnable: YES];

            if( hb_list_count( fList ) )
            {
                [self convertShow];
            }
            else
            {
                [fDriveDetector run];
            }
            break;

        default:
            break;
    }
}

- (void) convertShow
{
    int i, j;

    fConvertCheckArray = [[NSMutableArray alloc] initWithCapacity:
        hb_list_count( fList )];
    [fConvertAudioPopUp removeAllItems];
    [fConvertSubtitlePopUp removeAllItems];
    [fConvertSubtitlePopUp addItemWithTitle: @"None"];
    for( i = 0; i < hb_list_count( fList ); i++ )
    {
        /* Default is to convert titles longer than 30 minutes. */
        hb_title_t * title = hb_list_item( fList, i );
        [fConvertCheckArray addObject: [NSNumber numberWithBool:
            ( 60 * title->hours + title->minutes > 30 )]];

        /* Update audio popup */
        hb_audio_t * audio;
        for( j = 0; j < hb_list_count( title->list_audio ); j++ )
        {
            audio = hb_list_item( title->list_audio, j );
            [fConvertAudioPopUp addItemWithTitle:
                [NSString stringWithUTF8String: audio->lang_simple]];
        }
		[fConvertAudioPopUp selectItemWithTitle: @"English"];

        /* Update subtitle popup */
        hb_subtitle_t * subtitle;
        for( j = 0; j < hb_list_count( title->list_subtitle ); j++ )
        {
            subtitle = hb_list_item( title->list_subtitle, j );
            [fConvertSubtitlePopUp addItemWithTitle:
                [NSString stringWithUTF8String: subtitle->lang]];
        }
    }
    [fConvertTableView reloadData];

    NSRect frame  = [fWindow frame];
    float  offset = [fConvertView frame].size.height -
                    [fOpenView frame].size.height;
    frame.origin.y    -= offset;
    frame.size.height += offset;
    [fWindow setContentView: fEmptyView];
    [fWindow setFrame: frame display: YES animate: YES];
    [fWindow setContentView: fConvertView];

    /* Folder popup */
    NSMenuItem * item = [fConvertFolderPopUp itemAtIndex: 0];
    [item setTitle: [fConvertFolderString lastPathComponent]];
    NSImage * image32 = [[NSWorkspace sharedWorkspace] iconForFile:
        fConvertFolderString];
    NSImage * image16 = [[NSImage alloc] initWithSize:
        NSMakeSize(16,16)];
    [image16 lockFocus];
    [[NSGraphicsContext currentContext]
        setImageInterpolation: NSImageInterpolationHigh];
    [image32 drawInRect: NSMakeRect(0,0,16,16)
        fromRect: NSMakeRect(0,0,32,32) operation: NSCompositeCopy
        fraction: 1.0];
    [image16 unlockFocus];                                                      
    [item setImage: image16];
    [image16 release];

    [self convertEnable: YES];
}

- (void) convertEnable: (BOOL) b
{
    [fConvertTableView setEnabled: b];
    [fConvertFolderPopUp setEnabled: b];
    [fConvertFormatPopUp setEnabled: b];
    [fConvertAspectPopUp setEnabled: b];
    [fConvertMaxWidthPopUp setEnabled: b];
    [fConvertAudioPopUp setEnabled: b];
    [fConvertSubtitlePopUp setEnabled: b];
    [fConvertOpenButton setEnabled: b];
    if( b )
    {
        [fConvertGoButton setTitle: @"Convert"];
        [fConvertGoButton setAction: @selector(convertGo:)];
    }
    else
    {
        [fConvertGoButton setTitle: @"Cancel"];
        [fConvertGoButton setAction: @selector(convertCancel:)];
    }
}

- (void) convertTimer: (NSTimer *) timer
{
    hb_state_t s;
    hb_get_state( fHandle, &s );
    switch( s.state )
    {
#define p s.param.working
        case HB_STATE_WORKING:
        {
            float progress_total = ( p.progress + p.job_cur - 1 ) / p.job_count;
            NSMutableString * string = [NSMutableString
                stringWithFormat: @"Converting: %.1f %%",
                100.0 * progress_total];
			hb_log("Progress %.1f", progress_total * 100.0);
            if( p.seconds > -1 )
            {
                [string appendFormat: @" (%.1f fps, ", p.rate_avg];
                if( p.hours > 0 )
                {
                    [string appendFormat: @"%d hour%s %d min%s",
                        p.hours, p.hours == 1 ? "" : "s",
                        p.minutes, p.minutes == 1 ? "" : "s"];
                }
                else if( p.minutes > 0 )
                {
                    [string appendFormat: @"%d min%s %d sec%s",
                        p.minutes, p.minutes == 1 ? "" : "s",
                        p.seconds, p.seconds == 1 ? "" : "s"];
                }
                else
                {
                    [string appendFormat: @"%d second%s",
                        p.seconds, p.seconds == 1 ? "" : "s"];
                }
                [string appendString: @" left)"];
            }
            [fConvertInfoString setStringValue: string];
            [fConvertIndicator setIndeterminate: NO];
            [fConvertIndicator setDoubleValue: 100.0 * p.progress];
            break;
        }
#undef p

        case HB_STATE_WORKDONE:
		{
			[timer invalidate];
            [fConvertIndicator setIndeterminate: NO];
            [fConvertIndicator setDoubleValue: 0.0];
            [self convertEnable: YES];
			
#define p s.param.workdone
			switch(p.error)
			{
				case HB_ERROR_NONE:
					[fConvertInfoString setStringValue: @"Done."];
					break;
				case HB_ERROR_CANCELED:
					[fConvertInfoString setStringValue: @"Canceled."];
					break;
				case HB_ERROR_UNKNOWN:
					[fConvertInfoString setStringValue: @"Unknown Error."];
					break;
			}
#undef p

			hb_job_t * job;
            while( ( job = hb_job( fHandle, 0 ) ) )
            {
                hb_rem( fHandle, job );
            }
			break;
		}
        default:
            break;
    }
}

@end
