/*  This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>

@import HandBrakeKit;

NS_ASSUME_NONNULL_BEGIN

@protocol HBRemoteCoreProtocol

- (void)setDVDNav:(BOOL)enabled;
- (void)initGlobal;
- (void)closeGlobal;

- (void)setUpWithLogLevel:(NSInteger)level name:(NSString *)name;

- (void)setLogLevel:(NSInteger)logLevel;

- (void)provideResourceAccessWithBookmarks:(NSArray<NSData *> *)bookmarks;

- (void)setAutomaticallyPreventSleep:(BOOL)automaticallyPreventSleep;

- (void)preventSleep;
- (void)allowSleep;

- (void)scanURL:(NSURL *)url titleIndex:(NSUInteger)index previews:(NSUInteger)previewsNum minDuration:(NSUInteger)seconds keepPreviews:(BOOL)keepPreviews hardwareDecoder:(BOOL)hardwareDecoder keepDuplicateTitles:(BOOL)keepDuplicateTitles withReply:(void (^)(HBCoreResult))reply;
- (void)cancelScan;

- (void)encodeJob:(HBJob *)job withReply:(void (^)(HBCoreResult))reply;
- (void)cancelEncode;

- (void)pauseEncode;
- (void)resumeEncode;

@end

@protocol HBRemoteProgressProtocol

- (void)updateState:(HBState)state;
- (void)updateProgress:(double)currentProgress hours:(int)hours minutes:(int)minutes seconds:(int)seconds state:(HBState)state info:(NSString *)info;

- (void)forwardOutput:(NSString *)text;
- (void)forwardError:(NSString *)text;

@end

NS_ASSUME_NONNULL_END

