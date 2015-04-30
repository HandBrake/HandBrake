/* HBStateFormatter.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBStateFormatter.h"

@implementation HBStateFormatter

- (instancetype)init
{
    self = [super init];
    if (self)
    {
        _twoLines = YES;
        _showPassNumber = YES;
    }
    return self;
}

- (NSString *)stateToString:(hb_state_t)s title:(NSString *)title
{
    NSMutableString *string = [NSMutableString string];

    switch (s.state)
    {
        #define p s.param.working

        case HB_STATE_SEARCHING:
        {
            [string appendFormat:
                        NSLocalizedString(@"Searching for start point… :  %.2f %%", nil),
                        100.0 * p.progress];

            if (p.seconds > -1)
            {
                [string appendFormat:NSLocalizedString(@" (ETA %02dh%02dm%02ds)", nil), p.hours, p.minutes, p.seconds];
            }

            break;
        }

        case HB_STATE_WORKING:
        {
            [string appendFormat:NSLocalizedString(@"Encoding %@ ", nil), title];

            if (_twoLines)
            {
                [string appendString:@"\n"];
            }

            if (_showPassNumber)
            {
                if (p.pass_id == HB_PASS_SUBTITLE)
                {
                    [string appendFormat:
                            NSLocalizedString(@"Pass %d %@ of %d, %.2f %%", nil),
                            p.pass,
                            NSLocalizedString(@"(subtitle scan)", nil),
                            p.pass_count, 100.0 * p.progress];
                }
                else
                {
                    [string appendFormat:
                            NSLocalizedString(@"Pass %d of %d, %.2f %%", nil),
                            p.pass, p.pass_count, 100.0 * p.progress];
                }
            }

            if (p.seconds > -1)
            {
                if (p.rate_cur > 0.0)
                {
                    [string appendFormat:
                     NSLocalizedString(@" (%.2f fps, avg %.2f fps, ETA %02dh%02dm%02ds)", nil),
                     p.rate_cur, p.rate_avg, p.hours, p.minutes, p.seconds];
                }
                else
                {
                    [string appendFormat:
                     NSLocalizedString(@" (ETA %02dh%02dm%02ds)", nil),
                     p.hours, p.minutes, p.seconds];
                }
            }

            break;
        }

        case HB_STATE_MUXING:
        {
            [string appendString:NSLocalizedString(@"Muxing…", nil)];
            break;
        }

        case HB_STATE_PAUSED:
        {
            [string appendString:NSLocalizedString(@"Paused", nil)];
            break;
        }

        #undef p
        case HB_STATE_SCANNING:
        {
            #define p s.param.scanning
            if (p.preview_cur)
            {
                [string appendFormat:
                        NSLocalizedString(@"Scanning title %d of %d, preview %d…", nil),
                        p.title_cur, p.title_count,
                        p.preview_cur];
            }
            else
            {
                [string appendFormat:
                        NSLocalizedString(@"Scanning title %d of %d…", nil),
                        p.title_cur, p.title_count];
            }
            #undef p
            break;
        }

        default:
            break;
    }

    return string;
}

- (CGFloat)stateToPercentComplete:(hb_state_t)s
{
    CGFloat progress = 0;

    switch (s.state)
    {
        case HB_STATE_WORKING:
            #define p s.param.working
            progress = (p.progress + p.pass - 1) / p.pass_count;
            #undef p

            break;

        case HB_STATE_SCANNING:
            #define p s.param.scanning
            progress = p.progress;
            #undef p

        default:
            break;
    }

    return progress;
}

@end
