/* HBQueueWorker.h

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueWorker.h"

#import "HBRemoteCore.h"
#import "HBJobOutputFileWriter.h"
#import "HBPreferencesKeys.h"

static void *HBQueueWorkerContext = &HBQueueWorkerContext;
static void *HBQueueWorkerLogLevelContext = &HBQueueWorkerLogLevelContext;

NSString * const HBQueueWorkerDidChangeStateNotification = @"HBQueueWorkerDidChangeStateNotification";

NSString * const HBQueueWorkerProgressNotification = @"HBQueueWorkerProgressNotification";
NSString * const HBQueueWorkerProgressNotificationPercentKey = @"HBQueueWorkerProgressNotificationPercentKey";
NSString * const HBQueueWorkerProgressNotificationHoursKey = @"HBQueueWorkerProgressNotificationHoursKey";
NSString * const HBQueueWorkerProgressNotificationMinutesKey = @"HBQueueWorkerProgressNotificationMinutesKey";
NSString * const HBQueueWorkerProgressNotificationSecondsKey = @"HBQueueWorkerProgressNotificationSecondsKey";
NSString * const HBQueueWorkerProgressNotificationInfoKey = @"HBQueueWorkerProgressNotificationInfoKey";

NSString * const HBQueueWorkerDidStartItemNotification = @"HBQueueWorkerDidStartItemNotification";
NSString * const HBQueueWorkerDidCompleteItemNotification = @"HBQueueWorkerDidCompleteItemNotification";
NSString * const HBQueueWorkerItemNotificationItemKey = @"HBQueueWorkerItemNotificationItemKey";

// LØSNING: Deklarerer HBPreserveTimeAndDateMetadata som en konstant
NSString * const HBPreserveTimeAndDateMetadata = @"HBPreserveTimeAndDateMetadata";

@interface HBQueueWorker ()

@property (nonatomic, readonly) HBRemoteCore *core;

@property (nonatomic, nullable) HBQueueJobItem *item;
@property (nonatomic, nullable) HBJobOutputFileWriter *currentLog;

@end

@implementation HBQueueWorker

- (instancetype)initWithXPCServiceName:(NSString *)serviceName
{
    self = [super init];
    if (self)
    {
        NSInteger loggingLevel = [NSUserDefaults.standardUserDefaults integerForKey:HBLoggingLevel];
        _core = [[HBRemoteCore alloc] initWithLogLevel:loggingLevel name:serviceName serviceName:serviceName];

        // Set up observers
        [self.core addObserver:self forKeyPath:@"state"
                       options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                       context:HBQueueWorkerContext];

        [NSUserDefaultsController.sharedUserDefaultsController addObserver:self forKeyPath:@"values.LoggingLevel"
                                                                    options:0 context:HBQueueWorkerLogLevelContext];
    }
    return self;
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBQueueWorkerContext)
    {
        [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerDidChangeStateNotification object:self];
    }
    else if (context == HBQueueWorkerLogLevelContext)
    {
        self.core.logLevel = [NSUserDefaults.standardUserDefaults integerForKey:HBLoggingLevel];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)dealloc
{
    [self.core removeObserver:self forKeyPath:@"state" context:HBQueueWorkerContext];
    [NSUserDefaultsController.sharedUserDefaultsController removeObserver:self forKeyPath:@"values.LoggingLevel" context:HBQueueWorkerLogLevelContext];
    [self.core invalidate];
}

- (void)invalidate
{
    [self.core invalidate];
}

- (BOOL)canEncode
{
    return self.item == nil;
}

- (BOOL)isEncoding
{
    return self.item != nil;
}

- (BOOL)canPause
{
    HBState s = self.core.state;
    return (s == HBStateWorking || s == HBStateMuxing);
}

- (void)pause
{
    [self.item pausedAtDate:[NSDate date]];
    [self.core pause];
}

- (BOOL)canResume
{
    return self.core.state == HBStatePaused;
}

- (void)resume
{
    [self.item resumedAtDate:[NSDate date]];
    [self.core resume];
}

- (void)completedItem:(HBQueueJobItem *)item result:(HBCoreResult)result
{
    NSParameterAssert(item);

    item.endedDate = [NSDate date];

    // Since we are done with this encode, tell output to stop writing to the
    // individual encode log.
    [self.core.stderrRedirect removeListener:self.currentLog];
    [self.core.stdoutRedirect removeListener:self.currentLog];

    self.currentLog = nil;

    // Mark the encode just finished
    [self.item setDoneWithResult:result];

    // After '[self.item setDoneWithResult:result];'
    // We retrieve the job and then proceed with our custom logic.

    HBJob *completedJob = self.item.job;

    // LØSNING: Henter verdien for "preserve time metadata" direkte fra HBJob-objektet
    // i stedet for NSUserDefaults for å unngå binding-problemer.
    BOOL preserveTimeMetadata = completedJob.preserveTimeMetadata;

    // LØSNING: Legger til en ny logglinje for å sjekke den faktiske verdien
    // som blir brukt i dette steget
    NSLog(@"Debugging HBJob.preserveTimeMetadata value before ExifTool logic: %d", preserveTimeMetadata);

    if (completedJob && preserveTimeMetadata && completedJob.fileURL && completedJob.destinationURL) {
        NSURL *originalFileURL = completedJob.fileURL;
        NSURL *destinationFileURL = completedJob.destinationURL;

        // MARK: Find ExifTool binary within the app bundle
        // Endret koden til å lete etter filen 'exiftool' basert på brukerens tilbakemelding.
        NSString *exifToolPath = [[NSBundle mainBundle] pathForResource:@"exiftool" ofType:nil];

        if (exifToolPath == nil || ![[NSFileManager defaultManager] fileExistsAtPath:exifToolPath]) {
            // LØSNING: En mer spesifikk feilmelding for å hjelpe med feilsøking
            NSLog(@"FEIL: ExifTool binary-filen 'exiftool' ble ikke funnet i app-pakken. Sørg for at filen er lagt til i 'Copy Files' i Build Phases for prosjektet ditt. Den forventede banen var: '%@'", exifToolPath);
        } else {
            // MARK: 1. Run ExifTool to copy metadata
            NSArray<NSString *> *exifToolArguments = @[
                @"-overwrite_original",
                @"-P",
                @"-TagsFromFile", originalFileURL.path,
                @"-AllDates",
                @"-FileModifyDate",
                @"-FileCreateDate",
                destinationFileURL.path
            ];

            NSTask *exifToolTask = [[NSTask alloc] init];
            [exifToolTask setLaunchPath:exifToolPath];
            [exifToolTask setArguments:exifToolArguments];

            NSPipe *outputPipe = [NSPipe pipe];
            [exifToolTask setStandardOutput:outputPipe];
            [exifToolTask setStandardError:outputPipe];
            NSFileHandle *readHandle = [outputPipe fileHandleForReading];

            @try {
                [exifToolTask launch];
                [exifToolTask waitUntilExit];

                NSData *outputData = [readHandle readDataToEndOfFile];
                NSString *outputString = [[NSString alloc] initWithData:outputData encoding:NSUTF8StringEncoding];

                if ([exifToolTask terminationStatus] == 0) {
                    NSLog(@"ExifTool kopierte dato-metadata vellykket for: %@", destinationFileURL.lastPathComponent);

                    // MARK: 2. Handle custom date from filename and adjust file timestamps
                    NSError *error = nil;
                    NSDictionary<NSFileAttributeKey, id> *fileAttributes = [[NSFileManager defaultManager] attributesOfItemAtPath:originalFileURL.path error:&error];

                    if (fileAttributes == nil) {
                        NSLog(@"Feil ved henting av filattributter for originalfilen: %@", error.localizedDescription);
                    } else {
                        NSDate *originalMDate = fileAttributes[NSFileModificationDate];
                        NSString *baseName = originalFileURL.lastPathComponent;

                        NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:@"[^0-9]" options:0 error:&error];
                        NSString *datePattern = [regex stringByReplacingMatchesInString:baseName options:0 range:NSMakeRange(0, baseName.length) withTemplate:@""];

                        if (datePattern.length >= 14) {
                            datePattern = [datePattern substringToIndex:14];
                            NSPredicate *isNumeric = [NSPredicate predicateWithFormat:@"SELF MATCHES %@", @"^\\d{14}$"];
                            
                            if ([isNumeric evaluateWithObject:datePattern]) {
                                NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
                                [formatter setDateFormat:@"yyyyMMddHHmmss"];
                                // LØSNING: Fjernet linjen som tvinger tidssone til UTC,
                                // slik at den lokale tidssonen brukes til å parse filnavndato.
                                

                                NSDate *customDate = [formatter dateFromString:datePattern];

                                if (customDate) {
                                    NSTimeInterval diffSeconds = fabs([customDate timeIntervalSinceDate:originalMDate]);

                                    if (diffSeconds > 60.0) {
                                        NSLog(@"Justerer tidsstempler basert på filnavn for: %@", destinationFileURL.lastPathComponent);
                                        BOOL setTimesResult = [[NSFileManager defaultManager] setAttributes:@{NSFileCreationDate: customDate, NSFileModificationDate: customDate} ofItemAtPath:destinationFileURL.path error:&error];
                                        if (!setTimesResult) {
                                            NSLog(@"Feil ved setting av tidsstempler til '%@': %@", destinationFileURL.lastPathComponent, error.localizedDescription);
                                        }
                                    } else {
                                        NSLog(@"Filnavndato er for nær originalfilens modifikasjonsdato for: %@. Bruker originalens tidsstempler.", destinationFileURL.lastPathComponent);
                                        BOOL setTimesResult = [[NSFileManager defaultManager] setAttributes:@{NSFileCreationDate: fileAttributes[NSFileCreationDate], NSFileModificationDate: originalMDate} ofItemAtPath:destinationFileURL.path error:&error];
                                        if (!setTimesResult) {
                                            NSLog(@"Feil ved setting av tidsstempler til '%@': %@", destinationFileURL.lastPathComponent, error.localizedDescription);
                                        }
                                    }
                                } else {
                                    NSLog(@"Kunne ikke parse dato fra filnavn '%@' for: %@", datePattern, destinationFileURL.lastPathComponent);
                                    BOOL setTimesResult = [[NSFileManager defaultManager] setAttributes:@{NSFileCreationDate: fileAttributes[NSFileCreationDate], NSFileModificationDate: originalMDate} ofItemAtPath:destinationFileURL.path error:&error];
                                    if (!setTimesResult) {
                                        NSLog(@"Feil ved setting av tidsstempler til '%@': %@", destinationFileURL.lastPathComponent, error.localizedDescription);
                                    }
                                }
                            } else {
                                NSLog(@"Filnavndato-mønster er ikke 14 sifre. Bruker originalens tidsstempler for: %@", destinationFileURL.lastPathComponent);
                                BOOL setTimesResult = [[NSFileManager defaultManager] setAttributes:@{NSFileCreationDate: fileAttributes[NSFileCreationDate], NSFileModificationDate: originalMDate} ofItemAtPath:destinationFileURL.path error:&error];
                                if (!setTimesResult) {
                                    NSLog(@"Feil ved setting av tidsstempler til '%@': %@", destinationFileURL.lastPathComponent, error.localizedDescription);
                                }
                            }
                        } else {
                            NSLog(@"Filnavndato-mønster er kortere enn 14 sifre. Bruker originalens tidsstempler for: %@", destinationFileURL.lastPathComponent);
                            BOOL setTimesResult = [[NSFileManager defaultManager] setAttributes:@{NSFileCreationDate: fileAttributes[NSFileCreationDate], NSFileModificationDate: originalMDate} ofItemAtPath:destinationFileURL.path error:&error];
                            if (!setTimesResult) {
                                NSLog(@"Feil ved setting av tidsstempler til '%@': %@", destinationFileURL.lastPathComponent, error.localizedDescription);
                            }
                        }
                    }
                } else {
                    NSLog(@"ExifTool feilet for: %@. Status: %d, Output:\n%@", destinationFileURL.lastPathComponent, [exifToolTask terminationStatus], outputString);
                }
            } @catch (NSException *e) {
                NSLog(@"Feil ved lansering av ExifTool: %@", e.reason);
            }
        }
    } else {
        NSLog(@"Originalfil eller destinasjonsfil URL mangler for ExifTool-operasjon for jobb: %@", completedJob.presetName);
    }
    
    self.item = nil;

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerProgressNotification
                                                      object:self
                                                    userInfo:@{HBQueueWorkerProgressNotificationPercentKey: @1.0,
                                                               HBQueueWorkerProgressNotificationInfoKey: @""}];

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerDidCompleteItemNotification
                                                      object:self
                                                    userInfo:@{HBQueueWorkerItemNotificationItemKey: item}];
}

/**
 * Here we actually tell hb_scan to perform the source scan, using the path to source and title number
 */
- (void)encodeItem:(HBQueueJobItem *)item
{
    NSParameterAssert(item);

    // now we mark the queue item as working so another instance can not come along and try to scan it while we are scanning
    item.startedDate = [NSDate date];
    item.state = HBQueueItemStateWorking;

    self.item = item;

    [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerDidStartItemNotification
                                                      object:self
                                                    userInfo:@{HBQueueWorkerItemNotificationItemKey: item}];

    // Tell HB to output a new activity log file for this encode
    self.currentLog = [[HBJobOutputFileWriter alloc] initWithJob:item.job];
    if (self.currentLog)
    {
        item.activityLogURL = self.currentLog.url;

        dispatch_queue_t mainQueue = dispatch_get_main_queue();
        [self.core.stderrRedirect addListener:self.currentLog queue:mainQueue];
        [self.core.stdoutRedirect addListener:self.currentLog queue:mainQueue];
    }

    // Progress handler
    void (^progressHandler)(HBState state, HBProgress progress, NSString *info) = ^(HBState state, HBProgress progress, NSString *info)
    {
        if (state == HBStateMuxing)
        {
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerProgressNotification
                                                              object:self
                                                            userInfo:@{HBQueueWorkerProgressNotificationPercentKey: @0,
                                                                       HBQueueWorkerProgressNotificationInfoKey: info}];
        }
        else
        {
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerProgressNotification
                                                              object:self
                                                            userInfo:@{HBQueueWorkerProgressNotificationPercentKey: @(progress.percent),
                                                                       HBQueueWorkerProgressNotificationHoursKey: @(progress.hours),
                                                                       HBQueueWorkerProgressNotificationMinutesKey: @(progress.minutes),
                                                                       HBQueueWorkerProgressNotificationSecondsKey: @(progress.seconds),
                                                                       HBQueueWorkerProgressNotificationInfoKey: info}];
        }
    };

    // Completion handler
    void (^completionHandler)(HBCoreResult result) = ^(HBCoreResult result)
    {
        if (result.code == HBCoreResultCodeDone)
        {
            [self realEncodeItem:item];
        }
        else
        {
            [self completedItem:item result:result];
        }
    };

    [item.job refreshSecurityScopedResources];

    // Only scan 10 previews before an encode - additional previews are
    // only useful for autocrop and static previews, which are already taken care of at this point
    [self.core scanURL:item.fileURL
            titleIndex:item.job.titleIdx
              previews:10
           minDuration:0
           maxDuration:0
          keepPreviews:NO
       hardwareDecoder:[NSUserDefaults.standardUserDefaults boolForKey:HBUseHardwareDecoder]
       keepDuplicateTitles:item.job.keepDuplicateTitles
       progressHandler:progressHandler
     completionHandler:completionHandler];
}

/**
 * This assumes that we have re-scanned and loaded up a new queue item to send to libhb
 */
- (void)realEncodeItem:(HBQueueJobItem *)item
{
    HBJob *job = item.job;
    // LØSNING: Setter en ny egenskap på HBJob-objektet basert på NSUserDefaults-verdien.
    // Dette gir oss en mer pålitelig verdi senere, i stedet for å stole på UI-bindingen.
    // Merk: Du må legge til 'preserveTimeMetadata' som en BOOL-egenskap i HBJob-klassen
    // (for eksempel i HBJob.h) for at denne koden skal kompilere.
    job.preserveTimeMetadata = [NSUserDefaults.standardUserDefaults boolForKey:HBPreserveTimeAndDateMetadata];

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

    // Progress handler
    void (^progressHandler)(HBState state, HBProgress progress, NSString *info) = ^(HBState state, HBProgress progress, NSString *info)
    {
        if (state == HBStateMuxing)
        {
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerProgressNotification
                                                              object:self
                                                            userInfo:@{HBQueueWorkerProgressNotificationPercentKey: @1,
                                                                       HBQueueWorkerProgressNotificationInfoKey: info}];
        }
        else
        {
            [NSNotificationCenter.defaultCenter postNotificationName:HBQueueWorkerProgressNotification
                                                              object:self
                                                            userInfo:@{HBQueueWorkerProgressNotificationPercentKey: @(progress.percent),
                                                                       HBQueueWorkerProgressNotificationHoursKey: @(progress.hours),
                                                                       HBQueueWorkerProgressNotificationMinutesKey: @(progress.minutes),
                                                                       HBQueueWorkerProgressNotificationSecondsKey: @(progress.seconds),
                                                                       HBQueueWorkerProgressNotificationInfoKey: info}];
        }
    };

    // Completion handler
    void (^completionHandler)(HBCoreResult result) = ^(HBCoreResult result)
    {
        [self completedItem:item result:result];
    };

    // We should be all setup so let 'er rip
    [self.core encodeJob:job progressHandler:progressHandler completionHandler:completionHandler];
}

/**
 * Cancels the current job
 */
- (void)cancel
{
    if (self.core.state == HBStateScanning)
    {
        [self.core cancelScan];
    }
    else
    {
        [self.core cancelEncode];
    }
}

@end
