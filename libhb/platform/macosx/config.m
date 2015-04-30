#import <Foundation/Foundation.h>

int osx_get_user_config_directory(char path[512])
{
    @autoreleasepool
    {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
                                                             NSUserDomainMask, YES);
        NSString  *dir = paths.firstObject;
        if (dir.UTF8String == nil)
        {
            return -1;
        }

        strncpy(path, dir.UTF8String, 511);
        path[511] = 0;
        return 0;
    }
}
