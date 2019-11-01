/*  HBDictTests.m

This file is part of the HandBrake source code.
Homepage: <http://handbrake.fr/>.
It may be used under the terms of the GNU General Public License. */

#import <XCTest/XCTest.h>
#import "NSDictionary+HBAdditions.h"
#include "handbrake.h"

@interface HBDictTests : XCTestCase

@property (nonatomic, strong) NSDictionary *dict;

@end

@implementation HBDictTests

- (void)setUp
{
    [super setUp];
    self.dict = @{@"DoubleKey": @(20.3),
                  @"StringKey": @"miao",
                  @"BoolKey": @YES,
                  @"ArrayKey": @[@"First", @"Second", @20]};
}

- (void)tearDown
{
    self.dict = nil;
    [super tearDown];
}

- (void)testNSDictionaryToHBDict
{
    hb_dict_t *hbdict = self.dict.hb_value;

    double doubleValue = hb_value_get_double(hb_dict_get(hbdict, "DoubleKey"));
    XCTAssertEqual(doubleValue, [self.dict[@"DoubleKey"] doubleValue]);

    const char *stringValue = hb_value_get_string(hb_dict_get(hbdict, "StringKey"));
    XCTAssertEqualObjects(@(stringValue), self.dict[@"StringKey"]);

    BOOL boolValue = (BOOL)hb_value_get_bool(hb_dict_get(hbdict, "BoolKey"));
    XCTAssertEqual(boolValue, [self.dict[@"BoolKey"] boolValue]);

    hb_value_array_t *array = hb_dict_get(hbdict, "ArrayKey");

    size_t count = hb_value_array_len(array);
    XCTAssertEqual(count, [self.dict[@"ArrayKey"] count]);

    const char *arrayString = hb_value_get_string(hb_value_array_get(array, 0));
    XCTAssertEqualObjects(@(arrayString), self.dict[@"ArrayKey"][0]);

    long long arrayInt = hb_value_get_int(hb_value_array_get(array, 2));
    XCTAssertEqual(arrayInt, [self.dict[@"ArrayKey"][2] integerValue]);

    hb_dict_free(&hbdict);
}

- (void)testNSDictionaryToHBDictToNSDictionary
{
    hb_dict_t *hbdict = self.dict.hb_value;
    NSDictionary *result =  [[NSDictionary alloc] initWithHBDict:hbdict];

    XCTAssertEqualObjects(result[@"DoubleKey"], self.dict[@"DoubleKey"]);
    XCTAssertEqualObjects(result[@"StringKey"], self.dict[@"StringKey"]);
    XCTAssertEqualObjects(result[@"BoolKey"], self.dict[@"BoolKey"]);
    XCTAssertEqualObjects(result[@"ArrayKey"], self.dict[@"ArrayKey"]);
}

- (void)testPerformanceHBDict
{
    NSDictionary *dict = self.dict;

    [self measureBlock:^{
        for (int i = 0; i < 10000; i++)
        {
            hb_dict_t *result = dict.hb_value;
            hb_dict_free(&result);
        }
    }];
}

- (void)testPerformanceNSDictionary
{
    NSDictionary *dict = self.dict;
    hb_dict_t *hbdict = dict.hb_value;

    [self measureBlock:^{
        for (int i = 0; i < 10000; i++)
        {
            __unused NSDictionary *result =  [[NSDictionary alloc] initWithHBDict:hbdict];
        }
    }];
}

@end
