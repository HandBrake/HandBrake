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
@property (nonatomic) NSTableView *tableView;
@property (nonatomic) NSSegmentedControl *filter;
@property (nonatomic) NSTextField *statusLabel;
@property (nonatomic) NSButton *locateButton;
@property (nonatomic) NSButton *relinkButton;

@end

@implementation HBQueueRelinkWindowController

- (instancetype)initWithJobs:(NSArray<HBJob *> *)jobs
{
    self = [super initWithWindow:nil];
    if (self)
    {
        _jobs = [[NSArray alloc] initWithArray:jobs copyItems:YES];
        _resources = [HBQueueRelinkResource resourcesForJobs:_jobs];
        [self loadWindow];
    }
    return self;
}

- (void)loadWindow
{
    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 760, 440)
                                                styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskResizable
                                                  backing:NSBackingStoreBuffered defer:NO];
    window.title = NSLocalizedString(@"Relink Queue Items", @"Queue relink window title");
    window.contentMinSize = NSMakeSize(760, 440);
    self.window = window;
    NSView *view = window.contentView;

    NSTextField *message = [NSTextField wrappingLabelWithString:NSLocalizedString(@"Locate the original sources and destination folders at their new locations. All jobs using each location will be updated. Encoding settings are retained.", @"Queue relink message")];
    self.filter = [NSSegmentedControl segmentedControlWithLabels:@[NSLocalizedString(@"Missing", @"Queue relink filter"), NSLocalizedString(@"All", @"Queue relink filter")]
                                                  trackingMode:NSSegmentSwitchTrackingSelectOne target:self action:@selector(filterChanged:)];
    self.filter.selectedSegment = 0;

    NSScrollView *scrollView = [[NSScrollView alloc] init];
    scrollView.hasVerticalScroller = YES;
    scrollView.autohidesScrollers = YES;
    scrollView.borderType = NSBezelBorder;
    self.tableView = [[NSTableView alloc] initWithFrame:NSMakeRect(0, 0, 720, 240)];
    self.tableView.usesAlternatingRowBackgroundColors = YES;
    self.tableView.delegate = self;
    self.tableView.dataSource = self;
    self.tableView.rowHeight = 24;
    NSArray *titles = @[NSLocalizedString(@"Type", @"Queue relink column"),
                        NSLocalizedString(@"Original Location", @"Queue relink column"),
                        NSLocalizedString(@"New Location", @"Queue relink column")];
    NSArray *identifiers = @[@"type", @"original", @"replacement"];
    for (NSUInteger index = 0; index < titles.count; index++)
    {
        NSTableColumn *column = [[NSTableColumn alloc] initWithIdentifier:identifiers[index]];
        column.title = titles[index];
        column.width = index == 0 ? 110 : 300;
        [self.tableView addTableColumn:column];
    }
    scrollView.documentView = self.tableView;

    self.locateButton = [NSButton buttonWithTitle:NSLocalizedString(@"Locate…", @"Queue relink button") target:self action:@selector(locate:)];
    NSButton *locateSources = [NSButton buttonWithTitle:NSLocalizedString(@"Locate Sources in Folder…", @"Queue relink button") target:self action:@selector(locateSources:)];
    locateSources.toolTip = NSLocalizedString(@"Match source names directly inside the selected folder.", @"Queue relink batch help");
    self.statusLabel = [NSTextField labelWithString:@""];
    NSButton *cancel = [NSButton buttonWithTitle:NSLocalizedString(@"Cancel", @"Queue relink button") target:self action:@selector(cancel:)];
    cancel.keyEquivalent = @"\e";
    self.relinkButton = [NSButton buttonWithTitle:NSLocalizedString(@"Relink", @"Queue relink button") target:self action:@selector(relink:)];
    self.relinkButton.keyEquivalent = @"\r";

    for (NSView *subview in @[message, self.filter, scrollView, self.locateButton, locateSources, self.statusLabel, cancel, self.relinkButton])
    {
        subview.translatesAutoresizingMaskIntoConstraints = NO;
        [view addSubview:subview];
    }
    [NSLayoutConstraint activateConstraints:@[
        [message.topAnchor constraintEqualToAnchor:view.topAnchor constant:20],
        [message.leadingAnchor constraintEqualToAnchor:view.leadingAnchor constant:20],
        [message.trailingAnchor constraintEqualToAnchor:view.trailingAnchor constant:-20],
        [self.filter.topAnchor constraintEqualToAnchor:message.bottomAnchor constant:16],
        [self.filter.leadingAnchor constraintEqualToAnchor:message.leadingAnchor],
        [scrollView.topAnchor constraintEqualToAnchor:self.filter.bottomAnchor constant:12],
        [scrollView.leadingAnchor constraintEqualToAnchor:message.leadingAnchor],
        [scrollView.trailingAnchor constraintEqualToAnchor:message.trailingAnchor],
        [scrollView.bottomAnchor constraintEqualToAnchor:self.locateButton.topAnchor constant:-12],
        [self.locateButton.leadingAnchor constraintEqualToAnchor:message.leadingAnchor],
        [locateSources.leadingAnchor constraintEqualToAnchor:self.locateButton.trailingAnchor constant:8],
        [locateSources.centerYAnchor constraintEqualToAnchor:self.locateButton.centerYAnchor],
        [self.locateButton.bottomAnchor constraintEqualToAnchor:self.relinkButton.topAnchor constant:-20],
        [self.relinkButton.trailingAnchor constraintEqualToAnchor:message.trailingAnchor],
        [self.relinkButton.bottomAnchor constraintEqualToAnchor:view.bottomAnchor constant:-20],
        [cancel.trailingAnchor constraintEqualToAnchor:self.relinkButton.leadingAnchor constant:-8],
        [cancel.centerYAnchor constraintEqualToAnchor:self.relinkButton.centerYAnchor],
        [self.statusLabel.leadingAnchor constraintEqualToAnchor:message.leadingAnchor],
        [self.statusLabel.centerYAnchor constraintEqualToAnchor:self.relinkButton.centerYAnchor],
        [self.statusLabel.trailingAnchor constraintLessThanOrEqualToAnchor:cancel.leadingAnchor constant:-12]
    ]];
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

- (void)filterChanged:(id)sender
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

- (void)locate:(id)sender
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

- (void)locateSources:(id)sender
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

- (void)cancel:(id)sender
{
    [self.window.sheetParent endSheet:self.window returnCode:NSModalResponseCancel];
}

- (void)relink:(id)sender
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
    NSTextField *cell = [tableView makeViewWithIdentifier:column.identifier owner:self];
    if (cell == nil)
    {
        cell = [NSTextField labelWithString:@""];
        cell.identifier = column.identifier;
        cell.lineBreakMode = NSLineBreakByTruncatingMiddle;
    }
    if ([column.identifier isEqualToString:@"type"])
    {
        cell.stringValue = resource.isSource ? NSLocalizedString(@"Source", @"Queue relink resource") : NSLocalizedString(@"Destination", @"Queue relink resource");
    }
    else if ([column.identifier isEqualToString:@"original"])
    {
        cell.stringValue = resource.originalURL.path ?: NSLocalizedString(@"No location", @"Queue relink resource status");
    }
    else
    {
        cell.stringValue = resource.replacementURL.path ?: (resource.isMissing ? NSLocalizedString(@"Missing or inaccessible", @"Queue relink resource status") : NSLocalizedString(@"Available", @"Queue relink resource status"));
    }
    cell.toolTip = cell.stringValue;
    return cell;
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    [self updateUI];
}

@end
