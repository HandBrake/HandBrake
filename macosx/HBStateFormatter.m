/* HBStateFormatter.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBStateFormatter.h"
#import "HBLocalizationUtilities.h"
#include "hb.h"

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

- (NSString *)stateToString:(hb_state_t)s
{
    NSMutableString *string = [NSMutableString string];

    switch (s.state)
    {
#define p s.param.working

        case HB_STATE_SEARCHING:
        {
            [string appendFormat:
             HBKitLocalizedString(@"Searching for start point:  %.2f %%", @"HBStateFormatter -> search pass display name"),
             100.0 * p.progress];

            if (p.seconds > -1)
            {
                [string appendFormat:HBKitLocalizedString(@" (ETA %02d:%02d:%02d)", @"HBStateFormatter -> search time format"), p.hours, p.minutes, p.seconds];
            }

            break;
        }

        case HB_STATE_WORKING:
        {
            if (_title)
            {
                [string appendFormat:HBKitLocalizedString(@"Encoding %@ ", @"HBStateFormatter -> work pass display name"), _title];
                if (_twoLines)
                {
                    [string appendString:@"\n"];
                }
            }

            if (_showPassNumber && p.pass_count > -1)
            {
                if (p.pass_id == HB_PASS_SUBTITLE)
                {
                    [string appendFormat:
                     HBKitLocalizedString(@"Pass %d %@ of %d, %.2f %%", @"HBStateFormatter -> work pass number format"),
                     p.pass,
                     HBKitLocalizedString(@"(subtitle scan)", @"HBStateFormatter -> work pass type format"),
                     p.pass_count, 100.0 * p.progress];
                }
                else
                {
                    [string appendFormat:
                     HBKitLocalizedString(@"Pass %d of %d, %.2f %%", @"HBStateFormatter -> work pass number format"),
                     p.pass, p.pass_count, 100.0 * p.progress];
                }
            }

            if (p.seconds > -1)
            {
                if (p.rate_cur > 0.0)
                {
                    [string appendFormat:
                     HBKitLocalizedString(@" (%.2f fps, avg %.2f fps, ETA %02d:%02d:%02d)", @"HBStateFormatter -> work time format"),
                     p.rate_cur, p.rate_avg, p.hours, p.minutes, p.seconds];
                }
                else
                {
                    [string appendFormat:
                     HBKitLocalizedString(@" (ETA %02d:%02d:%02d)", @"HBStateFormatter -> work time format"),
                     p.hours, p.minutes, p.seconds];
                }
            }

            break;
        }
#undef p

        case HB_STATE_MUXING:
        {
            [string appendString:HBKitLocalizedString(@"Muxing…", @"HBStateFormatter -> pass display name")];
            break;
        }

        case HB_STATE_PAUSED:
        {
            [string appendString:HBKitLocalizedString(@"Paused", @"HBStateFormatter -> pass display name")];
            break;
        }

        case HB_STATE_SCANNING:
        {
#define p s.param.scanning
            if (p.preview_cur)
            {
                [string appendFormat:
                 HBKitLocalizedString(@"Scanning title %d of %d, preview %d…", @"HBStateFormatter -> scan pass format"),
                 p.title_cur, p.title_count,
                 p.preview_cur];
            }
            else
            {
                [string appendFormat:
                 HBKitLocalizedString(@"Scanning title %d of %d…", @"HBStateFormatter -> scan pass format"),
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

- (float)stateToPercentComplete:(hb_state_t)s
{
    float progress = 0;

    switch (s.state)
    {
        case HB_STATE_SEARCHING:
        case HB_STATE_WORKING:
        case HB_STATE_PAUSED:
#define p s.param.working
            progress = (p.progress + p.pass - 1) / p.pass_count;
#undef p

            break;

        case HB_STATE_SCANNING:
#define p s.param.scanning
            progress = p.progress;
#undef p
            break;

        case HB_STATE_MUXING:
            progress = 1;
            break;

        default:
            break;
    }

    if (progress < 0)
    {
        progress = 0;
    }
    else if (progress > 1)
    {
        progress = 1;
    }

    return progress;
}

@end
