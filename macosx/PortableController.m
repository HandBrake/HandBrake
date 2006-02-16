#import "PortableController.h"

@interface PortableController (Private)

- (void) openBrowseDidEnd: (NSOpenPanel *) sheet returnCode: (int)
    returnCode contextInfo: (void *) contextInfo;
- (void) openEnable: (BOOL) b;
- (void) openTimer: (NSTimer *) timer;

- (void) convertShow;
- (void) convertTimer: (NSTimer *) timer;

@end

@implementation PortableController

/***********************************************************************
 * Application delegate methods
 **********************************************************************/
- (void) awakeFromNib
{
    /* Show the "Open DVD" interface */
    [self openEnable: YES];
    [fWindow setContentSize: [fOpenView frame].size];
    [fWindow setContentView: fOpenView];
    [fWindow center];
    [fWindow makeKeyAndOrderFront: nil];

    /* NSTableView initializations */
    NSTableColumn * tableColumn = [fConvertTableView
        tableColumnWithIdentifier: @"Check"];
    NSButtonCell * buttonCell = [[[NSButtonCell alloc]
        initTextCell: @""] autorelease];
    [buttonCell setEditable: YES];
    [buttonCell setButtonType: NSSwitchButton];
    [tableColumn setDataCell: buttonCell];

    /* Preferences */
    fConvertFolderString = [@"~/Movies" stringByExpandingTildeInPath];
    [fConvertFolderString retain];
}

- (void) applicationWillFinishLaunching: (NSNotification *) n
{
    fHandle = hb_init( HB_DEBUG_NONE, 0 );
    fList   = hb_get_titles( fHandle );
}

- (void) applicationWillTerminate: (NSNotification *) n
{
    hb_close( &fHandle );
}

/***********************************************************************
 * Tableview delegate methods
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
        else if( [[col identifier] isEqualToString: @"Length"] )
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
}

- (void) openMatrixChanged: (id) sender
{
    [self openEnable: YES];
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

    hb_scan( fHandle, [fOpenFolderString UTF8String], 0 );

    NSTimer * timer = [NSTimer scheduledTimerWithTimeInterval: 0.5
        target: self selector: @selector( openTimer: ) userInfo: nil
        repeats: YES];
}

- (void) convertGo: (id) sender
{
    int i;

    for( i = 0; i < hb_list_count( fList ); i++ )
    {
        if( ![[fConvertCheckArray objectAtIndex: i] boolValue] )
            continue;

        hb_title_t * title = hb_list_item( fList, i );
        hb_job_t   * job   = title->job;

        job->width = 320;
        for( ;; )
        {
            /* XXX */
            hb_fix_aspect( job, HB_KEEP_WIDTH );
            if( job->height == 240 )
            {
                break;
            }
            else if( job->height < 240 )
            {
                job->crop[2] += 2;
                job->crop[3] += 2;
            }
            else
            {
                job->crop[0] += 2;
                job->crop[1] += 2;
            }
        }
        job->vquality = -1.0;
        job->vbitrate = 600;
        job->vcodec   = HB_VCODEC_X264;
        job->h264_13  = 1;
        job->file     = strdup( [[NSString stringWithFormat:
            @"%@/%p - Title %d.mp4", fConvertFolderString, self,
            title->index] UTF8String] );
        hb_add( fHandle, job );
    }

    hb_start( fHandle );

    NSTimer * timer = [NSTimer scheduledTimerWithTimeInterval: 0.5
        target: self selector: @selector( convertTimer: ) userInfo: nil
        repeats: YES];
}

@end

/***********************************************************************
 * Private methods
 **********************************************************************/

@implementation PortableController (Private)

- (void) openBrowseDidEnd: (NSOpenPanel *) sheet returnCode: (int)
    returnCode contextInfo: (void *) contextInfo
{
    if( returnCode != NSOKButton )
        return;

    if( fOpenFolderString )
        [fOpenFolderString release];
    fOpenFolderString = [[[sheet filenames] objectAtIndex: 0] retain];
    [fOpenFolderField setStringValue: [fOpenFolderString lastPathComponent]];
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
            break;

        default:
            break;
    }
}

- (void) convertShow
{
    int i;

    fConvertCheckArray = [[NSMutableArray alloc] initWithCapacity:
        hb_list_count( fList )];
    for( i = 0; i < hb_list_count( fList ); i++ )
    {
        /* Default is to convert titles longer than 30 minutes. */
        hb_title_t * title = hb_list_item( fList, i );
        [fConvertCheckArray addObject: [NSNumber numberWithBool:
            ( 60 * title->hours + title->minutes > 30 )]];
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
}

- (void) convertTimer: (NSTimer *) timer
{
    hb_state_t s;
    hb_get_state( fHandle, &s );
    switch( s.state )
    {
#define p s.param.working
        case HB_STATE_WORKING:
            [fConvertIndicator setIndeterminate: NO];
            [fConvertIndicator setDoubleValue: 100.0 * p.progress];
            break;
#undef p

        case HB_STATE_WORKDONE:
            [timer invalidate];
            [fConvertIndicator setIndeterminate: NO];
            [fConvertIndicator setDoubleValue: 0.0];
            break;

        default:
            break;
    }
}

@end
