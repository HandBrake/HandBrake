/*  HBAutoNamer.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAutoNamer.h"

#import "HBJob+HBAdditions.h"
#import "HBPreferencesKeys.h"

static void *HBAutoNamerPrefsContext = &HBAutoNamerPrefsContext;
static void *HBAutoNamerContext = &HBAutoNamerContext;

@interface HBAutoNamer ()

@property (nonatomic) HBJob *job;
@property (nonatomic) NSArray<NSString *> *format;

@end

@implementation HBAutoNamer

- (instancetype)initWithJob:(HBJob *)job
{
    self = [super init];
    if (self)
    {
        _job = job;
        _format = [NSUserDefaults.standardUserDefaults objectForKey:HBAutoNamingFormat];
        [self addFormatObservers];
        [self addJobObservers];
        [self addPrefsObservers];
    }
    return self;
}

- (void)dealloc
{
    [self removeFormatObservers];
    [self removeJobObservers];
    [self removePrefsObservers];
}

#pragma mark - KVO

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBAutoNamerContext)
    {
        [self updateFileName];
    }
    else if (context == HBAutoNamerPrefsContext)
    {
        [self removeJobObservers];
        self.format = [NSUserDefaults.standardUserDefaults objectForKey:HBAutoNamingFormat];
        [self addJobObservers];
        [self updateFileName];
        [self updateFileExtension];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)addPrefsObservers
{
    NSUserDefaultsController *ud = [NSUserDefaultsController sharedUserDefaultsController];
    [ud addObserver:self forKeyPath:@"values.HBAutoNamingFormat" options:0 context:HBAutoNamerPrefsContext];
    [ud addObserver:self forKeyPath:@"values.HBAutoNamingRemoveUnderscore" options:0 context:HBAutoNamerPrefsContext];
    [ud addObserver:self forKeyPath:@"values.HBAutoNamingRemovePunctuation" options:0 context:HBAutoNamerPrefsContext];
    [ud addObserver:self forKeyPath:@"values.HBAutoNamingTitleCase" options:0 context:HBAutoNamerPrefsContext];
    [ud addObserver:self forKeyPath:@"values.HBAutoNamingISODateFormat" options:0 context:HBAutoNamerPrefsContext];
}

- (void)removePrefsObservers
{
    NSUserDefaultsController *ud = [NSUserDefaultsController sharedUserDefaultsController];
    [ud removeObserver:self forKeyPath:@"values.HBAutoNamingFormat" context:HBAutoNamerPrefsContext];
    [ud removeObserver:self forKeyPath:@"values.HBAutoNamingRemoveUnderscore" context:HBAutoNamerPrefsContext];
    [ud removeObserver:self forKeyPath:@"values.HBAutoNamingRemovePunctuation" context:HBAutoNamerPrefsContext];
    [ud removeObserver:self forKeyPath:@"values.HBAutoNamingTitleCase" context:HBAutoNamerPrefsContext];
    [ud removeObserver:self forKeyPath:@"values.HBAutoNamingISODateFormat" context:HBAutoNamerPrefsContext];
}

#pragma mark - File extension

- (void)addFormatObservers
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateFileExtension:) name:HBAudioEncoderChangedNotification object:self.job.audio];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateFileExtension:) name:HBChaptersChangedNotification object:self.job];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateFileExtension:) name:HBContainerChangedNotification object:self.job];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateFileExtension:) name:HBRangeChangedNotification object:self.job.range];
}

- (void)removeFormatObservers
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:HBAudioEncoderChangedNotification object:self.job.audio];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:HBChaptersChangedNotification object:self.job];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:HBContainerChangedNotification object:self.job];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:HBRangeChangedNotification object:_job.range];
}

- (void)updateFileExtension:(NSNotification *)notification
{
    NSUndoManager *undo = self.job.undo;

    if (self.job && !(undo.isUndoing || undo.isRedoing))
    {
        NSString *extension = self.job.automaticExt;
        if (![extension isEqualTo:self.job.destinationFileName.pathExtension])
        {
            self.job.destinationFileName = [[self.job.destinationFileName stringByDeletingPathExtension] stringByAppendingPathExtension:extension];
        }
    }
}

- (void)updateFileExtension
{
    [self updateFileExtension:nil];
}

#pragma mark - File name

- (void)addJobObservers
{
    for (NSString *formatKey in self.format)
    {
        if ([formatKey isEqualToString:@"{Chapters}"])
        {
            [self.job addObserver:self forKeyPath:@"range.chapterStart" options:0 context:HBAutoNamerContext];
            [self.job addObserver:self forKeyPath:@"range.chapterStop" options:0 context:HBAutoNamerContext];
        }
        else if ([formatKey isEqualToString:@"{Preset}"])
        {
            [self.job addObserver:self forKeyPath:@"presetName" options:0 context:HBAutoNamerContext];
        }
        else if ([formatKey isEqualToString:@"{Width}"])
        {
            [self.job addObserver:self forKeyPath:@"picture.storageWidth" options:0 context:HBAutoNamerContext];
        }
        else if ([formatKey isEqualToString:@"{Height}"])
        {
            [self.job addObserver:self forKeyPath:@"picture.storageHeight" options:0 context:HBAutoNamerContext];
        }
        else if ([formatKey isEqualToString:@"{Codec}"] ||
                 [formatKey isEqualToString:@"{Encoder}"] ||
                 [formatKey isEqualToString:@"{Bit-Depth}"])
        {
            [self.job addObserver:self forKeyPath:@"video.encoder" options:0 context:HBAutoNamerContext];
        }
        else if ([formatKey isEqualToString:@"{Quality/Bitrate}"] ||
                 [formatKey isEqualToString:@"{Quality-Type}"])
        {
            [self.job addObserver:self forKeyPath:@"video.qualityType" options:0 context:HBAutoNamerContext];
            [self.job addObserver:self forKeyPath:@"video.avgBitrate" options:0 context:HBAutoNamerContext];
            [self.job addObserver:self forKeyPath:@"video.quality" options:0 context:HBAutoNamerContext];
        }
    }
}

- (void)removeJobObservers
{
    for (NSString *formatKey in self.format)
    {
        if ([formatKey isEqualToString:@"{Chapters}"])
        {
            [self.job removeObserver:self forKeyPath:@"range.chapterStart" context:HBAutoNamerContext];
            [self.job removeObserver:self forKeyPath:@"range.chapterStop" context:HBAutoNamerContext];
        }
        else if ([formatKey isEqualToString:@"{Preset}"])
        {
            [self.job removeObserver:self forKeyPath:@"presetName" context:HBAutoNamerContext];
        }
        else if ([formatKey isEqualToString:@"{Width}"])
        {
            [self.job removeObserver:self forKeyPath:@"picture.storageWidth" context:HBAutoNamerContext];
        }
        else if ([formatKey isEqualToString:@"{Height}"])
        {
            [self.job removeObserver:self forKeyPath:@"picture.storageHeight" context:HBAutoNamerContext];
        }
        else if ([formatKey isEqualToString:@"{Codec}"] ||
                 [formatKey isEqualToString:@"{Encoder}"] ||
                 [formatKey isEqualToString:@"{Bit-Depth}"])
        {
            [self.job removeObserver:self forKeyPath:@"video.encoder" context:HBAutoNamerContext];
        }
        else if ([formatKey isEqualToString:@"{Quality/Bitrate}"] ||
                 [formatKey isEqualToString:@"{Quality-Type}"])
        {
            [self.job removeObserver:self forKeyPath:@"video.qualityType" context:HBAutoNamerContext];
            [self.job removeObserver:self forKeyPath:@"video.avgBitrate" context:HBAutoNamerContext];
            [self.job removeObserver:self forKeyPath:@"video.quality" context:HBAutoNamerContext];
        }
    }
}

- (void)updateFileName
{
    NSUndoManager *undo = self.job.undo;

    if ([NSUserDefaults.standardUserDefaults boolForKey:HBDefaultAutoNaming] && self.job && !(undo.isUndoing || undo.isRedoing))
    {
        // Generate a new file name
        NSString *fileName = self.job.automaticName;

        // Swap the old one with the new one
        self.job.destinationFileName = [NSString stringWithFormat:@"%@.%@", fileName, self.job.destinationFileName.pathExtension];
    }
}

@end
