/*  HBPreviewGenerator.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */
//

#import "HBPreviewGenerator.h"
#import "HBUtilities.h"

#import "HBCore.h"
#import "HBJob.h"
#import "HBPicture+UIAdditions.h"

@interface HBPreviewGenerator ()

@property (nonatomic, readonly) NSMutableDictionary *picturePreviews;
@property (nonatomic, readonly) NSUInteger imagesCount;
@property (nonatomic, readonly) HBCore *scanCore;
@property (nonatomic, readonly) HBJob *job;

@property (nonatomic) HBCore *core;

@end

@implementation HBPreviewGenerator

- (instancetype)initWithCore:(HBCore *)core job:(HBJob *)job
{
    self = [super init];
    if (self)
    {
        _scanCore = core;
        _job = job;
        _picturePreviews = [[NSMutableDictionary alloc] init];
        _imagesCount = [[[NSUserDefaults standardUserDefaults] objectForKey:@"PreviewsNumber"] intValue];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(imagesSettingsDidChange) name:HBPictureChangedNotification object:job.picture];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(imagesSettingsDidChange) name:HBFiltersChangedNotification object:job.filters];
    }
    return self;
}

#pragma mark -
#pragma mark Preview images

/**
 * Returns the picture preview at the specified index
 *
 * @param index picture index in title.
 */
- (CGImageRef) imageAtIndex: (NSUInteger) index shouldCache: (BOOL) cache
{
    if (index >= self.imagesCount)
        return nil;

    // The preview for the specified index may not currently exist, so this method
    // generates it if necessary.
    CGImageRef theImage = (CGImageRef)[self.picturePreviews objectForKey:@(index)];

    if (!theImage)
    {
        HBFilters *filters = self.job.filters;
        BOOL deinterlace = (filters.deinterlace && !filters.useDecomb) || (filters.decomb && filters.useDecomb);

        theImage = (CGImageRef)[(id)[self.scanCore copyImageAtIndex:index
                                                           forTitle:self.job.title
                                                       pictureFrame:self.job.picture
                                                        deinterlace:deinterlace] autorelease];
        if (cache && theImage)
        {
            [self.picturePreviews setObject:(id)theImage forKey:@(index)];
        }
    }

    return theImage;
}

/**
 * Purges all images from the cache. The next call to imageAtIndex: will cause a new
 * image to be generated.
 */
- (void) purgeImageCache
{
    [self.picturePreviews removeAllObjects];
}

- (void) imagesSettingsDidChange
{
    [self.delegate reloadPreviews];
}

- (NSString *)info
{
    return self.job.picture.info;
}

#pragma mark -
#pragma mark Preview movie

+ (NSURL *) generateFileURLForType:(NSString *) type
{
    NSURL *previewDirectory =  [[HBUtilities appSupportURL] URLByAppendingPathComponent:[NSString stringWithFormat:@"/Previews/%d", getpid()] isDirectory:YES];

    if (![[NSFileManager defaultManager] createDirectoryAtPath:previewDirectory.path
                                  withIntermediateDirectories:YES
                                                   attributes:nil
                                                        error:nil])
    {
        return nil;
    }

    return [previewDirectory URLByAppendingPathComponent:[NSString stringWithFormat:@"preview_temp.%@", type]];
}

/**
 * This function start the encode of a movie preview, the delegate will be
 * called with the updated the progress info and the fileURL.
 *
 * @param index picture index in title.
 * @param duration the duration in seconds of the preview movie.
 */
- (BOOL) createMovieAsyncWithImageAtIndex: (NSUInteger) index duration: (NSUInteger) seconds;
{
    // return if an encoding if already started.
    if (self.core || index >= self.imagesCount)
    {
        return NO;
    }

    NSURL *destURL = nil;
    // Generate the file url and directories.
    if (self.job.container & HB_MUX_MASK_MP4)
    {
        // we use .m4v for our mp4 files so that ac3 and chapters in mp4 will play properly.
        destURL = [HBPreviewGenerator generateFileURLForType:@"m4v"];
    }
    else if (self.job.container & HB_MUX_MASK_MKV)
    {
        destURL = [HBPreviewGenerator generateFileURLForType:@"mkv"];
    }

    // return if we couldn't get the fileURL.
    if (!destURL)
    {
        return NO;
    }

    // See if there is an existing preview file, if so, delete it.
    [[NSFileManager defaultManager] removeItemAtURL:destURL error:NULL];

    HBJob *job = [[self.job copy] autorelease];
    job.title = self.job.title;
    job.destURL = destURL;

    job.range.type = HBRangePreviewIndex;
    job.range.previewIndex = (int)index + 1;;
    job.range.previewsCount = (int)self.imagesCount;
    job.range.ptsToStop = seconds * 90000LL;

    // Note: unlike a full encode, we only send 1 pass regardless if the final encode calls for 2 passes.
    // this should suffice for a fairly accurate short preview and cuts our preview generation time in half.
    job.video.twoPass = NO;

    // Init the libhb core
    int loggingLevel = [[[NSUserDefaults standardUserDefaults] objectForKey:@"LoggingLevel"] intValue];
    self.core = [[[HBCore alloc] initWithLoggingLevel:loggingLevel] autorelease];
    self.core.name = @"PreviewCore";

    // start the actual encode
    [self.core encodeJob:job
         progressHandler:^(HBState state, hb_state_t hb_state) {
        switch (state) {
            case HBStateWorking:
            {
                NSMutableString *info = [NSMutableString stringWithFormat: @"Encoding preview:  %.2f %%", 100.0 * hb_state.param.working.progress];

                if (hb_state.param.working.seconds > -1)
                {
                    [info appendFormat:@" (%.2f fps, avg %.2f fps, ETA %02dh%02dm%02ds)",
                     hb_state.param.working.rate_cur, hb_state.param.working.rate_avg, hb_state.param.working.hours,
                     hb_state.param.working.minutes, hb_state.param.working.seconds];
                }

                double progress = 100.0 * hb_state.param.working.progress;
                
                [self.delegate updateProgress:progress info:info];
                break;
            }
            case HBStateMuxing:
                [self.delegate updateProgress:100.0 info:@"Muxing Preview…"];
                break;

            default:
                break;
        }
    }
    completionHandler:^(BOOL success) {
        // Encode done, call the delegate and close libhb handle
        if (success)
        {
            [self.delegate didCreateMovieAtURL:destURL];
        }
        else
        {
            [self.delegate didCancelMovieCreation];
        }
        self.core = nil;
    }];

    return YES;
}

/**
 * Cancels the encoding process
 */
- (void) cancel
{
    if (self.core.state == HBStateWorking || self.core.state == HBStatePaused)
    {
        [self.core cancelEncode];
    }
}

#pragma mark -

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [self.core cancelEncode];

    [_picturePreviews release];
    _picturePreviews = nil;

    [super dealloc];
}

@end
