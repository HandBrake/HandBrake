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

#import "HBDVDDetector.h"


@interface HBDVDDetector (Private)

- (NSString *)bsdNameForPath;
- (BOOL)pathHasVideoTS;
- (BOOL)deviceIsDVD;
- (io_service_t)getIOKitServiceForBSDName;
- (BOOL)isDVDService: (io_service_t)service;
- (BOOL)isWholeMediaService: (io_service_t)service;

@end


@implementation HBDVDDetector

+ (HBDVDDetector *)detectorForPath: (NSString *)aPath
{
    return [[[self alloc] initWithPath:aPath] autorelease];
}


- (HBDVDDetector *)initWithPath: (NSString *)aPath
{
    NSAssert(aPath, @"nil string passed to drive detector.");
	if( self = [super init] )	
	{
        path = [aPath retain];
        bsdName = nil;
	}
    return self;
}


- (void)dealloc
{
    [path release];
    path = nil;
    [bsdName release];
    bsdName = nil;
    [super dealloc];
}


- (BOOL)isVideoDVD
{
    if( !bsdName )
    {
        bsdName = [[self bsdNameForPath] retain];
    }
    return ( [self pathHasVideoTS] && [self deviceIsDVD] );
}


- (NSString *)devicePath
{
    if( !bsdName )
    {
        bsdName = [[self bsdNameForPath] retain];
    }
    return [NSString stringWithFormat:@"/dev/%@", bsdName];
}

@end


@implementation HBDVDDetector (Private)

- (NSString *)bsdNameForPath
{
    OSStatus err;
    FSRef ref;
    err = FSPathMakeRef( (const UInt8 *) [path fileSystemRepresentation],
                         &ref, NULL );	
    if( err != noErr )
    {
        return nil;
    }

    // Get the volume reference number.
    FSCatalogInfo catalogInfo;
    err = FSGetCatalogInfo( &ref, kFSCatInfoVolume, &catalogInfo, NULL, NULL,
                            NULL);
    if( err != noErr )
    {
        return nil;
    }
    FSVolumeRefNum volRefNum = catalogInfo.volume;

    // Now let's get the device name
    GetVolParmsInfoBuffer volumeParms;
    err = FSGetVolumeParms ( volRefNum, &volumeParms, sizeof( volumeParms ) );

    if( err != noErr )
    {
        return nil;
    }

    // A version 4 GetVolParmsInfoBuffer contains the BSD node name in the vMDeviceID field.
    // It is actually a char * value. This is mentioned in the header CoreServices/CarbonCore/Files.h.
    return [NSString stringWithCString:(char *)volumeParms.vMDeviceID];
}


- (BOOL)pathHasVideoTS
{
    // Check one level under the path
    if( [[NSFileManager defaultManager] fileExistsAtPath:
        [path stringByAppendingPathComponent:@"VIDEO_TS"]] )
    {
        return YES;
    }

    // Now check above the path
    return [[path pathComponents] containsObject:@"VIDEO_TS"];
}


- (BOOL)deviceIsDVD
{
    io_service_t service = [self getIOKitServiceForBSDName];
    if( service == IO_OBJECT_NULL )
    {
        return NO;
    }
    BOOL result = [self isDVDService:service];
    IOObjectRelease(service);
    return result;
}


- (io_service_t)getIOKitServiceForBSDName
{
    CFMutableDictionaryRef  matchingDict;
    matchingDict = IOBSDNameMatching( kIOMasterPortDefault, 0, [bsdName UTF8String] );
    if( matchingDict == NULL )
    {
        return IO_OBJECT_NULL;
    }
	
    // Fetch the object with the matching BSD node name. There should only be
    // one match, so IOServiceGetMatchingService is used instead of
    // IOServiceGetMatchingServices to simplify the code.
    return IOServiceGetMatchingService( kIOMasterPortDefault, matchingDict );    
}


- (BOOL)isDVDService: (io_service_t)service
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

    BOOL isDVD = NO;
    do
    {
        isDVD = ( [self isWholeMediaService:service] &&
                  IOObjectConformsTo(service, kIODVDMediaClass) );
        IOObjectRelease(service);
    } while( !isDVD && (service = IOIteratorNext(iter)) );
    IOObjectRelease( iter );

    return isDVD;
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