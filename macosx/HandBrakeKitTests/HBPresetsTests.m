/*  HBPresetsTests.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>

@import HandBrakeKit;

@interface HBPresetsTests : XCTestCase

@end

@implementation HBPresetsTests

- (void)setUp {
    [super setUp];
}

- (void)tearDown
{
    [super tearDown];
}

- (void)testManagerCreation
{
    HBPresetsManager *manager = [[HBPresetsManager alloc] init];

    XCTAssert(manager, @"Pass");
}

- (void)testDefaultPresets
{
    HBPresetsManager *manager = [[HBPresetsManager alloc] init];
    [manager generateBuiltInPresets];

    XCTAssert(manager.root.children.count > 1, @"Pass");
}

- (void)testCreationTime
{
    HBPresetsManager *manager = [[HBPresetsManager alloc] init];

    [self measureBlock:^{
        [manager generateBuiltInPresets];
    }];
}

- (void)testSave
{
    NSURL *tempURL = [NSURL fileURLWithPath:NSTemporaryDirectory()];
    NSURL *presetsURL = [tempURL URLByAppendingPathComponent:@"test.json" isDirectory:NO];
    HBPresetsManager *manager = [[HBPresetsManager alloc] initWithURL:presetsURL];
    [manager savePresets];

    XCTAssertTrue([presetsURL checkResourceIsReachableAndReturnError:NULL]);

    // Remove the temp files.
    [NSFileManager.defaultManager removeItemAtURL:presetsURL error:NULL];
}

- (void)testUpgrade
{
    NSURL *tempURL = [NSURL fileURLWithPath:NSTemporaryDirectory()];
    NSURL *presetsURL = [tempURL URLByAppendingPathComponent:@"test.json" isDirectory:NO];
    NSURL *modifiedPresetsURL = [tempURL URLByAppendingPathComponent:@"test2.json" isDirectory:NO];

    // Create a new presets manager with the defaults presets.
    HBPresetsManager *manager = [[HBPresetsManager alloc] initWithURL:presetsURL];
    [manager savePresets];

    // Read the json and change the version to the previous major
    // so it will kick in the import routine.
    NSData *data = [NSData dataWithContentsOfURL:presetsURL];
    NSMutableDictionary *dict = [NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:NULL];
    dict[@"VersionMajor"] = @([dict[@"VersionMajor"] integerValue] - 1);


    NSString *backupName = [NSString stringWithFormat:@"%@.%d.%d.%d.json",
                            modifiedPresetsURL.lastPathComponent.stringByDeletingPathExtension,
                            [dict[@"VersionMajor"] intValue],
                            [dict[@"VersionMinor"] intValue],
                            [dict[@"VersionMicro"] intValue]];
    NSURL *backupURL = [tempURL URLByAppendingPathComponent:backupName isDirectory:NO];

    NSData *modifiedData = [NSJSONSerialization dataWithJSONObject:dict options:0 error:NULL];
    [modifiedData writeToURL:modifiedPresetsURL atomically:YES];

    // Create a new manager and init it with the modified json.
    HBPresetsManager *newManager = [[HBPresetsManager alloc] initWithURL:modifiedPresetsURL];

    XCTAssert(newManager);
    XCTAssertTrue([backupURL checkResourceIsReachableAndReturnError:NULL]);

    // Remove the temp files.
    [NSFileManager.defaultManager removeItemAtURL:presetsURL error:NULL];
    [NSFileManager.defaultManager removeItemAtURL:modifiedPresetsURL error:NULL];
    [NSFileManager.defaultManager removeItemAtURL:backupURL error:NULL];
}

@end
