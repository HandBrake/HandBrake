/* HBQueueController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.m0k.org/>.
    It may be used under the terms of the GNU General Public License. */

#include "HBQueueController.h"

#if JOB_GROUPS
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
            return hb_job( h, index+1 );
    }
    return NULL;
}

#endif // JOB_GROUPS

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
            nil]];
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
- (void)showDetail: (BOOL)showDetail
{
    // clumsy - have to update UI
    [fDetailCheckbox setState:showDetail ? NSOnState : NSOffState];
    
    [fTaskView setRowHeight:showDetail ? 110.0 : 17.0];
    if ([fTaskView selectedRow] != -1)
        [fTaskView scrollRowToVisible:[fTaskView selectedRow]];
}

//------------------------------------------------------------------------------------
// Generates a multi-line text string that includes the job name on the first line
// followed by details of the job on subsequent lines. If the text is to be drawn as
// part of a highlighted cell, set isHighlighted to true. The returned string may
// contain multiple fonts and paragraph formating.
//------------------------------------------------------------------------------------
- (NSAttributedString *)attributedDescriptionForJob: (hb_job_t *)job
                                         withDetail: (BOOL)detail
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
    static NSDictionary* titleAttribute = [[NSDictionary dictionaryWithObject:
                [NSFont systemFontOfSize:[NSFont systemFontSize]] forKey:NSFontAttributeName] retain];

    finalString = [[[NSMutableAttributedString alloc] init] autorelease];

    // Title, in bold
    // Show the name of the source Note: use title->name instead of title->dvd since
    // name is just the chosen folder, instead of dvd which is the full path
    anAttributedString = [[[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:title->name] attributes:titleAttribute] autorelease];
    [finalString appendAttributedString:anAttributedString];

    if (!detail)
        return finalString;
        
    // Other info in plain
    aMutableString = [NSMutableString stringWithCapacity:200];
    

#if JOB_GROUPS
    // The subtitle scan doesn't contain all the stuff we need (like x264opts).
    // So grab the next job in the group for display purposes.
    if (job->pass == -1)
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
#endif

    NSString * chapterString = (job->chapter_start == job->chapter_end) ?
            [NSString stringWithFormat:@"Chapter %d", job->chapter_start] :
            [NSString stringWithFormat:@"Chapters %d through %d", job->chapter_start, job->chapter_end];
    
    // Scan pass
    if (job->pass == -1)
    {
        [aMutableString appendString:[NSString stringWithFormat:
                @"\nTitle %d, %@, Pass: Scan", title->index, chapterString]];
    }

    // Normal pass
    else
    {
#if JOB_GROUPS
        [aMutableString appendString:[NSString stringWithFormat:
                @"\nTitle %d, %@, %d-Pass",
                title->index, chapterString, MIN( 2, job->pass + 1 )]];
#else
        [aMutableString appendString:[NSString stringWithFormat:
                @"\nTitle %d, %@, Pass %d of %d",
                title->index, chapterString, MAX( 1, job->pass ), MIN( 2, job->pass + 1 )]];
#endif

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
        if (job->pass == -1)
            return NSLocalizedString( @"Analyzing subtitles", nil );
        if (job->pass == 1)
            return NSLocalizedString( @"Analyzing video", nil );
        else if ((job->pass == 0) ||  (job->pass == 2))
            return NSLocalizedString( @"Encoding movie", nil );
    }
    else if (s->state == HB_STATE_MUXING)
    {
        return NSLocalizedString( @"Muxing", nil );
    }
    else if (s->state == HB_STATE_PAUSED)
    {
        return NSLocalizedString( @"Paused", nil );
    }
    else if (s->state == HB_STATE_WORKDONE)
    {
        return NSLocalizedString( @"Done", nil );
    }
    
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
#if JOB_GROUPS
        float progress_total = 100.0 * ( p.progress + p.job_cur - 1 ) / p.job_count;
#else
        float progress_total = 100.0 * p.progress;
#endif
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
// Refresh start/pause button (fStartPauseButton) from current state.
//------------------------------------------------------------------------------------
- (void) updateStartPauseButton
{
    if (!fHandle) return;

    hb_state_t s;
    hb_get_state2 (fHandle, &s);

    if (s.state == HB_STATE_PAUSED)
    {
        [fStartPauseButton setEnabled:YES];
//        [fStartPauseButton setTitle:NSLocalizedString(@"Resume", nil)];
        [fStartPauseButton setImage:[NSImage imageNamed: @"Play"]];
   }
    
    else if ((s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
    {
        [fStartPauseButton setEnabled:YES];
//        [fStartPauseButton setTitle:NSLocalizedString(@"Pause", nil)];
        [fStartPauseButton setImage:[NSImage imageNamed: @"Pause"]];
    }

    else if (hb_count(fHandle) > 0)
    {
        [fStartPauseButton setEnabled:YES];
//        [fStartPauseButton setTitle:NSLocalizedString(@"Start", nil)];
        [fStartPauseButton setImage:[NSImage imageNamed: @"Play"]];
    }

    else
    {
        [fStartPauseButton setEnabled:NO];
//        [fStartPauseButton setTitle:NSLocalizedString(@"Start", nil)];
        [fStartPauseButton setImage:[NSImage imageNamed: @"Play"]];
    }
}

//------------------------------------------------------------------------------------
// Refresh queue count text field (fQueueCountField).
//------------------------------------------------------------------------------------
- (void)updateQueueCountField
{
    NSString * msg;
#if JOB_GROUPS
    int jobCount = fHandle ? hb_group_count(fHandle) : 0;
#else
    int jobCount = fHandle ? hb_count(fHandle) : 0;
#endif
    if (jobCount == 1)
        msg = NSLocalizedString(@"1 pending job", nil);
    else
        msg = [NSString stringWithFormat:NSLocalizedString(@"%d pending jobs", nil), jobCount];
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
        hb_get_state( fHandle, &s );
        job = hb_current_job(fHandle);
    }

    if (job)
    {	
        [fJobDescTextField setAttributedStringValue:[self attributedDescriptionForJob:job withDetail:YES withHighlighting:NO]];

        [self showCurrentJobPane:YES];
        [fProgressStatus setStringValue:[self progressStatusStringForJob:job state:&s]];
        [fProgressTimeRemaining setStringValue:[self progressTimeRemainingStringForJob:job state:&s]];
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
    [fTaskView noteNumberOfRowsChanged];
    [fTaskView reloadData];
    
    [self updateQueueCountField];
    [self updateStartPauseButton];
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
#if JOB_GROUPS
        hb_rem_group( fHandle, hb_group( fHandle, row ) );
#else
        hb_rem( fHandle, hb_job( fHandle, row ) );
#endif
        [self updateQueueUI];
    }
}

//------------------------------------------------------------------------------------
// Prompts user if the want to cancel encoding of current job. If so, hb_stop gets
// called.
//------------------------------------------------------------------------------------
- (IBAction)cancelCurrentJob: (id)sender
{
    if (!fHandle) return;
    
    hb_job_t * job = hb_current_job(fHandle);
    if (!job) return;

    // If command key is down, don't prompt
    BOOL hasCmdKeyMask = ([[NSApp currentEvent] modifierFlags] & NSCommandKeyMask) != 0;
    if (hasCmdKeyMask)
        hb_stop(fHandle);
    else
    {
        NSString * alertTitle = [NSString stringWithFormat:NSLocalizedString(@"Do you want to stop processing of %@?", nil),
                [NSString stringWithUTF8String:job->title->name]];
        
        NSBeginCriticalAlertSheet(
                alertTitle,
                NSLocalizedString(@"Stop Processing", nil), NSLocalizedString(@"Keep Processing", nil), nil, fQueueWindow, self,
                @selector(cancelCurrentJob:returnCode:contextInfo:), nil, nil,
                NSLocalizedString(@"Your movie will be lost if you don't continue processing.", nil),
                [NSString stringWithUTF8String:job->title->name]);

        // cancelCurrentJob:returnCode:contextInfo: will be called when the dialog is dismissed
    }
}

- (void) cancelCurrentJob: (NSWindow *)sheet returnCode: (int)returnCode contextInfo: (void *)contextInfo
{
    if (returnCode == NSAlertDefaultReturn)
        hb_stop(fHandle);
}

//------------------------------------------------------------------------------------
// Enables or disables the display of detail information for each job based on the 
// current value of the fDetailCheckbox control.
//------------------------------------------------------------------------------------
- (IBAction)detailChanged: (id)sender
{
    BOOL detail = [fDetailCheckbox state] == NSOnState;
    [[NSUserDefaults standardUserDefaults] setBool:detail forKey:@"QueueShowsDetail"];

    [self showDetail:detail];
}

//------------------------------------------------------------------------------------
// Toggles the processing of jobs on or off epending on the current state
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
#if JOB_GROUPS
    else if (hb_group_count(fHandle) > 0)
#else
    else if (hb_count(fHandle) > 0)
#endif
        hb_start (fHandle);
}

//------------------------------------------------------------------------------------
// awakeFromNib
//------------------------------------------------------------------------------------
- (void)awakeFromNib
{
    if (![fQueueWindow setFrameUsingName:@"Queue"])
        [fQueueWindow center];
    [fQueueWindow setFrameAutosaveName: @"Queue"];

    // Show/hide UI elements
    [self showDetail:[[NSUserDefaults standardUserDefaults] boolForKey:@"QueueShowsDetail"]];
    [self showCurrentJobPane:NO];
}


//------------------------------------------------------------------------------------
// windowWillClose
//------------------------------------------------------------------------------------
- (void)windowWillClose:(NSNotification *)aNotification
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"QueueWindowIsOpen"];
}

//------------------------------------------------------------------------------------
// NSTableView delegate
//------------------------------------------------------------------------------------
- (int)numberOfRowsInTableView: (NSTableView *)aTableView
{
#if JOB_GROUPS
    return fHandle ? hb_group_count(fHandle) : 0;
#else
    return fHandle ? hb_count(fHandle) : 0;
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
        
    hb_job_t * job;
#if JOB_GROUPS
    job = hb_group(fHandle, rowIndex);
#else
    job = hb_job(fHandle, rowIndex);
#endif
    if (!job)
        return @"";    // fatal error!

    if ([[aTableColumn identifier] isEqualToString:@"desc"])
    {
        BOOL highlighted = [aTableView isRowSelected:rowIndex] && [[aTableView window] isKeyWindow] && ([[aTableView window] firstResponder] == aTableView);
        BOOL showDetail = [fDetailCheckbox state] == NSOnState;
        return [self attributedDescriptionForJob:job withDetail:showDetail withHighlighting:highlighted];    
    }
    
    else if ([[aTableColumn identifier] isEqualToString:@"delete"])
        return @"";

    else if ([[aTableColumn identifier] isEqualToString:@"icon"])
        return [NSImage imageNamed:@"JobSmall"];

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

@end
