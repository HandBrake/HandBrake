/*  HBQueueRelinkWindowController.m $

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBQueueRelinkWindowController.h"
#import "HBQueueRelinkResource.h"

@import HandBrakeKit;

@interface HBQueueRelinkWindowController () <NSTableViewDataSource, NSTableViewDelegate>

@property (nonatomic) NSArray<HBQueueRelinkResource *> *resources;
@property (nonatomic) NSArray<HBQueueRelinkResource *> *visibleResources;
@property (nonatomic, weak) IBOutlet NSTableView *tableView;
@property (nonatomic, weak) IBOutlet NSSegmentedControl *filter;
@property (nonatomic, weak) IBOutlet NSTextField *statusLabel;
@property (nonatomic, weak) IBOutlet NSButton *locateButton;
@property (nonatomic, weak) IBOutlet NSButton *relinkButton;

@end

@implementation HBQueueRelinkWindowController

- (instancetype)initWithJobs:(NSArray<HBJob *> *)jobs
{
    self = [super initWithWindowNibName:@"HBQueueRelinkWindowController"];
    if (self)
    {
        _jobs = [[NSArray alloc] initWithArray:jobs copyItems:YES];
        _resources = [HBQueueRelinkResource resourcesForJobs:_jobs];
    }
    return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];

    // Nothing to show in the missing list, so show everything.
    NSUInteger missing = [self.resources indexOfObjectPassingTest:^BOOL(HBQueueRelinkResource *resource, NSUInteger idx, BOOL *stop) {
        return resource.isMissing;
    }];
    self.filter.selectedSegment = missing == NSNotFound ? 1 : 0;

    [self filterChanged:nil];
}

- (void)beginSheetForWindow:(NSWindow *)window completionHandler:(void (^)(NSModalResponse))handler
{
    [window beginSheet:self.window completionHandler:^(NSModalResponse response) {
        handler(response);
        // Release the open panels' sandbox grants only after the jobs have been saved.
        self.resources = @[];
        self.visibleResources = @[];
    }];
}

- (IBAction)filterChanged:(id)sender
{
    self.visibleResources = self.filter.selectedSegment == 1 ? self.resources :
        [self.resources filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(HBQueueRelinkResource *resource, NSDictionary *bindings) {
            return resource.isMissing;
        }]];
    [self.tableView reloadData];
    if (self.visibleResources.count)
    {
        [self.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];
    }
    [self updateUI];
}

- (void)updateUI
{
    NSUInteger located = 0;
    for (HBQueueRelinkResource *resource in self.resources)
    {
        located += resource.replacementURL != nil;
    }
    self.relinkButton.enabled = located > 0;
    self.locateButton.enabled = self.tableView.selectedRow >= 0;
    self.statusLabel.stringValue = [NSString stringWithFormat:NSLocalizedString(@"%lu of %lu locations matched", @"Queue relink status"), (unsigned long)located, (unsigned long)self.resources.count];
}

- (IBAction)locate:(id)sender
{
    NSInteger row = self.tableView.selectedRow;
    if (row < 0)
    {
        return;
    }
    HBQueueRelinkResource *resource = self.visibleResources[row];
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = resource.isSource;
    panel.canChooseDirectories = YES;
    panel.directoryURL = (resource.replacementURL ?: resource.originalURL).URLByDeletingLastPathComponent;
    panel.message = resource.originalURL.path ?: @"";
    panel.prompt = NSLocalizedString(@"Locate", @"Queue relink panel button");
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse response) {
        if (response == NSModalResponseOK)
        {
            HBSecurityAccessToken *token = [HBSecurityAccessToken tokenWithAlreadyAccessedObject:panel.URL];
            NSError *error = nil;
            if (![resource locateURL:panel.URL accessToken:token error:&error])
            {
                [self showError:error];
            }
            [self.tableView reloadData];
            [self updateUI];
        }
    }];
}

- (IBAction)locateSources:(id)sender
{
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = NO;
    panel.canChooseDirectories = YES;
    panel.message = NSLocalizedString(@"Choose the folder containing the original source files. Only unique matching names in this folder will be used.", @"Queue relink batch panel");
    [panel beginSheetModalForWindow:self.window completionHandler:^(NSModalResponse response) {
        if (response == NSModalResponseOK)
        {
            HBSecurityAccessToken *token = [HBSecurityAccessToken tokenWithAlreadyAccessedObject:panel.URL];
            NSCountedSet<NSString *> *names = [[NSCountedSet alloc] init];
            for (HBQueueRelinkResource *resource in self.resources)
            {
                if (resource.isSource && resource.originalURL)
                {
                    [names addObject:resource.originalURL.lastPathComponent.lowercaseString];
                }
            }
            NSUInteger matched = 0;
            for (HBQueueRelinkResource *resource in self.visibleResources)
            {
                if (resource.isSource && resource.originalURL && resource.replacementURL == nil &&
                    [names countForObject:resource.originalURL.lastPathComponent.lowercaseString] == 1)
                {
                    NSURL *url = [panel.URL URLByAppendingPathComponent:resource.originalURL.lastPathComponent];
                    matched += [resource locateURL:url accessToken:token error:NULL];
                }
            }
            if (matched == 0)
            {
                NSAlert *alert = [[NSAlert alloc] init];
                alert.messageText = NSLocalizedString(@"No matching sources were found in this folder.", @"Queue relink batch result");
                [alert beginSheetModalForWindow:self.window completionHandler:nil];
            }
            [self.tableView reloadData];
            [self updateUI];
        }
    }];
}

- (void)showError:(NSError *)error
{
    [[NSAlert alertWithError:error] beginSheetModalForWindow:self.window completionHandler:nil];
}

- (IBAction)cancel:(id)sender
{
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseCancel];
}

- (IBAction)relink:(id)sender
{
    if (!self.relinkButton.enabled)
    {
        return;
    }
    for (HBQueueRelinkResource *resource in self.resources)
    {
        NSError *error = nil;
        if (![resource validateReplacementWithError:&error])
        {
            [self showError:error];
            return;
        }
    }
    [self.resources makeObjectsPerformSelector:@selector(apply)];
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseOK];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    return self.visibleResources.count;
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)column row:(NSInteger)row
{
    HBQueueRelinkResource *resource = self.visibleResources[row];
    NSTableCellView *cell = [tableView makeViewWithIdentifier:column.identifier owner:self];
    NSString *value = nil;

    if ([column.identifier isEqualToString:@"type"])
    {
        value = resource.isSource ? NSLocalizedString(@"Source", @"Queue relink resource") : NSLocalizedString(@"Destination", @"Queue relink resource");
    }
    else if ([column.identifier isEqualToString:@"original"])
    {
        value = resource.originalURL.path ?: NSLocalizedString(@"No location", @"Queue relink resource status");
    }
    else
    {
        value = resource.replacementURL.path ?: (resource.isMissing ? NSLocalizedString(@"Missing or inaccessible", @"Queue relink resource status") : NSLocalizedString(@"Available", @"Queue relink resource status"));
    }

    cell.textField.stringValue = value;
    cell.toolTip = value;
    return cell;
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    [self updateUI];
}

@end
