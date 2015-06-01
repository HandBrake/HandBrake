/*  HBJobTests.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>

#import "HBMockTitle.h"
#import "HBJob.h"
#import "HBPicture.h"
#import "HBJob+UIAdditions.h"
#import "HBPresetsManager.h"
#import "HBPreset.h"

@interface HBJobTests : XCTestCase

@property (nonatomic, readonly) HBPresetsManager *manager;

@property (nonatomic, readwrite) HBPreset *preset;
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

    self.title = [[HBMockTitle alloc] init];

    self.job = [[HBJob alloc] initWithTitle:self.title andPreset:self.preset];
    self.job.destURL = [NSURL fileURLWithPath:@"/Dest.mp4"];
}

- (void)tearDown
{
    [super tearDown];
}

- (void)testJobCreation
{
    HBJob *job = [[HBJob alloc] init];

    XCTAssert(job, @"Pass");
}

- (void)testApplyPreset
{
    HBMockTitle *title = [[HBMockTitle alloc] init];
    HBPreset *preset = self.manager.defaultPreset;

    HBJob *job = [[HBJob alloc] initWithTitle:title andPreset:preset];
    job.destURL = [NSURL fileURLWithPath:@"/Dest.mp4"];

    [job applyPreset:preset];
}

- (void)testAudio
{
    XCTAssertEqual(self.job.audio.tracks.count, 2);
}

- (void)testPictureSize
{
    XCTAssertEqual(self.job.picture.width, 1254);
    XCTAssertEqual(self.job.picture.height, 678);
}

- (void)testAutoCrop
{
    XCTAssertEqual([self.preset.content[@"PictureAutoCrop"] boolValue], self.job.picture.autocrop);
}

- (void)testAutoCropValues
{
    XCTAssertEqual(self.title.autoCropTop, self.job.picture.cropTop);
    XCTAssertEqual(self.title.autoCropBottom, self.job.picture.cropBottom);
    XCTAssertEqual(self.title.autoCropLeft, self.job.picture.cropLeft);
    XCTAssertEqual(self.title.autoCropRight, self.job.picture.cropRight);
}

@end
