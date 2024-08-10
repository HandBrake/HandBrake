/*  HBJobOutputFileWriter.h $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBJobOutputFileWriter.h"
#import "HBPreferencesKeys.h"

@interface HBJobOutputFileWriter ()

@property (nonatomic, readonly) NSURL *outputFolderURL;
@property (nonatomic, readwrite) BOOL accessingSecurityScopedFile;

@end

@implementation HBJobOutputFileWriter

- (nullable instancetype)initWithJob:(HBJob *)job
{
    // Establish the log file location to write to.
    // We need to get the current time in YY-MM-DD HH-MM-SS format to put at the beginning of the name of the log file
    time_t _now = time(NULL);
    struct tm *now = localtime(&_now);
    NSString *dateForLogTitle = [NSString stringWithFormat:@"%02d-%02d-%02d %02d-%02d-%02d",
                                 now->tm_year + 1900,
                                 now->tm_mon + 1,
                                 now->tm_mday,now->tm_hour,
                                 now->tm_min, now->tm_sec];

    // Assemble the new log file name as YY-MM-DD HH-MM-SS mymoviename.txt
    NSString *outputDateFileName = [NSString stringWithFormat:@"%@ %@.txt",
                                    job.destinationFileName.stringByDeletingPathExtension,
                                    dateForLogTitle];

    NSURL *destinationURL = nil;

    if ([NSUserDefaults.standardUserDefaults boolForKey:HBEncodeLogLocation])
    {
        // if we are putting it in the same directory with the movie
        destinationURL = [job.destinationFolderURL URLByAppendingPathComponent:outputDateFileName isDirectory:NO];

#ifdef __SANDBOX_ENABLED__
        _outputFolderURL = job.destinationFolderURL;
        _accessingSecurityScopedFile = [_outputFolderURL startAccessingSecurityScopedResource];
#endif

    }
    else
    {
        // if we are putting it in the default ~/Libraries/Application Support/HandBrake/EncodeLogs logs directory
        NSURL *encodeLogDirectory = [[HBUtilities appSupportURL] URLByAppendingPathComponent:@"EncodeLogs" isDirectory:YES];
        destinationURL = [encodeLogDirectory URLByAppendingPathComponent:outputDateFileName isDirectory:NO];
    }

    self = [super initWithFileURL:destinationURL];
    if (self)
    {
        // Additional header info.
        [self write:job.destinationFileName];
        [self write:@"\nPreset: "];
        [self write:job.presetName];
        [self write:@"\n"];
    }

    return self;
}

- (void)dealloc
{
#ifdef __SANDBOX_ENABLED__
    if (_accessingSecurityScopedFile)
    {
        [_outputFolderURL.URLByDeletingLastPathComponent stopAccessingSecurityScopedResource];
    }
#endif
}

@end
