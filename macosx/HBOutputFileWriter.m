/*  HBOutputFileWriter.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBOutputFileWriter.h"

@import HandBrakeKit.HBUtilities;

#define HB_OUTPUT_MAX 1000000

@implementation HBOutputFileWriter
{
    FILE *f;
    NSDateFormatter *_formatter;
    uint32_t _count;
}

- (nullable instancetype)initWithFileURL:(NSURL *)url
{
    self = [super init];
    if (self)
    {
        NSError *error;
        BOOL result;
        result = [NSFileManager.defaultManager createDirectoryAtURL:url.URLByDeletingLastPathComponent
                                        withIntermediateDirectories:YES
                                                         attributes:nil
                                                              error:&error];
        if (!result)
        {
            [HBUtilities writeToActivityLog:"Error: couldn't open activity log file, %@", error];
            return nil;
        }

        _url = [url copy];

        f = fopen(url.fileSystemRepresentation, "w");
        if (!f)
        {
            return nil;
        }

        f = freopen(NULL, "a", f);
        if (!f)
        {
            return nil;
        }

        _formatter = [[NSDateFormatter alloc] init];
        _formatter.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
        _formatter.dateFormat = @"yyyy-MM-dd'T'HH:mm:ssZZZZZ";
        _formatter.timeZone = [NSTimeZone timeZoneForSecondsFromGMT:0];
        _count = 0;

        [self writeHeaderForReason:@"Session"];
    }

    return self;
}

- (void)dealloc
{
    fclose(f);
}

- (void)writeHeaderForReason:(NSString *)reason
{
    [self write:[NSString stringWithFormat:@"HandBrake Activity Log for %@: %@\n%@\n",
                 reason,
                 [_formatter stringFromDate:[NSDate date]],
                 [HBUtilities handBrakeVersion]]];
}

- (void)write:(NSString *)text
{
    if (f == NULL)
    {
        return;
    }

    if (_count > HB_OUTPUT_MAX)
    {
        // Avoid creating enormous log files
        // in case of repeated errors
        [self clear];
    }

    fprintf(f, "%s", text.UTF8String);
    fflush(f);

    _count += 1;
}

- (void)redirect:(NSString *)text type:(HBRedirectType)type
{
    [self write:text];
}

- (void)clear
{
    _count = 0;

    if (f == NULL)
    {
        return;
    }

    f = freopen(NULL, "w", f);

    if (f == NULL)
    {
        return;
    }

    f = freopen(NULL, "a", f);

    if (f == NULL)
    {
        return;
    }

    [self writeHeaderForReason:@"Session (Cleared)"];
}

@end
