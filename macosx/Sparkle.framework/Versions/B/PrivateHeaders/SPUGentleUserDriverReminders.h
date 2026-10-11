//
//  SPUGentleUserDriverReminders.h
//  Sparkle
//
//  Copyright © 2022 Sparkle Project. All rights reserved.
//

#ifndef SPUGentleUserDriverReminders_h
#define SPUGentleUserDriverReminders_h

/**
 A private protocol for user drivers implementing gentle scheduled reminders
 */
@protocol SPUGentleUserDriverReminders

- (void)logGentleScheduledUpdateReminderWarningIfNeeded;

- (void)resetTimeSinceOpportuneUpdateNotice;

@end

#endif /* SPUGentleUserDriverReminders_h */
