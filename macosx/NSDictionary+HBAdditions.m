/*  NSDictionary+HBAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "NSDictionary+HBAdditions.h"

#pragma mark - NSDictionary to hb_dict_t

@implementation NSObject (HBValueAdditions)

- (hb_value_t *)hb_value
{
    return NULL;
}

@end

@implementation NSNumber (HBValueAdditions)

- (hb_value_t *)hb_value
{
    hb_value_t *result = NULL;

    const char *objCType = self.objCType;

    if((strcmp(objCType, @encode(float))) == 0 ||
       (strcmp(objCType, @encode(double))) == 0)
    {
        result = hb_value_double(self.doubleValue);
    }
    else if((strcmp(objCType, @encode(BOOL))) == 0)
    {
        result = hb_value_bool(self.boolValue);
    }
    else
    {
        result = hb_value_int(self.integerValue);
    }

    return result;
}

@end

@implementation NSString (HBValueAdditions)

- (nullable hb_value_t *)hb_value
{
    return hb_value_string(self.UTF8String);
}

@end

@implementation NSArray (HBValueAdditions)

- (hb_value_array_t *)hb_value
{
    hb_value_array_t *result = hb_value_array_init();

    for (id obj in self)
    {
        hb_value_t *val = [obj hb_value];
        if (val)
        {
            hb_value_array_append(result, val);
        }
    }

    return result;
}

@end

@implementation NSDictionary (HBValueAdditions)

- (instancetype)initWithHBDict:(const hb_dict_t *)dict
{
    self = [self init];
    if (self)
    {
        self = convertDictToObjcType(dict);
    }
    return self;
}

- (hb_dict_t *)hb_value
{
    hb_dict_t *result = hb_dict_init();

    [self enumerateKeysAndObjectsUsingBlock:^(id  _Nonnull key, id  _Nonnull obj, BOOL * _Nonnull stop)
    {
        NSAssert([key isKindOfClass:[NSString class]], @"[NSDictionary+HBAdditions] unsupported key type");

        hb_value_t *val = [obj hb_value];
        if (val)
        {
            hb_dict_set(result, [key UTF8String], val);
        }
     }];

    return result;
}

#pragma mark - hb_dict_t to NSDictionary

static id valueToObjcValue(const hb_value_t *val)
{
    hb_value_type_t val_type = hb_value_type(val);
    id result = nil;

    switch (val_type)
    {
        case HB_VALUE_TYPE_INT:
            result = @(hb_value_get_int(val));
            break;

        case HB_VALUE_TYPE_DOUBLE:
            result = @(hb_value_get_double(val));
            break;

        case HB_VALUE_TYPE_BOOL:
            result = @((BOOL)hb_value_get_bool(val));
            break;

        case HB_VALUE_TYPE_STRING:
            result = @(hb_value_get_string(val));
            break;

        case HB_VALUE_TYPE_DICT:
            result = convertDictToObjcType(val);
            break;

        case HB_VALUE_TYPE_ARRAY:
            result = convertArrayToObjcType(val);
            break;

        case HB_VALUE_TYPE_NULL:
            result = [NSNull null];
            break;

        default:
            break;
    }

    return result;
}

static NSDictionary * convertDictToObjcType(const hb_dict_t *dict)
{
    NSMutableDictionary *result = [NSMutableDictionary dictionary];
    hb_dict_iter_t iter;

    for (iter  = hb_dict_iter_init(dict);
         iter != HB_DICT_ITER_DONE;
         iter  = hb_dict_iter_next(dict, iter))
    {
        const NSString *key = @(hb_dict_iter_key(iter));
        const hb_value_t *val = hb_dict_iter_value(iter);

        id objcType = valueToObjcValue(val);

        if (objcType)
        {
            result[key] = objcType;
        }
    }

    return [result copy];
}

static NSArray * convertArrayToObjcType(const hb_value_array_t *array)
{
    size_t count = hb_value_array_len(array);
    NSMutableArray *result = [NSMutableArray arrayWithCapacity:count];

    for (int ii = 0; ii < count; ii++)
    {
        const hb_value_t *val = hb_value_array_get(array, ii);
        id objcType = valueToObjcValue(val);

        if (objcType)
        {
            [result addObject:objcType];
        }
    }

    return [result copy];
}

@end
