/**
 * @file
 * Implementation of class HBPreferencesController.
 */

#import "HBPreferencesController.h"

@import HandBrakeKit.HBUtilities;

NSString * const HBShowOpenPanelAtLaunch              = @"HBShowOpenPanelAtLaunch";
NSString * const HBShowSummaryPreview                 = @"HBShowSummaryPreview";
NSString * const HBKeepPresetEdits               = @"HBKeepPresetEdits";
NSString * const HBUseSourceFolderDestination    = @"HBUseSourceFolderDestination";

NSString * const HBRecursiveScan                      = @"HBRecursiveScan";
NSString * const HBExcludedFileExtensions             = @"HBExcludedFileExtensions";
NSString * const HBLastDestinationDirectoryURL        = @"HBLastDestinationDirectoryURL";
NSString * const HBLastDestinationDirectoryBookmark   = @"HBLastDestinationDirectoryBookmark";
NSString * const HBLastSourceDirectoryURL             = @"HBLastSourceDirectoryURL";

NSString * const HBDefaultAutoNaming             = @"DefaultAutoNaming";
NSString * const HBAutoNamingFormat              = @"HBAutoNamingFormat";
NSString * const HBAutoNamingRemoveUnderscore    = @"HBAutoNamingRemoveUnderscore";
NSString * const HBAutoNamingRemovePunctuation   = @"HBAutoNamingRemovePunctuation";
NSString * const HBAutoNamingTitleCase           = @"HBAutoNamingTitleCase";
NSString * const HBAutoNamingISODateFormat       = @"HBAutoNamingISODateFormat";

NSString * const HBDefaultMpegExtension          = @"DefaultMpegExtension";

NSString * const HBCqSliderFractional            = @"HBx264CqSliderFractional";
NSString * const HBUseDvdNav                     = @"UseDvdNav";
NSString * const HBUseHardwareDecoder            = @"HBUseHardwareDecoder";
NSString * const HBAlwaysUseHardwareDecoder      = @"HBAlwaysUseHardwareDecoder";
NSString * const HBMinTitleScanSeconds           = @"MinTitleScanSeconds";
NSString * const HBPreviewsNumber                = @"PreviewsNumber";
NSString * const HBKeepDuplicateTitles           = @"HBKeepDuplicateTitles";

NSString * const HBLoggingLevel                  = @"LoggingLevel";
NSString * const HBEncodeLogLocation             = @"EncodeLogLocation";
NSString * const HBClearOldLogs                  = @"HBClearOldLogs";

NSString * const HBQueuePauseIfLowSpace          = @"HBQueuePauseIfLowSpace";
NSString * const HBQueueMinFreeSpace             = @"HBQueueMinFreeSpace";
NSString * const HBQueuePauseOnBatteryPower      = @"HBQueuePauseOnBatteryPower";

NSString * const HBQueueAutoClearCompletedItems  = @"HBQueueAutoClearCompletedItems";
NSString * const HBQueueAutoClearCompletedItemsAtLaunch = @"HBQueueAutoClearCompletedItemsAtLaunch";

NSString * const HBQueueWorkerCounts             = @"HBQueueWorkerCounts";

NSString * const HBResetWhenDoneOnLaunch         = @"HBResetWhenDoneOnLaunch";
NSString * const HBQueueDoneAction               = @"HBQueueDoneAction";
NSString * const HBQueueNotificationWhenDone     = @"HBQueueNotificationWhenDone";
NSString * const HBQueueNotificationWhenJobDone  = @"HBQueueNotificationWhenJobDone";
NSString * const HBQueueNotificationPlaySound    = @"HBQueueNotificationPlaySound";
NSString * const HBSendToAppEnabled              = @"HBSendToAppEnabled";
NSString * const HBSendToApp                     = @"HBSendToApp";

#define TOOLBAR_GENERAL         @"TOOLBAR_GENERAL"
#define TOOLBAR_OUTPUT_NAME     @"TOOLBAR_OUTPUT_NAME"
#define TOOLBAR_QUEUE           @"TOOLBAR_QUEUE"
#define TOOLBAR_ADVANCED        @"TOOLBAR_ADVANCED"

// KVO Context
static void *HBPreferencesControllerContext = &HBPreferencesControllerContext;

@protocol HBFileExtensionDelegate
- (void)extensionDidChange;
@end

@interface HBFileExtension : NSObject

@property (nonatomic) NSString *extension;
@property (nonatomic, readonly, weak) id<HBFileExtensionDelegate> delegate;

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithDelegate:(id<HBFileExtensionDelegate>)delegate;
- (instancetype)initWithExtension:(NSString *)extension delegate:(id<HBFileExtensionDelegate>)delegate;

@end

@implementation HBFileExtension

- (instancetype)initWithDelegate:(id<HBFileExtensionDelegate>)delegate
{
    self = [super self];
    if (self)
    {
        _extension = NSLocalizedString(@"extension", "Default name for a newly added excluded file extension");
        _delegate = delegate;
    }
    return self;
}

- (instancetype)initWithExtension:(NSString *)extension delegate:(id<HBFileExtensionDelegate>)delegate
{
    self = [super self];
    if (self)
    {
        _extension = extension;
        _delegate = delegate;
    }
    return self;
}

- (void)setExtension:(NSString *)extension
{
    if ([extension isEqualToString:_extension] == NO)
    {
        _extension = extension;
        [self.delegate extensionDidChange];
    }
}

- (BOOL)validateExtension:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    // Return an error is the name is empty
    if (![*ioValue length])
    {
        if (outError)
        {
            *outError = [[NSError alloc] initWithDomain:@"HBErrorDomain"
                                                   code:0
                                               userInfo:@{NSLocalizedDescriptionKey:@"The file extension cannot be empty.",
                                                          NSLocalizedRecoverySuggestionErrorKey:@"Please enter a file extension."}];
        }
        return NO;
    }

    return YES;
}

@end

/**
 * This class controls the preferences window of HandBrake. Default values for
 * all preferences and user defaults are specified in class method
 * @c registerUserDefaults. The preferences window is loaded from
 * Preferences.nib file when HBPreferencesController is initialized.
 *
 * All preferences are bound to user defaults in Interface Builder, therefore
 * no getter/setter code is needed in this file (unless more complicated
 * preference settings are added that cannot be handled with Cocoa bindings).
 */

@interface HBPreferencesController () <NSTokenFieldDelegate, NSToolbarDelegate, HBFileExtensionDelegate>

@property (nonatomic, weak) IBOutlet NSView *generalView;
@property (nonatomic, weak) IBOutlet NSView *fileNameView;
@property (nonatomic, weak) IBOutlet NSView *queueView;
@property (nonatomic, weak) IBOutlet NSView *advancedView;

@property (nonatomic, weak) IBOutlet NSTokenField *formatTokenField;
@property (nonatomic, weak) IBOutlet NSTokenField *builtInTokenField;
@property (nonatomic, readonly, strong) NSArray *buildInFormatTokens;
@property (nonatomic, strong) NSArray *matches;

@property (nonatomic, weak) IBOutlet NSSegmentedControl *excludedExtensionsControl;
@property (nonatomic, weak) IBOutlet NSArrayController *excludedExtensionsController;
@property (nonatomic, weak) IBOutlet NSTableView *excludedExtensionsTableView;
@property (nonatomic) NSMutableArray<HBFileExtension *> *excludedExtensions;

@property (nonatomic) BOOL hardwareDecodersCheckboxesEnabled;

@property (class, nonatomic, getter=areHardwareDecoderSupported) BOOL hardwareDecoderSupported;

@end

@implementation HBPreferencesController

static BOOL _hardwareDecoderSupported = NO;

+ (BOOL)areHardwareDecoderSupported
{
    return _hardwareDecoderSupported;
}

+ (void)setHardwareDecoderSupported:(BOOL)hardwareDecoderSupported
{
    _hardwareDecoderSupported = hardwareDecoderSupported;
}

/// Registers default values to user defaults. This is called immediately
/// when HandBrake starts, from [HBAppDelegate init].
+ (void)registerUserDefaults
{
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    NSData *moviesData = [NSKeyedArchiver archivedDataWithRootObject:HBUtilities.defaultDestinationFolderURL requiringSecureCoding:YES error:NULL];

    if (@available(macOS 13, *))
    {
        HBPreferencesController.hardwareDecoderSupported = YES;
    }
    else
    {
        HBPreferencesController.hardwareDecoderSupported = NO;
    }

    [defaults registerDefaults:@{
        HBShowOpenPanelAtLaunch:            @YES,
        HBShowSummaryPreview:               @YES,
        HBKeepPresetEdits:                  @YES,
        HBDefaultMpegExtension:             @".mp4",
        HBUseDvdNav:                        @YES,
        HBUseHardwareDecoder:               @NO,
        HBAlwaysUseHardwareDecoder:         @NO,
        HBRecursiveScan:                    @NO,
        HBExcludedFileExtensions:           @[@"jpg", @"png", @"srt", @"ssa", @"ass", @"txt"],
        HBLastDestinationDirectoryURL:      moviesData,
        HBLastSourceDirectoryURL:           moviesData,
        HBUseSourceFolderDestination:       @NO,
        HBDefaultAutoNaming:                @NO,
        HBAutoNamingFormat:                 @[@"{Source}", @" ", @"{Title}"],
        HBAutoNamingISODateFormat:          @NO,
        HBResetWhenDoneOnLaunch:            @NO,
        HBLoggingLevel:                     @1,
        HBClearOldLogs:                     @YES,
        HBEncodeLogLocation:                @NO,
        HBMinTitleScanSeconds:              @10,
        HBPreviewsNumber:                   @10,
        HBCqSliderFractional:               @2,
        HBQueuePauseIfLowSpace:             @YES,
        HBQueueMinFreeSpace:                @"2",
        HBQueuePauseOnBatteryPower:         @NO,
        HBQueueAutoClearCompletedItems:     @NO,
        HBQueueAutoClearCompletedItemsAtLaunch: @YES,
        HBQueueWorkerCounts:                @1,
        HBQueueDoneAction:                  @0,
        HBQueueNotificationWhenDone:        @YES,
        HBQueueNotificationWhenJobDone:     @NO,
        HBQueueNotificationPlaySound:       @NO
    }];

    if (HBPreferencesController.areHardwareDecoderSupported == NO && [defaults boolForKey:HBUseHardwareDecoder] == YES)
    {
        [defaults setBool:NO forKey:HBUseHardwareDecoder];
    }

    // Overwrite the update check interval because previous versions
    // could be set to a daily check.
    NSUInteger week = 60 * 60 * 24 * 7;
    [defaults setObject:@(week) forKey:@"SUScheduledCheckInterval"];
}

- (instancetype)init
{
    self = [super initWithWindowNibName:@"Preferences"];
    return self;
}

- (void)showWindow:(id)sender
{
    if (!self.window.isVisible)
    {
        [self.window center];
    }

    [super showWindow:sender];
}

- (void)windowDidLoad
{
    if (@available (macOS 11, *))
    {
        self.window.toolbarStyle = NSWindowToolbarStylePreference;
    }

    NSToolbar *toolbar = [[NSToolbar alloc] initWithIdentifier:@"Preferences Toolbar"];
    toolbar.delegate = self;
    toolbar.allowsUserCustomization = NO;
    toolbar.displayMode = NSToolbarDisplayModeIconAndLabel;
    toolbar.sizeMode = NSToolbarSizeModeRegular;
    self.window.toolbar = toolbar;

    self.hardwareDecodersCheckboxesEnabled = HBPreferencesController.areHardwareDecoderSupported;

    // Format token field initialization
    [self.formatTokenField setTokenizingCharacterSet:[NSCharacterSet characterSetWithCharactersInString:@"%%"]];
    [self.formatTokenField setCompletionDelay:0.2];

    _buildInFormatTokens = @[@"{Source}", @"{Title}", @"{Chapters}", @"{Preset}",
                             @"{Width}", @"{Height}", @"{Codec}",
                             @"{Encoder}", @"{Bit-Depth}", @"{Quality/Bitrate}", @"{Quality-Type}",
                             @"{Date}", @"{Time}", @"{Creation-Date}", @"{Creation-Time}",
                             @"{Modification-Date}", @"{Modification-Time}"];
    [self.builtInTokenField setTokenizingCharacterSet:[NSCharacterSet characterSetWithCharactersInString:@"%%"]];
    [self.builtInTokenField setStringValue:[self.buildInFormatTokens componentsJoinedByString:@"%%"]];

    // Excluded file extension initialization
    self.excludedExtensions = [[NSMutableArray alloc] init];
    for (NSString *extension in [NSUserDefaults.standardUserDefaults arrayForKey:HBExcludedFileExtensions])
    {
        if ([extension isKindOfClass:[NSString class]])
        {
            [self.excludedExtensions addObject:[[HBFileExtension alloc] initWithExtension:extension delegate:self]];
        }
    }
    self.excludedExtensionsController.content = self.excludedExtensions;

    [self.excludedExtensionsController addObserver:self
                                        forKeyPath:@"selectedObjects"
                                           options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial
                                           context:HBPreferencesControllerContext];
    [self.excludedExtensionsController addObserver:self
                                        forKeyPath:@"arrangedObjects"
                                           options:NSKeyValueObservingOptionNew
                                           context:HBPreferencesControllerContext];

    // Set default tab
    toolbar.selectedItemIdentifier = TOOLBAR_GENERAL;
    [self setPrefView:nil];
}

- (nullable NSToolbarItem *)toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSToolbarItemIdentifier)itemIdentifier willBeInsertedIntoToolbar:(BOOL)flag
{
    NSImage *image = nil;

    if ([itemIdentifier isEqualToString:TOOLBAR_GENERAL])
    {
        if (@available (macOS 11, *))
        {
            image = [NSImage imageWithSystemSymbolName:@"gearshape" accessibilityDescription:@"General preferences"];
        }
        else
        {
            image = [NSImage imageNamed:NSImageNamePreferencesGeneral];
        }
        return [self toolbarItemWithIdentifier:itemIdentifier
                                         label:NSLocalizedString(@"General", @"Preferences General Toolbar Item")
                                         image:image];
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_OUTPUT_NAME])
    {
        if (@available (macOS 11, *))
        {
            image = [NSImage imageWithSystemSymbolName:@"character.textbox" accessibilityDescription:@"File output preferences"];
        }
        else
        {
            image = [NSImage imageNamed:NSImageNameAdvanced];
        }
        return [self toolbarItemWithIdentifier:itemIdentifier
                                         label:NSLocalizedString(@"File name", @"Preferences Output File Name Toolbar Item")
                                         image:image];
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_QUEUE])
    {
        if (@available (macOS 13, *))
        {
            image = [NSImage imageWithSystemSymbolName:@"photo.stack" accessibilityDescription:@"Queue preferences"];
        }
        else if (@available (macOS 11, *))
        {
            image = [NSImage imageNamed:@"photo.stack"];
        }
        else
        {
            image = [NSImage imageNamed:@"showqueue"];
        }
        return [self toolbarItemWithIdentifier:itemIdentifier
                                         label:NSLocalizedString(@"Queue", @"Preferences Queue Toolbar Item")
                                         image:image];
    }
    else if ([itemIdentifier isEqualToString:TOOLBAR_ADVANCED])
    {
        if (@available (macOS 11, *))
        {
            image = [NSImage imageWithSystemSymbolName:@"gearshape.2" accessibilityDescription:@"General preferences"];
        }
        else
        {
            image = [NSImage imageNamed:NSImageNameAdvanced];
        }
        return [self toolbarItemWithIdentifier:itemIdentifier
                                         label:NSLocalizedString(@"Advanced", @"Preferences Advanced Toolbar Item")
                                         image:image];
    }

    return nil;
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarSelectableItemIdentifiers:(NSToolbar *)toolbar
{
    return [self toolbarDefaultItemIdentifiers:toolbar];
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar
{
    return [self toolbarAllowedItemIdentifiers:toolbar];
}

- (NSArray<NSToolbarItemIdentifier> *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar
{
    return @[TOOLBAR_GENERAL, TOOLBAR_OUTPUT_NAME, TOOLBAR_QUEUE, TOOLBAR_ADVANCED];
}

- (IBAction)browseSendToApp:(id)sender
{
    NSUserDefaults *ud = NSUserDefaults.standardUserDefaults;
    NSOpenPanel *panel = [NSOpenPanel openPanel];

    [panel setAllowsMultipleSelection:NO];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowedFileTypes:@[@"app"]];
    [panel setMessage:NSLocalizedString(@"Select the desired external application", @"Preferences -> send to app destination open panel")];

    NSURL *sendToAppDirectory;
	if ([ud stringForKey:@"HBLastSendToAppDirectory"])
	{
		sendToAppDirectory = [ud URLForKey:@"HBLastSendToAppDirectory"];
	}
	else
	{
		sendToAppDirectory = [NSURL fileURLWithPath:@"/Applications" isDirectory:YES];
	}
    [panel setDirectoryURL:sendToAppDirectory];

    [panel beginSheetModalForWindow:[self window] completionHandler:^(NSInteger result)
     {
        if (result == NSModalResponseOK)
        {
            NSURL *sendToAppURL = panel.URL;
            NSURL *sendToAppDirectoryURL = sendToAppURL.URLByDeletingLastPathComponent;
            [ud setURL:sendToAppDirectoryURL forKey:@"HBLastSendToAppDirectory"];

            // We set the name of the app to send to in the display field
            NSString *sendToAppName = sendToAppURL.lastPathComponent.stringByDeletingPathExtension;
            [ud setObject:sendToAppName forKey:HBSendToApp];

            [sendToAppURL stopAccessingSecurityScopedResource];
        }
    }];
}

#pragma mark - Format Token Field Delegate

- (NSString *)tokenField:(NSTokenField *)tokenField displayStringForRepresentedObject:(id)representedObject
{
    if ([representedObject rangeOfString: @"{"].location == 0 && [representedObject length] > 1)
    {
        return [self localizedStringForToken:representedObject];
    }

    return representedObject;
}

- (NSString *)localizedStringForToken:(NSString *)tokenString
{
    if ([tokenString isEqualToString:@"{Source}"])
    {
        return NSLocalizedString(@"Source", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Title}"])
    {
        return NSLocalizedString(@"Title", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Chapters}"])
    {
        return NSLocalizedString(@"Chapters", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Preset}"])
    {
        return NSLocalizedString(@"Preset", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Width}"])
    {
        return NSLocalizedString(@"Width", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Height}"])
    {
        return NSLocalizedString(@"Height", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Codec}"])
    {
        return NSLocalizedString(@"Codec", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Encoder}"])
    {
        return NSLocalizedString(@"Encoder", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Bit-Depth}"])
    {
        return NSLocalizedString(@"Bit Depth", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Quality/Bitrate}"])
    {
        return NSLocalizedString(@"Quality/Bitrate", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Quality-Type}"])
    {
        return NSLocalizedString(@"Quality Type", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Date}"])
    {
        return NSLocalizedString(@"Date", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Time}"])
    {
        return NSLocalizedString(@"Time", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Creation-Date}"])
    {
        return NSLocalizedString(@"Creation-Date", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Creation-Time}"])
    {
        return NSLocalizedString(@"Creation-Time", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Modification-Date}"])
    {
        return NSLocalizedString(@"Modification-Date", "Preferences -> Output Name Token");
    }
    else if ([tokenString isEqualToString:@"{Modification-Time}"])
    {
        return NSLocalizedString(@"Modification-Time", "Preferences -> Output Name Token");
    }

    return tokenString;
}

- (NSTokenStyle)tokenField:(NSTokenField *)tokenField styleForRepresentedObject:(id)representedObject
{
    if ([representedObject rangeOfString: @"{"].location == 0)
    {
        return NSTokenStyleRounded;
    }
    else
    {
        return NSTokenStyleNone;
    }
}

- (id)tokenField:(NSTokenField *)tokenField representedObjectForEditingString:(NSString *)editingString
{
    return editingString;
}

- (NSArray *)tokenField:(NSTokenField *)tokenField completionsForSubstring:(NSString *)substring indexOfToken:(NSInteger)tokenIndex
    indexOfSelectedItem:(NSInteger *)selectedIndex
{
    self.matches = [self.buildInFormatTokens filteredArrayUsingPredicate:
                    [NSPredicate predicateWithFormat:@"SELF beginswith[cd] %@", substring]];
    return self.matches;
}

- (NSString *)tokenField:(NSTokenField *)tokenField editingStringForRepresentedObject:(id)representedObject
{
    if ([representedObject rangeOfString: @"{"].location == 0)
    {
        return [NSString stringWithFormat:@"%%%@%%", representedObject];
    }
    else
    {
        return representedObject;
    }
}

- (NSArray *)tokenField:(NSTokenField *)tokenField shouldAddObjects:(NSArray *)tokens atIndex:(NSUInteger)index
{
    return tokens;
}

- (BOOL)tokenField:(NSTokenField *)tokenField writeRepresentedObjects:(NSArray *)objects toPasteboard:(NSPasteboard *)pboard
{
    NSString *format = [objects componentsJoinedByString:@"%%"];
    [pboard setString:format forType:NSPasteboardTypeString];

    return YES;
}

#pragma mark - Excluded file extensions

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (context == HBPreferencesControllerContext)
    {
        if ([keyPath isEqualToString:@"selectedObjects"])
        {
            BOOL selected = self.excludedExtensionsController.selectedObjects.count > 0;
            [self.excludedExtensionsControl setEnabled:selected forSegment:1];
        }
        else if ([keyPath isEqualToString:@"arrangedObjects"])
        {
            [self extensionDidChange];
        }
    }
    else
    {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}

- (void)extensionDidChange
{
    NSMutableArray<NSString *> *extensions = [NSMutableArray array];
    for (HBFileExtension *extension in self.excludedExtensions)
    {
        [extensions addObject:extension.extension];
    }
    [NSUserDefaults.standardUserDefaults setObject:extensions forKey:HBExcludedFileExtensions];
}

- (IBAction)extensionDoubleClickAction:(NSTableView *)sender
{
    if (sender.clickedRow > -1)
    {
        [sender editColumn:0 row:sender.clickedRow withEvent:nil select:YES];
    }
}

- (IBAction)addFileExtension:(id)sender
{
    if ([sender selectedSegment])
    {
        [self.excludedExtensionsController removeObjectsAtArrangedObjectIndexes:self.excludedExtensionsController.selectionIndexes];
    }
    else
    {
        HBFileExtension *extension = [[HBFileExtension alloc] initWithDelegate:self];
        [self.excludedExtensionsController addObject:extension];

        NSInteger row = [self.excludedExtensionsController.arrangedObjects count] - 1;
        if (row > -1)
        {
            [self.excludedExtensionsTableView selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
            [self.excludedExtensionsTableView editColumn:0 row:row withEvent:nil select:YES];
        }
    }
}

#pragma mark - Private methods

- (void)setPrefView:(id)sender
{
    NSView *view = self.generalView;
    if (sender)
    {
        NSString *identifier = [sender itemIdentifier];
        if ([identifier isEqualToString:TOOLBAR_OUTPUT_NAME])
        {
            view = self.fileNameView;
        }
        else if ([identifier isEqualToString:TOOLBAR_QUEUE])
        {
            view = self.queueView;
        }
        else if ([identifier isEqualToString:TOOLBAR_ADVANCED])
        {
            view = self.advancedView;
        }
    }

    NSWindow *window =  self.window;
    if (window.contentView == view)
    {
        return;
    }

    window.contentView = view;

    if (window.isVisible)
    {
        view.hidden = YES;

        [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context) {
            context.allowsImplicitAnimation = YES;
            [window layoutIfNeeded];
        } completionHandler:^{
            view.hidden = NO;
        }];
    }

    // set title label
    if (sender)
    {
        window.title = [sender label];
    }
    else
    {
        NSToolbar *toolbar = window.toolbar;
        NSString *itemIdentifier = toolbar.selectedItemIdentifier;
        for (NSToolbarItem *item in toolbar.items)
        {
            if ([item.itemIdentifier isEqualToString:itemIdentifier])
            {
                window.title = item.label;
                break;
            }
        }
    }
}

- (NSToolbarItem *)toolbarItemWithIdentifier:(NSString *)identifier
                                       label:(NSString *)label
                                       image:(NSImage *)image
{
    NSToolbarItem *item = [[NSToolbarItem alloc] initWithItemIdentifier:identifier];
    item.label = label;
    item.image = image;
    item.action = @selector(setPrefView:);
    item.autovalidates = NO;
    return item;
}

@end
