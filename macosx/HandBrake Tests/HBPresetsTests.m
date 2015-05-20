/*  HBPresetsTests.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>

#import "HBPreset.h"
#import "HBPresetsManager.h"

@interface HBPresetsTests : XCTestCase

@end

@implementation HBPresetsTests

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testManagerCreation {
    HBPresetsManager *manager = [[HBPresetsManager alloc] init];

    XCTAssert(manager, @"Pass");
}

- (void)testDefaultPresets {
    HBPresetsManager *manager = [[HBPresetsManager alloc] init];
    [manager generateBuiltInPresets];

    XCTAssert(manager.root.children.count > 1, @"Pass");
}

- (void)testCreationTime {
    HBPresetsManager *manager = [[HBPresetsManager alloc] init];

    [self measureBlock:^{
        [manager generateBuiltInPresets];

    }];
}

@end
