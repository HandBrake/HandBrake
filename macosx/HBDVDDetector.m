/**
 * HBDriveDetector.m
 * 8/17/2007
 *
 * This file is part of the HandBrake source code.
 * Homepage: <http://handbrake.fr/>.
 * It may be used under the terms of the GNU General Public License.
 */

#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>
#include <IOKit/storage/IOBDMedia.h>
#include <sys/mount.h>

#import "HBDVDDetector.h"


@implementation HBDVDDetector
{
    NSString *path;
    NSString *bsdName;
}

- (instancetype)init
{
    @throw nil;
}

+ (HBDVDDetector *)detectorForPath: (NSString *)aPath
{
    return [[self alloc] initWithPath:aPath];
}


- (HBDVDDetector *)initWithPath: (NSString *)aPath
{
    NSAssert(aPath, @"nil string passed to drive detector.");
	if( self = [super init] )
	{
        path = aPath;
        bsdName = nil;
	}
    return self;
}


- (BOOL)isVideoDVD
{
    return ( [self pathHasVideoTS] && [self deviceIsDVD] );
}


- (BOOL)isVideoBluRay
{
    return ( [self pathHasBDMV] && [self deviceIsBluRay] );
}


- (NSString *)devicePath
{
    return [NSString stringWithFormat:@"/dev/%@", [self bsdName]];
}

#pragma mark - DVD

- (BOOL)pathHasVideoTS
{
    // Check one level under the path
    if ([NSFileManager.defaultManager fileExistsAtPath:[path stringByAppendingPathComponent:@"VIDEO_TS"]])
    {
        return YES;
    }

    // Now check above the path
    return [path.pathComponents containsObject:@"VIDEO_TS"];
}


- (BOOL)deviceIsDVD
{
    return [self deviceIs:kIODVDMediaClass];
}

#pragma mark - BluRay

- (BOOL)pathHasBDMV
{
    // Check one level under the path
    if ([NSFileManager.defaultManager fileExistsAtPath:[path stringByAppendingPathComponent:@"BDMV"]])
    {
        return YES;
    }

    // Now check above the path
    return [path.pathComponents containsObject:@"BDMV"];
}


- (BOOL)deviceIsBluRay
{
    return [self deviceIs:kIOBDMediaClass];
}

#pragma mark - Common

- (NSString *)bsdName
{
    if ( bsdName )
    {
        return bsdName;
    }

    struct statfs s;
    statfs(path.fileSystemRepresentation, &s);

    bsdName = @(s.f_mntfromname);

    if ([bsdName hasPrefix:@"/dev/"])
    {
        bsdName = [bsdName substringFromIndex:5];
    }

    return bsdName;
}


- (BOOL)deviceIs:(const io_name_t)class
{
    io_service_t service = [self getIOKitServiceForBSDName];
    if( service == IO_OBJECT_NULL )
    {
        return NO;
    }
    BOOL result = [self isService:service class:class];
    IOObjectRelease(service);
    return result;
}


- (io_service_t)getIOKitServiceForBSDName
{
    CFMutableDictionaryRef  matchingDict;
    matchingDict = IOBSDNameMatching( kIOMasterPortDefault, 0, [[self bsdName] UTF8String] );
    if( matchingDict == NULL )
    {
        return IO_OBJECT_NULL;
    }

    // Fetch the object with the matching BSD node name. There should only be
    // one match, so IOServiceGetMatchingService is used instead of
    // IOServiceGetMatchingServices to simplify the code.
    return IOServiceGetMatchingService( kIOMasterPortDefault, matchingDict );
}


- (BOOL)isService: (io_service_t)service class:(const io_name_t)class
{
    // Find the IOMedia object that represents the entire (whole) media that the
    // volume is on.
    //
    // If the volume is on partitioned media, the whole media object will be a
    // parent of the volume's media object. If the media is not partitioned, the
    // volume's media object will be the whole media object.
    //
    // The whole media object is indicated in the IORegistry by the presence of
    // a property with the key "Whole" and value "Yes".

    // Create an iterator across all parents of the service object passed in.
    kern_return_t  kernResult;
    io_iterator_t  iter;
    kernResult = IORegistryEntryCreateIterator( service,
                                                kIOServicePlane,
                                                kIORegistryIterateRecursively | kIORegistryIterateParents,
                                                &iter );
    if( kernResult != KERN_SUCCESS )
    {
        return NO;
    }
    if( iter == IO_OBJECT_NULL )
    {
        return NO;
    }


    // A reference on the initial service object is released in the do-while loop below,
    // so add a reference to balance.
    IOObjectRetain( service );

    BOOL conformsToClass = NO;
    do
    {
        conformsToClass = ( [self isWholeMediaService:service] &&
                            IOObjectConformsTo(service, class) );
        IOObjectRelease(service);
    } while( !conformsToClass && (service = IOIteratorNext(iter)) );
    IOObjectRelease( iter );

    return conformsToClass;
}


- (BOOL)isWholeMediaService: (io_service_t)service
{
    //
    // Determine if the object passed in represents an IOMedia (or subclass) object.
    // If it does, test the "Whole" property.
    //

    Boolean isWholeMedia = NO;

    if( IOObjectConformsTo(service, kIOMediaClass) )
    {
        CFTypeRef wholeMedia;
        wholeMedia = IORegistryEntryCreateCFProperty( service,
                                                      CFSTR(kIOMediaWholeKey),
                                                      kCFAllocatorDefault,
                                                      0);
        if( !wholeMedia )
        {
            return NO;
        }
        isWholeMedia = CFBooleanGetValue( (CFBooleanRef)wholeMedia );
        CFRelease(wholeMedia);
    }

    return isWholeMedia;
}


@end
