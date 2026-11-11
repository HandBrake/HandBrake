/*  HBQueueArchive.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueArchive.h"
#import "HBQueueJobItem.h"
#import "HBQueueActionItem.h"

NSString * const HBQueueArchiveFileExtension = @"hbqueue";

// Increase it when the layout of the file changes.
static const NSInteger HBQueueArchiveVersion = 1;

static NSString * const HBQueueArchiveVersionKey = @"HBQueueArchiveVersion";
static NSString * const HBQueueArchiveItemsKey = @"HBQueueArchiveItems";

@implementation HBQueueArchive

+ (BOOL)writeItems:(NSArray<id<HBQueueItem>> *)items toURL:(NSURL *)url error:(NSError **)outError
{
    NSKeyedArchiver *archiver = [[NSKeyedArchiver alloc] initRequiringSecureCoding:YES];
    [archiver encodeInteger:HBQueueArchiveVersion forKey:HBQueueArchiveVersionKey];
    [archiver encodeObject:items forKey:HBQueueArchiveItemsKey];
    [archiver finishEncoding];

    return [archiver.encodedData writeToURL:url options:NSDataWritingAtomic error:outError];
}

+ (nullable NSArray<id<HBQueueItem>> *)itemsWithContentsOfURL:(NSURL *)url error:(NSError **)outError
{
    NSData *data = [NSData dataWithContentsOfURL:url options:0 error:outError];
    if (data == nil)
    {
        return nil;
    }

    NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingFromData:data error:NULL];
    NSInteger version = [unarchiver decodeIntegerForKey:HBQueueArchiveVersionKey];
    NSArray<id<HBQueueItem>> *items = nil;

    if (version > 0 && version <= HBQueueArchiveVersion)
    {
        NSSet *classes = [NSSet setWithObjects:[NSArray class], [HBQueueJobItem class], [HBQueueActionStopItem class], nil];
        items = [unarchiver decodeObjectOfClasses:classes forKey:HBQueueArchiveItemsKey];
    }
    [unarchiver finishDecoding];

    // The jobs refuse to be decoded if their own version changed.
    if (items.count == 0)
    {
        if (outError)
        {
            NSString *reason = version > 0 ?
                NSLocalizedString(@"It was created by a different version of HandBrake.", @"Queue import error") :
                NSLocalizedString(@"It is not a HandBrake queue.", @"Queue import error");
            *outError = [NSError errorWithDomain:NSCocoaErrorDomain
                                            code:NSFileReadCorruptFileError
                                        userInfo:@{NSLocalizedDescriptionKey: [NSString stringWithFormat:NSLocalizedString(@"The queue “%@” could not be imported.", @"Queue import error"), url.lastPathComponent],
                                                   NSLocalizedRecoverySuggestionErrorKey: reason,
                                                   NSURLErrorKey: url}];
        }
        return nil;
    }

    for (id<HBQueueItem> item in items)
    {
        item.state = HBQueueItemStateReady;
    }

    return items;
}

@end
