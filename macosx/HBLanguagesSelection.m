/*  HBLanguagesSelection.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBLanguagesSelection.h"
#include "lang.h"

@implementation HBLang

- (instancetype)initWithLanguage:(NSString *)value iso639_2code:(NSString *)code
{
    self = [super init];
    if (self)
    {
        _language = [value retain];
        _iso639_2 = [code retain];
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
    [_language release];
}

@end

@implementation HBLanguagesSelection

- (instancetype)initWithLanguages:(NSArray *)languages
{
    self = [super init];
    if (self)
    {
        NSMutableArray *internal = [[NSMutableArray alloc] init];

        int insertedItems = 0;
        const iso639_lang_t *lang = lang_get_next(NULL);
        for (lang = lang_get_next(lang); lang != NULL; lang = lang_get_next(lang))
        {
            NSString *nativeLanguage = strlen(lang->native_name) ? @(lang->native_name) : @(lang->eng_name);

            HBLang *item = [[[HBLang alloc] initWithLanguage:nativeLanguage
                                                iso639_2code:@(lang->iso639_2)] autorelease];
            if ([languages containsObject:item.iso639_2])
            {
                item.isSelected = YES;
                [internal insertObject:item atIndex:insertedItems++];
            }
            else
            {
                [internal addObject:item];
            }
        }

        // Add the (Any) item.
        HBLang *item = [[[HBLang alloc] initWithLanguage:@"(Any)"
                                            iso639_2code:@"und"] autorelease];
        if ([languages containsObject:item.iso639_2])
        {
            item.isSelected = YES;
        }
        [internal insertObject:item atIndex:0];

        _languagesArray = internal;
    }

    return self;
}

- (NSArray *)selectedLanguages
{
    NSMutableArray *selected = [[[NSMutableArray alloc] init] autorelease];
    for (HBLang *lang in self.languagesArray)
    {
        if (lang.isSelected)
        {
            [selected addObject:lang.iso639_2];
        }
    }

    return [[selected copy] autorelease];
}

- (void)dealloc
{
    [super dealloc];
    [_languagesArray release];
}

@end

NSString *kHBLanguagesDragRowsType = @"kHBLanguagesDragRowsType";

@implementation HBLanguageArrayController

- (void)setShowSelectedOnly:(BOOL)showSelectedOnly
{
    _showSelectedOnly = showSelectedOnly;
    [self rearrangeObjects];
}

- (NSArray *)arrangeObjects:(NSArray *)objects
{
    if (!self.showSelectedOnly) {
        return [super arrangeObjects:objects];
    }

    // Returns a filtered array with only the selected items
    NSMutableArray *filteredObjects = [NSMutableArray arrayWithCapacity:[objects count]];
    for (id item in objects)
    {
        if ([[item valueForKeyPath:@"isSelected"] boolValue])
        {
            [filteredObjects addObject:item];
        }
    }

    return [super arrangeObjects:filteredObjects];
}

@end
