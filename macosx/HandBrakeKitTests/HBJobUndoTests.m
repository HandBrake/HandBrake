/*  HBJobUndoTests.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <XCTest/XCTest.h>

#import "HBMockTitle.h"
#import "HBJob.h"
#import "HBChapter.h"
#import "HBAudioTrack.h"
#import "HBSubtitlesTrack.h"
#import "HBPicture.h"
#import "HBPresetsManager.h"
#import "HBPreset.h"

@interface HBJobUndoTests : XCTestCase

@property (nonatomic, readonly) HBPresetsManager *manager;

@property (nonatomic, readwrite) HBPreset *preset;
@property (nonatomic, readwrite) HBTitle *title;
@property (nonatomic, readwrite) HBJob *job;
@property (nonatomic, readwrite) HBJob *modifiedJob;

@end

@implementation HBJobUndoTests

- (void)setUp
{
    [super setUp];

    _manager = [[HBPresetsManager alloc] init];
    [_manager generateBuiltInPresets];

    self.preset = self.manager.defaultPreset;

    self.title = [[HBMockTitle alloc] init];

    self.job = [[HBJob alloc] initWithTitle:self.title andPreset:self.preset];
    self.job.destURL = [NSURL fileURLWithPath:@"/Dest.mp4"];

    NSUndoManager *undoManager = [[NSUndoManager alloc] init];
    undoManager.groupsByEvent = NO;

    self.modifiedJob = [self.job copy];
    self.modifiedJob.undo = undoManager;

    [self.manager.root enumerateObjectsUsingBlock:^(HBPreset * _Nonnull obj, NSIndexPath * _Nonnull idx, BOOL * _Nonnull stop)
    {
        if (obj.isLeaf)
        {
            [undoManager beginUndoGrouping];
            [self.modifiedJob applyPreset:obj];
            [undoManager endUndoGrouping];
        }
    }];

    // Container changed
    [undoManager beginUndoGrouping];
    self.modifiedJob.container = 0x200000;
    [undoManager endUndoGrouping];

    // Chapters changed
    [undoManager beginUndoGrouping];
    self.modifiedJob.chapterTitles[0].title = @"Test chapter";
    self.modifiedJob.range.type = HBRangeTypeFrames;
    [undoManager endUndoGrouping];

    // Undo, redo and undo again the whole thing one time
    while (undoManager.canUndo)
    {
        [undoManager undo];
    }
    while (undoManager.canRedo)
    {
        [undoManager redo];
    }
    while (undoManager.canUndo)
    {
        [undoManager undo];
    }
}

- (void)tearDown {
    [super tearDown];
}

- (void)testJob
{
    XCTAssertEqualObjects(self.job.presetName, self.modifiedJob.presetName);
    XCTAssertEqual(self.job.container, self.modifiedJob.container);

    XCTAssertEqual(self.job.mp4HttpOptimize, self.modifiedJob.mp4HttpOptimize);
    XCTAssertEqual(self.job.mp4iPodCompatible, self.modifiedJob.mp4iPodCompatible);

    XCTAssertEqual(self.job.angle, self.modifiedJob.angle);
}

- (void)testAudio
{
    XCTAssertEqual([self.job.audio countOfTracks], [self.modifiedJob.audio countOfTracks]);

    for (NSUInteger idx = 0; idx < [self.job.audio countOfTracks]; idx++)
    {
        HBAudioTrack *t1 = self.job.audio.tracks[idx];
        HBAudioTrack *t2 = self.modifiedJob.audio.tracks[idx];

        XCTAssertEqualObjects(t1.track, t2.track);
    }
}

- (void)testRange
{
    XCTAssertEqual(self.job.range.type, self.modifiedJob.range.type);
}

- (void)testPicture
{
    XCTAssertEqual(self.job.picture.width, self.modifiedJob.picture.width);
    XCTAssertEqual(self.job.picture.height, self.modifiedJob.picture.height);

    XCTAssertEqual(self.job.picture.displayWidth, self.modifiedJob.picture.displayWidth);
    XCTAssertEqual(self.job.picture.parWidth, self.modifiedJob.picture.parWidth);
    XCTAssertEqual(self.job.picture.parHeight, self.modifiedJob.picture.parHeight);

    XCTAssertEqual(self.job.picture.autocrop, self.modifiedJob.picture.autocrop);

    XCTAssertEqual(self.job.picture.cropTop, self.modifiedJob.picture.cropTop);
    XCTAssertEqual(self.job.picture.cropBottom, self.modifiedJob.picture.cropBottom);
    XCTAssertEqual(self.job.picture.cropLeft, self.modifiedJob.picture.cropLeft);
    XCTAssertEqual(self.job.picture.cropRight, self.modifiedJob.picture.cropRight);
}

- (void)testFilters
{
    XCTAssertEqualObjects(self.job.filters.deinterlace, self.modifiedJob.filters.deinterlace);
    XCTAssertEqualObjects(self.job.filters.deinterlacePreset, self.modifiedJob.filters.deinterlacePreset);
    XCTAssertEqualObjects(self.job.filters.deinterlaceCustomString, self.modifiedJob.filters.deinterlaceCustomString);

    XCTAssertEqualObjects(self.job.filters.detelecine, self.modifiedJob.filters.detelecine);
    XCTAssertEqualObjects(self.job.filters.detelecineCustomString, self.modifiedJob.filters.detelecineCustomString);

    XCTAssertEqualObjects(self.job.filters.denoise, self.modifiedJob.filters.denoise);
    XCTAssertEqualObjects(self.job.filters.denoisePreset, self.modifiedJob.filters.denoisePreset);
    XCTAssertEqualObjects(self.job.filters.denoiseTune, self.modifiedJob.filters.denoiseTune);
    XCTAssertEqualObjects(self.job.filters.denoiseCustomString, self.modifiedJob.filters.denoiseCustomString);

    XCTAssertEqual(self.job.filters.deblock, self.modifiedJob.filters.deblock);
    XCTAssertEqual(self.job.filters.grayscale, self.modifiedJob.filters.grayscale);
}

- (void)testSubtitles
{
    XCTAssertEqual([self.job.subtitles countOfTracks], [self.modifiedJob.subtitles countOfTracks]);

    for (NSUInteger idx = 0; idx < [self.job.subtitles countOfTracks]; idx++)
    {
        HBSubtitlesTrack *t1 = self.job.subtitles.tracks[idx];
        HBSubtitlesTrack *t2 = self.modifiedJob.subtitles.tracks[idx];

        XCTAssertEqual(t1.sourceTrackIdx, t2.sourceTrackIdx);
        XCTAssertEqual(t1.type, t2.type);
        XCTAssertEqual(t1.burnedIn, t2.burnedIn);
        XCTAssertEqual(t1.forcedOnly, t2.forcedOnly);
        XCTAssertEqual(t1.def, t2.def);
    }
}

- (void)testChapters
{
    XCTAssertEqual(self.job.chaptersEnabled, self.modifiedJob.chaptersEnabled);

    for (NSUInteger idx = 0; idx < [self.job.subtitles countOfTracks]; idx++)
    {
        XCTAssertEqualObjects(self.job.chapterTitles[idx].title, self.modifiedJob.chapterTitles[idx].title);
    }
}

@end
