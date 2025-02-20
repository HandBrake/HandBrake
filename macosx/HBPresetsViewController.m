/*  HBPresetsViewController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBPresetsViewController.h"
#import "HBAddCategoryController.h"
#import "HBRenamePresetController.h"
#import "HBFilePromiseProvider.h"
#import "HBOutlineView.h"
#import "NSArray+HBAdditions.h"

@import HandBrakeKit;

// KVO Context
static void *HBPresetsViewControllerContext = &HBPresetsViewControllerContext;

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
        else if ([self.objectValue isSupported] == NO)
        {
            self.textField.textColor = [NSColor disabledControlTextColor];
        }
        else
        {
            self.textField.textColor = [NSColor systemBlueColor];
        }
    }
    else if ([self.objectValue isSupported] == NO)
    {
        self.textField.textColor = [NSColor disabledControlTextColor];
    }
    else
    {
        self.textField.textColor = [NSColor controlTextColor];
    }
}

@end

@interface HBPresetsViewController () <NSOutlineViewDelegate, NSOutlineViewDataSource, NSFilePromiseProviderDelegate>

@property (nonatomic, strong) HBPresetsManager *manager;
@property (nonatomic, readwrite) HBPreset *selectedPresetInternal;
@property (nonatomic, weak) IBOutlet NSTreeController *treeController;
@property (nonatomic, weak) IBOutlet NSSegmentedControl *actionsControl;

@property (nonatomic, strong) IBOutlet NSTextField *headerLabel;
@property (nonatomic, strong) IBOutlet NSLayoutConstraint *headerBottomConstraint;

/**
 *  Helper var for drag & drop
 */
@property (nonatomic, strong, nullable) NSArray *dragNodesArray;

/**
 *  The status (expanded or not) of the categories.
 */
@property (nonatomic, strong) NSMutableArray *expandedNodes;

@property (nonatomic, unsafe_unretained) IBOutlet HBOutlineView *outlineView;


@end

@implementation HBPresetsViewController

- (instancetype)initWithPresetManager:(HBPresetsManager *)presetManager
{
    self = [super initWithNibName:@"Presets" bundle:nil];
    if (self)
    {
        _manager = presetManager;
        _selectedPresetInternal = presetManager.defaultPreset;
        _expandedNodes = [[NSArray arrayWithArray:[NSUserDefaults.standardUserDefaults
                                                   objectForKey:@"HBPreviewViewExpandedStatus"]] mutableCopy];
        _showHeader = YES;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    // drag and drop support
	[self.outlineView registerForDraggedTypes:@[kHandBrakeInternalPBoardType, NSPasteboardTypeFileURL]];
    [self.outlineView setDraggingSourceOperationMask:NSDragOperationCopy forLocal:NO];
    [self.outlineView setDraggingSourceOperationMask:NSDragOperationMove forLocal:YES];

    // Re-expand the items
    [self expandNodes:self.treeController.arrangedObjects.childNodes];

    // Update header state
    self.showHeader = _showHeader;

    [self.treeController setSelectionIndexPath:[self.manager indexPathOfPreset:self.selectedPreset]];
    [self.treeController addObserver:self forKeyPath:@"selectedObjects" options:NSKeyValueObservingOptionNew context:HBPresetsViewControllerContext];

    [self.actionsControl setEnabled:self.enabled forSegment:0];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBPresetsViewControllerContext)
    {
        HBPreset *selectedNode = self.treeController.selectedObjects.firstObject;
        if (selectedNode && selectedNode.isLeaf && selectedNode.isSupported && selectedNode != self.selectedPresetInternal)
        {
            self.selectedPresetInternal = selectedNode;
            [self.delegate selectionDidChange];
        }
        [self.actionsControl setEnabled:selectedNode != nil forSegment:1];
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (nullable HBPreset *)targetedItem
{
    NSUInteger row = self.outlineView.targetedRowIndexes.firstIndex;
    if (row != -1)
    {
        NSTreeNode *node = [self.outlineView itemAtRow:row];
        return [node representedObject];
    }

    return nil;
}

- (NSArray<HBPreset *> *)targetedItems
{
    NSMutableArray *presets = [NSMutableArray array];
    [self.outlineView.targetedRowIndexes enumerateIndexesUsingBlock:^(NSUInteger index, BOOL *stop) {
        NSTreeNode *node = [self.outlineView itemAtRow:index];
        [presets addObject:[node representedObject]];
    }];

    return presets;
}

- (BOOL)validateUserInterfaceItem:(id < NSValidatedUserInterfaceItem >)anItem
{
    SEL action = anItem.action;

    if (action == @selector(exportPreset:) ||
        action == @selector(deletePreset:))
    {
        if (self.outlineView.targetedRowIndexes.count == 0)
        {
            return NO;
        }
    }
    if (action == @selector(renamePreset:))
    {
        HBPreset *preset = [self targetedItem];
        if (!preset || preset.isBuiltIn)
        {
            return NO;
        }
    }
    else if (action == @selector(setDefault:))
    {
        HBPreset *preset = [self targetedItem];
        if (!preset || !preset.isLeaf || !preset.isSupported)
        {
            return NO;
        }
    }

    return YES;
}

- (void)setEnabled:(BOOL)enabled
{
    _enabled = enabled;
    [self.actionsControl setEnabled:enabled forSegment:0];
}

#pragma mark - Import Export Preset(s)

- (nonnull NSString *)fileNameForPreset:(HBPreset *)preset
{
    NSString *name = preset.name == nil || preset.name.length == 0 ? @"Unnamed preset" : preset.name;
    return [name stringByAppendingPathExtension:@"json"];
}

- (nonnull NSURL *)lastPresetExportDirectoryURL
{
    NSURL *defaultExportDirectory = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop" isDirectory:YES];
    NSURL *lastPresetExportDirectoryURL = [NSUserDefaults.standardUserDefaults URLForKey:@"LastPresetExportDirectoryURL"];
    return lastPresetExportDirectoryURL ? lastPresetExportDirectoryURL : defaultExportDirectory;
}

- (void)doExportPresets:(NSArray<HBPreset *> *)presets
{
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.title = NSLocalizedString(@"Export presets", @"Export presets save panel title");
    panel.directoryURL = self.lastPresetExportDirectoryURL;
    panel.canChooseFiles = NO;
    panel.canChooseDirectories = YES;
    panel.allowsMultipleSelection = NO;
    panel.prompt = NSLocalizedString(@"Save", @"Export presets panel prompt");

    [panel beginWithCompletionHandler:^(NSInteger result)
     {
        if (result == NSModalResponseOK)
        {
            [NSUserDefaults.standardUserDefaults setURL:panel.URL forKey:@"LastPresetExportDirectoryURL"];

            for (HBPreset *preset in presets)
            {
                NSError *error = NULL;
                NSString *fileName = [self fileNameForPreset:preset];
                NSURL *url = [panel.URL URLByAppendingPathComponent:fileName isDirectory:NO];
                BOOL success = [preset writeToURL:url atomically:YES removeRoot:NO error:&error];
                if (success == NO)
                {
                    [self presentError:error];
                }
            }
            [panel.URL stopAccessingSecurityScopedResource];
        }
    }];
}

- (void)doExportPreset:(HBPreset *)preset
{
    NSSavePanel *panel = [NSSavePanel savePanel];
    panel.title = NSLocalizedString(@"Export preset", @"Export presets save panel title");
    panel.directoryURL = self.lastPresetExportDirectoryURL;
    panel.nameFieldStringValue = [self fileNameForPreset:preset];

    [panel beginWithCompletionHandler:^(NSInteger result)
     {
         if (result == NSModalResponseOK)
         {
             NSURL *presetExportDirectory = [panel.URL URLByDeletingLastPathComponent];
             [NSUserDefaults.standardUserDefaults setURL:presetExportDirectory forKey:@"LastPresetExportDirectoryURL"];

             NSError *error = NULL;
             BOOL success = [preset writeToURL:panel.URL atomically:YES removeRoot:NO error:&error];
             if (success == NO)
             {
                 [self presentError:error];
             }
         }
     }];
}

- (IBAction)exportPreset:(id)sender
{
    NSArray<HBPreset *> *selectedPresets = [self targetedItems];
    if (selectedPresets.count == 1)
    {
        [self doExportPreset:selectedPresets.firstObject];
    }
    else if (selectedPresets.count > 1)
    {
        [self doExportPresets:selectedPresets];
    }
}

- (void)doImportPreset:(NSArray<NSURL *> *)URLs atIndexPath:(nullable NSIndexPath *)indexPath
{
    if (indexPath == nil)
    {
        for (HBPreset *preset in self.manager.root.children)
        {
            if (preset.isBuiltIn == NO && preset.isLeaf == NO)
            {
                indexPath = [[self.manager indexPathOfPreset:preset] indexPathByAddingIndex:0];
            }
        }

        if (indexPath == nil)
        {
            indexPath = [NSIndexPath indexPathWithIndex:self.manager.root.countOfChildren];
        }
    }

    for (NSURL *url in URLs)
    {
        NSError *error;
        HBPreset *preset = [[HBPreset alloc] initWithContentsOfURL:url error:&error];

        if (preset)
        {
            for (HBPreset *child in preset.children)
            {
                [self.treeController insertObject:child atArrangedObjectIndexPath:indexPath];
            }
            [self.manager savePresets];
        }
        else
        {
            [self presentError:error];
        }
    }
}

- (IBAction)importPreset:(id)sender
{
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.title = NSLocalizedString(@"Import presets", @"Import preset open panel title");
    panel.allowsMultipleSelection = YES;
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowedFileTypes = @[@"json"];

    if ([NSUserDefaults.standardUserDefaults URLForKey:@"LastPresetImportDirectoryURL"])
    {
        panel.directoryURL = [NSUserDefaults.standardUserDefaults URLForKey:@"LastPresetImportDirectoryURL"];
    }
    else
    {
        panel.directoryURL = [[NSURL fileURLWithPath:NSHomeDirectory()] URLByAppendingPathComponent:@"Desktop" isDirectory:YES];
    }

    [panel beginWithCompletionHandler:^(NSInteger result)
     {
         [NSUserDefaults.standardUserDefaults setURL:panel.directoryURL forKey:@"LastPresetImportDirectoryURL"];

         if (result == NSModalResponseOK)
         {
             [self doImportPreset:panel.URLs atIndexPath:nil];
         }

         for (NSURL *url in panel.URLs)
         {
             [url stopAccessingSecurityScopedResource];
         }
     }];
}

#pragma mark - UI Methods

- (void)setShowHeader:(BOOL)showHeader
{
    _showHeader = showHeader;

    self.headerLabel.hidden = !showHeader;
    self.headerBottomConstraint.active = showHeader;
}

- (IBAction)clicked:(id)sender
{
    if (self.delegate && [self.treeController.selectedObjects.firstObject isLeaf] && [self.treeController.selectedObjects.firstObject isSupported])
    {
        [self.delegate selectionDidChange];
    }
}

- (IBAction)renamed:(id)sender
{
    if (self.delegate && [self.treeController.selectedObjects.firstObject isLeaf])
    {
        [self.delegate selectionDidChange];
    }
}

- (IBAction)segmentedActions:(NSSegmentedControl *)sender
{
    if (sender.selectedSegment == 0)
    {
        [self addNewPreset:self];
    }
    else
    {
        [self deletePreset:self];
    }
}

- (IBAction)addNewPreset:(id)sender
{
    if (self.delegate)
    {
        [self.delegate showAddPresetPanel:sender];
    }
}

- (IBAction)renamePreset:(id)sender
{
    HBPreset *preset = [self targetedItem];
    __block HBRenamePresetController *renamePresetController = [[HBRenamePresetController alloc] initWithPreset:preset
                                                                                                  presetManager:self.manager];

    NSModalResponse returnCode = [NSApp runModalForWindow:renamePresetController.window];
    if (returnCode == NSModalResponseOK)
    {
        if (self.delegate && preset.isLeaf)
        {
            [self.delegate selectionDidChange];
        }
    }
}

- (IBAction)deletePreset:(id)sender
{
    if (self.treeController.canRemove)
    {
        // Alert user before deleting preset
        NSAlert *alert = [[NSAlert alloc] init];
        alert.alertStyle = NSAlertStyleCritical;
        alert.informativeText = NSLocalizedString(@"You can't undo this action.", @"Delete preset alert -> informative text");

        NSArray<HBPreset *> *presets = [self targetedItems];

        if (presets.count > 1)
        {
            alert.messageText = NSLocalizedString(@"Are you sure you want to permanently delete the selected presets?", @"Delete preset alert -> message");
            [alert addButtonWithTitle:NSLocalizedString(@"Delete Presets", @"Delete preset alert -> first button")];
        }
        else
        {
            alert.messageText = NSLocalizedString(@"Are you sure you want to permanently delete the selected preset?", @"Delete preset alert -> message");
            [alert addButtonWithTitle:NSLocalizedString(@"Delete Preset", @"Delete preset alert -> first button")];
        }

        if (@available(macOS 11, *))
        {
            alert.buttons.lastObject.hasDestructiveAction = true;
        }
        [alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"Delete preset alert -> second button")];

        NSInteger status = [alert runModal];
        if (status == NSAlertFirstButtonReturn)
        {
            for (HBPreset *preset in presets.reverseObjectEnumerator)
            {
                NSIndexPath *indexPath = [self.manager indexPathOfPreset:preset];
                [self.manager deletePresetAtIndexPath:indexPath];
            }
            [self setSelection:self.manager.defaultPreset];
        }
    }
}

- (IBAction)insertCategory:(id)sender
{
    HBAddCategoryController *addCategoryController = [[HBAddCategoryController alloc] initWithPresetManager:self.manager];

    NSModalResponse returnCode = [NSApp runModalForWindow:addCategoryController.window];
    if (returnCode == NSModalResponseOK)
    {
        NSIndexPath *indexPath = [self.manager indexPathOfPreset:addCategoryController.category];
        [self.treeController setSelectionIndexPath:indexPath];
    }
}

- (IBAction)setDefault:(id)sender
{
    HBPreset *selectedNode = [self targetedItem];
    if (selectedNode.isLeaf)
    {
        self.manager.defaultPreset = selectedNode;
    }
}

- (void)setSelectedPreset:(HBPreset *)selectedPreset
{
    _selectedPresetInternal = selectedPreset;
    [self setSelection:selectedPreset];
}

- (HBPreset *)selectedPreset
{
    return _selectedPresetInternal;
}

- (void)setSelection:(HBPreset *)preset
{
    NSIndexPath *idx = [self.manager indexPathOfPreset:preset];

    if (idx)
    {
        [self.treeController setSelectionIndexPath:idx];
    }
}

- (IBAction)updateBuiltInPresets:(id)sender
{
    [self.manager generateBuiltInPresets];

    // Re-expand the items
    [self expandNodes:self.treeController.arrangedObjects.childNodes];
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
    HBPreset *node = [notification.userInfo[@"NSObject"] representedObject];
    if (![self.expandedNodes containsObject:@(node.hash)])
    {
        [self.expandedNodes addObject:@(node.hash)];
        [NSUserDefaults.standardUserDefaults setObject:self.expandedNodes forKey:@"HBPreviewViewExpandedStatus"];
    }
}

- (void)outlineViewItemDidCollapse:(NSNotification *)notification
{
    HBPreset *node = [notification.userInfo[@"NSObject"] representedObject];
    [self.expandedNodes removeObject:@(node.hash)];
    [NSUserDefaults.standardUserDefaults setObject:self.expandedNodes forKey:@"HBPreviewViewExpandedStatus"];
}

#pragma mark - Drag & Drops

- (void)outlineView:(NSOutlineView *)outlineView draggingSession:(NSDraggingSession *)session willBeginAtPoint:(NSPoint)screenPoint forItems:(NSArray *)draggedItems
{
    self.dragNodesArray = draggedItems;
}

- (void)outlineView:(NSOutlineView *)outlineView draggingSession:(NSDraggingSession *)session endedAtPoint:(NSPoint)screenPoint operation:(NSDragOperation)operation
{
    self.dragNodesArray = nil;
}

- (id<NSPasteboardWriting>)outlineView:(NSOutlineView *)outlineView
               pasteboardWriterForItem:(id)item
{
    HBFilePromiseProvider *filePromise = [[HBFilePromiseProvider alloc] initWithFileType:@"public.text" delegate:self];
    filePromise.userInfo = [item representedObject];
    return filePromise;
}

- (nonnull NSString *)filePromiseProvider:(nonnull NSFilePromiseProvider *)filePromiseProvider fileNameForType:(nonnull NSString *)fileType
{
    return [self fileNameForPreset:filePromiseProvider.userInfo];
}

- (void)filePromiseProvider:(nonnull NSFilePromiseProvider *)filePromiseProvider writePromiseToURL:(nonnull NSURL *)url completionHandler:(nonnull void (^)(NSError * _Nullable))completionHandler
{
    NSError *error = NULL;
    [filePromiseProvider.userInfo writeToURL:url atomically:YES removeRoot:NO error:&error];
    completionHandler(error);
}

 - (NSDragOperation)outlineView:(NSOutlineView *)ov
                  validateDrop:(id <NSDraggingInfo>)info
                  proposedItem:(id)item
            proposedChildIndex:(NSInteger)index
{
	NSDragOperation result = NSDragOperationNone;

    if (info.draggingSource == nil)
    {
        if ([[item representedObject] isBuiltIn] ||
            ([[item representedObject] isLeaf] && index == -1))
        {
            result = NSDragOperationNone;
        }
        else
        {
            result = NSDragOperationCopy;
        }
    }
    else if ([self.dragNodesArray HB_allSatisfy:^BOOL(id  _Nonnull object) { return [[object representedObject] isBuiltIn] == NO; }])
    {
        if ([[item representedObject] isBuiltIn] ||
            ([[item representedObject] isLeaf] && index == -1) ||
            [self.dragNodesArray containsObject:item])
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

- (void)handleInternalDrops:(NSPasteboard *)pboard withIndexPath:(NSIndexPath *)indexPath
{
	// user is doing an intra app drag within the outline view:
	NSArray *newNodes = self.dragNodesArray;

	// move the items to their new place (we do this backwards, otherwise they will end up in reverse order)
	NSInteger idx;
	for (idx = newNodes.count - 1; idx >= 0; idx--)
	{
		[self.treeController moveNode:newNodes[idx] toIndexPath:indexPath];

        // Call manually this because the NSTreeController doesn't call
        // the KVC accessors method for the root node.
        if (indexPath.length == 1)
        {
            [self.manager performSelector:@selector(nodeDidChange:) withObject:nil];
        }
	}

	// keep the moved nodes selected
	NSMutableArray *indexPathList = [NSMutableArray array];
    for (id node in newNodes)
    {
        [indexPathList addObject:[node indexPath]];
    }
	[self.treeController setSelectionIndexPaths:indexPathList];
}

- (void)handleExternalDrops:(NSPasteboard *)pboard withIndexPath:(NSIndexPath *)indexPath
{
    NSArray<NSURL *> *URLs = [pboard readObjectsForClasses:@[[NSURL class]] options:nil];

    [self doImportPreset:URLs atIndexPath:indexPath];

    for (NSURL *url in URLs)
    {
        [url stopAccessingSecurityScopedResource];
    }
}

- (BOOL)outlineView:(NSOutlineView *)ov acceptDrop:(id <NSDraggingInfo>)info item:(id)targetItem childIndex:(NSInteger)index
{
	BOOL result = NO;

    // note that "targetItem" is a NSTreeNode proxy
	// find the index path to insert our dropped object(s)
	NSIndexPath *indexPath;
	if (targetItem)
	{
		// drop down inside the tree node: fetch the index path to insert our dropped node
        indexPath = index == -1 ? [[targetItem indexPath] indexPathByAddingIndex:0] : [[targetItem indexPath] indexPathByAddingIndex:index];
	}
	else
	{
		// drop at the top root level
		if (index == -1)	// drop area might be ambiguous (not at a particular location)
        {
			indexPath = [NSIndexPath indexPathWithIndex:self.manager.root.children.count]; // drop at the end of the top level
        }
		else
        {
			indexPath = [NSIndexPath indexPathWithIndex:index]; // drop at a particular place at the top level
        }
	}

    NSPasteboard *pboard = info.draggingPasteboard;

	// check the dragging type
	if ([pboard availableTypeFromArray:@[kHandBrakeInternalPBoardType]])
	{
		// user is doing an intra-app drag within the outline view
		[self handleInternalDrops:pboard withIndexPath:indexPath];
		result = YES;
	}
    else if ([pboard availableTypeFromArray:@[NSPasteboardTypeFileURL]])
    {
        [self handleExternalDrops:pboard withIndexPath:indexPath];
        result = YES;
    }

	return result;
}

@end
