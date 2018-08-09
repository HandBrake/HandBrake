/*  HBAddCategoryController.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBAddCategoryController.h"

#import "HBPresetsManager.h"
#import "HBPreset.h"

@interface HBAddCategoryController () <NSTextFieldDelegate>

@property (nonatomic, strong) IBOutlet NSTextField *name;
@property (nonatomic, strong) IBOutlet NSButton *createButton;

@property (nonatomic, strong) HBPresetsManager *manager;
@property (nonatomic, readwrite) HBPreset *category;

@end

@implementation HBAddCategoryController

- (instancetype)initWithPresetManager:(HBPresetsManager *)manager
{
    self = [super initWithWindowNibName:@"HBAddCategoryController"];
    if (self)
    {
        NSParameterAssert(manager);
        _manager = manager;
    }
    return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];

    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(controlTextDidChange:)
                                                 name:NSControlTextDidChangeNotification object:nil];
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSControlTextDidChangeNotification object:nil];
}

- (void)controlTextDidChange:(NSNotification *)obj {
    self.createButton.enabled = self.name.stringValue.length > 0 ? YES : NO;
}

- (IBAction)dismiss:(id)sender
{
    if (self.window.isSheet)
    {
        [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseCancel];
    }
    else
    {
        [NSApp stopModalWithCode:NSModalResponseCancel];
        [self.window orderOut:self];
    }
}

- (IBAction)create:(id)sender
{
    self.category = [[HBPreset alloc] initWithCategoryName:self.name.stringValue builtIn:NO];
    [self.manager addPreset:self.category];

    if (self.window.isSheet)
    {
        [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseOK];
    }
    else
    {
        [NSApp stopModalWithCode:NSModalResponseOK];
        [self.window orderOut:self];
    }
}


@end
