/*  NSDictionary+HBAdditions.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "NSDictionary+HBAdditions.h"

@implementation NSDictionary (HBAddtions)

- (instancetype)initWithHBDict:(const hb_dict_t *)dict
{
    self = [self init];
    if (self)
    {
        self = convertDictToObjcType(dict);
    }
    return self;
}

- (hb_dict_t *)hb_dictValue
{
    return convertObjcDictToHBValue(self);
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

#pragma mark - NSDictionary to hb_dict_t

static hb_value_t * objcValueToHBValue(id val)
{
    hb_value_t *result = NULL;

    if ([val isKindOfClass:[NSNumber class]])
    {
        NSNumber *number = val;
        const char *objCType = number.objCType;

        if((strcmp(objCType, @encode(float))) == 0 ||
           (strcmp(objCType, @encode(double))) == 0)
        {
            result = hb_value_double(number.doubleValue);
        }
        else if((strcmp(objCType, @encode(BOOL))) == 0)
        {
            result = hb_value_bool(number.boolValue);
        }
        else
        {
            result = hb_value_int(number.integerValue);
        }

    }
    else if ([val isKindOfClass:[NSString class]])
    {
        NSString *string = val;
        result = hb_value_string(string.UTF8String);

    }
    else if ([val isKindOfClass:[NSDictionary class]])
    {
        result = convertObjcDictToHBValue(val);
    }
    else if ([val isKindOfClass:[NSArray class]])
    {
        result = convertObjcArrayToHBValue(val);
    }
    
    return result;
}

static hb_dict_t * convertObjcDictToHBValue(NSDictionary *dict)
{
    hb_dict_t *result = hb_dict_init();

    [dict enumerateKeysAndObjectsUsingBlock:^(id  _Nonnull key, id  _Nonnull obj, BOOL * _Nonnull stop)
    {
        if ([key isKindOfClass:[NSString class]])
        {
            hb_value_t *val = objcValueToHBValue(obj);
            if (val)
            {
                hb_dict_set(result, [key UTF8String], objcValueToHBValue(obj));
            }
        }
    }];

    return result;
}

static hb_value_array_t * convertObjcArrayToHBValue(NSArray *array)
{
    hb_value_array_t *result = hb_value_array_init();

    for (id obj in array)
    {
        hb_value_t *val = objcValueToHBValue(obj);
        if (val)
        {
            hb_value_array_append(result, objcValueToHBValue(obj));
        }
    }

    return result;
}

@end
