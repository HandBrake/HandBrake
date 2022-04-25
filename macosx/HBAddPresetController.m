/*  HBAddPresetController.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAddPresetController.h"

#import "HBAddCategoryController.h"

#import "HBAudioDefaultsController.h"
#import "HBSubtitlesDefaultsController.h"

@interface HBAddPresetController ()

@property (nonatomic, unsafe_unretained) IBOutlet NSTextField *name;
@property (nonatomic, unsafe_unretained) IBOutlet NSTextField *desc;

@property (nonatomic, unsafe_unretained) IBOutlet NSPopUpButton *categories;

@property (nonatomic, unsafe_unretained) IBOutlet NSPopUpButton *picSettingsPopUp;
@property (nonatomic, unsafe_unretained) IBOutlet NSTextField *picWidth;
@property (nonatomic, unsafe_unretained) IBOutlet NSTextField *picHeight;
@property (nonatomic, unsafe_unretained) IBOutlet NSView *picWidthHeightBox;

@property (nonatomic, strong) HBPreset *preset;
@property (nonatomic, strong) HBMutablePreset *mutablePreset;

@property (nonatomic, strong) HBPreset *selectedCategory;

@property (nonatomic, strong) HBPresetsManager *manager;

@property (nonatomic) int width;
@property (nonatomic) int height;

@property (nonatomic) HBPictureResolutionLimitMode resolutionLimitMode;

@property (nonatomic, readwrite, strong) NSWindowController *defaultsController;


@end

@implementation HBAddPresetController

- (instancetype)initWithPreset:(HBPreset *)preset presetManager:(HBPresetsManager *)manager customWidth:(int)customWidth customHeight:(int)customHeight resolutionLimitMode:(HBPictureResolutionLimitMode)resolutionLimitMode
{
    self = [super initWithWindowNibName:@"AddPreset"];
    if (self)
    {
        NSParameterAssert(preset);
        _mutablePreset = [preset mutableCopy];
        [_mutablePreset resetBuiltInAndDefaultState];
        _manager = manager;
        _width = customWidth;
        _height = customHeight;
        _resolutionLimitMode = resolutionLimitMode;
    }
    return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];

    // Build the categories menu, and select the last used or the first
    [self buildCategoriesMenu];
    if ([self.categories indexOfItemWithTag:2] == -1)
    {
        HBPreset *category = [[HBPreset alloc] initWithCategoryName:NSLocalizedString(@"My Presets", @"Add preset window -> My Presets") builtIn:NO];
        [self.manager addPreset:category];
        NSMenuItem *item = [self buildMenuItemWithCategory:category];
        [self.categories.menu insertItem:item atIndex:2];
    }
    NSString *lastUsedCategory = [NSUserDefaults.standardUserDefaults stringForKey:@"HBLastUsedCategory"];
    if (lastUsedCategory)
    {
        [self.categories selectItemWithTitle:lastUsedCategory];
    }
    if (self.categories.selectedItem.tag != 2)
    {
        [self.categories selectItemWithTag:2];
    }
    self.selectedCategory = self.categories.selectedItem.representedObject;

    // Populate the preset picture settings popup.
    // Use [NSMenuItem tag] to store preset values for each option.

    [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"None", @"Add preset window -> picture setting")];
    [self.picSettingsPopUp.lastItem setTag:HBPictureResolutionLimitModeNone];

    [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"4320p 8K Ultra HD", @"Add preset window -> picture setting")];
    [self.picSettingsPopUp.lastItem setTag:HBPictureResolutionLimitMode8K];

    [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"2160p 4K Ultra HD", @"Add preset window -> picture setting")];
    [self.picSettingsPopUp.lastItem setTag:HBPictureResolutionLimitMode4K];

    [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"1440p 2.5K Quad HD", @"Add preset window -> picture setting")];
    [self.picSettingsPopUp.lastItem setTag:HBPictureResolutionLimitMode1440p];

    [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"1080p HD", @"Add preset window -> picture setting")];
    [self.picSettingsPopUp.lastItem setTag:HBPictureResolutionLimitMode1080p];

    [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"720p HD", @"Add preset window -> picture setting")];
    [self.picSettingsPopUp.lastItem setTag:HBPictureResolutionLimitMode720p];

    [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"576p PAL SD", @"Add preset window -> picture setting")];
    [self.picSettingsPopUp.lastItem setTag:HBPictureResolutionLimitMode576p];

    [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"480p NTSC SD", @"Add preset window -> picture setting")];
    [self.picSettingsPopUp.lastItem setTag:HBPictureResolutionLimitMode480p];

    [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"Custom", @"Add preset window -> picture setting")];
    [self.picSettingsPopUp.lastItem setTag:HBPictureResolutionLimitModeCustom];

    [self.picSettingsPopUp selectItemWithTag:self.resolutionLimitMode];

    // Initialize custom height and width settings to current values
    [self.picWidth setIntValue:self.width];
    [self.picHeight setIntValue:self.height];
    [self addPresetPicDropdownChanged:nil];
}

/**
 *  Adds the presets list to the menu.
 */
- (void)buildCategoriesMenu
{
    for (HBPreset *preset in self.manager.root.children)
    {
        if (preset.isBuiltIn == NO && preset.isLeaf == NO)
        {
            [self.categories.menu addItem:[self buildMenuItemWithCategory:preset]];
        }
    }
}

- (NSMenuItem *)buildMenuItemWithCategory:(HBPreset *)preset
{
    NSMenuItem *item = [[NSMenuItem alloc] init];
    item.title = preset.name;
    item.toolTip = preset.presetDescription;
    item.tag = 2;

    item.action = @selector(selectCategoryFromMenu:);
    item.representedObject = preset;

    return item;
}

- (IBAction)showNewCategoryWindow:(id)sender
{
    HBAddCategoryController *addCategoryController = [[HBAddCategoryController alloc] initWithPresetManager:self.manager];

    [self.window beginSheet:addCategoryController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK)
        {
            NSMenuItem *item = [self buildMenuItemWithCategory:addCategoryController.category];
            [self.categories.menu insertItem:item atIndex:2];
        }

        [self.categories selectItemWithTag:2];
        [self selectCategoryFromMenu:self.categories.selectedItem];
    }];
}

- (IBAction)selectCategoryFromMenu:(NSMenuItem *)sender
{
    self.selectedCategory = sender.representedObject;
}

- (IBAction)addPresetPicDropdownChanged:(id)sender
{
    self.picWidthHeightBox.hidden = self.picSettingsPopUp.selectedItem.tag != HBPictureResolutionLimitModeCustom;
}

- (IBAction)showAudioSettingsSheet:(id)sender
{
    HBAudioDefaults *defaults = [[HBAudioDefaults alloc] init];
    [defaults applyPreset:self.mutablePreset error:NULL];

    self.defaultsController = [[HBAudioDefaultsController alloc] initWithSettings:defaults];

    [self.window beginSheet:self.defaultsController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK)
        {
            [defaults writeToPreset:self.mutablePreset];
        }
        self.defaultsController = nil;
    }];
}

- (IBAction)showSubtitlesSettingsSheet:(id)sender
{
    HBSubtitlesDefaults *defaults = [[HBSubtitlesDefaults alloc] init];
    [defaults applyPreset:self.mutablePreset error:NULL];

    self.defaultsController = [[HBSubtitlesDefaultsController alloc] initWithSettings:defaults];

    [self.window beginSheet:self.defaultsController.window completionHandler:^(NSModalResponse returnCode) {
        if (returnCode == NSModalResponseOK)
        {
            [defaults writeToPreset:self.mutablePreset];
        }
        self.defaultsController = nil;
    }];
}

- (IBAction)add:(id)sender
{
    if (self.name.stringValue.length == 0)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"The preset name cannot be empty.", @"Add preset window -> name alert message")];
        [alert setInformativeText:NSLocalizedString(@"Please enter a name.", @"Add preset window -> name alert informative text")];
        [alert runModal];
    }
    else
    {
        HBMutablePreset *newPreset = self.mutablePreset;

        newPreset.name = self.name.stringValue;
        newPreset.presetDescription = self.desc.stringValue;

        switch (self.picSettingsPopUp.selectedTag) {
            case HBPictureResolutionLimitModeNone:
                newPreset[@"PictureWidth"] = @0;
                newPreset[@"PictureHeight"] = @0;
                break;
            case HBPictureResolutionLimitMode8K:
                newPreset[@"PictureWidth"] = @7680;
                newPreset[@"PictureHeight"] = @4320;
                break;
            case HBPictureResolutionLimitMode4K:
                newPreset[@"PictureWidth"] = @3840;
                newPreset[@"PictureHeight"] = @2160;
                break;
            case HBPictureResolutionLimitMode1440p:
                newPreset[@"PictureWidth"] = @2560;
                newPreset[@"PictureHeight"] = @1440;
                break;
            case HBPictureResolutionLimitMode1080p:
                newPreset[@"PictureWidth"] = @1920;
                newPreset[@"PictureHeight"] = @1080;
                break;
            case HBPictureResolutionLimitMode720p:
                newPreset[@"PictureWidth"] = @1280;
                newPreset[@"PictureHeight"] = @720;
                break;
            case HBPictureResolutionLimitMode576p:
                newPreset[@"PictureWidth"] = @720;
                newPreset[@"PictureHeight"] = @576;
                break;
            case HBPictureResolutionLimitMode480p:
                newPreset[@"PictureWidth"] = @720;
                newPreset[@"PictureHeight"] = @480;
                break;
            case HBPictureResolutionLimitModeCustom:
            default:
                newPreset[@"PictureWidth"] = @(self.picWidth.integerValue);
                newPreset[@"PictureHeight"] = @(self.picHeight.integerValue);
                break;
        }

        // Always use Picture Filter settings for the preset
        newPreset[@"UsesPictureFilters"] = @YES;

        [newPreset cleanUp];

        self.preset = [newPreset copy];
        [self.selectedCategory insertObject:self.preset inChildrenAtIndex:self.selectedCategory.countOfChildren];

        [NSUserDefaults.standardUserDefaults setObject:self.categories.selectedItem.title forKey:@"HBLastUsedCategory"];
        [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseOK];
    }
}

- (IBAction)cancel:(id)sender
{
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseCancel];
}

- (IBAction)openUserGuide:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[HBUtilities.documentationBaseURL URLByAppendingPathComponent:@"advanced/custom-presets.html"]];
}

@end

@interface HBAddPresetController (TouchBar) <NSTouchBarProvider, NSTouchBarDelegate>
@end

@implementation HBAddPresetController (TouchBar)

@dynamic touchBar;

static NSTouchBarItemIdentifier HBTouchBarGroup = @"fr.handbrake.buttonsGroup";
static NSTouchBarItemIdentifier HBTouchBarAdd = @"fr.handbrake.openSource";
static NSTouchBarItemIdentifier HBTouchBarCancel = @"fr.handbrake.addToQueue";

- (NSTouchBar *)makeTouchBar
{
    NSTouchBar *bar = [[NSTouchBar alloc] init];
    bar.delegate = self;

    bar.defaultItemIdentifiers = @[NSTouchBarItemIdentifierOtherItemsProxy, HBTouchBarGroup];
    bar.principalItemIdentifier = HBTouchBarGroup;

    return bar;
}

- (NSTouchBarItem *)touchBar:(NSTouchBar *)touchBar makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
{
    if ([identifier isEqualTo:HBTouchBarGroup])
    {
        NSCustomTouchBarItem *cancelItem = [[NSCustomTouchBarItem alloc] initWithIdentifier:HBTouchBarAdd];
        cancelItem.customizationLabel = NSLocalizedString(@"Cancel", @"Touch bar");
        NSButton *cancelButton = [NSButton buttonWithTitle:NSLocalizedString(@"Cancel", @"Touch bar") target:self action:@selector(cancel:)];
        [cancelButton.widthAnchor constraintGreaterThanOrEqualToConstant:160].active = YES;
        cancelItem.view = cancelButton;

        NSCustomTouchBarItem *addItem = [[NSCustomTouchBarItem alloc] initWithIdentifier:HBTouchBarCancel];
        addItem.customizationLabel = NSLocalizedString(@"Add Preset", @"Touch bar");
        NSButton *addButton = [NSButton buttonWithTitle:NSLocalizedString(@"Add Preset", @"Touch bar") target:self action:@selector(add:)];
        [addButton.widthAnchor constraintGreaterThanOrEqualToConstant:160].active = YES;
        addButton.keyEquivalent = @"\r";
        addItem.view = addButton;

        NSGroupTouchBarItem *item = [NSGroupTouchBarItem groupItemWithIdentifier:identifier items:@[cancelItem, addItem]];
        return item;
    }

    return nil;
}

@end
