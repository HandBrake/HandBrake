/*  HBPresetsViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPresetsViewController.h"
#import "HBPresetsManager.h"
#import "HBPreset.h"

// drag and drop pasteboard type
#define kHandBrakePresetPBoardType @"handBrakePresetPBoardType"

@interface HBPresetCellView : NSTableCellView
@end

@implementation HBPresetCellView

- (void)setBackgroundStyle:(NSBackgroundStyle)backgroundStyle
{
    [super setBackgroundStyle:backgroundStyle];

    // Customize the built-in preset text color
    if ([self.objectValue isBuiltIn])
    {
        if (backgroundStyle == NSBackgroundStyleDark)
        {
            self.textField.textColor = [NSColor selectedControlTextColor];
        }
        else
        {
            self.textField.textColor = [NSColor blueColor];
        }
    }
    else
    {
        self.textField.textColor = [NSColor controlTextColor];
    }
}

@end

@interface HBPresetsViewController () <NSOutlineViewDelegate>

@property (nonatomic, strong) HBPresetsManager *presets;
@property (nonatomic, unsafe_unretained) IBOutlet NSTreeController *treeController;

/**
 *  Helper var for drag & drop
 */
@property (nonatomic, strong) NSArray *dragNodesArray;

/**
 *  The status (expanded or not) of the folders.
 */
@property (nonatomic, strong) NSMutableArray *expandedNodes;

@property (unsafe_unretained) IBOutlet NSOutlineView *outlineView;

@end

@implementation HBPresetsViewController

@synthesize enabled = _enabled;

- (instancetype)initWithPresetManager:(HBPresetsManager *)presetManager
{
    self = [super initWithNibName:@"Presets" bundle:nil];
    if (self)
    {
        _presets = presetManager;
        _expandedNodes = [[NSArray arrayWithArray:[[NSUserDefaults standardUserDefaults]
                                                   objectForKey:@"HBPreviewViewExpandedStatus"]] mutableCopy];
    }
    return self;
}

- (void)loadView
{
    [super loadView];

    // drag and drop support
	[self.outlineView registerForDraggedTypes:@[kHandBrakePresetPBoardType]];

    // Re-expand the items
    [self expandNodes:[self.treeController.arrangedObjects childNodes]];

    [self.treeController setSelectionIndexPath:[self.presets indexPathOfPreset:self.presets.defaultPreset]];
}

- (BOOL)validateUserInterfaceItem:(id < NSValidatedUserInterfaceItem >)anItem
{
    SEL action = anItem.action;

    if (action == @selector(exportPreset:))
    {
        if (![[self.treeController selectedObjects] firstObject])
        {
            return NO;
        }
    }
    if (action == @selector(setDefault:))
    {
        if (![[[self.treeController selectedObjects] firstObject] isLeaf])
        {
            return NO;
        }
    }


    return YES;
}

#pragma mark -
#pragma mark Import Export Preset(s)

- (IBAction)exportPreset:(id)sender
{
    // Find the current selection, it can be a folder too.
    HBPreset *selectedPreset = [[[self.treeController selectedObjects] firstObject] copy];

    // Open a panel to let the user choose where and how to save the export file
    NSSavePanel *panel = [NSSavePanel savePanel];
    panel.title = NSLocalizedString(@"Export presets", nil);

    // We get the current file name and path from the destination field here
    NSURL *defaultExportDirectory = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop"];
    panel.directoryURL = defaultExportDirectory;
    panel.nameFieldStringValue = [NSString stringWithFormat:@"%@.json", selectedPreset.name];

    [panel beginWithCompletionHandler:^(NSInteger result)
     {
         if (result == NSFileHandlingPanelOKButton)
         {
             NSURL *presetExportDirectory = [panel.URL URLByDeletingLastPathComponent];
             [[NSUserDefaults standardUserDefaults] setURL:presetExportDirectory forKey:@"LastPresetExportDirectoryURL"];

             [selectedPreset writeToURL:panel.URL atomically:YES format:HBPresetFormatJson removeRoot:NO];
         }
     }];
}

- (IBAction)importPreset:(id)sender
{
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.title = NSLocalizedString(@"Import presets", nil);
    panel.allowsMultipleSelection = YES;
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowedFileTypes = @[@"plist", @"xml", @"json"];

    if ([[NSUserDefaults standardUserDefaults] URLForKey:@"LastPresetImportDirectoryURL"])
    {
        panel.directoryURL = [[NSUserDefaults standardUserDefaults] URLForKey:@"LastPresetImportDirectoryURL"];
    }
    else
    {
        panel.directoryURL = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop"];
    }

    [panel beginWithCompletionHandler:^(NSInteger result)
     {
         [[NSUserDefaults standardUserDefaults] setURL:panel.directoryURL forKey:@"LastPresetImportDirectoryURL"];

         if (result == NSFileHandlingPanelOKButton)
         {
             for (NSURL *url in panel.URLs)
             {
                 NSError *error;
                 HBPreset *import = [[HBPreset alloc] initWithContentsOfURL:url error:&error];

                 if (import == nil)
                 {
                     [self presentError:error];
                 }

                 for (HBPreset *child in import.children)
                 {
                     [self.presets addPreset:child];
                 }
             }
         }
     }];
}

#pragma mark - UI Methods

- (IBAction)clicked:(id)sender
{
    if (self.delegate && [[self.treeController.selectedObjects firstObject] isLeaf])
    {
        [self.delegate selectionDidChange];
    }
}

- (IBAction)addNewPreset:(id)sender
{
    if (self.delegate)
    {
        [self.delegate showAddPresetPanel:sender];
    }
}

- (IBAction)deletePreset:(id)sender
{
    if ([self.treeController canRemove])
    {
        // Save the current selection path and apply it again after the deletion
        NSIndexPath *currentSelection = [self.treeController selectionIndexPath];
        /* Alert user before deleting preset */
        NSAlert *alert = [NSAlert alertWithMessageText:NSLocalizedString(@"Are you sure you want to permanently delete the selected preset?", nil)
                                         defaultButton:NSLocalizedString(@"Delete Preset", nil)
                                       alternateButton:NSLocalizedString(@"Cancel", nil)
                                           otherButton:nil
                             informativeTextWithFormat:NSLocalizedString(@"You can't undo this action.", nil)];
        [alert setAlertStyle:NSCriticalAlertStyle];

        NSInteger status = [alert runModal];

        if (status == NSAlertDefaultReturn)
        {
            [self.presets deletePresetAtIndexPath:[self.treeController selectionIndexPath]];
        }
        [self.treeController setSelectionIndexPath:currentSelection];
    }
}

- (IBAction)insertFolder:(id)sender
{
    NSIndexPath *selectionIndexPath = [self.treeController selectionIndexPath];
    if (!selectionIndexPath || [[[self.treeController selectedObjects] firstObject] isBuiltIn])
    {
        selectionIndexPath = [NSIndexPath indexPathWithIndex:self.presets.root.children.count];
    }

    HBPreset *node = [[HBPreset alloc] initWithFolderName:@"New Folder" builtIn:NO];
    [self.treeController insertObject:node atArrangedObjectIndexPath:selectionIndexPath];
}

- (IBAction)setDefault:(id)sender
{
    HBPreset *selectedNode = [[self.treeController selectedObjects] firstObject];
    if ([[selectedNode valueForKey:@"isLeaf"] boolValue])
    {
        self.presets.defaultPreset = selectedNode;
    }
}

- (void)deselect
{
    [self.treeController setSelectionIndexPath:nil];
}

- (void)setSelection:(HBPreset *)preset
{
    NSIndexPath *idx = [self.presets indexPathOfPreset:preset];

    if (idx)
    {
        [self.treeController setSelectionIndexPath:idx];
    }
}

- (HBPreset *)selectedPreset
{
    HBPreset *selectedNode = [[self.treeController selectedObjects] firstObject];
    if ([[selectedNode valueForKey:@"isLeaf"] boolValue])
    {
        return selectedNode;
    }
    else
    {
        return self.presets.defaultPreset;
    }
}

- (IBAction)updateBuiltInPresets:(id)sender
{
    [self.presets generateBuiltInPresets];

    // Re-expand the items
    [self expandNodes:[self.treeController.arrangedObjects childNodes]];
}

#pragma mark - Expanded node persistence methods

- (void)expandNodes:(NSArray *)childNodes
{
    for (id node in childNodes)
    {
        [self expandNodes:[node childNodes]];
        if ([self.expandedNodes containsObject:@([[node representedObject] hash])])
            [self.outlineView expandItem:node expandChildren:YES];
    }
}

- (void)outlineViewItemDidExpand:(NSNotification *)notification
{
    HBPreset *node = [[[notification userInfo] valueForKey:@"NSObject"] representedObject];
    if (![self.expandedNodes containsObject:@(node.hash)])
    {
        [self.expandedNodes addObject:@(node.hash)];
        [[NSUserDefaults standardUserDefaults] setObject:self.expandedNodes forKey:@"HBPreviewViewExpandedStatus"];
    }
}

- (void)outlineViewItemDidCollapse:(NSNotification *)notification
{
    HBPreset *node = [[[notification userInfo] valueForKey:@"NSObject"] representedObject];
    [self.expandedNodes removeObject:@(node.hash)];
    [[NSUserDefaults standardUserDefaults] setObject:self.expandedNodes forKey:@"HBPreviewViewExpandedStatus"];
}

#pragma mark - Drag & Drops

/**
 *  draggingSourceOperationMaskForLocal <NSDraggingSource override>
 */
- (NSDragOperation)draggingSession:(NSDraggingSession *)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context
{
    return NSDragOperationMove;
}

/**
 *  outlineView:writeItems:toPasteboard
 */
- (BOOL)outlineView:(NSOutlineView *)ov writeItems:(NSArray *)items toPasteboard:(NSPasteboard *)pboard
{
    // Return no if we are trying to drag a built-in preset
    for (id item in items) {
        if ([[item representedObject] isBuiltIn])
            return NO;
    }

    [pboard declareTypes:@[kHandBrakePresetPBoardType] owner:self];

	// keep track of this nodes for drag feedback in "validateDrop"
	self.dragNodesArray = items;

	return YES;
}

/**
 *	outlineView:validateDrop:proposedItem:proposedChildrenIndex:
 *
 *	This method is used by NSOutlineView to determine a valid drop target.
 */
 - (NSDragOperation)outlineView:(NSOutlineView *)ov
                  validateDrop:(id <NSDraggingInfo>)info
                  proposedItem:(id)item
            proposedChildIndex:(NSInteger)index
{
	NSDragOperation result = NSDragOperationNone;

	if (!item)
	{
        if (index == 0)
        {
            // don't allow to drop on top
            result = NSDragOperationNone;
        }
        else
        {
            // no item to drop on
            result = NSDragOperationGeneric;
        }
	}
	else
	{
        if (index == -1 || [[item representedObject] isBuiltIn] || [self.dragNodesArray containsObject:item])
        {
            // don't allow dropping on a child
            result = NSDragOperationNone;
        }
        else
        {
            // drop location is a container
            result = NSDragOperationMove;
        }
	}

	return result;
}

/**
 *	handleInternalDrops:pboard:withIndexPath:
 *
 *	The user is doing an intra-app drag within the outline view.
 */
- (void)handleInternalDrops:(NSPasteboard *)pboard withIndexPath:(NSIndexPath *)indexPath
{
	// user is doing an intra app drag within the outline view:
	NSArray *newNodes = self.dragNodesArray;

	// move the items to their new place (we do this backwards, otherwise they will end up in reverse order)
	NSInteger idx;
	for (idx = ([newNodes count] - 1); idx >= 0; idx--)
	{
		[self.treeController moveNode:newNodes[idx] toIndexPath:indexPath];

        // Call manually this because the NSTreeController doesn't call
        // the KVC accessors method for the root node.
        if (indexPath.length == 1)
        {
            [self.presets performSelector:@selector(nodeDidChange:) withObject:nil];
        }
	}

	// keep the moved nodes selected
	NSMutableArray *indexPathList = [NSMutableArray array];
    for (NSUInteger i = 0; i < [newNodes count]; i++)
	{
		[indexPathList addObject:[newNodes[i] indexPath]];
	}
	[self.treeController setSelectionIndexPaths: indexPathList];
}

/**
 *	outlineView:acceptDrop:item:childIndex
 *
 *	This method is called when the mouse is released over an outline view that previously decided to allow a drop
 *	via the validateDrop method. The data source should incorporate the data from the dragging pasteboard at this time.
 *	'index' is the location to insert the data as a child of 'item', and are the values previously set in the validateDrop: method.
 *
 */
- (BOOL)outlineView:(NSOutlineView *)ov acceptDrop:(id <NSDraggingInfo>)info item:(id)targetItem childIndex:(NSInteger)index
{
	// note that "targetItem" is a NSTreeNode proxy
	//
	BOOL result = NO;

	// find the index path to insert our dropped object(s)
	NSIndexPath *indexPath;
	if (targetItem)
	{
		// drop down inside the tree node:
		// feth the index path to insert our dropped node
		indexPath = [[targetItem indexPath] indexPathByAddingIndex:index];
	}
	else
	{
		// drop at the top root level
		if (index == -1)	// drop area might be ambibuous (not at a particular location)
			indexPath = [NSIndexPath indexPathWithIndex:self.presets.root.children.count]; // drop at the end of the top level
		else
			indexPath = [NSIndexPath indexPathWithIndex:index]; // drop at a particular place at the top level
	}

	NSPasteboard *pboard = [info draggingPasteboard];	// get the pasteboard

	// check the dragging type -
	if ([pboard availableTypeFromArray:@[kHandBrakePresetPBoardType]])
	{
		// user is doing an intra-app drag within the outline view
		[self handleInternalDrops:pboard withIndexPath:indexPath];
		result = YES;
	}

	return result;
}

@end
