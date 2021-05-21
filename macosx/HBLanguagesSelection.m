/*  HBLanguagesSelection.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBLanguagesSelection.h"
#import "HBPasteboardItem.h"
#include "handbrake/lang.h"

@implementation HBLang

- (instancetype)init
{
    @throw nil;
}

- (instancetype)initWithLanguage:(NSString *)value iso639_2code:(NSString *)code
{
    self = [super init];
    if (self)
    {
        _language = value;
        _iso639_2 = code;
    }
    return self;
}

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBLang *lang = [[self class] allocWithZone:zone];

    lang->_isSelected = self.isSelected;
    lang->_language = self.language;
    lang->_iso639_2 = self.iso639_2;

    return lang;
}

- (void)setIsSelected:(BOOL)isSelected
{
    if (_isSelected != isSelected)
    {
        [[self.undo prepareWithInvocationTarget:self] setIsSelected:_isSelected];
    }
    _isSelected = isSelected;
}

- (NSString *)description
{
    return self.language;
}

@end

@implementation HBLanguagesSelection

- (instancetype)init
{
    self = [self initWithLanguages:@[]];
    return self;
}

- (instancetype)initWithLanguages:(NSArray<NSString *> *)languages
{
    self = [super init];
    if (self)
    {
        NSMutableArray<HBLang *> *internal = [[NSMutableArray alloc] init];
        NSMutableArray<HBLang *> *selected = [[NSMutableArray alloc] init];

        const iso639_lang_t *lang;
        for (lang = lang_get_any(); lang != NULL; lang = lang_get_next(lang))
        {
            NSString *nativeLanguage;

            if (!strncmp(lang->iso639_2, "any", 3))
            {
                nativeLanguage = NSLocalizedString(@"Any", @"HBLanguage -> Any language");
            }
            else if (!strncmp(lang->iso639_2, "und", 3))
            {
                nativeLanguage = NSLocalizedString(@"Unknown", @"HBLanguage -> Unknown language");
            }
            else
            {
                nativeLanguage = strlen(lang->native_name) ? @(lang->native_name) : @(lang->eng_name);
            }

            HBLang *item = [[HBLang alloc] initWithLanguage:nativeLanguage
                                                iso639_2code:@(lang->iso639_2)];
            if ([languages containsObject:item.iso639_2])
            {
                item.isSelected = YES;
                [selected addObject:item];
            }
            else
            {
                [internal addObject:item];
            }
        }

        // Insert the selected items
        // in the original order.
        [selected sortUsingComparator:^NSComparisonResult(id obj1, id obj2) {
            return [languages indexOfObject:[obj1 iso639_2]] - [languages indexOfObject:[obj2 iso639_2]];
        }];

        [internal insertObjects:selected
                      atIndexes:[NSIndexSet indexSetWithIndexesInRange:(NSMakeRange(1, selected.count))]];

        _languagesArray = internal;
    }

    return self;
}

- (NSArray *)selectedLanguages
{
    NSMutableArray *selected = [[NSMutableArray alloc] init];
    for (HBLang *lang in self.languagesArray)
    {
        if (lang.isSelected)
        {
            [selected addObject:lang.iso639_2];
        }
    }

    return [selected copy];
}

- (void)setUndo:(NSUndoManager *)undo
{
    _undo = undo;
    for (HBLang *lang in self.languagesArray)
    {
        lang.undo = undo;
    }
}


@end

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

- (void)awakeFromNib
{
	[self.tableView registerForDraggedTypes:@[tableViewIndex]];
	self.isDraggingEnabled = YES;
}

#pragma mark - NSTableView Delegate

- (nullable id <NSPasteboardWriting>)tableView:(NSTableView *)tableView pasteboardWriterForRow:(NSInteger)row
{
    if (self.isDraggingEnabled)
    {
        if (self.showSelectedOnly)
        {
            // If the show selected only filter
            // is enabled, we need to modify the rowIndexes
            // to match the position of the elements in the unfiltered array
            NSArray *filteredArray = [[self arrangedObjects] copy];
            self.showSelectedOnly = NO;
            NSArray *unfilteredArray = [self arrangedObjects];

            row = [unfilteredArray indexOfObject:filteredArray[row]];
        }
        return [[HBPasteboardItem alloc] initWithIndex:row];
    }
    else
    {
        return nil;
    }
}

- (NSDragOperation)tableView:(NSTableView *)view
                validateDrop:(id <NSDraggingInfo>)info
                 proposedRow:(NSInteger)row
       proposedDropOperation:(NSTableViewDropOperation)operation
{
    NSDragOperation dragOp = NSDragOperationNone;

    // if drag source is our own table view, it's a move or a copy
    if (info.draggingSource == view)
	{
		// At a minimum, allow move
		dragOp = NSDragOperationMove;
    }

    [view setDropRow:row dropOperation:NSTableViewDropAbove];

    return dragOp;
}

- (BOOL)tableView:(NSTableView *)view acceptDrop:(id <NSDraggingInfo>)info row:(NSInteger)row dropOperation:(NSTableViewDropOperation)operation
{
    if (info.draggingSourceOperationMask == NSDragOperationCopy)
    {
        return NO;
    }

    if (info.draggingSource == view)
    {
        NSMutableIndexSet *indexes = [[NSMutableIndexSet alloc] init];
        for (NSPasteboardItem *item in info.draggingPasteboard.pasteboardItems)
        {
            NSNumber *index = [item propertyListForType:tableViewIndex];
            [indexes addIndex:index.integerValue];
        }

        NSUInteger i = [indexes countOfIndexesInRange:NSMakeRange(0, row)];

        // Rearrange the objects.
        [self moveObjectsInArrangedObjectsFromIndexes:indexes toIndex:row];

        // Update the selection.
        row -= i;
        NSIndexSet *selectionSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(row, indexes.count)];
        [view selectRowIndexes:selectionSet byExtendingSelection:NO];

		return YES;
    }

    return NO;
}

- (void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex
{
    NSArray	*objects = [self arrangedObjects];

    NSInteger aboveInsertIndexCount = 0;
    NSInteger removeIndex;

    NSUInteger thisIndex = [indexSet lastIndex];
    while (thisIndex != NSNotFound)
	{
		if (thisIndex >= insertIndex)
		{
			removeIndex = thisIndex + aboveInsertIndexCount;
			aboveInsertIndexCount += 1;
		}
		else
		{
			removeIndex = thisIndex;
			insertIndex -= 1;
		}

		// Get the object we're moving
		id object = objects[removeIndex];

		// Move the object
		[self removeObjectAtArrangedObjectIndex:removeIndex];
		[self insertObject:object atArrangedObjectIndex:insertIndex];

		thisIndex = [indexSet indexLessThanIndex:thisIndex];
    }
}

@end
