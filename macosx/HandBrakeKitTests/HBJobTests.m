/*  HBJobTests.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>

@import HandBrakeKit;

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

    NSURL *sampleURL = [NSURL fileURLWithPath:@"/test.mp4" isDirectory:NO];

    self.queue = dispatch_queue_create("fr.handbrake.testQueue", DISPATCH_QUEUE_SERIAL);
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);

    self.core = [[HBCore alloc] initWithLogLevel:1 queue:self.queue];
    [self.core scanURL:sampleURL titleIndex:0 previews:1 minDuration:0 keepPreviews:NO progressHandler:^(HBState state, HBProgress progress, NSString * _Nonnull info) {

    } completionHandler:^(HBCoreResult result) {
        dispatch_semaphore_signal(sem);
    }];

    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);

    self.title = self.core.titles.firstObject;

    self.job = [[HBJob alloc] initWithTitle:self.title preset:self.preset];
    self.job.destinationFolderURL = [NSURL fileURLWithPath:@"/" isDirectory:YES];
    self.job.destinationFileName = @"Dest.mp4";
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

    HBJob *job = [[HBJob alloc] initWithTitle:self.title preset:preset];

    XCTAssertNotNil(self.job);

    self.job.destinationFolderURL = [NSURL fileURLWithPath:@"/" isDirectory:YES];
    self.job.destinationFileName = @"Dest.mp4";
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
    XCTAssertEqual([self.preset[@"PictureCropMode"] boolValue], self.job.picture.cropMode);
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
