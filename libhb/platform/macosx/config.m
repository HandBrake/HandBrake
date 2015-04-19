#import <Foundation/Foundation.h>
/* #import <Cocoa/Cocoa.h> */

void osx_get_user_config_directory(char path[512])
{
    @autoreleasepool {
        NSArray *paths = NSSearchPathForDirectoriesInDomains(
                        NSApplicationSupportDirectory, NSUserDomainMask, YES);
        NSString *dir = paths[0];
        strncpy(path, dir.UTF8String, 512);
        path[511] = 0;
    }
}

