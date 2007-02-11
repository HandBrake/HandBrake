/* DriveDetector.m $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */

#include <paths.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>

#include "DriveDetector.h"
#include "mediafork.h"

@interface DriveDetector (Private)

- (void) detectTimer: (NSTimer *) timer;

@end

@implementation DriveDetector

- (void) dealloc
{
    [fDrives release];
    [super dealloc];
}

- (id) initWithCallback: (id) target selector: (SEL) selector
{
    fTarget   = target;
    fSelector = selector;
    
    fCount  = -1;
    fDrives = [[NSMutableDictionary alloc] initWithCapacity: 1];
    
    return self;
}

- (void) run
{
    /* Set up a timer to check devices every second */
    fTimer = [NSTimer scheduledTimerWithTimeInterval: 1.0 target: self
        selector: @selector( detectTimer: ) userInfo: nil repeats: YES];
    [[NSRunLoop currentRunLoop] addTimer: fTimer forMode:
        NSModalPanelRunLoopMode];

    /* Do a first update right away */
    [fTimer fire];
}

- (void) stop
{
    [fTimer invalidate];
}

- (void) detectTimer: (NSTimer *) timer
{
    /* Scan DVD drives (stolen from VLC) */
    io_object_t            next_media;
    mach_port_t            master_port;
    kern_return_t          kern_result;
    io_iterator_t          media_iterator;
    CFMutableDictionaryRef classes_to_match;

    kern_result = IOMasterPort( MACH_PORT_NULL, &master_port );
    if( kern_result != KERN_SUCCESS )
    {
        return;
    }

    classes_to_match = IOServiceMatching( kIODVDMediaClass );
    if( classes_to_match == NULL )
    {
        return;
    }

    CFDictionarySetValue( classes_to_match, CFSTR( kIOMediaEjectableKey ),
                          kCFBooleanTrue );

    kern_result = IOServiceGetMatchingServices( master_port,
            classes_to_match, &media_iterator );
    if( kern_result != KERN_SUCCESS )
    {
        return;
    }

    [fDrives removeAllObjects];

    next_media = IOIteratorNext( media_iterator );
    if( next_media )
    {
        char * name;
        char psz_buf[0x32];
        size_t dev_path_length;
        CFTypeRef str_bsd_path;
        do
        {
            str_bsd_path =
                IORegistryEntryCreateCFProperty( next_media,
                                                 CFSTR( kIOBSDNameKey ),
                                                 kCFAllocatorDefault,
                                                 0 );
            if( str_bsd_path == NULL )
            {
                IOObjectRelease( next_media );
                continue;
            }

            snprintf( psz_buf, sizeof(psz_buf), "%s%c", _PATH_DEV, 'r' );
            dev_path_length = strlen( psz_buf );

            if( CFStringGetCString( (CFStringRef) str_bsd_path,
                                    (char*)&psz_buf + dev_path_length,
                                    sizeof(psz_buf) - dev_path_length,
                                    kCFStringEncodingASCII ) )
            {
                if( ( name = hb_dvd_name( psz_buf ) ) )
                {
                    [fDrives setObject: [NSString stringWithCString: psz_buf]
                        forKey: [NSString stringWithCString: name]];
                }
            }

            CFRelease( str_bsd_path );

            IOObjectRelease( next_media );

        } while( ( next_media = IOIteratorNext( media_iterator ) ) );
    }

    IOObjectRelease( media_iterator );

    if( [fDrives count] != fCount )
    {
        [fTarget performSelector: fSelector withObject: fDrives];
        fCount = [fDrives count];
    }
}

@end
