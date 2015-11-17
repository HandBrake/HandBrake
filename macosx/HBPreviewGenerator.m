/*  HBPreviewGenerator.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */
//

#import "HBPreviewGenerator.h"
#import "HBUtilities.h"

#import "HBCore.h"
#import "HBJob.h"
#import "HBStateFormatter.h"
#import "HBPicture+UIAdditions.h"

@interface HBPreviewGenerator ()

@property (nonatomic, readonly) NSCache *picturePreviews;
@property (nonatomic, readonly, weak) HBCore *scanCore;
@property (nonatomic, readonly, strong) HBJob *job;

@property (nonatomic, strong) HBCore *core;

@end

@implementation HBPreviewGenerator

- (instancetype)init
{
    @throw nil;
}

- (instancetype)initWithCore:(HBCore *)core job:(HBJob *)job
{
    self = [super init];
    if (self)
    {
        _scanCore = core;
        _job = job;

        _picturePreviews = [[NSCache alloc] init];
        // Limit the cache to 60 1080p previews, the cost is in pixels
        _picturePreviews.totalCostLimit = 60 * 1920 * 1080;

        _imagesCount = [_scanCore imagesCountForTitle:self.job.title];

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(imagesSettingsDidChange) name:HBPictureChangedNotification object:job.picture];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(imagesSettingsDidChange) name:HBFiltersChangedNotification object:job.filters];
    }
    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self.core cancelEncode];
}

#pragma mark -
#pragma mark Preview images

/**
 * Returns the picture preview at the specified index
 *
 * @param index picture index in title.
 */
- (CGImageRef) copyImageAtIndex: (NSUInteger) index shouldCache: (BOOL) cache
{
    if (index >= self.imagesCount)
        return nil;

    // The preview for the specified index may not currently exist, so this method
    // generates it if necessary.
    CGImageRef theImage = (__bridge CGImageRef)([self.picturePreviews objectForKey:@(index)]);

    if (!theImage)
    {
        HBFilters *filters = self.job.filters;
        BOOL deinterlace = (![filters.deinterlace isEqualToString:@"off"]);

        theImage = (CGImageRef)[self.scanCore copyImageAtIndex:index
                                                           forTitle:self.job.title
                                                       pictureFrame:self.job.picture
                                                        deinterlace:deinterlace];
        if (cache && theImage)
        {
            // The cost is the number of pixels of the image
            NSUInteger previewCost = CGImageGetWidth(theImage) * CGImageGetHeight(theImage);
            [self.picturePreviews setObject:(__bridge id)(theImage) forKey:@(index) cost:previewCost];
        }
    }
    else
    {
        CFRetain(theImage);
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

- (CGSize)imageSize
{
    return CGSizeMake(self.job.picture.displayWidth, self.job.picture.height);
}

- (void) imagesSettingsDidChange
{
    // Purge the existing picture previews so they get recreated the next time
    // they are needed.

    [self purgeImageCache];
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

    HBJob *job = [self.job copy];
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
    int level = [[[NSUserDefaults standardUserDefaults] objectForKey:@"LoggingLevel"] intValue];
    self.core = [[HBCore alloc] initWithLogLevel:level name:@"PreviewCore"];

    HBStateFormatter *formatter = [[HBStateFormatter alloc] init];
    formatter.twoLines = NO;
    formatter.showPassNumber = NO;

    // start the actual encode
    [self.core encodeJob:job
         progressHandler:^(HBState state, hb_state_t hb_state) {
             [self.delegate updateProgress:[formatter stateToPercentComplete:hb_state]
                                      info:[formatter stateToString:hb_state title:@"preview"]];
         }
       completionHandler:^(HBCoreResult result) {
           // Encode done, call the delegate and close libhb handle
           if (result == HBCoreResultDone)
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

@end
