/*  HBDistributedArray.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBDistributedArray.h"

#include <semaphore.h>

/**
 * HBProxyArrayObject wraps an object inside a proxy
 * to make it possible to keep a reference to an array
 * object even if the underlying has been swapped
 */

@interface HBProxyArrayObject : NSProxy

- (instancetype)initWithObject:(id)object;

@property (nonatomic, retain) id representedObject;
@property (nonatomic, readonly) NSString *uuid;

@end

@implementation HBProxyArrayObject

- (instancetype)initWithObject:(id)object
{
    _representedObject = [object retain];

    return self;
}

- (void)dealloc
{
    [_representedObject release];
    [super dealloc];
}

- (NSMethodSignature *)methodSignatureForSelector:(SEL)selector
{
    return [self.representedObject methodSignatureForSelector:selector];
}

- (void)forwardInvocation:(NSInvocation *)invocation
{
    [invocation invokeWithTarget:self.representedObject];
}

- (NSString *)uuid
{
    return [self.representedObject uuid];
}

@end

NSString *HBDistributedArrayChanged = @"HBDistributedArrayChanged";
NSString *HBDistributedArraWrittenToDisk = @"HBDistributedArraWrittenToDisk";

@interface HBDistributedArray ()

@property (nonatomic, readonly) NSMutableArray *array;
@property (nonatomic, readonly) NSURL *fileURL;
@property (nonatomic, readwrite) NSTimeInterval modifiedTime;

@property (nonatomic, readonly) sem_t *mutex;

@end

@implementation HBDistributedArray

- (instancetype)initWithURL:(NSURL *)fileURL
{
    self = [super init];
    if (self)
    {
        _fileURL = [fileURL copy];
        _array = [[NSMutableArray alloc] init];

        NSFileManager *fileManager = [NSFileManager defaultManager];
        if (![fileManager fileExistsAtPath:_fileURL.path])
        {
            if (![fileManager fileExistsAtPath:_fileURL.URLByDeletingLastPathComponent.path])
            {
                [fileManager createDirectoryAtPath:_fileURL.URLByDeletingLastPathComponent.path withIntermediateDirectories:YES attributes:nil error:NULL];
            }
            [fileManager createFileAtPath:_fileURL.path contents:nil attributes:nil];
        }

        NSArray *runningInstances = [NSRunningApplication runningApplicationsWithBundleIdentifier:[[NSBundle mainBundle] bundleIdentifier]];
        const char *name = [NSString stringWithFormat:@"/%@.hblock", _fileURL.lastPathComponent].UTF8String;

        // Unlink the semaphore if we are the only
        // instance running, this fixes the case where
        // HB crashed while the sem is locked.
        if (runningInstances.count == 1)
        {
            sem_unlink(name);
        }

        // Use a named semaphore as a mutex for now
        // it can cause a deadlock if an instance
        // crashed while it has the lock on the semaphore.
        _mutex = sem_open(name, O_CREAT, 0777, 1);
        if (_mutex == SEM_FAILED) {
            NSLog(@"%s: %d\n", "Error in creating semaphore: ", errno);
        }

        [[NSDistributedNotificationCenter defaultCenter] addObserver:self selector:@selector(handleNotification:) name:HBDistributedArraWrittenToDisk object:nil];

        // Load the array from disk
        [self lock];
        [self reload];
        [self unlock];
    }

    return self;
}

- (void)dealloc
{
    [self lock];
    [self synchronize];
    [self unlock];

    [_fileURL release];
    _fileURL = nil;
    [_array release];
    _array = nil;

    [[NSDistributedNotificationCenter defaultCenter] removeObserver:self];

    sem_close(_mutex);

    [super dealloc];
}

- (void)lock
{
    sem_wait(self.mutex);
}

- (void)unlock
{
    sem_post(self.mutex);
}

- (void)beginTransaction
{
    [self lock];
    // We got the lock, need to check if
    // someone else modified the file
    // while we were locked, because we
    // could have not received the notification yet
    NSDate *date = nil;
    [self.fileURL getResourceValue:&date forKey:NSURLAttributeModificationDateKey error:nil];
    if (date.timeIntervalSinceReferenceDate > self.modifiedTime)
    {
        // File was modified while we waited on the lock
        // reload it
        [self reload];
    }
}

- (void)commit
{
    [self synchronize];
    [self unlock];
}

- (void)postNotification
{
    [[NSNotificationCenter defaultCenter] postNotificationName:HBDistributedArrayChanged object:self];
}

/**
 *  Handle the distributed notification
 */
- (void)handleNotification:(NSNotification *)notification
{
    if (!([notification.object integerValue] == getpid()))
    {
        if ([notification.userInfo[@"path"] isEqualToString:self.fileURL.path])
        {
            [self lock];
            [self reload];
            [self unlock];
        }
    }
}

/**
 *  Reload the array from disk
 */
- (void)reload
{
    NSMutableArray *jobsArray = nil;
    @try
    {
        jobsArray = [NSKeyedUnarchiver unarchiveObjectWithFile:self.fileURL.path];
    }
    @catch (NSException *exception)
    {
        jobsArray = nil;
    }

    // Swap the proxy objects representation with the new
    // one read from disk
    NSMutableArray *proxyArray = [NSMutableArray array];
    for (id anObject in jobsArray)
    {
        NSString *uuid = [anObject uuid];

        HBProxyArrayObject *proxy = nil;
        for (HBProxyArrayObject *temp in self.array)
        {
            if ([[temp uuid] isEqualToString:uuid])
            {
                temp.representedObject = anObject;
                proxy = temp;
                break;
            }
        }

        if (proxy)
        {
            [proxyArray addObject:proxy];
        }
        else
        {
            [proxyArray addObject:[self wrapObjectIfNeeded:anObject]];
        }
    }

    [self setArray:proxyArray];
    [self postNotification];

    // Update the time, so we can avoid reloaded the file from disk later.
    self.modifiedTime = [NSDate timeIntervalSinceReferenceDate];
}

/**
 *  Writes the changes to disk
 */
- (void)synchronize
{
    NSMutableArray *temp = [NSMutableArray array];

    // Unwrap the array objects and save them to disk
    for (HBProxyArrayObject *proxy in self)
    {
        [temp addObject:proxy.representedObject];
    }

    if (![NSKeyedArchiver archiveRootObject:temp toFile:self.fileURL.path])
    {
        NSLog(@"failed to write the queue to disk");
    }

    // Send a distributed notification.
    [[NSDistributedNotificationCenter defaultCenter] postNotificationName:HBDistributedArraWrittenToDisk
                                                                   object:[NSString stringWithFormat:@"%d", getpid()]
                                                                 userInfo:@{@"path": self.fileURL.path}
                                                       deliverImmediately:YES];

    // Update the time, so we can avoid reloaded the file from disk later.
    self.modifiedTime = [NSDate timeIntervalSinceReferenceDate];
}

/**
 *  Wraps an object inside a HBObjectProxy instance
 *  if it's not already wrapped.
 *
 *  @param anObject the object to wrap
 *
 *  @return a wrapped object
 */
- (id)wrapObjectIfNeeded:(id)anObject
{
    if ([[anObject class] isEqual:[HBProxyArrayObject class]])
    {
        return anObject;
    }
    else
    {
        return [[[HBProxyArrayObject alloc] initWithObject:anObject] autorelease];
    }
}

#pragma mark - Methods needed to subclass NSMutableArray

- (void)insertObject:(id)anObject atIndex:(NSUInteger)index
{
    [self.array insertObject:[self wrapObjectIfNeeded:anObject] atIndex:index];
}

- (void)removeObjectAtIndex:(NSUInteger)index
{
    [self.array removeObjectAtIndex:index];
}

- (void)addObject:(id)anObject
{
    [self.array addObject:[self wrapObjectIfNeeded:anObject]];
}

- (void)removeLastObject
{
    [self.array removeLastObject];
}

- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)anObject
{
    [self.array replaceObjectAtIndex:index withObject:[self wrapObjectIfNeeded:anObject]];
}

- (NSUInteger)count
{
    return [self.array count];
}

- (id)objectAtIndex:(NSUInteger)index
{
    return [self.array objectAtIndex:index];
}

@end
