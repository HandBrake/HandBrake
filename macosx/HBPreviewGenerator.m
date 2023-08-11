/*  HBPreviewGenerator.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */
//

#import "HBPreviewGenerator.h"
#import "HBPreferencesKeys.h"
#import "HBJob+HBAdditions.h"

@import HandBrakeKit;

@interface HBPreviewGenerator ()

@property (nonatomic, readonly, weak) HBCore *scanCore;
@property (nonatomic, readonly, strong) HBJob *job;

@property (nonatomic, readonly) NSCache<NSNumber *, id> *previewsCache;
@property (nonatomic, readonly) NSCache<NSNumber *, id> *smallPreviewsCache;

@property (nonatomic, readonly) dispatch_queue_t queue;
@property (nonatomic, readonly) dispatch_group_t group;
@property (nonatomic, readonly) _Atomic bool invalidated;

@property (nonatomic, strong) HBCore *core;

@property (nonatomic) BOOL reloadInQueue;

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

        _previewsCache = [[NSCache alloc] init];
        // Limit the cache to 60 1080p previews, the cost is in pixels
        _previewsCache.totalCostLimit = 60 * 1920 * 1080;

        _smallPreviewsCache = [[NSCache alloc] init];
        _smallPreviewsCache.totalCostLimit = 60 * 320 * 180;

        _imagesCount = [_scanCore imagesCountForTitle:self.job.title];

        _queue = dispatch_queue_create("fr.handbrake.PreviewQueue", DISPATCH_QUEUE_SERIAL);
        _group = dispatch_group_create();

        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(imagesSettingsDidChange) name:HBPictureChangedNotification object:job.picture];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(imagesSettingsDidChange) name:HBFiltersChangedNotification object:job.filters];
    }
    return self;
}

- (void)dealloc
{
    _invalidated = true;

    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [[NSRunLoop mainRunLoop] cancelPerformSelectorsWithTarget:self];
    [self.core cancelEncode];
}

#pragma mark -
#pragma mark Preview images

/**
 * Returns the picture preview at the specified index
 *
 * @param index picture index in title.
 */
- (nullable CGImageRef) copyImageAtIndex: (NSUInteger) index shouldCache: (BOOL) cache
{
    if (index >= self.imagesCount)
    {
        return nil;
    }

    // The preview for the specified index may not currently exist, so this method
    // generates it if necessary.
    CGImageRef theImage = (__bridge CGImageRef)([_previewsCache objectForKey:@(index)]);

    if (!theImage)
    {
        theImage = [self.scanCore copyImageAtIndex:index job:self.job];
        if (cache && theImage)
        {
            // The cost is the number of pixels of the image
            NSUInteger previewCost = CGImageGetWidth(theImage) * CGImageGetHeight(theImage);
            [self.previewsCache setObject:(__bridge id)(theImage) forKey:@(index) cost:previewCost];
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
    [self.previewsCache removeAllObjects];
}

- (CGSize)imageSize
{
    return CGSizeMake(self.job.picture.displayWidth, self.job.picture.displayHeight);
}

- (void)imagesSettingsDidChange
{
    // Purge the existing picture previews so they get recreated the next time
    // they are needed.
    [self purgeImageCache];

    // Enqueue the reload call on the main runloop
    // to avoid reloading the same image multiple times.
    if (self.reloadInQueue == NO)
    {
        [[NSRunLoop mainRunLoop] performSelector:@selector(postReloadNotification) target:self argument:nil order:0 modes:@[NSDefaultRunLoopMode]];
        self.reloadInQueue = YES;
    }
}

- (void)postReloadNotification
{
    [self.delegate reloadPreviews];
    self.reloadInQueue = NO;
}

- (NSString *)info
{
    return self.job.picture.info;
}

#pragma mark - Small previews

- (void)copySmallImageAtIndex:(NSUInteger)index completionHandler:(void (^)(__nullable CGImageRef result))handler
{
    if (_invalidated || index >= self.imagesCount)
    {
        handler(NULL);
        return;
    }

    dispatch_group_async(_group, _queue,^{

        if (self->_invalidated || index >= self.imagesCount)
        {
            handler(NULL);
            return;
        }

        CGImageRef image;

        // First try to look in the small previews cache
        image = (__bridge CGImageRef)([self->_smallPreviewsCache objectForKey:@(index)]);

        if (image != NULL)
        {
            handler(image);
            return;
        }

        // Else try the normal cache
        image = (__bridge CGImageRef)([self->_previewsCache objectForKey:@(index)]);

        if (image == NULL)
        {
            image = [self.scanCore copyImageAtIndex:index job:self.job];
            CFAutorelease(image);
        }

        if (image != NULL)
        {
            CGImageRef scaledImage = CreateScaledCGImageFromCGImage(image, 30);
            // The cost is the number of pixels of the image
            NSUInteger previewCost = CGImageGetWidth(scaledImage) * CGImageGetHeight(scaledImage);
            [self.smallPreviewsCache setObject:(__bridge id)(scaledImage) forKey:@(index) cost:previewCost];
            handler(scaledImage);
            return;
        }

        handler(NULL);
    });
}

#pragma mark -
#pragma mark Preview movie

+ (NSURL *) generateFileURLForType:(NSString *) type
{
    NSURL *previewDirectory = [[HBUtilities appSupportURL] URLByAppendingPathComponent:[NSString stringWithFormat:@"/Previews/%d", getpid()] isDirectory:YES];

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
- (BOOL) createMovieAsyncWithImageAtIndex: (NSUInteger) index duration: (NSUInteger) seconds
{
    // return if an encoding if already started.
    if (self.core || index >= self.imagesCount)
    {
        return NO;
    }

    // Generate the file url and directories.
    NSString *extension = self.job.automaticExt;
    NSURL *destURL = [HBPreviewGenerator generateFileURLForType:extension];

    // return if we couldn't get the fileURL.
    if (!destURL)
    {
        return NO;
    }

    // See if there is an existing preview file, if so, delete it.
    [[NSFileManager defaultManager] removeItemAtURL:destURL error:NULL];

    HBJob *job = [self.job copy];
    job.title = self.job.title;
    job.destinationFileName = destURL.lastPathComponent;
    job.destinationFolderURL = destURL.URLByDeletingLastPathComponent;

    job.range.type = HBRangePreviewIndex;
    job.range.previewIndex = (int)index + 1;;
    job.range.previewsCount = (int)self.imagesCount;
    job.range.ptsToStop = seconds * 90000LL;

    // Note: unlike a full encode, we only send 1 pass regardless if the final encode calls for 2 passes.
    // this should suffice for a fairly accurate short preview and cuts our preview generation time in half.
    job.video.multiPass = NO;

    if ([NSUserDefaults.standardUserDefaults boolForKey:HBUseHardwareDecoder])
    {
        job.hwDecodeUsage = HBJobHardwareDecoderUsageFullPathOnly;

        if ([NSUserDefaults.standardUserDefaults boolForKey:HBAlwaysUseHardwareDecoder])
        {
            job.hwDecodeUsage = HBJobHardwareDecoderUsageAlways;
        }
    }
    else
    {
        job.hwDecodeUsage = HBJobHardwareDecoderUsageNone;
    }

    // Init the libhb core
    NSInteger level = [NSUserDefaults.standardUserDefaults integerForKey:HBLoggingLevel];
    self.core = [[HBCore alloc] initWithLogLevel:level name:@"PreviewCore"];
    self.core.automaticallyPreventSleep = YES;

    HBStateFormatter *formatter = [[HBStateFormatter alloc] init];
    formatter.twoLines = NO;
    formatter.showPassNumber = NO;
    formatter.title = NSLocalizedString(@"preview", @"Preview -> progress formatter title");

    self.core.stateFormatter = formatter;

    // start the actual encode
    [self.core encodeJob:job
         progressHandler:^(HBState state, HBProgress progress, NSString *info) {
             [self.delegate updateProgress:progress.percent
                                      info:info];
         }
       completionHandler:^(HBCoreResult result) {
           // Encode done, call the delegate and close libhb handle
           if (result.code == HBCoreResultCodeDone)
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
- (void)cancel
{
    if (self.core.state == HBStateWorking || self.core.state == HBStatePaused)
    {
        [self.core cancelEncode];
    }
}

- (void)invalidate
{
    _invalidated = true;
    dispatch_group_wait(_group, DISPATCH_TIME_FOREVER);
}

@end
