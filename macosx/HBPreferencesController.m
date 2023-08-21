/**
 * @file
 * Implementation of class HBPreferencesController.
 */

#import "HBPreferencesController.h"

@import HandBrakeKit.HBUtilities;

NSString * const HBShowOpenPanelAtLaunch              = @"HBShowOpenPanelAtLaunch";
NSString * const HBShowSummaryPreview                 = @"HBShowSummaryPreview";

NSString * const HBRecursiveScan                      = @"HBRecursiveScan";
NSString * const HBLastDestinationDirectoryURL        = @"HBLastDestinationDirectoryURL";
NSString * const HBLastDestinationDirectoryBookmark   = @"HBLastDestinationDirectoryBookmark";
NSString * const HBLastSourceDirectoryURL             = @"HBLastSourceDirectoryURL";

NSString * const HBDefaultMpegExtension          = @"DefaultMpegExtension";

NSString * const HBAlertWhenDone                 = @"HBAlertWhenDone";
NSString * const HBResetWhenDoneOnLaunch         = @"HBResetWhenDoneOnLaunch";
NSString * const HBAlertWhenDoneSound            = @"HBAlertWhenDoneSound";
NSString * const HBSendToAppEnabled              = @"HBSendToAppEnabled";
NSString * const HBSendToApp                     = @"HBSendToApp";

NSString * const HBUseSourceFolderDestination    = @"HBUseSourceFolderDestination";

NSString * const HBDefaultAutoNaming             = @"DefaultAutoNaming";
NSString * const HBAutoNamingFormat              = @"HBAutoNamingFormat";
NSString * const HBAutoNamingRemoveUnderscore    = @"HBAutoNamingRemoveUnderscore";
NSString * const HBAutoNamingRemovePunctuation   = @"HBAutoNamingRemovePunctuation";
NSString * const HBAutoNamingTitleCase           = @"HBAutoNamingTitleCase";
NSString * const HBAutoNamingISODateFormat       = @"HBAutoNamingISODateFormat";

NSString * const HBCqSliderFractional            = @"HBx264CqSliderFractional";
NSString * const HBUseDvdNav                     = @"UseDvdNav";
NSString * const HBUseHardwareDecoder            = @"HBUseHardwareDecoder";
NSString * const HBAlwaysUseHardwareDecoder      = @"HBAlwaysUseHardwareDecoder";
NSString * const HBMinTitleScanSeconds           = @"MinTitleScanSeconds";
NSString * const HBPreviewsNumber                = @"PreviewsNumber";

NSString * const HBLoggingLevel                  = @"LoggingLevel";
NSString * const HBEncodeLogLocation             = @"EncodeLogLocation";
NSString * const HBClearOldLogs                  = @"HBClearOldLogs";

NSString * const HBQueuePauseIfLowSpace          = @"HBQueuePauseIfLowSpace";
NSString * const HBQueueMinFreeSpace             = @"HBQueueMinFreeSpace";
NSString * const HBQueueAutoClearCompletedItems  = @"HBQueueAutoClearCompletedItems";
NSString * const HBQueueWorkerCounts             = @"HBQueueWorkerCounts";

NSString * const HBKeepPresetEdits               = @"HBKeepPresetEdits";

#define TOOLBAR_GENERAL     @"TOOLBAR_GENERAL"
#define TOOLBAR_ADVANCED    @"TOOLBAR_ADVANCED"

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

@interface HBPreferencesController () <NSTokenFieldDelegate, NSToolbarDelegate>
{
    IBOutlet NSView         * fGeneralView, * fAdvancedView;
    IBOutlet NSTextField    * fSendEncodeToAppField;
}

@property (nonatomic, unsafe_unretained) IBOutlet NSTokenField *formatTokenField;
@property (nonatomic, unsafe_unretained) IBOutlet NSTokenField *builtInTokenField;
@property (nonatomic, readonly, strong) NSArray *buildInFormatTokens;
@property (nonatomic, strong) NSArray *matches;

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

/**
 * +[HBPreferencesController registerUserDefaults]
 *
 * Registers default values to user defaults. This is called immediately
 * when HandBrake starts, from [HBController init].
 */
+ (void)registerUserDefaults
{
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;

    NSURL *moviesURL = HBUtilities.defaultDestinationFolderURL;

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
        HBDefaultMpegExtension:             @".mp4",
        HBUseDvdNav:                        @YES,
        HBUseHardwareDecoder:               @NO,
        HBAlwaysUseHardwareDecoder:         @NO,
        HBRecursiveScan:                    @NO,
        HBLastDestinationDirectoryURL:      [NSKeyedArchiver archivedDataWithRootObject:moviesURL],
        HBLastSourceDirectoryURL:           [NSKeyedArchiver archivedDataWithRootObject:moviesURL],
        HBUseSourceFolderDestination:       @NO,
        HBDefaultAutoNaming:                @NO,
        HBAutoNamingFormat:                 @[@"{Source}", @" ", @"{Title}"],
        HBAutoNamingISODateFormat:          @NO,
        HBAlertWhenDone:                    @(HBDoneActionNotification),
        HBResetWhenDoneOnLaunch:            @NO,
        HBAlertWhenDoneSound:               @YES,
        HBLoggingLevel:                     @1,
        HBClearOldLogs:                     @YES,
        HBEncodeLogLocation:                @NO,
        HBMinTitleScanSeconds:              @10,
        HBPreviewsNumber:                   @10,
        HBCqSliderFractional:               @2,
        HBQueuePauseIfLowSpace:             @YES,
        HBQueueMinFreeSpace:                @"2",
        HBQueueAutoClearCompletedItems:     @NO,
        HBQueueWorkerCounts:                @1,
        HBKeepPresetEdits:                  @YES
    }];

    if (HBPreferencesController.areHardwareDecoderSupported == NO &&
        [defaults boolForKey:HBUseHardwareDecoder] == YES)
    {
        [defaults setBool:NO forKey:HBUseHardwareDecoder];
    }

    // Overwrite the update check interval because previous versions
    // could be set to a daily check.
    NSUInteger week = 60 * 60 * 24 * 7;
    [defaults setObject:@(week) forKey:@"SUScheduledCheckInterval"];
}

/**
 * -[HBPreferencesController init]
 *
 * Initializes the preferences controller by loading Preferences.nib file.
 *
 */
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

/**
 *
 * Called after all the outlets in the nib file have been attached. Sets up the
 * toolbar and shows the "General" pane.
 *
 */
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

    toolbar.selectedItemIdentifier = TOOLBAR_GENERAL;
    [self setPrefView:nil];
}

- (NSToolbarItem *)toolbar: (NSToolbar *)toolbar
     itemForItemIdentifier: (NSString *)ident
 willBeInsertedIntoToolbar: (BOOL)flag
{
    if ([ident isEqualToString:TOOLBAR_GENERAL])
    {
        NSImage *image = [NSImage imageNamed:NSImageNamePreferencesGeneral];
        if (@available (macOS 11, *))
        {
            image = [NSImage imageWithSystemSymbolName:@"gearshape" accessibilityDescription:@"General preferences"];
        }
        return [self toolbarItemWithIdentifier:ident
                                         label:NSLocalizedString(@"General", @"Preferences General Toolbar Item")
                                         image:image];
    }
    else if ([ident isEqualToString:TOOLBAR_ADVANCED])
    {
        NSImage *image = [NSImage imageNamed:NSImageNameAdvanced];
        if (@available (macOS 11, *))
        {
            image = [NSImage imageWithSystemSymbolName:@"gearshape.2" accessibilityDescription:@"General preferences"];
        }
        return [self toolbarItemWithIdentifier:ident
                                         label:NSLocalizedString(@"Advanced", @"Preferences Advanced Toolbar Item")
                                         image:image];
    }

    return nil;
}

- (NSArray *) toolbarSelectableItemIdentifiers: (NSToolbar *) toolbar
{
    return [self toolbarDefaultItemIdentifiers: toolbar];
}

- (NSArray *) toolbarDefaultItemIdentifiers: (NSToolbar *) toolbar
{
    return [self toolbarAllowedItemIdentifiers: toolbar];
}

- (NSArray *) toolbarAllowedItemIdentifiers: (NSToolbar *) toolbar
{
    return @[TOOLBAR_GENERAL, TOOLBAR_ADVANCED];
}

/* Manage the send encode to xxx.app windows and field */
/*Opens the app browse window*/
- (IBAction) browseSendToApp: (id) sender
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
            [self->fSendEncodeToAppField setStringValue:sendToAppName];

            [ud setObject:self->fSendEncodeToAppField.stringValue forKey:HBSendToApp];

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

#pragma mark - Private methods

- (void) setPrefView: (id) sender
{
    NSView *view = fGeneralView;
    if (sender)
    {
        NSString *identifier = [sender itemIdentifier];
        if([identifier isEqualToString:TOOLBAR_ADVANCED])
        {
            view = fAdvancedView;
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

/**
 * -[HBPreferencesController(Private) toolbarItemWithIdentifier:label:image:]
 *
 * Shared code for creating the NSToolbarItems for the Preferences toolbar.
 *
 */
- (NSToolbarItem *)toolbarItemWithIdentifier: (NSString *)identifier
                                       label: (NSString *)label
                                       image: (NSImage *)image
{
    NSToolbarItem *item = [[NSToolbarItem alloc] initWithItemIdentifier:identifier];
    [item setLabel:label];
    [item setImage:image];
    [item setAction:@selector(setPrefView:)];
    [item setAutovalidates:NO];
    return item;
}

@end
