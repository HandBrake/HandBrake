/*  This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Foundation/Foundation.h>
#import "HBRedirect.h"

@import HandBrakeKit;

NS_ASSUME_NONNULL_BEGIN

@interface HBRemoteCore : NSObject

- (instancetype)initWithLogLevel:(NSInteger)level name:(NSString *)name serviceName:(NSString *)serviceName;
- (void)invalidate;

@property (nonatomic, readwrite) NSInteger logLevel;

@property (nonatomic, readonly) HBState state;

@property (nonatomic, readonly) HBRedirect *stdoutRedirect;
@property (nonatomic, readonly) HBRedirect *stderrRedirect;

@property (nonatomic, readwrite) BOOL automaticallyPreventSleep;

- (void)preventSleep;
- (void)allowSleep;

- (void)scanURL:(NSURL *)url titleIndex:(NSUInteger)index previews:(NSUInteger)previewsNum minDuration:(NSUInteger)seconds keepPreviews:(BOOL)keepPreviews hardwareDecoder:(BOOL)hardwareDecoder keepDuplicateTitles:(BOOL)keepDuplicateTitles progressHandler:(nonnull HBCoreProgressHandler)progressHandler completionHandler:(nonnull HBCoreCompletionHandler)completionHandler;

- (void)cancelScan;

- (void)encodeJob:(HBJob *)job progressHandler:(HBCoreProgressHandler)progressHandler completionHandler:(HBCoreCompletionHandler)completionHandler;

- (void)cancelEncode;

- (void)pause;

- (void)resume;

@end

NS_ASSUME_NONNULL_END
