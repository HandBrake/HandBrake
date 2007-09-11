/* HBQueueController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.m0k.org/>.
    It may be used under the terms of the GNU General Public License. */

#include "HBQueueController.h"
#include "Controller.h"

#define HB_QUEUE_DRAGGING 0
#define HBQueueDataType         @"HBQueueDataType"

// UNI_QUEUE turns on the feature where the first item in the queue NSTableView is the
// current job followed by the jobs in hblib's queue. In this scheme, fCurrentJobPane
// disappers.
#define HB_UNI_QUEUE 0

#define HB_ROW_HEIGHT_DETAIL       98.0
#define HB_ROW_HEIGHT_NO_DETAIL    17.0
#define HB_ROW_HEIGHT_ACTIVE_JOB   60.0

//------------------------------------------------------------------------------------
#pragma mark Job group functions
//------------------------------------------------------------------------------------
// These could be part of hblib if we think hblib should have knowledge of groups.
// For now, I see groups as a metaphor that HBQueueController provides.

/**
 * Returns the number of jobs groups in the queue.
 * @param h Handle to hb_handle_t.
 * @return Number of job groups.
 */
static int hb_group_count(hb_handle_t * h)    
{
    hb_job_t * job;
    int count = 0;
    int index = 0;
    while( ( job = hb_job( h, index++ ) ) )
    {
        if (job->sequence_id == 0)
            count++;
    }
    return count;
}

/**
 * Returns handle to the first job in the i-th group within the job list.
 * @param h Handle to hb_handle_t.
 * @param i Index of group.
 * @returns Handle to hb_job_t of desired job.
 */
static hb_job_t * hb_group(hb_handle_t * h, int i)    
{
    hb_job_t * job;
    int count = 0;
    int index = 0;
    while( ( job = hb_job( h, index++ ) ) )
    {
        if (job->sequence_id == 0)
        {
            if (count == i)
                return job;
            count++;
        }
    }
    return NULL;
}

/**
 * Removes a groups of jobs from the job list.
 * @param h Handle to hb_handle_t.
 * @param job Handle to the first job in the group.
 */
static void hb_rem_group( hb_handle_t * h, hb_job_t * job )
{
    // Find job in list
    hb_job_t * j;
    int index = 0;
    while( ( j = hb_job( h, index ) ) )
    {
        if (j == job)
        {
            // Delete this job plus the following ones in the sequence
            hb_rem( h, job );
            while( ( j = hb_job( h, index ) ) && (j->sequence_id != 0) )
                hb_rem( h, j );
            return;
        }
        else
            index++;
    }
}

/**
 * Returns handle to the next job after the given job.
 * @param h Handle to hb_handle_t.
 * @param job Handle to the a job in the group.
 * @returns Handle to hb_job_t of desired job or NULL if no such job.
 */
static hb_job_t * hb_next_job( hb_handle_t * h, hb_job_t * job )
{
    hb_job_t * j = NULL;
    int index = 0;
    while( ( j = hb_job( h, index++ ) ) )
    {
        if (j == job)
            return hb_job( h, index );
    }
    return NULL;
}

#pragma mark -
//------------------------------------------------------------------------------------
// HBJob
//------------------------------------------------------------------------------------

#if HB_OUTLINE_QUEUE

@interface HBJob : NSObject
{
    hb_job_t                *fJob;
}
+ (HBJob*) jobWithJob: (hb_job_t *) job;
- (id) initWithJob: (hb_job_t *) job;
- (hb_job_t *) job;
@end

@implementation HBJob
+ (HBJob*) jobWithJob: (hb_job_t *) job
{
    return [[[HBJob alloc] initWithJob:job] autorelease];
}

- (id) initWithJob: (hb_job_t *) job
{
    if (self = [super init])
    {
        // job is not owned by HBJob. It does not get dealloacted when HBJob is released.
        fJob = job;
    }
    return self; 
}

- (hb_job_t*) job
{
    return fJob;
}

@end

#endif // HB_OUTLINE_QUEUE

#pragma mark -

// Toolbar identifiers
static NSString*    HBQueueToolbar                            = @"HBQueueToolbar";
static NSString*    HBStartPauseResumeToolbarIdentifier       = @"HBStartPauseResumeToolbarIdentifier";
static NSString*    HBShowDetailToolbarIdentifier             = @"HBShowDetailToolbarIdentifier";
static NSString*    HBShowGroupsToolbarIdentifier             = @"HBShowGroupsToolbarIdentifier";


@implementation HBQueueController

//------------------------------------------------------------------------------------
// init
//------------------------------------------------------------------------------------
- (id)init
{
    if (self = [super init])
    {
        // Our defaults
        [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
            @"NO",      @"QueueWindowIsOpen",
            @"NO",      @"QueueShowsDetail",
            @"YES",     @"QueueShowsJobsAsGroups",
            nil]];

        fShowsDetail = [[NSUserDefaults standardUserDefaults] boolForKey:@"QueueShowsDetail"];
#if HB_OUTLINE_QUEUE
        fShowsJobsAsGroups = YES;
#else
        fShowsJobsAsGroups = [[NSUserDefaults standardUserDefaults] boolForKey:@"QueueShowsJobsAsGroups"];
#endif

#if HB_OUTLINE_QUEUE
        fEncodes = [[NSMutableArray arrayWithCapacity:0] retain];
#endif
    }
    return self; 
}

//------------------------------------------------------------------------------------
// dealloc
//------------------------------------------------------------------------------------
- (void)dealloc
{
    [fAnimation release];
    
    // clear the delegate so that windowWillClose is not attempted
    if ([fQueueWindow delegate] == self)
        [fQueueWindow setDelegate:nil];
    
#if HB_OUTLINE_QUEUE
    [fEncodes release];
#endif

    [super dealloc];
}

//------------------------------------------------------------------------------------
// Receive HB handle
//------------------------------------------------------------------------------------
- (void)setHandle: (hb_handle_t *)handle
{
    fHandle = handle;
}

//------------------------------------------------------------------------------------
// Receive HBController
//------------------------------------------------------------------------------------
- (void)setHBController: (HBController *)controller
{
    fHBController = controller;
}

//------------------------------------------------------------------------------------
// Displays and brings the queue window to the front
//------------------------------------------------------------------------------------
- (IBAction) showQueueWindow: (id)sender
{
    if (!fQueueWindow)
    {
        BOOL loadSucceeded = [NSBundle loadNibNamed:@"Queue" owner:self] && fQueueWindow;
        NSAssert(loadSucceeded, @"Could not open Queue nib file");
    }

    [self updateQueueUI];
    [self updateCurrentJobUI];

    [fQueueWindow makeKeyAndOrderFront: self];

    [[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"QueueWindowIsOpen"];
}
//------------------------------------------------------------------------------------
// Show or hide the current job pane (fCurrentJobPane).
//------------------------------------------------------------------------------------
- (void) showCurrentJobPane: (BOOL)showPane
{
    if (showPane != fCurrentJobHidden)
        return;
    
    // Things to keep in mind:
    // - When the current job pane is shown, it occupies the upper portion of the
    //   window with the queue occupying the bottom portion of the window.
    // - When the current job pane is hidden, it slides up and out of view.
    //   NSView setHidden is NOT used. The queue pane is resized to occupy the full
    //   window.
    
    NSRect windowFrame = [[fCurrentJobPane superview] frame];
    NSRect queueFrame, jobFrame;
    if (showPane)
        NSDivideRect(windowFrame, &jobFrame, &queueFrame, NSHeight([fCurrentJobPane frame]), NSMaxYEdge);
    else
    {
        queueFrame = windowFrame;
        jobFrame = [fCurrentJobPane frame];
        jobFrame.origin.y = NSHeight(windowFrame);
    }
    
    // Move fCurrentJobPane
    NSDictionary * dict1 = [NSDictionary dictionaryWithObjectsAndKeys:
        fCurrentJobPane, NSViewAnimationTargetKey,
        [NSValue valueWithRect:jobFrame], NSViewAnimationEndFrameKey,
        nil];

    // Resize fQueuePane
    NSDictionary * dict2 = [NSDictionary dictionaryWithObjectsAndKeys:
        fQueuePane, NSViewAnimationTargetKey,
        [NSValue valueWithRect:queueFrame], NSViewAnimationEndFrameKey,
        nil];

    if (!fAnimation)
        fAnimation = [[NSViewAnimation alloc] initWithViewAnimations:nil];

    [fAnimation setViewAnimations:[NSArray arrayWithObjects:dict1, dict2, nil]];
    [fAnimation setDuration:0.25];
    [fAnimation setAnimationBlockingMode:NSAnimationBlocking]; // prevent user from resizing the window during an animation
    [fAnimation startAnimation];
    fCurrentJobHidden = !showPane;
}

//------------------------------------------------------------------------------------
// Enables or disables the display of detail information for each job.
//------------------------------------------------------------------------------------
- (void)setShowsDetail: (BOOL)showsDetail
{
    fShowsDetail = showsDetail;
    
    [[NSUserDefaults standardUserDefaults] setBool:showsDetail forKey:@"QueueShowsDetail"];
    [[NSUserDefaults standardUserDefaults] synchronize];

    [fTaskView setRowHeight:showsDetail ? HB_ROW_HEIGHT_DETAIL : HB_ROW_HEIGHT_NO_DETAIL];
#if HB_UNI_QUEUE
    if (hb_count(fHandle))
        [fTaskView noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndex:0]];
#endif
#if HB_OUTLINE_QUEUE

        [fOutlineView noteHeightOfRowsWithIndexesChanged:
            [NSIndexSet indexSetWithIndexesInRange:
                NSMakeRange(0,[fOutlineView numberOfRows])
                ]];
#endif

    if ([fTaskView selectedRow] != -1)
        [fTaskView scrollRowToVisible:[fTaskView selectedRow]];
}

//------------------------------------------------------------------------------------
// Enables or disables the grouping of job passes into one item in the UI.
//------------------------------------------------------------------------------------
- (void)setShowsJobsAsGroups: (BOOL)showsGroups
{
#if HB_OUTLINE_QUEUE
    return; // Can't modify this value. It's always YES.
#endif
    fShowsJobsAsGroups = showsGroups;
    
    [[NSUserDefaults standardUserDefaults] setBool:showsGroups forKey:@"QueueShowsJobsAsGroups"];
    [[NSUserDefaults standardUserDefaults] synchronize];

    [self updateQueueUI];
    if ([fTaskView selectedRow] != -1)
        [fTaskView scrollRowToVisible:[fTaskView selectedRow]];
}

//------------------------------------------------------------------------------------
// Returns a 16x16 image that represents a job pass.
//------------------------------------------------------------------------------------
- (NSImage *)smallImageForPass: (int)pass
{
    switch (pass)
    {
        case -1: return [NSImage imageNamed: @"JobPassSubtitleSmall"];
        case  1: return [NSImage imageNamed: @"JobPassFirstSmall"];
        case  2: return [NSImage imageNamed: @"JobPassSecondSmall"];
        default: return [NSImage imageNamed: @"JobPassUnknownSmall"];
    }
}

//------------------------------------------------------------------------------------
// Returns a 64x64 image that represents a job pass.
//------------------------------------------------------------------------------------
- (NSImage *)largeImageForPass: (int)pass
{
    switch (pass)
    {
        case -1: return [NSImage imageNamed: @"JobPassSubtitleLarge"];
        case  1: return [NSImage imageNamed: @"JobPassFirstLarge"];
        case  2: return [NSImage imageNamed: @"JobPassSecondLarge"];
        default: return [NSImage imageNamed: @"JobPassUnknownLarge"];
    }
}

#if HB_OUTLINE_QUEUE
//------------------------------------------------------------------------------------
// Rebuilds the contents of fEncodes which is a array of encodes and HBJobs.
//------------------------------------------------------------------------------------
- (void)rebuildEncodes
{
    [fEncodes removeAllObjects];

    NSMutableArray * aJobGroup = [NSMutableArray arrayWithCapacity:0];
    hb_job_t * nextJob = hb_group( fHandle, 0 );
    while( nextJob )
    {
        if (nextJob->sequence_id == 0)
        {
            // Encountered a new group. Add the current one to fEncodes and then start a new one.
            if ([aJobGroup count] > 0)
            {
                [fEncodes addObject:aJobGroup];
                aJobGroup = [NSMutableArray arrayWithCapacity:0];
            }
        }
        [aJobGroup addObject: [HBJob jobWithJob:nextJob]];
        nextJob = hb_next_job (fHandle, nextJob);
    }
    if ([aJobGroup count] > 0)
        [fEncodes addObject:aJobGroup];
}
#endif

//------------------------------------------------------------------------------------
// Generates a multi-line text string that includes the job name on the first line
// followed by details of the job on subsequent lines. If the text is to be drawn as
// part of a highlighted cell, set isHighlighted to true. The returned string may
// contain multiple fonts and paragraph formating.
//------------------------------------------------------------------------------------
- (NSAttributedString *)attributedDescriptionForJob: (hb_job_t *)job
                                          withTitle: (BOOL)withTitle
                                         withDetail: (BOOL)withDetail
                                   withHighlighting: (BOOL)highlighted
{
    NSMutableAttributedString * finalString;   // the return value
    NSAttributedString* anAttributedString;    // a temp string for building up attributed substrings
    NSMutableString* aMutableString;           // a temp string for non-attributed substrings
    hb_title_t * title = job->title;
    
    NSMutableParagraphStyle *ps = [[NSParagraphStyle defaultParagraphStyle] mutableCopy];
    [ps setLineBreakMode:NSLineBreakByClipping];

    static NSDictionary* detailAttribute = [[NSDictionary dictionaryWithObjectsAndKeys:
                [NSFont systemFontOfSize:10.0], NSFontAttributeName,
                [NSColor darkGrayColor], NSForegroundColorAttributeName,
                ps, NSParagraphStyleAttributeName,
                nil] retain];
    static NSDictionary* detailHighlightedAttribute = [[NSDictionary dictionaryWithObjectsAndKeys:
                [NSFont systemFontOfSize:10.0], NSFontAttributeName,
                [NSColor whiteColor], NSForegroundColorAttributeName,
                ps, NSParagraphStyleAttributeName,
                nil] retain];
    static NSDictionary* titleAttribute = [[NSDictionary dictionaryWithObjectsAndKeys:
                [NSFont systemFontOfSize:[NSFont systemFontSize]], NSFontAttributeName,
                ps, NSParagraphStyleAttributeName,
                nil] retain];

    finalString = [[[NSMutableAttributedString alloc] init] autorelease];

    // Title, in bold
    // Show the name of the source Note: use title->name instead of title->dvd since
    // name is just the chosen folder, instead of dvd which is the full path
    if (withTitle)
    {
        anAttributedString = [[[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:title->name] attributes:titleAttribute] autorelease];
        [finalString appendAttributedString:anAttributedString];
    }
    
    // Other info in plain
    
    aMutableString = [NSMutableString stringWithCapacity:200];

    // The subtitle scan doesn't contain all the stuff we need (like x264opts).
    // So grab the next job in the group for display purposes.
    if (fShowsJobsAsGroups && job->pass == -1)
    {
        // When job is the one currently being processed, then the next in its group
        // is the the first job in the queue.
        hb_job_t * nextjob;
        if (job == hb_current_job(fHandle))
            nextjob = hb_job(fHandle, 0);
        else
            nextjob = hb_next_job(fHandle, job);
        if (nextjob)    // Overly cautious in case there is no next job!
            job = nextjob;
    }

    if (withTitle)
    {
        NSString * chapterString = (job->chapter_start == job->chapter_end) ?
                [NSString stringWithFormat:@"Chapter %d", job->chapter_start] :
                [NSString stringWithFormat:@"Chapters %d through %d", job->chapter_start, job->chapter_end];
    
        // Scan pass
        if (job->pass == -1)
        {
            [aMutableString appendString:[NSString stringWithFormat:
                    @"  (Title %d, %@, Subtitle Scan)", title->index, chapterString]];
        }
        else
        {
            if (fShowsJobsAsGroups)
                [aMutableString appendString:[NSString stringWithFormat:
                        @"  (Title %d, %@, %d-Pass)",
                        title->index, chapterString, MIN( 2, job->pass + 1 )]];
            else
                [aMutableString appendString:[NSString stringWithFormat:
                        @"  (Title %d, %@, Pass %d of %d)",
                        title->index, chapterString, MAX( 1, job->pass ), MIN( 2, job->pass + 1 )]];
        }
    }
    
    // End of title stuff
    
    
    // Normal pass - show detail
    if (withDetail && job->pass != -1)
    {
        NSString * jobFormat;
        NSString * jobPictureDetail;
        NSString * jobVideoDetail;
        NSString * jobVideoCodec;
        NSString * jobVideoQuality;
        NSString * jobAudioDetail;
        NSString * jobAudioCodec;

        /* Muxer settings (File Format in the gui) */
        if (job->mux == 65536 || job->mux == 131072 || job->mux == 1048576)
            jobFormat = @"MP4"; // HB_MUX_MP4,HB_MUX_PSP,HB_MUX_IPOD
        else if (job->mux == 262144)
            jobFormat = @"AVI"; // HB_MUX_AVI
        else if (job->mux == 524288)
            jobFormat = @"OGM"; // HB_MUX_OGM
        else if (job->mux == 2097152)
            jobFormat = @"MKV"; // HB_MUX_MKV
        else
            jobFormat = @"unknown";
        
        // 2097152
        /* Video Codec settings (Encoder in the gui) */
        if (job->vcodec == 1)
            jobVideoCodec = @"FFmpeg"; // HB_VCODEC_FFMPEG
        else if (job->vcodec == 2)
            jobVideoCodec = @"XviD"; // HB_VCODEC_XVID
        else if (job->vcodec == 4)
        {
            /* Deterimine for sure how we are now setting iPod uuid atom */
            if (job->h264_level) // We are encoding for iPod
                jobVideoCodec = @"x264 (H.264 iPod)"; // HB_VCODEC_X264    
            else
                jobVideoCodec = @"x264 (H.264 Main)"; // HB_VCODEC_X264
        }
        else
            jobVideoCodec = @"unknown";
        
        /* Audio Codecs (Second half of Codecs in the gui) */
        if (job->acodec == 256)
            jobAudioCodec = @"AAC"; // HB_ACODEC_FAAC
        else if (job->acodec == 512)
            jobAudioCodec = @"MP3"; // HB_ACODEC_LAME
        else if (job->acodec == 1024)
            jobAudioCodec = @"Vorbis"; // HB_ACODEC_VORBIS
        else if (job->acodec == 2048)
            jobAudioCodec = @"AC3"; // HB_ACODEC_AC3
        else
            jobAudioCodec = @"unknown";
        /* Show Basic File info */
        if (job->chapter_markers == 1)
            [aMutableString appendString:[NSString stringWithFormat:@"\nFormat: %@ Container, %@ Video + %@ Audio, Chapter Markers", jobFormat, jobVideoCodec, jobAudioCodec]];
        else
            [aMutableString appendString:[NSString stringWithFormat:@"\nFormat: %@ Container, %@ Video + %@ Audio", jobFormat, jobVideoCodec, jobAudioCodec]];
            
        /*Picture info*/
        /*integers for picture values deinterlace, crop[4], keep_ratio, grayscale, pixel_ratio, pixel_aspect_width, pixel_aspect_height,
         maxWidth, maxHeight */
        if (job->pixel_ratio == 1)
        {
            int titlewidth = title->width - job->crop[2] - job->crop[3];
            int displayparwidth = titlewidth * job->pixel_aspect_width / job->pixel_aspect_height;
            int displayparheight = title->height - job->crop[0] - job->crop[1];
            jobPictureDetail = [NSString stringWithFormat:@"Picture: %dx%d (%dx%d Anamorphic)", displayparwidth, displayparheight, job->width, displayparheight];
        }
        else
            jobPictureDetail = [NSString stringWithFormat:@"Picture: %dx%d", job->width, job->height];
        if (job->keep_ratio == 1)
            jobPictureDetail = [jobPictureDetail stringByAppendingString:@" Keep Aspect Ratio"];
        
        if (job->grayscale == 1)
            jobPictureDetail = [jobPictureDetail stringByAppendingString:@", Grayscale"];
        
        if (job->deinterlace == 1)
            jobPictureDetail = [jobPictureDetail stringByAppendingString:@", Deinterlace"];
        /* Show Picture info */    
        [aMutableString appendString:[NSString stringWithFormat:@"\n%@", jobPictureDetail]];
        
        /* Detailed Video info */
        if (job->vquality <= 0 || job->vquality >= 1)
            jobVideoQuality =[NSString stringWithFormat:@"%d kbps", job->vbitrate];
        else
        {
            NSNumber * vidQuality;
            vidQuality = [NSNumber numberWithInt:job->vquality * 100];
            /* this is screwed up kind of. Needs to be formatted properly */
            if (job->crf == 1)
                jobVideoQuality =[NSString stringWithFormat:@"%@%% CRF", vidQuality];            
            else
                jobVideoQuality =[NSString stringWithFormat:@"%@%% CQP", vidQuality];
        }
        
        if (job->vrate_base == 1126125)
        {
            /* NTSC FILM 23.976 */
            jobVideoDetail = [NSString stringWithFormat:@"Video: %@, %@, 23.976 fps", jobVideoCodec, jobVideoQuality];
        }
        else if (job->vrate_base == 900900)
        {
            /* NTSC 29.97 */
            jobVideoDetail = [NSString stringWithFormat:@"Video: %@, %@, 29.97 fps", jobVideoCodec, jobVideoQuality];
        }
        else
        {
            /* Everything else */
            jobVideoDetail = [NSString stringWithFormat:@"Video: %@, %@, %d fps", jobVideoCodec, jobVideoQuality, job->vrate / job->vrate_base];
        }
        
        /* Add the video detail string to the job filed in the window */
        [aMutableString appendString:[NSString stringWithFormat:@"\n%@", jobVideoDetail]];
        
        /* if there is an x264 option string, lets add it here*/
        /*NOTE: Due to size, lets get this in a tool tip*/
        
        if (job->x264opts)
            [aMutableString appendString:[NSString stringWithFormat:@"\nx264 Options: %@", [NSString stringWithUTF8String:job->x264opts]]];
        
        /* Audio Detail */
        if ([jobAudioCodec isEqualToString: @"AC3"])
            jobAudioDetail = [NSString stringWithFormat:@"Audio: %@, Pass-Through", jobAudioCodec];
        else
            jobAudioDetail = [NSString stringWithFormat:@"Audio: %@, %d kbps, %d Hz", jobAudioCodec, job->abitrate, job->arate];
        
        /* we now get the audio mixdown info for each of the two gui audio tracks */
        /* lets do it the long way here to get a handle on things.
            Hardcoded for two tracks for gui: audio_mixdowns[i] audio_mixdowns[i] */
        int ai; // counter for each audios [] , macgui only allows for two audio tracks currently
        for( ai = 0; ai < 2; ai++ )
        {
            if (job->audio_mixdowns[ai] == HB_AMIXDOWN_MONO)
                jobAudioDetail = [jobAudioDetail stringByAppendingString:[NSString stringWithFormat:@", Track %d: Mono",ai + 1]];
            if (job->audio_mixdowns[ai] == HB_AMIXDOWN_STEREO)
                jobAudioDetail = [jobAudioDetail stringByAppendingString:[NSString stringWithFormat:@", Track %d: Stereo",ai + 1]];
            if (job->audio_mixdowns[ai] == HB_AMIXDOWN_DOLBY)
                jobAudioDetail = [jobAudioDetail stringByAppendingString:[NSString stringWithFormat:@", Track %d: Dolby Surround",ai + 1]];
            if (job->audio_mixdowns[ai] == HB_AMIXDOWN_DOLBYPLII)
                jobAudioDetail = [jobAudioDetail stringByAppendingString:[NSString stringWithFormat:@", Track %d: Dolby Pro Logic II",ai + 1]];
            if (job->audio_mixdowns[ai] == HB_AMIXDOWN_6CH)
                jobAudioDetail = [jobAudioDetail stringByAppendingString:[NSString stringWithFormat:@", Track %d: 6-channel discreet",ai + 1]];
        }
        
        /* Add the Audio detail string to the job filed in the window */
        [aMutableString appendString:[NSString stringWithFormat: @"\n%@", jobAudioDetail]];
        
        /*Destination Field */
        [aMutableString appendString:[NSString stringWithFormat:@"\nDestination: %@", [NSString stringWithUTF8String:job->file]]];
    }
    
    anAttributedString = [[[NSAttributedString alloc] initWithString:aMutableString attributes:highlighted ? detailHighlightedAttribute : detailAttribute] autorelease];
    [finalString appendAttributedString:anAttributedString];

            
    return finalString;
}

//------------------------------------------------------------------------------------
// Generate string to display in UI.
//------------------------------------------------------------------------------------
- (NSString *) progressStatusStringForJob: (hb_job_t *)job state: (hb_state_t *)s
{
    if (s->state == HB_STATE_WORKING)
    {
        NSString * msg;
        if (job->pass == -1)
            msg = NSLocalizedString( @"Analyzing subtitles", nil );
        else if (job->pass == 1)
            msg = NSLocalizedString( @"Analyzing video", nil );
        else if ((job->pass == 0) ||  (job->pass == 2))
            msg = NSLocalizedString( @"Encoding movie", nil );
        else
            return @""; // unknown condition!
            
        if( s->param.working.seconds > -1 )
        {
            return [NSString stringWithFormat:
                NSLocalizedString( @"%@ (%.2f fps, avg %.2f fps)", nil ),
                msg, s->param.working.rate_cur, s->param.working.rate_avg];
        }
        else
            return msg;

    }

    else if (s->state == HB_STATE_MUXING)
        return NSLocalizedString( @"Muxing", nil );

    else if (s->state == HB_STATE_PAUSED)
        return NSLocalizedString( @"Paused", nil );

    else if (s->state == HB_STATE_WORKDONE)
        return NSLocalizedString( @"Done", nil );
    
    return @"";
}

//------------------------------------------------------------------------------------
// Generate string to display in UI.
//------------------------------------------------------------------------------------
- (NSString *) progressTimeRemainingStringForJob: (hb_job_t *)job state: (hb_state_t *)s
{
    if (s->state == HB_STATE_WORKING)
    {
        #define p s->param.working
        if (p.seconds < 0)
            return @"";
        
        // Minutes always needed
        NSString * minutes;
        if (p.minutes > 1)
          minutes = [NSString stringWithFormat:NSLocalizedString( @"%d minutes ", nil ), p.minutes];
        else if (p.minutes == 1)
          minutes = NSLocalizedString( @"1 minute ", nil );
        else
          minutes = @"";
        
        if (p.hours >= 1)
        {
            NSString * hours;
            if (p.hours > 1)
              hours = [NSString stringWithFormat:NSLocalizedString( @"%d hours ", nil ), p.hours];
            else
              hours = NSLocalizedString( @"1 hour ", nil );

            return [NSString stringWithFormat:NSLocalizedString( @"%@%@remaining", nil ), hours, minutes];
        }
        
        else
        {
            NSString * seconds;
            if (p.seconds > 1)
              seconds = [NSString stringWithFormat:NSLocalizedString( @"%d seconds ", nil ), p.seconds];
            else
              seconds = NSLocalizedString( @"1 second ", nil );

            return [NSString stringWithFormat:NSLocalizedString( @"%@%@remaining", nil ), minutes, seconds];
        }

/* here is code that does it more like the Finder
        if( p.seconds > -1 )
        {
            float estHours = (p.hours + (p.minutes / 60.0));
            float estMinutes = (p.minutes + (p.seconds / 60.0));

            if (estHours > 1.5)
                return [NSString stringWithFormat:NSLocalizedString( @"Time remaining: About %d hours", nil ), lrintf(estHours)];
            else if (estHours > 0.983)    // 59 minutes
                return NSLocalizedString( @"Time remaining: About 1 hour", nil );
            else if (estMinutes > 1.5)
                return [NSString stringWithFormat:NSLocalizedString( @"Time remaining: About %d minutes", nil ), lrintf(estMinutes)];
            else if (estMinutes > 0.983)    // 59 seconds
                return NSLocalizedString( @"Time remaining: About 1 minute", nil );
            else if (p.seconds <= 5)
                return NSLocalizedString( @"Time remaining: Less than 5 seconds", nil );
            else if (p.seconds <= 10)
                return NSLocalizedString( @"Time remaining: Less than 10 seconds", nil );
            else
                return NSLocalizedString( @"Time remaining: Less than 1 minute", nil );
        }
        else
            return NSLocalizedString( @"Time remaining: Calculating...", nil );
*/
        #undef p
    }
    
    return @"";
}

//------------------------------------------------------------------------------------
// Refresh progress bar (fProgressBar) from current state.
//------------------------------------------------------------------------------------
- (void) updateProgressBarWithState: (hb_state_t *)s
{
    if (s->state == HB_STATE_WORKING)
    {
        #define p s->param.working
        [fProgressBar setIndeterminate:NO];

        float progress_total = fShowsJobsAsGroups ?
                100.0 * ( p.progress + p.job_cur - 1 ) / p.job_count :
                100.0 * p.progress;

        [fProgressBar setDoubleValue:progress_total];
        #undef p
    }
    
    else if (s->state == HB_STATE_MUXING)
    {
        #define p s->param.muxing
        [fProgressBar setIndeterminate:YES];
        [fProgressBar startAnimation:nil];
        #undef p
    }

    else if (s->state == HB_STATE_WORKDONE)
    {
        [fProgressBar setIndeterminate:NO];
        [fProgressBar setDoubleValue:0.0];
    }
}

//------------------------------------------------------------------------------------
// Refresh queue count text field (fQueueCountField).
//------------------------------------------------------------------------------------
- (void)updateQueueCountField
{
    NSString * msg;
    int jobCount;
    
    if (fShowsJobsAsGroups)
    {
        jobCount = fHandle ? hb_group_count(fHandle) : 0;
        if (jobCount == 1)
            msg = NSLocalizedString(@"1 pending encode", nil);
        else
            msg = [NSString stringWithFormat:NSLocalizedString(@"%d pending encodes", nil), jobCount];
    }
    else
    {
        jobCount = fHandle ? hb_count(fHandle) : 0;
        if (jobCount == 1)
            msg = NSLocalizedString(@"1 pending pass", nil);
        else
            msg = [NSString stringWithFormat:NSLocalizedString(@"%d pending passes", nil), jobCount];

    }

    [fQueueCountField setStringValue:msg];
}

//------------------------------------------------------------------------------------
// Refresh the UI in the current job pane. Should be called whenever the current job
// being processed has changed or when progress has changed.
//------------------------------------------------------------------------------------
- (void)updateCurrentJobUI
{
    hb_state_t s;
    hb_job_t * job = nil;
    
    if (fHandle)
    {
        hb_get_state2( fHandle, &s );
        job = hb_current_job(fHandle);
    }

    if (job)
    {
        [fJobDescTextField setAttributedStringValue:[self attributedDescriptionForJob:job withTitle:YES withDetail:YES withHighlighting:NO]];

        [self showCurrentJobPane:YES];
        [fJobIconView setImage: fShowsJobsAsGroups ? [NSImage imageNamed:@"JobLarge"] : [self largeImageForPass: job->pass] ];
        
        NSString * statusMsg = [self progressStatusStringForJob:job state:&s];
        NSString * timeMsg = [self progressTimeRemainingStringForJob:job state:&s];
        if ([timeMsg length] > 0)
            statusMsg = [NSString stringWithFormat:@"%@ - %@", statusMsg, timeMsg];
        [fProgressTextField setStringValue:statusMsg];
        [self updateProgressBarWithState:&s];
    }
    else
    {
        [fJobDescTextField setStringValue:NSLocalizedString(@"No job processing", nil)];

        [self showCurrentJobPane:NO];
        [fProgressBar stopAnimation:nil];    // just in case in was animating
    }
}

//------------------------------------------------------------------------------------
// Refresh the UI in the queue pane. Should be called whenever the content of HB's job
// list has changed so that HBQueueController can sync up.
//------------------------------------------------------------------------------------
- (void)updateQueueUI
{
#if HB_OUTLINE_QUEUE
    [self rebuildEncodes];
    [fOutlineView noteNumberOfRowsChanged];
    [fOutlineView reloadData];
#endif
    [fTaskView noteNumberOfRowsChanged];
    [fTaskView reloadData];
    
    [self updateQueueCountField];
}

//------------------------------------------------------------------------------------
// Deletes the selected job from HB and the queue UI
//------------------------------------------------------------------------------------
- (IBAction)removeSelectedJob: (id)sender
{
    if (!fHandle) return;
    
    int row = [sender selectedRow];
    if (row != -1)
    {
#if HB_UNI_QUEUE
        if (row == 0)
        {
            [self cancelCurrentJob:sender];
        }
        else
        {
            row--;
            if (fShowsJobsAsGroups)
                hb_rem_group( fHandle, hb_group( fHandle, row ) );
            else
                hb_rem( fHandle, hb_job( fHandle, row ) );
        }
#else
        if (fShowsJobsAsGroups)
            hb_rem_group( fHandle, hb_group( fHandle, row ) );
        else
            hb_rem( fHandle, hb_job( fHandle, row ) );
#endif
        [self updateQueueUI];
    }
}

//------------------------------------------------------------------------------------
// Prompts user if the want to cancel encoding of current job. If so, doCancelCurrentJob
// gets called.
//------------------------------------------------------------------------------------
- (IBAction)cancelCurrentJob: (id)sender
{
    [fHBController Cancel:sender];
}

//------------------------------------------------------------------------------------
// Turns on the display of detail information for each job. Does nothing if detail is
// already turned on.
//------------------------------------------------------------------------------------
- (IBAction)showDetail: (id)sender
{
    if (!fShowsDetail)
        [self setShowsDetail:YES];
}

//------------------------------------------------------------------------------------
// Turns off the display of detail information for each job. Does nothing if detail is
// already turned off.
//------------------------------------------------------------------------------------
- (IBAction)hideDetail: (id)sender
{
    if (fShowsDetail)
        [self setShowsDetail:NO];
}

//------------------------------------------------------------------------------------
// Turns on displaying of jobs as groups by calling setShowsJobsAsGroups:YES. Does
// nothing if groups are already turned on. 
//------------------------------------------------------------------------------------
- (IBAction)showJobsAsGroups: (id)sender
{
    if (!fShowsJobsAsGroups)
        [self setShowsJobsAsGroups:YES];
}

//------------------------------------------------------------------------------------
// Turns on displaying of jobs as individual items by calling setShowsJobsAsGroups:NO.
// Does nothing if groups are already turned off. 
//------------------------------------------------------------------------------------
- (IBAction)showJobsAsPasses: (id)sender
{
    if (fShowsJobsAsGroups)
        [self setShowsJobsAsGroups:NO];
}

//------------------------------------------------------------------------------------
// Toggles the processing of jobs on or off depending on the current state
//------------------------------------------------------------------------------------
- (IBAction)toggleStartPause: (id)sender
{
    if (!fHandle) return;
    
    hb_state_t s;
    hb_get_state2 (fHandle, &s);

    if (s.state == HB_STATE_PAUSED)
        hb_resume (fHandle);
    else if ((s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
        hb_pause (fHandle);
    else
    {
        if (fShowsJobsAsGroups)
        {
            if (hb_group_count(fHandle) > 0)
                [fHBController doRip];
        }
        else if (hb_count(fHandle) > 0)
            [fHBController doRip];
    }    
}

#pragma mark -
#pragma mark Toolbar

//------------------------------------------------------------------------------------
// setupToolbar
//------------------------------------------------------------------------------------
- (void)setupToolbar
{
    // Create a new toolbar instance, and attach it to our window 
    NSToolbar *toolbar = [[[NSToolbar alloc] initWithIdentifier: HBQueueToolbar] autorelease];
    
    // Set up toolbar properties: Allow customization, give a default display mode, and remember state in user defaults 
    [toolbar setAllowsUserCustomization: YES];
    [toolbar setAutosavesConfiguration: YES];
    [toolbar setDisplayMode: NSToolbarDisplayModeIconAndLabel];
    
    // We are the delegate
    [toolbar setDelegate: self];
    
    // Attach the toolbar to our window 
    [fQueueWindow setToolbar: toolbar];
}

//------------------------------------------------------------------------------------
// toolbar:itemForItemIdentifier:willBeInsertedIntoToolbar:
//------------------------------------------------------------------------------------
- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar
        itemForItemIdentifier:(NSString *)itemIdentifier
        willBeInsertedIntoToolbar:(BOOL)flag
{
    // Required delegate method: Given an item identifier, this method returns an item.
    // The toolbar will use this method to obtain toolbar items that can be displayed
    // in the customization sheet, or in the toolbar itself.
    
    NSToolbarItem *toolbarItem = nil;
    
    if ([itemIdentifier isEqual: HBStartPauseResumeToolbarIdentifier])
    {
        toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdentifier] autorelease];
		
        // Set the text label to be displayed in the toolbar and customization palette 
		[toolbarItem setLabel: @"Start"];
		[toolbarItem setPaletteLabel: @"Start/Pause"];
		
		// Set up a reasonable tooltip, and image
		[toolbarItem setToolTip: @"Start Encoding"];
		[toolbarItem setImage: [NSImage imageNamed: @"Play"]];
		
		// Tell the item what message to send when it is clicked 
		[toolbarItem setTarget: self];
		[toolbarItem setAction: @selector(toggleStartPause:)];
	}
    
    else if ([itemIdentifier isEqual: HBShowDetailToolbarIdentifier])
    {
        toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdentifier] autorelease];
		
        // Set the text label to be displayed in the toolbar and customization palette 
		[toolbarItem setLabel: @"Detail"];
		[toolbarItem setPaletteLabel: @"Detail"];
		
		// Set up a reasonable tooltip, and image
		[toolbarItem setToolTip: @"Displays detailed information in the queue"];
		[toolbarItem setImage: [NSImage imageNamed: @"Detail"]];
		
		// Tell the item what message to send when it is clicked 
		[toolbarItem setTarget: self];
		[toolbarItem setAction: fShowsDetail ? @selector(hideDetail:) : @selector(showDetail:)];
	}
    
    else if ([itemIdentifier isEqual: HBShowGroupsToolbarIdentifier])
    {
        toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdentifier] autorelease];
		
/*
        // Set the text label to be displayed in the toolbar and customization palette 
		[toolbarItem setLabel: @"Passes"];
		[toolbarItem setPaletteLabel: @"Passes"];
		
		// Set up a reasonable tooltip, and image
		[toolbarItem setToolTip: @"Displays individual passes in the queue"];
		[toolbarItem setImage: [NSImage imageNamed: @"Passes"]];
        
		// Tell the item what message to send when it is clicked 
		[toolbarItem setTarget: self];
		[toolbarItem setAction: fShowsJobsAsGroups ? @selector(showJobsAsPasses:) : @selector(showJobsAsGroups:)];
*/
  
// Various attempts at other button types in the toolbar. A matrix worked fine to display
// a button for encodes & passes, but ultimately I decided to go with a single button
// called "Passes" that toggles on or off. All these suffer from the fact taht you need
// to override NSToolbarItem for them in order to validate their state.
		[toolbarItem setLabel: @"View"];
		[toolbarItem setPaletteLabel: @"View"];

        NSButtonCell * buttonCell = [[[NSButtonCell alloc] initImageCell:nil] autorelease];
        [buttonCell setBezelStyle:NSShadowlessSquareBezelStyle];
        [buttonCell setButtonType:NSToggleButton];
        [buttonCell setBordered:NO];
        [buttonCell setImagePosition:NSImageOnly];

        NSMatrix * matrix = [[[NSMatrix alloc] initWithFrame:NSMakeRect(0,0,54,25)
                mode:NSRadioModeMatrix
                prototype:buttonCell
                numberOfRows:1
                numberOfColumns:2] autorelease];
        [matrix setCellSize:NSMakeSize(27, 25)];
        [matrix setIntercellSpacing:NSMakeSize(0, 0)];
        [matrix selectCellAtRow:0 column:(fShowsJobsAsGroups ? 0 : 1)];

        buttonCell = [matrix cellAtRow:0 column:0];
        [buttonCell setTitle:@""];
        [buttonCell setImage:[NSImage imageNamed: @"Encodes"]];
        [buttonCell setAlternateImage:[NSImage imageNamed: @"EncodesPressed"]];
		[buttonCell setAction: @selector(showJobsAsGroups:)];
		[matrix setToolTip: @"Displays encodes in the queue" forCell:buttonCell];

        buttonCell = [matrix cellAtRow:0 column:1];
        [buttonCell setTitle:@""];
        [buttonCell setImage:[NSImage imageNamed: @"Passes"]];
        [buttonCell setAlternateImage:[NSImage imageNamed: @"PassesPressed"]];
		[buttonCell setAction: @selector(showJobsAsPasses:)];
		[matrix setToolTip: @"Displays individual passes in the queue" forCell:buttonCell];

        [toolbarItem setMinSize: [matrix frame].size];
        [toolbarItem setMaxSize: [matrix frame].size];
		[toolbarItem setView: matrix];

/*
        NSSegmentedControl * segControl = [[[NSSegmentedControl alloc] initWithFrame:NSMakeRect(0,0,20,20)] autorelease];
        [[segControl cell] setControlSize:NSSmallControlSize];
        [segControl setSegmentCount:2];
        [segControl setLabel:@"Encodes" forSegment:0];
        [segControl setLabel:@"Passes" forSegment:1];
        [segControl setImage:[NSImage imageNamed:@"Delete"] forSegment:0];
        [segControl setImage:[NSImage imageNamed:@"Delete"] forSegment:1];
        [segControl setSelectedSegment: (fShowsJobsAsGroups ? 0 : 1)];
        [segControl sizeToFit];
        [toolbarItem setMinSize: [segControl frame].size];
        [toolbarItem setMaxSize: [segControl frame].size];
		[toolbarItem setView: segControl];
*/

/*
        NSButton * button = [[[NSButton alloc] initWithFrame:NSMakeRect(0,0,32,32)] autorelease];
        [button setButtonType:NSSwitchButton];
        [button setTitle:@""];
        [button setState: fShowsJobsAsGroups ? NSOnState : NSOffState];
        [toolbarItem setMinSize: NSMakeSize(20,20)];
        [toolbarItem setMaxSize: NSMakeSize(20,20)];
		[toolbarItem setView: button];
		
		// Tell the item what message to send when it is clicked 
		[toolbarItem setTarget: self];
		[toolbarItem setAction: @selector(jobGroupsChanged:)];
*/
	}
    
    return toolbarItem;
}

//------------------------------------------------------------------------------------
// toolbarDefaultItemIdentifiers:
//------------------------------------------------------------------------------------
- (NSArray *) toolbarDefaultItemIdentifiers: (NSToolbar *) toolbar
{
    // Required delegate method: Returns the ordered list of items to be shown in the
    // toolbar by default.
    
    return [NSArray arrayWithObjects:
        HBStartPauseResumeToolbarIdentifier,
		NSToolbarSeparatorItemIdentifier,
		HBShowGroupsToolbarIdentifier,
        HBShowDetailToolbarIdentifier,
        nil];
}

//------------------------------------------------------------------------------------
// toolbarAllowedItemIdentifiers:
//------------------------------------------------------------------------------------
- (NSArray *) toolbarAllowedItemIdentifiers: (NSToolbar *) toolbar
{
    // Required delegate method: Returns the list of all allowed items by identifier.
    // By default, the toolbar does not assume any items are allowed, even the
    // separator. So, every allowed item must be explicitly listed.

    return [NSArray arrayWithObjects:
        HBStartPauseResumeToolbarIdentifier,
		HBShowGroupsToolbarIdentifier,
        HBShowDetailToolbarIdentifier,
		NSToolbarCustomizeToolbarItemIdentifier,
		NSToolbarFlexibleSpaceItemIdentifier,
        NSToolbarSpaceItemIdentifier,
		NSToolbarSeparatorItemIdentifier,
        nil];
}

//------------------------------------------------------------------------------------
// validateToolbarItem:
//------------------------------------------------------------------------------------
- (BOOL) validateToolbarItem: (NSToolbarItem *) toolbarItem
{
    // Optional method: This message is sent to us since we are the target of some
    // toolbar item actions.

    if (!fHandle) return NO;

    BOOL enable = NO;

    hb_state_t s;
    hb_get_state2 (fHandle, &s);

    if ([[toolbarItem itemIdentifier] isEqual: HBStartPauseResumeToolbarIdentifier])
    {
        if (s.state == HB_STATE_PAUSED)
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"Play"]];
			[toolbarItem setLabel: @"Resume"];
			[toolbarItem setPaletteLabel: @"Resume"];
			[toolbarItem setToolTip: @"Resume Encoding"];
       }
        
        else if ((s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"Pause"]];
			[toolbarItem setLabel: @"Pause"];
			[toolbarItem setPaletteLabel: @"Pause"];
			[toolbarItem setToolTip: @"Pause Encoding"];
        }

        else if (hb_count(fHandle) > 0)
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"Play"]];
			[toolbarItem setLabel: @"Start"];
			[toolbarItem setPaletteLabel: @"Start"];
			[toolbarItem setToolTip: @"Start Encoding"];
        }

        else
        {
            enable = NO;
            [toolbarItem setImage:[NSImage imageNamed: @"Play"]];
			[toolbarItem setLabel: @"Start"];
			[toolbarItem setPaletteLabel: @"Start"];
			[toolbarItem setToolTip: @"Start Encoding"];
        }
	}
    
    else if ([[toolbarItem itemIdentifier] isEqual: HBShowGroupsToolbarIdentifier])
    {
        enable = hb_count(fHandle) > 0;
		[toolbarItem setAction: fShowsJobsAsGroups ? @selector(showJobsAsPasses:) : @selector(showJobsAsGroups:)];
        if (fShowsJobsAsGroups)
        {
            [toolbarItem setImage: [NSImage imageNamed: @"Passes"]];
            [toolbarItem setToolTip: @"Displays individual passes in the queue"];
        }
        else
        {
            [toolbarItem setImage: [NSImage imageNamed: @"PassesPressed"]];
            [toolbarItem setToolTip: @"Displays encodes in the queue"];
        }
    }
    
    else if ([[toolbarItem itemIdentifier] isEqual: HBShowDetailToolbarIdentifier])
    {
        enable = hb_count(fHandle) > 0;
		[toolbarItem setAction: fShowsDetail ? @selector(hideDetail:) : @selector(showDetail:)];
        if (fShowsDetail)
        {
            [toolbarItem setImage: [NSImage imageNamed: @"DetailPressed"]];
            [toolbarItem setToolTip: @"Hides detailed information in the queue"];
        }
        else
        {
            [toolbarItem setImage: [NSImage imageNamed: @"Detail"]];
            [toolbarItem setToolTip: @"Displays detailed information in the queue"];
        }
    }

	return enable;
}

#pragma mark -

//------------------------------------------------------------------------------------
// awakeFromNib
//------------------------------------------------------------------------------------
- (void)awakeFromNib
{
    [self setupToolbar];
    
    if (![fQueueWindow setFrameUsingName:@"Queue"])
        [fQueueWindow center];
    [fQueueWindow setFrameAutosaveName: @"Queue"];
    [fQueueWindow setExcludedFromWindowsMenu:YES];
    
    // Show/hide UI elements
    [self setShowsDetail:fShowsDetail];
    [self setShowsJobsAsGroups:fShowsJobsAsGroups];
    [self showCurrentJobPane:NO];

#if HB_QUEUE_DRAGGING
    [fTaskView registerForDraggedTypes: [NSArray arrayWithObject:HBQueueDataType] ];
#endif

#if HB_OUTLINE_QUEUE
    // Don't allow autoresizing of main column, else the "delete" column will get
    // pushed out of view.
    [fOutlineView setAutoresizesOutlineColumn: NO];
#endif
}


//------------------------------------------------------------------------------------
// windowWillClose
//------------------------------------------------------------------------------------
- (void)windowWillClose:(NSNotification *)aNotification
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"QueueWindowIsOpen"];
}

#pragma mark -
#pragma mark NSTableView delegate

//------------------------------------------------------------------------------------
// NSTableView delegate
//------------------------------------------------------------------------------------
- (int)numberOfRowsInTableView: (NSTableView *)aTableView
{
#if HB_UNI_QUEUE
    int numItems = hb_current_job(fHandle) ? 1 : 0;
    if (fShowsJobsAsGroups)
        return numItems + hb_group_count(fHandle);
    else
        return numItems + hb_count(fHandle);
#else
    if (fShowsJobsAsGroups)
        return hb_group_count(fHandle);
    else
        return hb_count(fHandle);
#endif
}

//------------------------------------------------------------------------------------
// NSTableView delegate
//------------------------------------------------------------------------------------
- (id)tableView: (NSTableView *)aTableView
      objectValueForTableColumn: (NSTableColumn *)aTableColumn
                            row: (int)rowIndex
{
    if (!fHandle)
        return @"";    // fatal error!
        
    hb_job_t * job = nil;

#if HB_UNI_QUEUE
    // Looking for the current job?
    int jobIndex = rowIndex;
    if (hb_current_job(fHandle))
    {
        if (rowIndex == 0)
            job = hb_current_job(fHandle);
        else
            jobIndex = rowIndex - 1;
    }
    
    if (!job)
    {
        if (fShowsJobsAsGroups)
            job = hb_group(fHandle, jobIndex);
        else
            job = hb_job(fHandle, jobIndex);
    }
#else
    if (fShowsJobsAsGroups)
        job = hb_group(fHandle, rowIndex);
    else
        job = hb_job(fHandle, rowIndex);
#endif
    
    if (!job)
        return @"";    // fatal error!

    if ([[aTableColumn identifier] isEqualToString:@"desc"])
    {
        BOOL highlighted = [aTableView isRowSelected:rowIndex] && [[aTableView window] isKeyWindow] && ([[aTableView window] firstResponder] == aTableView);
        return [self attributedDescriptionForJob:job withTitle:YES withDetail:fShowsDetail withHighlighting:highlighted];    
    }
    
    else if ([[aTableColumn identifier] isEqualToString:@"delete"])
        return @"";

    else if ([[aTableColumn identifier] isEqualToString:@"icon"])
        return fShowsJobsAsGroups ? [NSImage imageNamed:@"JobSmall"] : [self smallImageForPass: job->pass];

    return @"";
}

//------------------------------------------------------------------------------------
// NSTableView delegate
//------------------------------------------------------------------------------------
- (void)tableView: (NSTableView *)aTableView
        willDisplayCell: (id)aCell
         forTableColumn: (NSTableColumn *)aTableColumn
                    row: (int)rowIndex
{
    if ([[aTableColumn identifier] isEqualToString:@"delete"])
    {
        BOOL highlighted = [aTableView isRowSelected:rowIndex] && [[aTableView window] isKeyWindow] && ([[aTableView window] firstResponder] == aTableView);
        if (highlighted)
        {
            [aCell setImage:[NSImage imageNamed:@"DeleteHighlight"]];
            [aCell setAlternateImage:[NSImage imageNamed:@"DeleteHighlightPressed"]];
        }
        else
        {
            [aCell setImage:[NSImage imageNamed:@"Delete"]];
        }
    }
}

//------------------------------------------------------------------------------------
// NSTableView delegate
//------------------------------------------------------------------------------------
#if HB_UNI_QUEUE
- (float)tableView:(NSTableView *)tableView heightOfRow:(int)row
{
    if ((row == 0) && hb_current_job(fHandle))
        return HB_ROW_HEIGHT_ACTIVE_JOB;
    else 
        return fShowsDetail ? HB_ROW_HEIGHT_DETAIL : HB_ROW_HEIGHT_NO_DETAIL;
}
#endif

#if HB_QUEUE_DRAGGING
- (BOOL)tableView:(NSTableView *)tv writeRowsWithIndexes:(NSIndexSet *)rowIndexes toPasteboard:(NSPasteboard*)pboard
{
    // Copy the row numbers to the pasteboard.
    NSData *data = [NSKeyedArchiver archivedDataWithRootObject:rowIndexes];
    [pboard declareTypes:[NSArray arrayWithObject:HBQueueDataType] owner:self];
    [pboard setData:data forType:HBQueueDataType];
    return YES;
}
#endif

#if HB_QUEUE_DRAGGING
- (NSDragOperation)tableView:(NSTableView*)tv validateDrop:(id <NSDraggingInfo>)info proposedRow:(int)row proposedDropOperation:(NSTableViewDropOperation)op
{
    // Add code here to validate the drop
    NSLog(@"validate Drop");
    return NSDragOperationEvery;
}
#endif

#if HB_QUEUE_DRAGGING
- (BOOL)tableView:(NSTableView *)aTableView acceptDrop:(id <NSDraggingInfo>)info
            row:(int)row dropOperation:(NSTableViewDropOperation)operation
{
    NSPasteboard* pboard = [info draggingPasteboard];
    NSData* rowData = [pboard dataForType:HBQueueDataType];
    NSIndexSet* rowIndexes = [NSKeyedUnarchiver unarchiveObjectWithData:rowData];
    int dragRow = [rowIndexes firstIndex];
 
    // Move the specified row to its new location...
    
    return YES;
}
#endif


#if HB_OUTLINE_QUEUE

- (id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item
{
    if (item == nil)
        return [fEncodes objectAtIndex:index];
    else
        return [item objectAtIndex:index];
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
{
    return ! [item isKindOfClass:[HBJob class]];
}

- (int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
    if (item == nil)
        return [fEncodes count];
    else
        return [item count];
}

- (float)outlineView:(NSOutlineView *)outlineView heightOfRowByItem:(id)item
{
    if (fShowsDetail && [item isKindOfClass:[HBJob class]])
        return HB_ROW_HEIGHT_DETAIL;
    else
        return HB_ROW_HEIGHT_NO_DETAIL;
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
    BOOL highlighted = [outlineView isRowSelected:[outlineView rowForItem: item]] && [[outlineView window] isKeyWindow] && ([[outlineView window] firstResponder] == outlineView);
    if ([item isKindOfClass:[HBJob class]])
    {
        if ([[tableColumn identifier] isEqualToString:@"desc"])
        {
            hb_job_t * job = [item job];
//            return [self attributedDescriptionForJob:job withTitle:NO withDetail:fShowsDetail withHighlighting:highlighted];
            if (job->pass == -1)
                return @"Subtitle Scan";
            else
            {
                int passNum = MAX( 1, job->pass );
                if (passNum == 1)
                    return @"1st Pass";
                if (passNum == 2)
                    return @"2nd Pass";
                else
                    return [NSString stringWithFormat: @"Pass %d", passNum];
            }
        }
    }
    
    else
    {
        hb_job_t * job = [[item objectAtIndex:0] job];
        if ([[tableColumn identifier] isEqualToString:@"desc"])
            return [self attributedDescriptionForJob:job withTitle:YES withDetail:NO withHighlighting:highlighted];    
    }

    return @"";
}

- (void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    if ([[tableColumn identifier] isEqualToString:@"desc"])
    {
        if ([item isKindOfClass:[HBJob class]])
            [cell setImage:[self smallImageForPass: [item job]->pass]];
        else
            [cell setImage:[NSImage imageNamed:@"JobSmall"]];
    }
    
    else if ([[tableColumn identifier] isEqualToString:@"delete"])
    {
        // The Delete action can only be applied for group items, not indivdual jobs.
        if ([item isKindOfClass:[HBJob class]])
        {
            [cell setEnabled: NO];
            [cell setImage: nil];
        }
        else
        {
            [cell setEnabled: YES];
            BOOL highlighted = [outlineView isRowSelected:[outlineView rowForItem: item]] && [[outlineView window] isKeyWindow] && ([[outlineView window] firstResponder] == outlineView);
            if (highlighted)
            {
                [cell setImage:[NSImage imageNamed:@"DeleteHighlight"]];
                [cell setAlternateImage:[NSImage imageNamed:@"DeleteHighlightPressed"]];
            }
            else
                [cell setImage:[NSImage imageNamed:@"Delete"]];
        }
    }
}

#endif

@end
