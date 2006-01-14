#include "QueueController.h"

@implementation QueueController

- (void) SetHandle: (hb_handle_t *) handle
{
    fHandle = handle;
}

- (void) AddTextField: (NSString *) string rect: (NSRect *) rect
{
    NSTextField * textField;

    rect->origin.x     = 10;
    rect->origin.y    -= 17;
    rect->size.height  = 17;
    textField = [[NSTextField alloc] initWithFrame: *rect];

    [textField setEditable: NO];
    [textField setSelectable: NO];
    [textField setDrawsBackground: NO];
    [textField setBordered: NO];
    [textField setStringValue: string];

    [fTaskView addSubview: textField];
}

- (void) removeTask: (id) sender
{
    hb_rem( fHandle, hb_job( fHandle, [sender tag] ) );
    [self performSelectorOnMainThread: @selector( Update: )
        withObject: sender waitUntilDone: NO];
}

- (void) AddButton: (NSRect *) rect tag: (int) tag
{
    NSButton * button;

    rect->origin.x     = rect->size.width - 90;
    rect->origin.y    -= 20;
    rect->size.width   = 100;
    rect->size.height  = 20;
    button = [[NSButton alloc] initWithFrame: *rect];
    rect->size.width   = rect->origin.x + 90;

    [button setTitle: @"Remove"];
    [button setBezelStyle: NSRoundedBezelStyle];
    [button setFont: [NSFont systemFontOfSize:
        [NSFont systemFontSizeForControlSize: NSSmallControlSize]]];
    [[button cell] setControlSize: NSSmallControlSize];

    [button setTag: tag];
    [button setTarget: self];
    [button setAction: @selector( removeTask: )];

    [fTaskView addSubview: button];

    NSBox * box;

    rect->origin.x     = 15;
    rect->origin.y    -= 10;
    rect->size.width  -= 10;
    rect->size.height  = 1;
    box = [[NSBox alloc] initWithFrame: *rect];
    [box setBoxType: NSBoxSeparator];
    rect->origin.y    -= 10;
    rect->size.width  += 10;

    [fTaskView addSubview: box];
}

- (IBAction) Update: (id) sender
{
    int i;
    hb_job_t * j;
    hb_title_t * title;

    NSSize size = [fScrollView contentSize];
    int height = MAX( 20 + 125 * hb_count( fHandle ), size.height );
    [fTaskView setFrame: NSMakeRect(0,0,size.width,height)];

    NSRect rect = NSMakeRect(10,height-10,size.width-20,10);

    NSArray * subviews = [fTaskView subviews];
    while( [subviews count] > 0 )
    {
        [[subviews objectAtIndex: 0]
            removeFromSuperviewWithoutNeedingDisplay];
    }

    for( i = 0; i < hb_count( fHandle ); i++ )
    {
        j = hb_job( fHandle, i );
        title = j->title;
        
        [self AddTextField: [NSString stringWithFormat:
            @"DVD: %s", title->dvd] rect: &rect];
        [self AddTextField: [NSString stringWithFormat:
            @"Title: %d", title->index] rect: &rect];
        [self AddTextField: [NSString stringWithFormat:
            @"Chapters: %d to %d", j->chapter_start, j->chapter_end]
            rect: &rect];
        [self AddTextField: [NSString stringWithFormat:
            @"Pass: %d of %d", MAX( 1, j->pass ), MIN( 2, j->pass + 1 )]
            rect: &rect];
        [self AddTextField: [NSString stringWithFormat:
            @"Destination: %s", j->file] rect: &rect];
        [self AddButton: &rect tag: i];
    }

    [fTaskView scrollPoint: NSMakePoint(0,height)];
    [fTaskView setNeedsDisplay: YES];
}

- (IBAction) ClosePanel: (id) sender
{
    [NSApp stopModal];
}

@end
