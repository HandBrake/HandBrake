/*  HBJobTests.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>

#import "HBJob.h"
#import "HBPresetsManager.h"
#import "HBPreset.h"

@interface HBJobTests : XCTestCase

@property (nonatomic, readonly) HBPresetsManager *manager;

@end

@implementation HBJobTests

- (void)setUp {
    [super setUp];

    _manager = [[HBPresetsManager alloc] init];
    [_manager generateBuiltInPresets];

    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testJobCreation {
    HBJob *job = [[HBJob alloc] init];

    XCTAssert(job, @"Pass");
}

@end
