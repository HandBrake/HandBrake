/**
 * @file
 * Implementation of class HBPreferencesController.
 */

#import "HBPreferencesController.h"
#import "HBLanguagesSelection.h"

#define TOOLBAR_GENERAL     @"TOOLBAR_GENERAL"
#define TOOLBAR_AUDIO       @"TOOLBAR_AUDIO"
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

@interface HBPreferencesController () <NSTokenFieldDelegate>
{
    IBOutlet NSView         * fGeneralView, * fAudioView, * fAdvancedView;
    IBOutlet NSTextField    * fSendEncodeToAppField;
}

/* Manage the send encode to xxx.app windows and field */
- (IBAction) browseSendToApp: (id) sender;

- (void) setPrefView: (id) sender;
- (NSToolbarItem *)toolbarItemWithIdentifier: (NSString *)identifier
                                       label: (NSString *)label
                                       image: (NSImage *)image;

@property (unsafe_unretained) IBOutlet NSTokenField *formatTokenField;
@property (unsafe_unretained) IBOutlet NSTokenField *builtInTokenField;
@property (nonatomic, readonly, strong) NSArray *buildInFormatTokens;
@property (nonatomic, strong) NSArray *matches;

@property (nonatomic, strong) HBLanguagesSelection *languages;

@end

@implementation HBPreferencesController

/**
 * +[HBPreferencesController registerUserDefaults]
 *
 * Registers default values to user defaults. This is called immediately
 * when HandBrake starts, from [HBController init].
 */
+ (void)registerUserDefaults
{
    NSString *desktopDirectory = [NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES) firstObject];
    NSURL *desktopURL = [NSURL fileURLWithPath:desktopDirectory isDirectory:YES];

    [[NSUserDefaults standardUserDefaults] registerDefaults:@{
        @"HBShowOpenPanelAtLaunch":         @YES,
        @"DefaultLanguage":                 @"English",
        @"DefaultMpegExtension":            @"Auto",
        @"UseDvdNav":                       @"YES",
        @"HBDefaultPresetsDrawerShow":      @YES,
        // Archive the URL because they aren't supported in plist.
        @"HBLastDestinationDirectory":      [NSKeyedArchiver archivedDataWithRootObject:desktopURL],
        @"HBLastSourceDirectory":           [NSKeyedArchiver archivedDataWithRootObject:desktopURL],
        @"DefaultAutoNaming":               @NO,
        @"HBAlertWhenDone":                 @(HBDoneActionNotification),
        @"AlertWhenDoneSound":              @"YES",
        @"LoggingLevel":                    @"1",
        @"HBClearOldLogs":                  @YES,
        @"EncodeLogLocation":               @"NO",
        @"MinTitleScanSeconds":             @"10",
        @"PreviewsNumber":                  @"10",
        @"x264CqSliderFractional":          @"0.50",
        @"HBShowAdvancedTab":               @NO,
        @"HBAutoNamingFormat":              @[@"{Source}", @" ", @"{Title}"],
        // Hash of the default folders, until there is a better way.
        @"HBPreviewViewExpandedStatus":     @[@(4097268371718322522), @(3576901712372066251)],
        @"HBDrawerSize":                    NSStringFromSize(NSMakeSize(184, 591))
        }];

    // Overwrite the update check interval because previous versions
    // could be set to a dayly check.
    NSUInteger week = 60 * 60 * 24 * 7;
    [[NSUserDefaults standardUserDefaults] setObject:@(week) forKey:@"SUScheduledCheckInterval"];
}

/**
 * -[HBPreferencesController init]
 *
 * Initializes the preferences controller by loading Preferences.nib file.
 *
 */
- (instancetype)init
{
    if (self = [super initWithWindowNibName:@"Preferences"])
    {
        _languages = [[HBLanguagesSelection alloc] init];
    }
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
 * -[HBPreferencesController awakeFromNib]
 *
 * Called after all the outlets in the nib file have been attached. Sets up the
 * toolbar and shows the "General" pane.
 *
 */
- (void) awakeFromNib
{
    NSToolbar * toolbar = [[NSToolbar alloc] initWithIdentifier: @"Preferences Toolbar"];
    [toolbar setDelegate: self];
    [toolbar setAllowsUserCustomization: NO];
    [toolbar setDisplayMode: NSToolbarDisplayModeIconAndLabel];
    [toolbar setSizeMode: NSToolbarSizeModeRegular];
    [[self window] setToolbar: toolbar];

    // Format token field initialization
    [self.formatTokenField setTokenizingCharacterSet:[NSCharacterSet characterSetWithCharactersInString:@"%%"]];
    [self.formatTokenField setCompletionDelay:0.2];

    _buildInFormatTokens = @[@"{Source}", @"{Title}", @"{Date}", @"{Time}", @"{Chapters}", @"{Quality/Bitrate}"];
    [self.builtInTokenField setTokenizingCharacterSet:[NSCharacterSet characterSetWithCharactersInString:@"%%"]];
    [self.builtInTokenField setStringValue:[self.buildInFormatTokens componentsJoinedByString:@"%%"]];

    [toolbar setSelectedItemIdentifier: TOOLBAR_GENERAL];
    [self setPrefView:nil];
}

- (NSToolbarItem *)toolbar: (NSToolbar *)toolbar
     itemForItemIdentifier: (NSString *)ident
 willBeInsertedIntoToolbar: (BOOL)flag
{
    if ( [ident isEqualToString:TOOLBAR_GENERAL] )
    {
        return [self toolbarItemWithIdentifier:ident
                                         label:NSLocalizedString(@"General", @"Preferences General Toolbar Item")
                                         image:[NSImage imageNamed:@"settings"]];
    }
    else if ( [ident isEqualToString:TOOLBAR_AUDIO] )
    {
        return [self toolbarItemWithIdentifier:ident
                                         label:NSLocalizedString(@"Audio", @"Preferences Audio Toolbar Item")
                                         image:[NSImage imageNamed:@"audio"]];
    }
    else if ( [ident isEqualToString:TOOLBAR_ADVANCED] )
    {
        return [self toolbarItemWithIdentifier:ident
                                         label:NSLocalizedString(@"Advanced", @"Preferences Advanced Toolbar Item")
                                         image:[NSImage imageNamed:@"advanced"]];
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
    return @[TOOLBAR_GENERAL, TOOLBAR_AUDIO, TOOLBAR_ADVANCED];
}

/* Manage the send encode to xxx.app windows and field */
/*Opens the app browse window*/
- (IBAction) browseSendToApp: (id) sender
{
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection:NO];
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];

    NSString *sendToAppDirectory;
	if ([[NSUserDefaults standardUserDefaults] stringForKey:@"LastSendToAppDirectory"])
	{
		sendToAppDirectory = [[NSUserDefaults standardUserDefaults] stringForKey:@"LastSendToAppDirectory"];
	}
	else
	{
		sendToAppDirectory = @"/Applications";
	}
    [panel setDirectoryURL:[NSURL fileURLWithPath:sendToAppDirectory]];

    [panel beginSheetModalForWindow:[self window] completionHandler:^(NSInteger result) {
        if (result == NSOKButton)
        {
            NSURL *sendToAppURL = [panel URL];
            NSURL *sendToAppDirectoryURL = [sendToAppURL URLByDeletingLastPathComponent];
            [[NSUserDefaults standardUserDefaults] setObject:[sendToAppDirectoryURL path] forKey:@"LastSendToAppDirectory"];

            NSString *sendToAppName = [[sendToAppURL lastPathComponent] stringByDeletingPathExtension];
            /* we set the name of the app to send to in the display field */
            [fSendEncodeToAppField setStringValue:sendToAppName];
            [[NSUserDefaults standardUserDefaults] setObject:[fSendEncodeToAppField stringValue] forKey:@"HBSendToApp"];
        }
    }];
}

#pragma mark - Format Token Field Delegate

- (NSString *)tokenField:(NSTokenField *)tokenField displayStringForRepresentedObject:(id)representedObject
{
    if ([representedObject rangeOfString: @"{"].location == 0)
    {
        return [(NSString *)representedObject substringWithRange:NSMakeRange(1, [(NSString*)representedObject length]-2)];
    }

    return representedObject;
}

- (NSTokenStyle)tokenField:(NSTokenField *)tokenField styleForRepresentedObject:(id)representedObject
{
    if ([representedObject rangeOfString: @"{"].location == 0)
    {
        return NSRoundedTokenStyle;
    }
    else
    {
        return NSPlainTextTokenStyle;
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
    NSView * view = fGeneralView;
    if( sender )
    {
        NSString * identifier = [sender itemIdentifier];
        if( [identifier isEqualToString: TOOLBAR_AUDIO] )
            view = fAudioView;
        else if( [identifier isEqualToString: TOOLBAR_ADVANCED] )
            view = fAdvancedView;
        else;
    }

    NSWindow * window = [self window];
    if( [window contentView] == view )
        return;

    NSRect windowRect = [window frame];
    CGFloat difference = ( [view frame].size.height - [[window contentView] frame].size.height );
    windowRect.origin.y -= difference;
    windowRect.size.height += difference;

    [view setHidden: YES];
    [window setContentView: view];
    [window setFrame: windowRect display: YES animate: YES];
    [view setHidden: NO];

    //set title label
    if( sender )
        [window setTitle: [sender label]];
    else
    {
        NSToolbar * toolbar = [window toolbar];
        NSString * itemIdentifier = [toolbar selectedItemIdentifier];
        for( NSToolbarItem * item in [toolbar items] )
            if( [[item itemIdentifier] isEqualToString: itemIdentifier] )
            {
                [window setTitle: [item label]];
                break;
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
