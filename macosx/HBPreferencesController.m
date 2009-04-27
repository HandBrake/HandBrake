/**
 * @file
 * Implementation of class HBPreferencesController.
 */

#import "HBPreferencesController.h"
#define TOOLBAR_GENERAL     @"TOOLBAR_GENERAL"
#define TOOLBAR_PICTURE     @"TOOLBAR_PICTURE"
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

@interface HBPreferencesController (Private)

- (void) setPrefView: (id) sender;
- (NSToolbarItem *)toolbarItemWithIdentifier: (NSString *)identifier
                                       label: (NSString *)label
                                       image: (NSImage *)image;

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
    NSString *desktopDirectory =  [@"~/Desktop" stringByExpandingTildeInPath];

    [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:
        @"YES",             @"CheckForUpdates",
        @"Open Source",     @"LaunchSourceBehavior",
        @"English",         @"DefaultLanguage",
        @"NO",              @"DefaultMpegName",
        @"YES",             @"DefaultCrf",
        @"NO",              @"UseDvdNav",
        @"",                @"DefAdvancedx264Flags",
        @"YES",             @"DefaultPresetsDrawerShow",
        desktopDirectory,   @"LastDestinationDirectory",
        desktopDirectory,   @"LastSourceDirectory",
        @"NO",              @"DefaultAutoNaming",
        @"NO",              @"DisableDvdAutoDetect",
        @"Alert Window",    @"AlertWhenDone",
        @"1",               @"LoggingLevel",
        @"NO",              @"EncodeLogLocation",
        @"10",              @"PreviewsNumber",
        @"",                @"Drawer Size",
        @"0.25",              @"x264CqSliderFractional",
        nil]];
}

/**
 * -[HBPreferencesController init]
 *
 * Initializes the preferences controller by loading Preferences.nib file.
 *
 */
- (id)init
{
    if (self = [super initWithWindowNibName:@"Preferences"])
    {
        NSAssert([self window], @"[HBPreferencesController init] window outlet is not connected in Preferences.nib");
    }
    return self;
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
    NSToolbar * toolbar = [[[NSToolbar alloc] initWithIdentifier: @"Preferences Toolbar"] autorelease];
    [toolbar setDelegate: self];
    [toolbar setAllowsUserCustomization: NO];
    [toolbar setDisplayMode: NSToolbarDisplayModeIconAndLabel];
    [toolbar setSizeMode: NSToolbarSizeModeRegular];
    [[self window] setToolbar: toolbar];

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
                                         image:[NSImage imageNamed:NSImageNamePreferencesGeneral]];
    }
    else if ( [ident isEqualToString:TOOLBAR_PICTURE] )
    {
        return [self toolbarItemWithIdentifier:ident
                                         label:NSLocalizedString(@"Picture", @"Preferences Picture Toolbar Item")
                                         image:[NSImage imageNamed:@"pref-picture"]];
    }
    else if ( [ident isEqualToString:TOOLBAR_AUDIO] )
    {
        return [self toolbarItemWithIdentifier:ident
                                         label:NSLocalizedString(@"Audio", @"Preferences Audio Toolbar Item")
                                         image:[NSImage imageNamed:@"pref-audio"]];
    }
    else if ( [ident isEqualToString:TOOLBAR_ADVANCED] )
    {
        return [self toolbarItemWithIdentifier:ident
                                         label:NSLocalizedString(@"Advanced", @"Preferences Advanced Toolbar Item")
                                         image:[NSImage imageNamed:NSImageNameAdvanced]];
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
    return [NSArray arrayWithObjects: TOOLBAR_GENERAL, TOOLBAR_PICTURE,
                                        TOOLBAR_AUDIO, TOOLBAR_ADVANCED, nil];
}

@end

@implementation HBPreferencesController (Private)

- (void) setPrefView: (id) sender
{
    NSView * view = fGeneralView;
    if( sender )
    {
        NSString * identifier = [sender itemIdentifier];
        if( [identifier isEqualToString: TOOLBAR_PICTURE] )
            view = fPictureView;
        else if( [identifier isEqualToString: TOOLBAR_AUDIO] )
            view = fAudioView;
        else if( [identifier isEqualToString: TOOLBAR_ADVANCED] )
            view = fAdvancedView;
        else;
    }

    NSWindow * window = [self window];
    if( [window contentView] == view )
        return;

    NSRect windowRect = [window frame];
    CGFloat difference = ( [view frame].size.height - [[window contentView] frame].size.height ) * [window userSpaceScaleFactor];
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
    return [item autorelease];
}

@end
