//
//  SUStatusChecker.h
//  Sparkle
//
//  Created by Evan Schoenberg on 7/6/06.
//

#import <Cocoa/Cocoa.h>
#import <Sparkle/SUUpdater.h>

@class SUStatusChecker;

@protocol SUStatusCheckerDelegate <NSObject>
//versionString will be nil and isNewVersion will be NO if version checking fails.
- (void)statusChecker:(SUStatusChecker *)statusChecker foundVersion:(NSString *)versionString isNewVersion:(BOOL)isNewVersion;
@end

@interface SUStatusChecker : SUUpdater {
	id<SUStatusCheckerDelegate> scDelegate;
}

// Create a status checker which will notifiy delegate once the appcast version is determined.
// Notification occurs via the method defined in the SUStatusCheckerDelegate informal protocol.
+ (SUStatusChecker *)statusCheckerForDelegate:(id<SUStatusCheckerDelegate>)delegate;

@end
