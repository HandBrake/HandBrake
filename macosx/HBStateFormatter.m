/* HBStateFormatter.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBStateFormatter.h"
#import "HBLocalizationUtilities.h"
#include "handbrake/handbrake.h"

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
            NSString *desc = [NSString localizedStringWithFormat:HBKitLocalizedString(@"Searching for start point:  %.2f %%", @"HBStateFormatter -> search pass display name"),
             100.0 * p.progress];
            [string appendString:desc];

            if (p.seconds > -1)
            {
                NSString *eta = [NSString stringWithFormat:@"%02d:%02d:%02d", p.hours, p.minutes, p.seconds];
                [string appendFormat:HBKitLocalizedString(@" (ETA %@)", @"HBStateFormatter -> search time format"), eta];
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

            if (_showPassNumber)
            {
                if (p.pass_count > -1)
                {
                    if (p.pass_id == HB_PASS_SUBTITLE)
                    {
                        NSString *desc = [NSString localizedStringWithFormat:HBKitLocalizedString(@"Pass %d %@ of %d, %.2f %%", @"HBStateFormatter -> work pass number format"),
                                          p.pass,
                                          HBKitLocalizedString(@"(subtitle scan)", @"HBStateFormatter -> work pass type format"),
                                          p.pass_count, 100.0 * p.progress];
                        [string appendString:desc];
                    }
                    else
                    {
                        NSString *desc = [NSString localizedStringWithFormat:HBKitLocalizedString(@"Pass %d of %d, %.2f %%", @"HBStateFormatter -> work pass number format"),
                                          p.pass, p.pass_count, 100.0 * p.progress];
                        [string appendString:desc];
                    }
                }
                else
                {
                    [string appendString:HBKitLocalizedString(@"Pass 1", @"HBStateFormatter -> work first pass number format")];
                }
            }

            if (p.rate_avg > 0.0 || p.seconds > -1)
            {
                NSString *eta = [NSString stringWithFormat:@"%02d:%02d:%02d", p.hours, p.minutes, p.seconds];

                if (p.rate_avg > 0.0 && p.seconds == -1)
                {
                    NSString *desc = [NSString localizedStringWithFormat:HBKitLocalizedString(@" (%.2f fps, avg %.2f fps)", @"HBStateFormatter -> work time format"),
                                      p.rate_cur, p.rate_avg];
                   [string appendString:desc];
                }
                else if (p.rate_avg > 0.0)
                {
                     NSString *desc = [NSString localizedStringWithFormat:HBKitLocalizedString(@" (%.2f fps, avg %.2f fps, ETA %@)", @"HBStateFormatter -> work time format"),
                                       p.rate_cur, p.rate_avg, eta];
                    [string appendString:desc];
                }
                else if (p.seconds > -1)
                {
                    NSString *desc = [NSString localizedStringWithFormat:HBKitLocalizedString(@" (ETA %@)", @"HBStateFormatter -> work time format"),
                                      eta];
                    [string appendString:desc];
                }
            }

            break;
        }
#undef p

        case HB_STATE_MUXING:
        {
            [string appendString:HBKitLocalizedString(@"Muxing", @"HBStateFormatter -> pass display name")];
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
                 HBKitLocalizedString(@"Scanning title %d of %d, preview %d", @"HBStateFormatter -> scan pass format"),
                 p.title_cur, p.title_count,
                 p.preview_cur];
            }
            else
            {
                [string appendFormat:
                 HBKitLocalizedString(@"Scanning title %d of %d", @"HBStateFormatter -> scan pass format"),
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
            if (p.pass_count > 0)
            {
                progress = (p.progress + p.pass - 1) / p.pass_count;
            }
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
