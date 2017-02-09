/*  HBJobTests.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>

#import "HBCore.h"
#import "HBTitle.h"
#import "HBJob.h"
#import "HBPicture.h"
#import "HBJob+UIAdditions.h"
#import "HBPresetsManager.h"
#import "HBPreset.h"
#import "HBMutablePreset.h"

@interface HBJobTests : XCTestCase

@property (nonatomic, readonly) HBPresetsManager *manager;
@property (nonatomic, readwrite) HBPreset *preset;

@property (nonatomic, readwrite) dispatch_queue_t queue;
@property (nonatomic, readwrite) HBCore *core;

@property (nonatomic, readwrite) HBTitle *title;
@property (nonatomic, readwrite) HBJob *job;

@end

@implementation HBJobTests

- (void)setUp
{
    [super setUp];

    _manager = [[HBPresetsManager alloc] init];
    [_manager generateBuiltInPresets];

    self.preset = self.manager.defaultPreset;

    NSURL *sampleURL = [NSURL fileURLWithPath:@"test.mp4"];

    self.queue = dispatch_queue_create("fr.handbrake.testQueue", DISPATCH_QUEUE_SERIAL);
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);

    self.core = [[HBCore alloc] initWithLogLevel:1 queue:self.queue];
    [self.core scanURL:sampleURL titleIndex:0 previews:1 minDuration:0 progressHandler:^(HBState state, HBProgress progress, NSString * _Nonnull info) {

    } completionHandler:^(HBCoreResult result) {
        dispatch_semaphore_signal(sem);
    }];

    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);

    self.title = self.core.titles.firstObject;

    self.job = [[HBJob alloc] initWithTitle:self.title andPreset:self.preset];
    self.job.outputURL = [NSURL fileURLWithPath:@"/"];
    self.job.outputFileName = @"Dest.mp4";
}

- (void)tearDown
{
    [super tearDown];
}

- (void)testJobCreation
{
    HBJob *job = [[HBJob alloc] init];

    XCTAssertNotNil(job, @"Pass");
}

- (void)testApplyPreset
{
    HBPreset *preset = self.manager.defaultPreset;

    XCTAssertNotNil(self.title);

    HBJob *job = [[HBJob alloc] initWithTitle:self.title andPreset:preset];

    XCTAssertNotNil(self.job);

    self.job.outputURL = [NSURL fileURLWithPath:@"/"];
    self.job.outputFileName = @"Dest.mp4";
    [job applyPreset:preset];
}

- (void)testAudio
{
    XCTAssertEqual(self.job.audio.tracks.count, 1);
}

- (void)testPictureSize
{
    XCTAssertEqual(self.job.picture.width, 1280);
    XCTAssertEqual(self.job.picture.height, 720);
}

- (void)testAutoCrop
{
    XCTAssertEqual([self.preset[@"PictureAutoCrop"] boolValue], self.job.picture.autocrop);
}

- (void)testAutoCropValues
{
    XCTAssertNotNil(self.title);
    XCTAssertNotNil(self.job);

    XCTAssertEqual(self.title.autoCropTop, self.job.picture.cropTop);
    XCTAssertEqual(self.title.autoCropBottom, self.job.picture.cropBottom);
    XCTAssertEqual(self.title.autoCropLeft, self.job.picture.cropLeft);
    XCTAssertEqual(self.title.autoCropRight, self.job.picture.cropRight);
}

- (void)testCustomAnamorphic
{
    HBMutablePreset *preset = [self.preset mutableCopy];

    preset[@"UsesPictureSettings"] = @1;

    preset[@"PictureWidth"] = @720;
    preset[@"PictureHeight"] = @576;

    preset[@"PicturePAR"] = @"custom";
    preset[@"PicturePARWidth"] = @64;
    preset[@"PicturePARHeight"] = @45;

    HBJob *job = [self.job copy];
    job.title = self.job.title;
    [job applyPreset:preset];

    XCTAssertEqual(job.picture.width, 720);
    XCTAssertEqual(job.picture.height, 576);

    XCTAssertEqual(job.picture.displayWidth, 1024);
}

@end
