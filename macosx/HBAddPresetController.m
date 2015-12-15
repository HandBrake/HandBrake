/*  HBAddPresetController.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAddPresetController.h"
#import "HBPreset.h"
#import "HBMutablePreset.h"

typedef NS_ENUM(NSUInteger, HBAddPresetControllerMode) {
    HBAddPresetControllerModeNone,
    HBAddPresetControllerModeCustom,
    HBAddPresetControllerModeSourceMaximum,
};

@interface HBAddPresetController ()

@property (unsafe_unretained) IBOutlet NSTextField *name;
@property (unsafe_unretained) IBOutlet NSTextField *desc;

@property (unsafe_unretained) IBOutlet NSPopUpButton *picSettingsPopUp;
@property (unsafe_unretained) IBOutlet NSTextField *picWidth;
@property (unsafe_unretained) IBOutlet NSTextField *picHeight;
@property (unsafe_unretained) IBOutlet NSBox *picWidthHeightBox;

@property (nonatomic, strong) HBPreset *preset;
@property (nonatomic) int width;
@property (nonatomic) int height;

@property (nonatomic) BOOL defaultToCustom;


@end

@implementation HBAddPresetController

- (instancetype)initWithPreset:(HBPreset *)preset customWidth:(int)customWidth customHeight:(int)customHeight defaultToCustom:(BOOL)defaultToCustom
{
    self = [super initWithWindowNibName:@"AddPreset"];
    if (self)
    {
        NSParameterAssert(preset);
        _preset = preset;
        _width = customWidth;
        _height = customHeight;
        _defaultToCustom = defaultToCustom;
    }
    return self;
}

- (void)windowDidLoad {
    [super windowDidLoad];

    /*
     * Populate the preset picture settings popup.
     *
     * Custom is not applicable when the anamorphic mode is Strict.
     *
     * Use [NSMenuItem tag] to store preset values for each option.
     */

    // Default to Source Maximum
    HBAddPresetControllerMode mode = HBAddPresetControllerModeSourceMaximum;

    [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"None", nil)];
    [[self.picSettingsPopUp lastItem] setTag:HBAddPresetControllerModeNone];

    if (![self.preset[@"PicturePAR"] isEqualToString:@"strict"])
    {
        // not Strict, Custom is applicable
        [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"Custom", nil)];
        [[self.picSettingsPopUp lastItem] setTag:HBAddPresetControllerModeCustom];

        if (self.defaultToCustom)
        {
            mode = HBAddPresetControllerModeCustom;
        }
    }
    [self.picSettingsPopUp addItemWithTitle:NSLocalizedString(@"Source Maximum (post source scan)", nil)];
    [[self.picSettingsPopUp lastItem] setTag:HBAddPresetControllerModeSourceMaximum];


    [self.picSettingsPopUp selectItemWithTag:mode];

    // Initialize custom height and width settings to current values
    [self.picWidth setIntValue:self.width];
    [self.picHeight setIntValue:self.height];
    [self addPresetPicDropdownChanged:nil];
}

- (IBAction)addPresetPicDropdownChanged:(id)sender
{
    if (self.picSettingsPopUp.selectedItem.tag == HBAddPresetControllerModeCustom)
    {
        self.picWidthHeightBox.hidden = NO;
    }
    else
    {
        self.picWidthHeightBox.hidden = YES;
    }
}

- (IBAction)add:(id)sender
{
    if (self.name.stringValue.length == 0)
    {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:NSLocalizedString(@"The preset name cannot be empty.", @"")];
        [alert setInformativeText:NSLocalizedString(@"Please enter a name.", @"")];
        [alert runModal];
    }
    else
    {
        HBMutablePreset *newPreset = [self.preset mutableCopy];

        newPreset.name = self.name.stringValue;
        newPreset.presetDescription = self.desc.stringValue;

        // Get the picture size
        newPreset[@"PictureWidth"] = @(self.picWidth.integerValue);
        newPreset[@"PictureHeight"] = @(self.picHeight.integerValue);

        //Get the whether or not to apply pic Size and Cropping (includes Anamorphic)
        newPreset[@"UsesPictureSettings"] = @(self.picSettingsPopUp.selectedItem.tag);

        // Always use Picture Filter settings for the preset
        newPreset[@"UsesPictureFilters"] = @YES;

        [newPreset cleanUp];

        self.preset = [newPreset copy];

        [self.window orderOut:nil];
        [NSApp endSheet:self.window returnCode:NSModalResponseContinue];
    }
}

- (IBAction)cancel:(id)sender
{
    [self.window orderOut:nil];
    [NSApp endSheet:self.window returnCode:NSModalResponseAbort];
}

@end
