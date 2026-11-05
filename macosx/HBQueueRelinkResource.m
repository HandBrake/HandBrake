/*  HBQueueRelinkResource.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueRelinkResource.h"

@import HandBrakeKit;

@interface HBQueueRelinkResource ()

@property (nonatomic) NSMutableArray<HBJob *> *jobs;
@property (nonatomic) HBSecurityAccessToken *originalToken;
@property (nonatomic) HBSecurityAccessToken *replacementToken;

@end

@implementation HBQueueRelinkResource

+ (NSArray<HBQueueRelinkResource *> *)resourcesForJobs:(NSArray<HBJob *> *)jobs
{
    NSMutableArray<HBQueueRelinkResource *> *resources = [NSMutableArray array];
    NSMutableDictionary<NSURL *, HBQueueRelinkResource *> *sources = [NSMutableDictionary dictionary];
    NSMutableDictionary<NSURL *, HBQueueRelinkResource *> *destinations = [NSMutableDictionary dictionary];
    for (HBJob *job in jobs)
    {
        [job refreshSecurityScopedResources];
        for (NSNumber *source in @[@YES, @NO])
        {
            NSURL *url = source.boolValue ? job.fileURL : job.destinationFolderURL;
            NSMutableDictionary *map = source.boolValue ? sources : destinations;
            HBQueueRelinkResource *resource = url ? map[url] : nil;
            if (resource == nil)
            {
                resource = [[self alloc] initWithURL:url source:source.boolValue];
                if (url)
                {
                    map[url] = resource;
                }
                [resources addObject:resource];
            }
            [resource.jobs addObject:job];
        }
    }
    for (HBQueueRelinkResource *resource in resources)
    {
        resource->_missing = ![resource validateURL:resource.originalURL error:NULL];
    }
    return resources;
}

- (instancetype)initWithURL:(NSURL *)url source:(BOOL)source
{
    self = [super init];
    if (self)
    {
        _originalURL = url;
        _source = source;
        _jobs = [NSMutableArray array];
        _originalToken = [HBSecurityAccessToken tokenWithObject:url];
    }
    return self;
}

- (BOOL)validateURL:(NSURL *)url error:(NSError **)error
{
    NSFileManager *manager = NSFileManager.defaultManager;
    BOOL directory = NO;
    BOOL exists = url.isFileURL && [manager fileExistsAtPath:url.path isDirectory:&directory];
    BOOL accessible = exists && [manager isReadableFileAtPath:url.path];
    if (!self.isSource)
    {
        accessible = accessible && directory && [manager isWritableFileAtPath:url.path];
    }
    else if (directory)
    {
        // Single-file jobs cannot be relinked to a directory.
        for (HBJob *job in self.jobs)
        {
            accessible = accessible && !job.isStream;
        }
    }
    if (!accessible && error)
    {
        NSString *description = self.isSource ?
            NSLocalizedString(@"The source cannot be read. Choose the original media at its new location.", @"Queue relink source error") :
            NSLocalizedString(@"The destination must be a readable, writable folder.", @"Queue relink destination error");
        *error = [NSError errorWithDomain:NSCocoaErrorDomain
                                    code:exists ? NSFileReadNoPermissionError : NSFileReadNoSuchFileError
                                userInfo:@{NSLocalizedDescriptionKey: description,
                                           NSFilePathErrorKey: url.path ?: @""}];
    }
    return accessible;
}

- (BOOL)locateURL:(NSURL *)url accessToken:(HBSecurityAccessToken *)token error:(NSError **)error
{
    NSParameterAssert(url);
    NSParameterAssert(token);
    if (![self validateURL:url error:error])
    {
        return NO;
    }
    _replacementURL = url;
    self.replacementToken = token;
    return YES;
}

- (BOOL)validateReplacementWithError:(NSError **)error
{
    return self.replacementURL == nil || [self validateURL:self.replacementURL error:error];
}

- (void)apply
{
    if (self.replacementURL)
    {
        for (HBJob *job in self.jobs)
        {
            if (self.isSource)
            {
                [job relinkSourceToURL:self.replacementURL];
            }
            else
            {
                job.destinationFolderURL = self.replacementURL;
            }
        }
    }
}

@end
