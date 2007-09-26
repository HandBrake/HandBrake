/* HBQueueController

    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.m0k.org/>.
    It may be used under the terms of the GNU General Public License. */

#include "HBQueueController.h"
#include "Controller.h"
#import "HBImageAndTextCell.h"

// UNI_QUEUE turns on the feature where the first item in the queue NSTableView is the
// current job followed by the jobs in hblib's queue. In this scheme, fCurrentJobPane
// disappers.
#define HB_UNI_QUEUE 0             // <--- NOT COMPLETELY FUNCTIONAL YET

#define HB_ROW_HEIGHT_TITLE_ONLY           17.0

// Pasteboard type for or drag operations
#define HBQueuePboardType            @"HBQueuePboardType"


//------------------------------------------------------------------------------------
// NSMutableAttributedString (HBAdditions)
//------------------------------------------------------------------------------------

@interface NSMutableAttributedString (HBAdditions)
- (void) appendString: (NSString*)aString withAttributes: (NSDictionary *)aDictionary;
@end

@implementation NSMutableAttributedString (HBAdditions)
- (void) appendString: (NSString*)aString withAttributes: (NSDictionary *)aDictionary
{
    NSAttributedString * s = [[[NSAttributedString alloc]
        initWithString: aString
        attributes: aDictionary] autorelease];
    [self appendAttributedString: s];
}
@end

//------------------------------------------------------------------------------------
#pragma mark -
//------------------------------------------------------------------------------------

@implementation HBQueueOutlineView

- (void)viewDidEndLiveResize
{
    // Since we disabled calculating row heights during a live resize, force them to
    // recalculate now.
    [self noteHeightOfRowsWithIndexesChanged:
            [NSIndexSet indexSetWithIndexesInRange: NSMakeRange(0, [self numberOfRows])]];
    [super viewDidEndLiveResize];
}

@end

//------------------------------------------------------------------------------------
#pragma mark -
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
        hbJob = job;
    }
    return self; 
}

- (hb_job_t*) job
{
    return hbJob;
}

//------------------------------------------------------------------------------------
// Generate string to display in UI.
//------------------------------------------------------------------------------------

- (NSMutableAttributedString *) attributedDescriptionWithHBHandle: (hb_handle_t *)handle
                               withIcon: (BOOL)withIcon
                              withTitle: (BOOL)withTitle
                           withPassName: (BOOL)withPassName
                         withFormatInfo: (BOOL)withFormatInfo
                        withDestination: (BOOL)withDestination
                        withPictureInfo: (BOOL)withPictureInfo
                          withVideoInfo: (BOOL)withVideoInfo
                           withx264Info: (BOOL)withx264Info
                          withAudioInfo: (BOOL)withAudioInfo
                       withSubtitleInfo: (BOOL)withSubtitleInfo

{
    NSMutableAttributedString * finalString = [[[NSMutableAttributedString alloc] initWithString: @""] autorelease];
    
    hb_title_t * title = hbJob->title;
    
    // Attributes
    static NSMutableParagraphStyle * ps = NULL;
    if (!ps)
    {
        ps = [[[NSParagraphStyle defaultParagraphStyle] mutableCopy] retain];
        [ps setHeadIndent: 40.0];
        [ps setParagraphSpacing: 1.0];
        [ps setTabStops:[NSArray array]];    // clear all tabs
        [ps addTabStop: [[[NSTextTab alloc] initWithType: NSLeftTabStopType location: 20.0] autorelease]];
    }

    static NSDictionary* detailAttribute = [[NSDictionary dictionaryWithObjectsAndKeys:
                [NSFont systemFontOfSize:10.0], NSFontAttributeName,
                ps, NSParagraphStyleAttributeName,
                nil] retain];
    static NSDictionary* detailBoldAttribute = [[NSDictionary dictionaryWithObjectsAndKeys:
                [NSFont boldSystemFontOfSize:10.0], NSFontAttributeName,
                ps, NSParagraphStyleAttributeName,
                nil] retain];
    static NSDictionary* titleAttribute = [[NSDictionary dictionaryWithObjectsAndKeys:
                [NSFont systemFontOfSize:[NSFont systemFontSize]], NSFontAttributeName,
                ps, NSParagraphStyleAttributeName,
                nil] retain];
    static NSDictionary* shortHeightAttribute = [[NSDictionary dictionaryWithObjectsAndKeys:
                [NSFont systemFontOfSize:2.0], NSFontAttributeName,
                nil] retain];

    // Title with summary
    if (withTitle)
    {
        if (withIcon)
        {
            NSFileWrapper * wrapper = [[[NSFileWrapper alloc] initWithPath:[[NSBundle mainBundle] pathForImageResource: @"JobSmall"]] autorelease];
            NSTextAttachment * imageAttachment = [[[NSTextAttachment alloc] initWithFileWrapper:wrapper] autorelease];

            NSDictionary* imageAttributes = [NSDictionary dictionaryWithObjectsAndKeys:
                            [NSNumber numberWithFloat: -2.0], NSBaselineOffsetAttributeName,
                            imageAttachment, NSAttachmentAttributeName,
                            ps, NSParagraphStyleAttributeName,
                            nil];

            NSAttributedString * imageAsString = [[[NSAttributedString alloc]
                    initWithString: [NSString stringWithFormat:@"%C%C", NSAttachmentCharacter, NSTabCharacter]
                    attributes: imageAttributes] autorelease];

            [finalString appendAttributedString:imageAsString];
        }
    
        // Note: use title->name instead of title->dvd since name is just the chosen
        // folder, instead of dvd which is the full path
        [finalString appendString:[NSString stringWithUTF8String:title->name] withAttributes:titleAttribute];
        
        NSString * summaryInfo;
    
        NSString * chapterString = (hbJob->chapter_start == hbJob->chapter_end) ?
                [NSString stringWithFormat:@"Chapter %d", hbJob->chapter_start] :
                [NSString stringWithFormat:@"Chapters %d through %d", hbJob->chapter_start, hbJob->chapter_end];

        BOOL hasIndepthScan = (hbJob->pass == -1);
        int numVideoPasses = 0;

        // To determine number of video passes, we need to skip past the subtitle scan.
        if (hasIndepthScan)
        {
            // When job is the one currently being processed, then the next in its group
            // is the the first job in the queue.
            hb_job_t * nextjob;
            if (hbJob == hb_current_job(handle))
                nextjob = hb_job(handle, 0);
            else
                nextjob = hb_next_job(handle, hbJob);
            if (nextjob)    // Overly cautious in case there is no next job!
                numVideoPasses = MIN( 2, nextjob->pass + 1 );
        }
        else
            numVideoPasses = MIN( 2, hbJob->pass + 1 );

        if (hasIndepthScan && numVideoPasses == 1)
            summaryInfo = [NSString stringWithFormat: @"  (Title %d, %@, Deep Scan, Single Video Pass)", title->index, chapterString];
        else if (hasIndepthScan && numVideoPasses > 1)
            summaryInfo = [NSString stringWithFormat: @"  (Title %d, %@, Deep Scan, %d Video Passes)", title->index, chapterString, numVideoPasses];
        else if (numVideoPasses == 1)
            summaryInfo = [NSString stringWithFormat: @"  (Title %d, %@, Single Video Pass)", title->index, chapterString];
        else
            summaryInfo = [NSString stringWithFormat: @"  (Title %d, %@, %d Video Passes)", title->index, chapterString, numVideoPasses];

        [finalString appendString:[NSString stringWithFormat:@"%@\n", summaryInfo] withAttributes:detailAttribute];
        
        // Insert a short-in-height line to put some white space after the title
        [finalString appendString:@"\n" withAttributes:shortHeightAttribute];
    }
    
    // End of title stuff
    

    // Pass Name
    if (withPassName)
    {
        if (withIcon)
        {
            NSString * imageName;
            switch (hbJob->pass)
            {
                case -1: imageName = @"JobPassSubtitleSmall"; break;
                case  0: imageName = @"JobPassFirstSmall"; break;
                case  1: imageName = @"JobPassFirstSmall"; break;
                case  2: imageName = @"JobPassSecondSmall"; break;
                default: imageName = @"JobPassUnknownSmall"; break;
            }

            NSFileWrapper * wrapper = [[[NSFileWrapper alloc] initWithPath:[[NSBundle mainBundle] pathForImageResource: imageName]] autorelease];
            NSTextAttachment * imageAttachment = [[[NSTextAttachment alloc] initWithFileWrapper:wrapper] autorelease];

            NSDictionary* imageAttributes = [NSDictionary dictionaryWithObjectsAndKeys:
                            [NSNumber numberWithFloat: -2.0], NSBaselineOffsetAttributeName,
                            imageAttachment, NSAttachmentAttributeName,
                            ps, NSParagraphStyleAttributeName,
                            nil];

            NSAttributedString * imageAsString = [[[NSAttributedString alloc]
                    initWithString: [NSString stringWithFormat:@"%C%C", NSAttachmentCharacter, NSTabCharacter]
                    attributes: imageAttributes] autorelease];

            [finalString appendAttributedString:imageAsString];
        }
    
        NSString * jobPassName;
        if (hbJob->pass == -1)
            jobPassName = NSLocalizedString (@"Deep Scan", nil);
        else
        {
            int passNum = MAX( 1, hbJob->pass );
            if (passNum == 0)
                jobPassName = NSLocalizedString (@"1st Pass", nil);
            else if (passNum == 1)
                jobPassName = NSLocalizedString (@"1st Pass", nil);
            else if (passNum == 2)
                jobPassName = NSLocalizedString (@"2nd Pass", nil);
            else
                jobPassName = [NSString stringWithFormat: NSLocalizedString(@"Pass %d", nil), passNum];
        }
        [finalString appendString:[NSString stringWithFormat:@"%@\n", jobPassName] withAttributes:detailBoldAttribute];
    }

    // Video Codec needed by FormatInfo and withVideoInfo
    NSString * jobVideoCodec = nil;
    if (withFormatInfo || withVideoInfo)
    {
        // 2097152
        /* Video Codec settings (Encoder in the gui) */
        if (hbJob->vcodec == HB_VCODEC_FFMPEG)
            jobVideoCodec = @"FFmpeg"; // HB_VCODEC_FFMPEG
        else if (hbJob->vcodec == HB_VCODEC_XVID)
            jobVideoCodec = @"XviD"; // HB_VCODEC_XVID
        else if (hbJob->vcodec == HB_VCODEC_X264)
        {
            /* Deterimine for sure how we are now setting iPod uuid atom */
            if (hbJob->h264_level) // We are encoding for iPod
                jobVideoCodec = @"x264 (H.264 iPod)"; // HB_VCODEC_X264    
            else
                jobVideoCodec = @"x264 (H.264 Main)"; // HB_VCODEC_X264
        }
    }
    if (jobVideoCodec == nil)
        jobVideoCodec = @"unknown";
    
    // Audio Codec needed by FormatInfo and AudioInfo
    NSString * jobAudioCodec = nil;
    if (withFormatInfo || withAudioInfo)
    {
        if (hbJob->acodec == 256)
            jobAudioCodec = @"AAC"; // HB_ACODEC_FAAC
        else if (hbJob->acodec == 512)
            jobAudioCodec = @"MP3"; // HB_ACODEC_LAME
        else if (hbJob->acodec == 1024)
            jobAudioCodec = @"Vorbis"; // HB_ACODEC_VORBIS
        else if (hbJob->acodec == 2048)
            jobAudioCodec = @"AC3"; // HB_ACODEC_AC3
    }
    if (jobAudioCodec == nil)
        jobAudioCodec = @"unknown";


    if (withFormatInfo)
    {
        NSString * jobFormatInfo;
        // Muxer settings (File Format in the gui)
        if (hbJob->mux == 65536 || hbJob->mux == 131072 || hbJob->mux == 1048576)
            jobFormatInfo = @"MP4"; // HB_MUX_MP4,HB_MUX_PSP,HB_MUX_IPOD
        else if (hbJob->mux == 262144)
            jobFormatInfo = @"AVI"; // HB_MUX_AVI
        else if (hbJob->mux == 524288)
            jobFormatInfo = @"OGM"; // HB_MUX_OGM
        else if (hbJob->mux == 2097152)
            jobFormatInfo = @"MKV"; // HB_MUX_MKV
        else
            jobFormatInfo = @"unknown";
                
        if (hbJob->chapter_markers == 1)
            jobFormatInfo = [NSString stringWithFormat:@"%@ Container, %@ Video + %@ Audio, Chapter Markers\n", jobFormatInfo, jobVideoCodec, jobAudioCodec];
        else
            jobFormatInfo = [NSString stringWithFormat:@"%@ Container, %@ Video + %@ Audio\n", jobFormatInfo, jobVideoCodec, jobAudioCodec];
            
        [finalString appendString: @"Format: " withAttributes:detailBoldAttribute];
        [finalString appendString: jobFormatInfo withAttributes:detailAttribute];
    }

    if (withDestination)
    {
        [finalString appendString: @"Destination: " withAttributes:detailBoldAttribute];
        [finalString appendString:[NSString stringWithFormat:@"%@\n", [NSString stringWithUTF8String:hbJob->file]] withAttributes:detailAttribute];
    }


    if (withPictureInfo)
    {
        NSString * jobPictureInfo;
        /*integers for picture values deinterlace, crop[4], keep_ratio, grayscale, pixel_ratio, pixel_aspect_width, pixel_aspect_height,
         maxWidth, maxHeight */
        if (hbJob->pixel_ratio == 1)
        {
            int titlewidth = title->width - hbJob->crop[2] - hbJob->crop[3];
            int displayparwidth = titlewidth * hbJob->pixel_aspect_width / hbJob->pixel_aspect_height;
            int displayparheight = title->height - hbJob->crop[0] - hbJob->crop[1];
            jobPictureInfo = [NSString stringWithFormat:@"%dx%d (%dx%d Anamorphic)", displayparwidth, displayparheight, hbJob->width, displayparheight];
        }
        else
            jobPictureInfo = [NSString stringWithFormat:@"%dx%d", hbJob->width, hbJob->height];
        if (hbJob->keep_ratio == 1)
            jobPictureInfo = [jobPictureInfo stringByAppendingString:@" Keep Aspect Ratio"];
        
        if (hbJob->grayscale == 1)
            jobPictureInfo = [jobPictureInfo stringByAppendingString:@", Grayscale"];
        
        if (hbJob->deinterlace == 1)
            jobPictureInfo = [jobPictureInfo stringByAppendingString:@", Deinterlace"];
        if (withIcon)   // implies indent the info
            [finalString appendString: @"\t" withAttributes:detailBoldAttribute];
        [finalString appendString: @"Picture: " withAttributes:detailBoldAttribute];
        [finalString appendString:[NSString stringWithFormat:@"%@\n", jobPictureInfo] withAttributes:detailAttribute];
    }
    
    if (withVideoInfo)
    {
        NSString * jobVideoQuality;
        NSString * jobVideoDetail;
        
        if (hbJob->vquality <= 0 || hbJob->vquality >= 1)
            jobVideoQuality = [NSString stringWithFormat:@"%d kbps", hbJob->vbitrate];
        else
        {
            NSNumber * vidQuality;
            vidQuality = [NSNumber numberWithInt:hbJob->vquality * 100];
            // this is screwed up kind of. Needs to be formatted properly.
            if (hbJob->crf == 1)
                jobVideoQuality = [NSString stringWithFormat:@"%@%% CRF", vidQuality];            
            else
                jobVideoQuality = [NSString stringWithFormat:@"%@%% CQP", vidQuality];
        }
        
        if (hbJob->vrate_base == 1126125)
        {
            /* NTSC FILM 23.976 */
            jobVideoDetail = [NSString stringWithFormat:@"%@, %@, 23.976 fps", jobVideoCodec, jobVideoQuality];
        }
        else if (hbJob->vrate_base == 900900)
        {
            /* NTSC 29.97 */
            jobVideoDetail = [NSString stringWithFormat:@"%@, %@, 29.97 fps", jobVideoCodec, jobVideoQuality];
        }
        else
        {
            /* Everything else */
            jobVideoDetail = [NSString stringWithFormat:@"%@, %@, %d fps", jobVideoCodec, jobVideoQuality, hbJob->vrate / hbJob->vrate_base];
        }
        if (withIcon)   // implies indent the info
            [finalString appendString: @"\t" withAttributes:detailBoldAttribute];
        [finalString appendString: @"Video: " withAttributes:detailBoldAttribute];
        [finalString appendString:[NSString stringWithFormat:@"%@\n", jobVideoDetail] withAttributes:detailAttribute];
    }
    
    if (withx264Info)
    {
        if (hbJob->vcodec == HB_VCODEC_X264 && hbJob->x264opts)
        {
            if (withIcon)   // implies indent the info
                [finalString appendString: @"\t" withAttributes:detailBoldAttribute];
            [finalString appendString: @"x264 Options: " withAttributes:detailBoldAttribute];
            [finalString appendString:[NSString stringWithFormat:@"%@\n", [NSString stringWithUTF8String:hbJob->x264opts]] withAttributes:detailAttribute];
        }
    }

    if (withAudioInfo)
    {
        NSString * jobAudioInfo;
        if ([jobAudioCodec isEqualToString: @"AC3"])
            jobAudioInfo = [NSString stringWithFormat:@"%@, Pass-Through", jobAudioCodec];
        else
            jobAudioInfo = [NSString stringWithFormat:@"%@, %d kbps, %d Hz", jobAudioCodec, hbJob->abitrate, hbJob->arate];
        
        /* we now get the audio mixdown info for each of the two gui audio tracks */
        /* lets do it the long way here to get a handle on things.
            Hardcoded for two tracks for gui: audio_mixdowns[i] audio_mixdowns[i] */
        int ai; // counter for each audios [] , macgui only allows for two audio tracks currently
        for( ai = 0; ai < 2; ai++ )
        {
            if (hbJob->audio_mixdowns[ai] == HB_AMIXDOWN_MONO)
                jobAudioInfo = [jobAudioInfo stringByAppendingString:[NSString stringWithFormat:@", Track %d: Mono", ai + 1]];
            if (hbJob->audio_mixdowns[ai] == HB_AMIXDOWN_STEREO)
                jobAudioInfo = [jobAudioInfo stringByAppendingString:[NSString stringWithFormat:@", Track %d: Stereo", ai + 1]];
            if (hbJob->audio_mixdowns[ai] == HB_AMIXDOWN_DOLBY)
                jobAudioInfo = [jobAudioInfo stringByAppendingString:[NSString stringWithFormat:@", Track %d: Dolby Surround", ai + 1]];
            if (hbJob->audio_mixdowns[ai] == HB_AMIXDOWN_DOLBYPLII)
                jobAudioInfo = [jobAudioInfo stringByAppendingString:[NSString stringWithFormat:@", Track %d: Dolby Pro Logic II", ai + 1]];
            if (hbJob->audio_mixdowns[ai] == HB_AMIXDOWN_6CH)
                jobAudioInfo = [jobAudioInfo stringByAppendingString:[NSString stringWithFormat:@", Track %d: 6-channel discreet", ai + 1]];
        }
        if (withIcon)   // implies indent the info
            [finalString appendString: @"\t" withAttributes:detailBoldAttribute];
        [finalString appendString: @"Audio: " withAttributes:detailBoldAttribute];
        [finalString appendString:[NSString stringWithFormat:@"%@\n", jobAudioInfo] withAttributes:detailAttribute];
    }
    
    if (withSubtitleInfo)
    {
        // hbJob->subtitle can == -1 in two cases:
        // autoselect: when pass == -1
        // none: when pass != -1
        if ((hbJob->subtitle == -1) && (hbJob->pass == -1))
        {
            if (withIcon)   // implies indent the info
                [finalString appendString: @"\t" withAttributes:detailBoldAttribute];
            [finalString appendString: @"Subtitles: " withAttributes:detailBoldAttribute];
            [finalString appendString: @"Autoselect " withAttributes:detailAttribute];
        }
        else if (hbJob->subtitle >= 0)
        {
            hb_subtitle_t * subtitle = (hb_subtitle_t *) hb_list_item( title->list_subtitle, 0 );
            if (subtitle)
            {
                if (withIcon)   // implies indent the info
                    [finalString appendString: @"\t" withAttributes:detailBoldAttribute];
                [finalString appendString: @"Subtitles: " withAttributes:detailBoldAttribute];
                [finalString appendString: [NSString stringWithCString: subtitle->lang] withAttributes:detailAttribute];
            }
        }
    }
    
    
    if ([[finalString string] hasSuffix: @"\n"])
        [finalString deleteCharactersInRange: NSMakeRange([[finalString string] length]-1, 1)];
    
    return finalString;
}

@end

#pragma mark -

//------------------------------------------------------------------------------------
// HBJobGroup
//------------------------------------------------------------------------------------

@implementation HBJobGroup

+ (HBJobGroup *) jobGroup;
{
    return [[[HBJobGroup alloc] init] autorelease];
}

- (id) init
{
    if (self = [super init])
    {
        fJobs = [[NSMutableArray arrayWithCapacity:0] retain];
        fDescription = [[NSMutableAttributedString alloc] initWithString: @""];
        [self setNeedsDescription: NO];
    }
    return self; 
}

- (void) dealloc
{
    [fJobs release];
    [super dealloc];
}

- (unsigned int) count
{
    return [fJobs count];
}

- (void) addJob: (HBJob *)aJob
{
    [fJobs addObject: aJob];
    [self setNeedsDescription: YES];
    fLastDescriptionHeight = 0;
    fLastDescriptionWidth = 0;
}

- (HBJob *) jobAtIndex: (unsigned)index
{
    return [fJobs objectAtIndex: index];
}

- (unsigned) indexOfJob: (HBJob *)aJob;
{
    return [fJobs indexOfObject: aJob];
}

- (NSEnumerator *) jobEnumerator
{
    return [fJobs objectEnumerator];
}

- (void) setNeedsDescription: (BOOL)flag
{
    fNeedsDescription = flag;
}

- (void) updateDescriptionWithHBHandle: (hb_handle_t *)handle
{
    [fDescription deleteCharactersInRange: NSMakeRange(0, [fDescription length])]; 

    if ([self count] == 0)
    {
        NSAssert(NO, @" jobgroup with no jobs");
        return;
    }
    
    HBJob * job = [self jobAtIndex:0];
    
    [fDescription appendAttributedString: [job attributedDescriptionWithHBHandle: handle
                             withIcon: NO
                            withTitle: YES
                         withPassName: NO
                       withFormatInfo: YES
                      withDestination: YES
                      withPictureInfo: NO
                        withVideoInfo: NO
                         withx264Info: NO
                        withAudioInfo: NO
                     withSubtitleInfo: NO]];

    static NSAttributedString * carriageReturn = [[NSAttributedString alloc] initWithString:@"\n"];
    
    NSEnumerator * e = [self jobEnumerator];
    while ( (job = [e nextObject]) )
    {
        int pass = [job job]->pass;
        [fDescription appendAttributedString:carriageReturn];
        [fDescription appendAttributedString:
            [job attributedDescriptionWithHBHandle: handle
                                 withIcon: YES
                                withTitle: NO
                             withPassName: YES
                           withFormatInfo: NO
                          withDestination: NO
                          withPictureInfo: pass != -1
                            withVideoInfo: pass != -1
                             withx264Info: pass != -1
                            withAudioInfo: pass == 0 || pass == 2
                         withSubtitleInfo: YES]];
    }
    
    fNeedsDescription = NO;
}

- (NSMutableAttributedString *) attributedDescriptionWithHBHandle: (hb_handle_t *)handle
{
    if (fNeedsDescription)
        [self updateDescriptionWithHBHandle: handle];
    return fDescription;
}

- (float) heightOfDescriptionForWidth:(float)width withHBHandle: (hb_handle_t *)handle
{
    // Try to return the cached value if no changes have happened since the last time
    if ((width == fLastDescriptionWidth) && (fLastDescriptionHeight != 0) && !fNeedsDescription)
        return fLastDescriptionHeight;
    
    if (fNeedsDescription)
        [self updateDescriptionWithHBHandle: handle];

    // Calculate the height    
    NSRect bounds = [fDescription boundingRectWithSize:NSMakeSize(width, 10000) options:NSStringDrawingUsesLineFragmentOrigin];
    fLastDescriptionHeight = bounds.size.height + 6.0; // add some border to bottom
    fLastDescriptionWidth = width;
    return fLastDescriptionHeight;

/* supposedly another way to do this, in case boundingRectWithSize isn't working
    NSTextView* tmpView = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, width, 1)];
    [[tmpView textStorage] setAttributedString:aString];
    [tmpView setHorizontallyResizable:NO];
    [tmpView setVerticallyResizable:YES];
//    [[tmpView textContainer] setHeightTracksTextView: YES];
//    [[tmpView textContainer] setContainerSize: NSMakeSize(width, 10000)];
    [tmpView sizeToFit];
    float height = [tmpView frame].size.height;
    [tmpView release];
    return height;
*/
}

- (float) lastDescriptionHeight
{
    return fLastDescriptionHeight;
}

@end


#pragma mark -

// Toolbar identifiers
static NSString*    HBQueueToolbar                            = @"HBQueueToolbar1";
static NSString*    HBQueueStartCancelToolbarIdentifier       = @"HBQueueStartCancelToolbarIdentifier";
static NSString*    HBQueuePauseResumeToolbarIdentifier       = @"HBQueuePauseResumeToolbarIdentifier";


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

        fJobGroups = [[NSMutableArray arrayWithCapacity:0] retain];

        BOOL loadSucceeded = [NSBundle loadNibNamed:@"Queue" owner:self] && fQueueWindow;
        NSAssert(loadSucceeded, @"Could not open Queue nib");
        NSAssert(fQueueWindow, @"fQueueWindow not found in Queue nib");
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
    
    [fJobGroups release];
    [fSavedExpandedItems release];

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
    if (showPane == fCurrentJobPaneShown)
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
    fCurrentJobPaneShown = showPane;
}

//------------------------------------------------------------------------------------
// Rebuilds the contents of fJobGroups which is a hierarchy of HBJobGroup and HBJobs.
//------------------------------------------------------------------------------------
- (void)rebuildJobGroups
{
    [fJobGroups autorelease];
    fJobGroups = [[NSMutableArray arrayWithCapacity:0] retain];

    HBJobGroup * aJobGroup = [HBJobGroup jobGroup];

    hb_job_t * nextJob = hb_group( fHandle, 0 );
    while( nextJob )
    {
        if (nextJob->sequence_id == 0)
        {
            // Encountered a new group. Add the current one to fJobGroups and then start a new one.
            if ([aJobGroup count] > 0)
            {
                [fJobGroups addObject: aJobGroup];
                aJobGroup = [HBJobGroup jobGroup];
            }
        }
        [aJobGroup addJob: [HBJob jobWithJob:nextJob]];
        nextJob = hb_next_job (fHandle, nextJob);
    }
    if ([aJobGroup count] > 0)
    {
        [fJobGroups addObject:aJobGroup];
    }
}

//------------------------------------------------------------------------------------
// Saves the state of the items that are currently expanded. Calling restoreOutlineViewState
// will restore the state of all items to match what was saved by saveOutlineViewState.
//------------------------------------------------------------------------------------
- (void) saveOutlineViewState
{
    if (!fSavedExpandedItems)
        fSavedExpandedItems = [[NSMutableIndexSet alloc] init];
    else
        [fSavedExpandedItems removeAllIndexes];
    
    // NB: This code is stuffing the address of each job into an index set. While it
    // works 99.9% of the time, it's not the ideal solution. We need unique ids in
    // each job, possibly using the existing sequence_id field. Could use the high
    // word as a unique encode id and the low word the sequence number.
    
    HBJobGroup * aJobGroup;
    NSEnumerator * e = [fJobGroups objectEnumerator];
    while ( (aJobGroup = [e nextObject]) )
    {
        if ([fOutlineView isItemExpanded: aJobGroup])
            [fSavedExpandedItems addIndex: (unsigned int)[[aJobGroup jobAtIndex:0] job]];
    }
    
    // Save the selection also. This is really UGLY code. Since I have to rebuild the
    // entire outline hierachy every time hblib changes its job list, there's no easy
    // way for me to remember the selection state other than saving off the first
    // hb_job_t item in each selected group. This is done by saving the object's
    // address. This could go away if I'd save a unique id in each job object.

    int selection = [fOutlineView selectedRow];
    if (selection == -1)
        fSavedSelectedItem = 0;
    else
    {
        HBJobGroup * jobGroup = [fOutlineView itemAtRow: selection];
        fSavedSelectedItem = (unsigned int)[[jobGroup jobAtIndex:0] job];
    }
    
}

//------------------------------------------------------------------------------------
// Restores the expanded state of items in the outline view to match those saved by a
// previous call to saveOutlineViewState.
//------------------------------------------------------------------------------------
- (void) restoreOutlineViewState
{
    if (fSavedExpandedItems)
    {
        HBJobGroup * aJobGroup;
        NSEnumerator * e = [fJobGroups objectEnumerator];
        while ( (aJobGroup = [e nextObject]) )
        {
            hb_job_t * j = [[aJobGroup jobAtIndex:0] job];
            if ([fSavedExpandedItems containsIndex: (unsigned int)j])
                [fOutlineView expandItem: aJobGroup];
        }
    }
    
    if (fSavedSelectedItem)
    {
        // Ugh. Have to cycle through each row looking for the previously selected job.
        // See the explanation in saveExpandedItems about the logic here.
        
        // Find out what hb_job_t was selected
        hb_job_t * j = (hb_job_t *)fSavedSelectedItem;
        
        int rowToSelect = -1;
        for (int i = 0; i < [fOutlineView numberOfRows]; i++)
        {
            HBJobGroup * jobGroup = [fOutlineView itemAtRow: i];
            // Test to see if the group's first job is a match
            if ([[jobGroup jobAtIndex:0] job] == j)
            {
                rowToSelect = i;
                break;
            }
        }
        if (rowToSelect == -1)
            [fOutlineView deselectAll: nil];
        else
            [fOutlineView selectRow:rowToSelect byExtendingSelection:NO];
    }
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
            msg = NSLocalizedString( @"Deep Scan", nil );
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
        float progress_total = 100.0 * ( p.progress + p.job_cur - 1 ) / p.job_count;
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
    
    jobCount = fHandle ? hb_group_count(fHandle) : 0;
    if (jobCount == 1)
        msg = NSLocalizedString(@"1 pending encode", nil);
    else
        msg = [NSString stringWithFormat:NSLocalizedString(@"%d pending encodes", nil), jobCount];

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
        if (fLastKnownCurrentJob != job)
        {
            HBJob * currentJob = [HBJob jobWithJob: job];
            
            switch (job->pass)
            {
                case -1:  // Subtitle scan
                    [fJobDescTextField setAttributedStringValue:
                        [currentJob attributedDescriptionWithHBHandle:fHandle
                                     withIcon: NO
                                    withTitle: YES
                                 withPassName: YES
                               withFormatInfo: NO
                              withDestination: NO
                              withPictureInfo: NO
                                withVideoInfo: NO
                                 withx264Info: NO
                                withAudioInfo: NO
                             withSubtitleInfo: YES]];
                    break;
                    
                case 1:  // video 1st pass
                    [fJobDescTextField setAttributedStringValue:
                        [currentJob attributedDescriptionWithHBHandle:fHandle
                                     withIcon: NO
                                    withTitle: YES
                                 withPassName: YES
                               withFormatInfo: NO
                              withDestination: NO
                              withPictureInfo: YES
                                withVideoInfo: YES
                                 withx264Info: YES
                                withAudioInfo: NO
                             withSubtitleInfo: NO]];
                    break;
                
                case 0:  // single pass
                case 2:  // video 2nd pass + audio
                    [fJobDescTextField setAttributedStringValue:
                        [currentJob attributedDescriptionWithHBHandle:fHandle
                                     withIcon: NO
                                    withTitle: YES
                                 withPassName: YES
                               withFormatInfo: NO
                              withDestination: NO
                              withPictureInfo: YES
                                withVideoInfo: YES
                                 withx264Info: YES
                                withAudioInfo: YES
                             withSubtitleInfo: YES]];
                    break;
                
                default: // unknown
                    [fJobDescTextField setAttributedStringValue:
                        [currentJob attributedDescriptionWithHBHandle:fHandle
                                     withIcon: NO
                                    withTitle: YES
                                 withPassName: YES
                               withFormatInfo: NO
                              withDestination: NO
                              withPictureInfo: YES
                                withVideoInfo: YES
                                 withx264Info: YES
                                withAudioInfo: YES
                             withSubtitleInfo: YES]];
            }

            [self showCurrentJobPane:YES];
            [fJobIconView setImage: [NSImage imageNamed:@"JobLarge"]];
        }

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
        
    fLastKnownCurrentJob = job;
}

//------------------------------------------------------------------------------------
// Refresh the UI in the queue pane. Should be called whenever the content of HB's job
// list has changed so that HBQueueController can sync up.
//------------------------------------------------------------------------------------
- (void)updateQueueUI
{
    [self saveOutlineViewState];
    [self rebuildJobGroups];
    [fOutlineView noteNumberOfRowsChanged];
    [fOutlineView reloadData];
    [self restoreOutlineViewState];    
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
            hb_rem_group( fHandle, hb_group( fHandle, row ) );
        }
#else
        HBJobGroup * jobGroup = [fOutlineView itemAtRow: row];
        hb_job_t * job = [[jobGroup jobAtIndex: 0] job];
        hb_rem_group( fHandle, job );
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
// Starts or cancels the processing of jobs depending on the current state
//------------------------------------------------------------------------------------
- (IBAction)toggleStartCancel: (id)sender
{
    if (!fHandle) return;
    
    hb_state_t s;
    hb_get_state2 (fHandle, &s);

    if ((s.state == HB_STATE_PAUSED) || (s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
        [fHBController Cancel: fQueuePane]; // sender == fQueuePane so that warning alert shows up on queue window

    else if (hb_group_count(fHandle) > 0)
        [fHBController doRip];
}

//------------------------------------------------------------------------------------
// Toggles the pause/resume state of hblib
//------------------------------------------------------------------------------------
- (IBAction)togglePauseResume: (id)sender
{
    if (!fHandle) return;
    
    hb_state_t s;
    hb_get_state2 (fHandle, &s);

    if (s.state == HB_STATE_PAUSED)
        hb_resume (fHandle);
    else if ((s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
        hb_pause (fHandle);
}

#if HB_OUTLINE_METRIC_CONTROLS
static float spacingWidth = 3.0;
- (IBAction)imageSpacingChanged: (id)sender;
{
    spacingWidth = [sender floatValue];
    [fOutlineView setNeedsDisplay: YES];
}
- (IBAction)indentChanged: (id)sender
{
    [fOutlineView setIndentationPerLevel: [sender floatValue]];
    [fOutlineView setNeedsDisplay: YES];
}
#endif


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
    
    if ([itemIdentifier isEqual: HBQueueStartCancelToolbarIdentifier])
    {
        toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdentifier] autorelease];
		
        // Set the text label to be displayed in the toolbar and customization palette 
		[toolbarItem setLabel: @"Start"];
		[toolbarItem setPaletteLabel: @"Start/Cancel"];
		
		// Set up a reasonable tooltip, and image
		[toolbarItem setToolTip: @"Start Encoding"];
		[toolbarItem setImage: [NSImage imageNamed: @"Play"]];
		
		// Tell the item what message to send when it is clicked 
		[toolbarItem setTarget: self];
		[toolbarItem setAction: @selector(toggleStartCancel:)];
	}
    
    if ([itemIdentifier isEqual: HBQueuePauseResumeToolbarIdentifier])
    {
        toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdentifier] autorelease];
		
        // Set the text label to be displayed in the toolbar and customization palette 
		[toolbarItem setLabel: @"Pause"];
		[toolbarItem setPaletteLabel: @"Pause/Resume"];
		
		// Set up a reasonable tooltip, and image
		[toolbarItem setToolTip: @"Pause Encoding"];
		[toolbarItem setImage: [NSImage imageNamed: @"Pause"]];
		
		// Tell the item what message to send when it is clicked 
		[toolbarItem setTarget: self];
		[toolbarItem setAction: @selector(togglePauseResume:)];
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
        HBQueueStartCancelToolbarIdentifier,
        HBQueuePauseResumeToolbarIdentifier,
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
        HBQueueStartCancelToolbarIdentifier,
        HBQueuePauseResumeToolbarIdentifier,
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

    if ([[toolbarItem itemIdentifier] isEqual: HBQueueStartCancelToolbarIdentifier])
    {
        if ((s.state == HB_STATE_PAUSED) || (s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"Stop"]];
			[toolbarItem setLabel: @"Stop"];
			[toolbarItem setToolTip: @"Stop Encoding"];
        }

        else if (hb_count(fHandle) > 0)
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"Play"]];
			[toolbarItem setLabel: @"Start"];
			[toolbarItem setToolTip: @"Start Encoding"];
        }

        else
        {
            enable = NO;
            [toolbarItem setImage:[NSImage imageNamed: @"Play"]];
			[toolbarItem setLabel: @"Start"];
			[toolbarItem setToolTip: @"Start Encoding"];
        }
	}
    
    if ([[toolbarItem itemIdentifier] isEqual: HBQueuePauseResumeToolbarIdentifier])
    {
        if (s.state == HB_STATE_PAUSED)
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"Play"]];
			[toolbarItem setLabel: @"Resume"];
			[toolbarItem setToolTip: @"Resume Encoding"];
       }
        
        else if ((s.state == HB_STATE_WORKING) || (s.state == HB_STATE_MUXING))
        {
            enable = YES;
            [toolbarItem setImage:[NSImage imageNamed: @"Pause"]];
			[toolbarItem setLabel: @"Pause"];
			[toolbarItem setToolTip: @"Pause Encoding"];
        }
        else
        {
            enable = NO;
            [toolbarItem setImage:[NSImage imageNamed: @"Pause"]];
			[toolbarItem setLabel: @"Pause"];
			[toolbarItem setToolTip: @"Pause Encoding"];
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

#if HB_QUEUE_DRAGGING
    [fOutlineView registerForDraggedTypes: [NSArray arrayWithObject:HBQueuePboardType] ];
    [fOutlineView setDraggingSourceOperationMask:NSDragOperationEvery forLocal:YES];
    [fOutlineView setVerticalMotionCanBeginDrag: YES];
#endif

    // Don't allow autoresizing of main column, else the "delete" column will get
    // pushed out of view.
    [fOutlineView setAutoresizesOutlineColumn: NO];
    [fOutlineView setIndentationPerLevel:21];

#if HB_OUTLINE_METRIC_CONTROLS
    [fIndentation setHidden: NO];
    [fSpacing setHidden: NO];
    [fIndentation setIntValue:[fOutlineView indentationPerLevel]];  // debug
    [fSpacing setIntValue:3];       // debug
#endif

    // Show/hide UI elements
    fCurrentJobPaneShown = YES;     // it's shown in the nib
    [self showCurrentJobPane:NO];
}


//------------------------------------------------------------------------------------
// windowWillClose
//------------------------------------------------------------------------------------
- (void)windowWillClose:(NSNotification *)aNotification
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"QueueWindowIsOpen"];
}

#pragma mark -

- (void)moveObjectsInArray:(NSMutableArray *)array fromIndexes:(NSIndexSet *)indexSet toIndex:(unsigned)insertIndex
{
	unsigned index = [indexSet lastIndex];
	unsigned aboveInsertIndexCount = 0;
	
	while (index != NSNotFound)
	{
		unsigned removeIndex;
		
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
}

#pragma mark -
#pragma mark NSOutlineView delegate

- (id)outlineView:(NSOutlineView *)outlineView child:(int)index ofItem:(id)item
{
    if (item == nil)
        return [fJobGroups objectAtIndex:index];
    
    // We are only one level deep, so we can't be asked about children
    NSAssert (NO, @"HBQueueController outlineView:child:ofItem: can't handle nested items.");
    return nil;
}

- (BOOL)outlineView:(NSOutlineView *)outlineView isItemExpandable:(id)item
{
    // Our outline view has no levels, but we can still expand every item. Doing so
    // just makes the row taller. See heightOfRowByItem below.
    return YES;
}

- (int)outlineView:(NSOutlineView *)outlineView numberOfChildrenOfItem:(id)item
{
    // Our outline view has no levels, so number of children will be zero for all
    // top-level items.
    if (item == nil)
        return [fJobGroups count];
    else
        return 0;
}

- (void)outlineViewItemDidCollapse:(NSNotification *)notification
{
    id item = [[notification userInfo] objectForKey:@"NSObject"];
    int row = [fOutlineView rowForItem:item];
    [fOutlineView noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(row,1)]];
}

- (void)outlineViewItemDidExpand:(NSNotification *)notification
{
    id item = [[notification userInfo] objectForKey:@"NSObject"];
    int row = [fOutlineView rowForItem:item];
    [fOutlineView noteHeightOfRowsWithIndexesChanged:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(row,1)]];
}

- (float)outlineView:(NSOutlineView *)outlineView heightOfRowByItem:(id)item
{
    if ([outlineView isItemExpanded: item])
    {
        // Short-circuit here if in a live resize primarily to fix a bug but also to
        // increase resposivness during a resize. There's a bug in NSTableView that
        // causes row heights to get messed up if you try to change them during a live
        // resize. So if in a live resize, simply return the previously calculated
        // height. The row heights will get fixed up after the resize because we have
        // implemented viewDidEndLiveResize to force all of them to be recalculated.
        if ([outlineView inLiveResize] && [item lastDescriptionHeight] > 0)
            return [item lastDescriptionHeight];
        
        float width = [[outlineView tableColumnWithIdentifier: @"desc"] width];
        // Column width is NOT what is ultimately used
        width -= 47;    // 26 pixels for disclosure triangle, 20 for icon, 1 for intercell spacing
        
        float height = [item heightOfDescriptionForWidth: width withHBHandle: fHandle];
        return height;
    }
    else
        return HB_ROW_HEIGHT_TITLE_ONLY;
}

- (id)outlineView:(NSOutlineView *)outlineView objectValueForTableColumn:(NSTableColumn *)tableColumn byItem:(id)item
{
    if ([[tableColumn identifier] isEqualToString:@"desc"])
        return [item attributedDescriptionWithHBHandle: fHandle];
    else
        return @"";
}

- (void)outlineView:(NSOutlineView *)outlineView willDisplayCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    if ([[tableColumn identifier] isEqualToString:@"desc"])
    {
#if HB_OUTLINE_METRIC_CONTROLS
        NSSize theSize = [cell imageSpacing];
        theSize.width = spacingWidth;
        [cell setImageSpacing: theSize];
#endif
        
        // Set the image here since the value returned from outlineView:objectValueForTableColumn: didn't specify the image part
        [cell setImage:[NSImage imageNamed:@"JobSmall"]];
    }
    
    else if ([[tableColumn identifier] isEqualToString:@"delete"])
    {
        // The Delete action can only be applied for group items, not indivdual jobs.
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

- (void)outlineView:(NSOutlineView *)outlineView willDisplayOutlineCell:(id)cell forTableColumn:(NSTableColumn *)tableColumn item:(id)item
{
    // By default, the discolsure image gets centered vertically in the cell. We want
    // always at the top.
    if ([outlineView isItemExpanded: item])
        [cell setImagePosition: NSImageAbove];
    else
        [cell setImagePosition: NSImageOnly];
}

#pragma mark -
#pragma mark NSOutlineView delegate (dragging related)

//------------------------------------------------------------------------------------
// NSTableView delegate
//------------------------------------------------------------------------------------

#if HB_QUEUE_DRAGGING
- (BOOL)outlineView:(NSOutlineView *)outlineView writeItems:(NSArray *)items toPasteboard:(NSPasteboard *)pboard
{
    // Don't retain since this is just holding temporaral drag information, and it is
    //only used during a drag!  We could put this in the pboard actually.
    fDraggedNodes = items;
    
    // Provide data for our custom type, and simple NSStrings.
    [pboard declareTypes:[NSArray arrayWithObjects: HBQueuePboardType, nil] owner:self];

    // the actual data doesn't matter since DragDropSimplePboardType drags aren't recognized by anyone but us!.
    [pboard setData:[NSData data] forType:HBQueuePboardType]; 

    return YES;
}
#endif

#if HB_QUEUE_DRAGGING
- (NSDragOperation)outlineView:(NSOutlineView *)outlineView validateDrop:(id <NSDraggingInfo>)info proposedItem:(id)item proposedChildIndex:(int)index
{
    // Add code here to validate the drop
	BOOL isOnDropTypeProposal = index == NSOutlineViewDropOnItemIndex;
    if (isOnDropTypeProposal)
        return NSDragOperationNone;
        
    return NSDragOperationGeneric;
}
#endif

#if HB_QUEUE_DRAGGING
- (BOOL)outlineView:(NSOutlineView *)outlineView acceptDrop:(id <NSDraggingInfo>)info item:(id)item childIndex:(int)index
{
    NSMutableIndexSet *moveItems = [NSMutableIndexSet indexSet];
    
    id obj;
    NSEnumerator *enumerator = [fDraggedNodes objectEnumerator];
    while (obj = [enumerator nextObject])
    {
        [moveItems addIndex:[fJobGroups indexOfObject:obj]];
    }

    // Rearrange the data and view
    [self saveOutlineViewState];
    [self moveObjectsInArray:fJobGroups fromIndexes:moveItems toIndex: index];
    [fOutlineView reloadData];
    [self restoreOutlineViewState];
        
    return YES;
}
#endif

@end
