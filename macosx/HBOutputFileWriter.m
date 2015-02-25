/*  HBOutputFileWriter.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBOutputFileWriter.h"
#import "HBUtilities.h"

@implementation HBOutputFileWriter
{
    FILE *f;
}

- (instancetype)initWithFileURL:(NSURL *)url;
{
    self = [super init];
    if (self)
    {

        [[NSFileManager defaultManager] createDirectoryAtPath:url.URLByDeletingLastPathComponent.path
                                  withIntermediateDirectories:YES
                                                   attributes:nil
                                                        error:NULL];

        _url = [url copy];
        f = fopen(url.path.fileSystemRepresentation, "w");
        f = freopen(NULL, "a", f);

        [self writeHeaderForReason:@"Session"];
    }

    return self;
}

- (void)dealloc
{
    fclose(f);
    [_url release];
    [super dealloc];
}

- (void)writeHeaderForReason:(NSString *)reason
{
    [self write:[NSString stringWithFormat:@"HandBrake Activity Log for %@: %@\n%@\n",
                 reason,
                 [[NSDate date] descriptionWithCalendarFormat:nil timeZone:nil locale:nil],
                 [HBUtilities handBrakeVersion]]];
}

- (void)write:(NSString *)text
{
    fprintf(f, "%s", text.UTF8String);
    fflush(f);
}

- (void)stdoutRedirect:(NSString *)text
{
    [self write:text];
}

- (void)stderrRedirect:(NSString *)text
{
    [self write:text];
}

- (void)clear
{
    f = freopen(NULL, "w", f);
    f = freopen(NULL, "a", f);

    [self writeHeaderForReason:@"Session (Cleared)"];
}

@end
