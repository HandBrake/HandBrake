/*  HBOutputFileWriter.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBOutputFileWriter.h"

@import HandBrakeKit.HBUtilities;

@implementation HBOutputFileWriter
{
    FILE *f;
    NSDateFormatter *_formatter;
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

    fprintf(f, "%s", text.UTF8String);
    fflush(f);
}

- (void)redirect:(NSString *)text type:(HBRedirectType)type
{
    [self write:text];
}

- (void)clear
{
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
