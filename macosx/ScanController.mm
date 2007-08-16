/*  $Id: ScanController.mm,v 1.10 2005/04/27 21:05:24 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.m0k.org/>.
   It may be used under the terms of the GNU General Public License. */
/* These are now called in DriveDetector.h
#include <paths.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>
*/

#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>

#include "ScanController.h"
//#include "DriveDetector.h"

#include "dvdread/dvd_reader.h"

#define _(a) NSLocalizedString(a,nil)
#define INSERT_STRING @"Insert a DVD"

@implementation ScanController



- (void) SetHandle: (hb_handle_t *) handle
{
    fHandle    = handle;
}

- (void) Show
{

	[self Browse: NULL];
}

- (void) Browse: (id) sender
{
    NSOpenPanel * panel;
	
    panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection: NO];
    [panel setCanChooseFiles: YES];
    [panel setCanChooseDirectories: YES ];
    NSString * sourceDirectory;
	if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastSourceDirectory"])
	{
		sourceDirectory = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastSourceDirectory"];
	}
	else
	{
		sourceDirectory = @"~/Desktop";
		sourceDirectory = [sourceDirectory stringByExpandingTildeInPath];
	}
   [panel beginSheetForDirectory: sourceDirectory file: nil types: nil
				   modalForWindow: fWindow modalDelegate: self
				   didEndSelector: @selector( BrowseDone:returnCode:contextInfo: )
					  contextInfo: nil];
}

- (void) BrowseDone: (NSOpenPanel *) sheet
		 returnCode: (int) returnCode contextInfo: (void *) contextInfo
{
    /* User selected a file to open */
	if( returnCode == NSOKButton )
    {
        [fStatusField setStringValue: _( @"Opening a new source ..." )];
		[fIndicator setHidden: NO];
	    [fIndicator setIndeterminate: YES];
        [fIndicator startAnimation: nil];
		
		
		/* we set the last source directory in the prefs here */
		NSString *sourceDirectory = [[[sheet filenames] objectAtIndex: 0] stringByDeletingLastPathComponent];
		[[NSUserDefaults standardUserDefaults] setObject:sourceDirectory forKey:@"LastSourceDirectory"];
		
        
        
        NSString *path = [[sheet filenames] objectAtIndex: 0];
       NSString *dvdPath = [self dvdDevicePathForVolume: path];
        if (dvdPath)
        {
            // The chosen path was actually a mount point for a DVD, so use the
            // raw block device instead.
            path = dvdPath;
        }

		hb_scan( fHandle, [path UTF8String], 0 );
		
		[self Cancel: nil];
    }
	else // User clicked Cancel in browse window
	{
		
		[self Cancel: nil];
		
	}
    
    
	
}


- (IBAction) Cancel: (id) sender
{
   [NSApp endSheet:fPanel];
	[fPanel orderOut:self];
}

Boolean IsDVD(io_service_t service)
{
    //
    // Determine if the object passed in represents an IOMedia (or subclass) object.
    // If it does, retrieve the "Whole" property.
    // If this is the whole media object, find out if it is a DVD.
    // If it isn't the whole media object, iterate across its parents in the IORegistry
    // until the whole media object is found.
    //
    
    Boolean isWholeMedia = NO;
    
    if (IOObjectConformsTo(service, kIOMediaClass))
    {
        CFTypeRef wholeMedia;
        
        wholeMedia = IORegistryEntryCreateCFProperty(service, 
                                                     CFSTR(kIOMediaWholeKey), 
                                                     kCFAllocatorDefault, 
                                                     0);
        
        if (!wholeMedia)
        {
            return NO;
        }
        else
        {                                        
            isWholeMedia = CFBooleanGetValue((CFBooleanRef)wholeMedia);
            CFRelease(wholeMedia);
        }
    }
    
    if (isWholeMedia && IOObjectConformsTo(service, kIODVDMediaClass))
    {
        return YES;
    }
    
    return NO;
}

Boolean FindDVD(io_service_t service)
{
    kern_return_t  kernResult;
    io_iterator_t  iter;
    
    // Create an iterator across all parents of the service object passed in.
    kernResult = IORegistryEntryCreateIterator(service,
                                               kIOServicePlane,
                                               kIORegistryIterateRecursively | kIORegistryIterateParents,
                                               &iter);
    
    if (kernResult != KERN_SUCCESS)
    {
        return NO;
    }

    if (iter == IO_OBJECT_NULL)
    {
        return NO;
    }

    Boolean isDVD;
    
    // A reference on the initial service object is released in the do-while loop below,
    // so add a reference to balance 
    IOObjectRetain(service);  
    
    do
    {
        isDVD = IsDVD(service);
        IOObjectRelease(service);
    } while (!isDVD && (service = IOIteratorNext(iter)));
    
    IOObjectRelease(iter);
    
    return isDVD;
}

Boolean DeviceIsDVD(char *bsdName)
{
    // The idea is that given the BSD node name corresponding to a volume,
    // I/O Kit can be used to find the information about the media, drive, bus, and so on
    // that is maintained in the IORegistry.
    //
    // In this sample, we find out if the volume is on a CD, DVD, or some other media.
    // This is done as follows:
    // 
    // 1. Find the IOMedia object that represents the entire (whole) media that the volume is on. 
    //
    // If the volume is on partitioned media, the whole media object will be a parent of the volume's
    // media object. If the media is not partitioned, (a floppy disk, for example) the volume's media
    // object will be the whole media object.
    // 
    // The whole media object is indicated in the IORegistry by the presence of a property with the key
    // "Whole" and value "Yes".
    //
    // 2. Determine which I/O Kit class the whole media object belongs to.
    //
    // 3. For DVD media, return YES;
    //
    
    CFMutableDictionaryRef  matchingDict;
    io_service_t      service;
    
    matchingDict = IOBSDNameMatching(kIOMasterPortDefault, 0, bsdName);
    if (matchingDict == NULL)
    {
        return NO;
    }
	
    // Fetch the object with the matching BSD node name.     // Note that there should only be one match, so IOServiceGetMatchingService is used instead of
    // IOServiceGetMatchingServices to simplify the code.
    service = IOServiceGetMatchingService(kIOMasterPortDefault, matchingDict);    
    
    if (service == IO_OBJECT_NULL) {
        return NO;
    }
	
    Boolean isDVD = FindDVD(service);
    IOObjectRelease(service);
    return isDVD;
}


- (NSString *)dvdDevicePathForVolume: (NSString *)volumePath
{
    OSStatus err;
	FSRef ref;
	FSVolumeRefNum actualVolume;
	err = FSPathMakeRef ( (const UInt8 *) [volumePath fileSystemRepresentation], &ref, NULL );
	
	// get a FSVolumeRefNum from mountPath
	if ( err != noErr )
    {
        return nil;
    }
    
    FSCatalogInfo   catalogInfo;
    err = FSGetCatalogInfo ( &ref,
                             kFSCatInfoVolume,
                             &catalogInfo,
                             NULL,
                             NULL,
                             NULL
                             );
    if ( err != noErr )
    {
        return nil;
    }

    actualVolume = catalogInfo.volume;
	
	// now let's get the device name
	GetVolParmsInfoBuffer volumeParms;
	HParamBlockRec pb;
	
	// Use the volume reference number to retrieve the volume parameters. See the documentation
	// on PBHGetVolParmsSync for other possible ways to specify a volume.
	pb.ioParam.ioNamePtr = NULL;
	pb.ioParam.ioVRefNum = actualVolume;
	pb.ioParam.ioBuffer = (Ptr) &volumeParms;
	pb.ioParam.ioReqCount = sizeof(volumeParms);
	
	// A version 4 GetVolParmsInfoBuffer contains the BSD node name in the vMDeviceID field.
	// It is actually a char * value. This is mentioned in the header CoreServices/CarbonCore/Files.h.
	err = PBHGetVolParmsSync(&pb);

    // Now that we have the device name, check to see if is a DVD or not.
	//[self DeviceIsDVD: ((char *)volumeParms.vMDeviceID)] // cocoa
    //if (!DeviceIsDVD)
	char *deviceID = (char *)volumeParms.vMDeviceID;
	//if (!DeviceIsDVD((char *)volumeParms.vMDeviceID)) // c
    if (!DeviceIsDVD(deviceID))
	{
        return nil;
    }
	return [NSString stringWithFormat: @"/dev/%s", deviceID];
}



@end
