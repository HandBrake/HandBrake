/* $Id: HBSubtitles.m,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */
//

#import "HBSubtitles.h"
#include "hb.h"

@implementation HBSubtitles
- (id)init 
{
    self = [super init];
    if( self != nil )
    {
        fTitle = NULL;
    }
    
    return self;
}


- (void)resetWithTitle:(hb_title_t *)title
{
    fTitle = title;
    
    if (!title)
    {
        return;
    }
    
    if (subtitleArray)
    {
        [subtitleArray release];
    }
    subtitleArray = [[NSMutableArray alloc] init];
    [self addSubtitleTrack];
}

#pragma mark -
#pragma mark Create new Subtitles

- (void)addSubtitleTrack
{
    [subtitleArray addObject:[self createSubtitleTrack]];
}

/* Creates a new subtitle track and stores it in an NSMutableDictionary */
- (NSDictionary *)createSubtitleTrack
{
    NSMutableDictionary *newSubtitleTrack = [[NSMutableDictionary alloc] init];
    /* Subtitle Source track popup index */
    [newSubtitleTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleSourceTrackNum"];
    /* Subtitle Source track popup language */
    [newSubtitleTrack setObject:@"None" forKey:@"subtitleSourceTrackName"];
    /* Subtitle Source track popup isPictureSub */
    [newSubtitleTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleSourceTrackisPictureSub"];
    /* Subtitle track forced state */
    [newSubtitleTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackForced"];
    /* Subtitle track burned state */
    [newSubtitleTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackBurned"];
    /* Subtitle track default state */
    [newSubtitleTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackDefault"];

    [newSubtitleTrack autorelease];
    return newSubtitleTrack;
}

- (NSMutableArray*) getSubtitleArray: (NSMutableArray *) subtitlesArray 
{
    //NSMutableArray *returnSubtitlesArray = [[NSMutableArray alloc] init];
    //[returnSubtitlesArray initWithArray:subtitleArray];
    //[returnSubtitlesArray autorelease];
    return subtitleArray;
}

- (void)containerChanged:(int) newContainer
{
container = newContainer;
}
   
#pragma mark -
#pragma mark Subtitle Table Delegate Methods
/* Table View delegate methods */
/* Returns the number of tracks displayed
 * NOTE: we return one more than the actual number of tracks
 * specified as we always keep one track set to "None" which is ignored
 * for setting up tracks, but is used to add tracks.
 */
- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
    if( fTitle == NULL || ![subtitleArray count])
    {
        return 0;
    }
    else
    {
        return [subtitleArray count];
    }
}

/* Used to tell the Table view which information is to be displayed per item */
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
    NSString *cellEntry = @"__DATA ERROR__";
    
    /* we setup whats displayed given the column identifier */
    if ([[aTableColumn identifier] isEqualToString:@"track"])
    {
        /* 'track' is a popup of all available source subtitle tracks for the given title */
        
        
        NSPopUpButtonCell *cellTrackPopup = [[NSPopUpButtonCell alloc] init];
        [cellTrackPopup autorelease];
        /* Set the Popups properties */
        /* Following two lines can be used to show kind of a text field with indicator arrows which
         * will popup when clicked on. Comment out for a standard style popup */
        //[cellTrackPopup setShowsBorderOnlyWhileMouseInside:YES];
        //[cellTrackPopup setBordered:NO];
        
        [cellTrackPopup setControlSize:NSSmallControlSize];
        [cellTrackPopup setFont:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]]];
        
        
        /* Add our initial "None" track which we use to add source tracks or remove tracks.
         * "None" is always index 0.
         */
        [[cellTrackPopup menu] addItemWithTitle: @"None" action: NULL keyEquivalent: @""];
        
        /* Foreign Audio Search (index 1 in the popup) is only available for the first track */
        if (rowIndex == 0)
        {
            [[cellTrackPopup menu] addItemWithTitle: @"Foreign Audio Search - (Bitmap)" action: NULL keyEquivalent: @""];
        }
        
        if (fTitle)
        {
            hb_subtitle_t *subtitle;
            hb_subtitle_config_t sub_config;
            int i;
            for(i = 0; i < hb_list_count( fTitle->list_subtitle ); i++ )
            {
                NSString * trackTypeString = @"";
                subtitle = (hb_subtitle_t *) hb_list_item( fTitle->list_subtitle, i );
                sub_config = subtitle->config;
                
                if (subtitle->format == PICTURESUB)
                {
                    trackTypeString = @"- (Bitmap)";
                }
                else
                {
                    trackTypeString = @"- (Text)";
                }
                
                
                NSString *popupMenuItemDescription = [NSString stringWithFormat:@"%d - %@ %@",i,[NSString stringWithUTF8String:subtitle->lang],trackTypeString];
                
                [[cellTrackPopup menu] addItemWithTitle: popupMenuItemDescription action: NULL keyEquivalent: @""];
            }
        }
        
        [aTableColumn setDataCell:cellTrackPopup];
        
    }
    else if ([[aTableColumn identifier] isEqualToString:@"forced"])
    {
    /* 'forced' is a checkbox to determine if a given source track is only to be forced */
    NSButtonCell *cellForcedCheckBox = [[NSButtonCell alloc] init];
    [cellForcedCheckBox autorelease];
    [cellForcedCheckBox setButtonType:NSSwitchButton];
    [cellForcedCheckBox setImagePosition:NSImageOnly];
    [cellForcedCheckBox setAllowsMixedState:NO];
    [aTableColumn setDataCell:cellForcedCheckBox];

    }
    else if ([[aTableColumn identifier] isEqualToString:@"burned"])
    {
    /* 'burned' is a checkbox to determine if a given source track is only to be burned */
    NSButtonCell *cellBurnedCheckBox = [[NSButtonCell alloc] init];
    [cellBurnedCheckBox autorelease];
    [cellBurnedCheckBox setButtonType:NSSwitchButton];
    [cellBurnedCheckBox setImagePosition:NSImageOnly];
    [cellBurnedCheckBox setAllowsMixedState:NO];
    [aTableColumn setDataCell:cellBurnedCheckBox];
    }
    else if ([[aTableColumn identifier] isEqualToString:@"default"])
    {
    NSButtonCell *cellDefaultCheckBox = [[NSButtonCell alloc] init];
    [cellDefaultCheckBox autorelease];
    [cellDefaultCheckBox setButtonType:NSSwitchButton];
    [cellDefaultCheckBox setImagePosition:NSImageOnly];
    [cellDefaultCheckBox setAllowsMixedState:NO];
    [aTableColumn setDataCell:cellDefaultCheckBox];
    }
    else
    {
    cellEntry = nil;    
    }

    return cellEntry;
}

/* Called whenever a widget in the table is edited or changed, we use it to record the change in the controlling array 
 * including removing and adding new tracks via the "None" ("track" index of 0) */
- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
    
    if ([[aTableColumn identifier] isEqualToString:@"track"])
    {
        [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:[anObject intValue]] forKey:@"subtitleSourceTrackNum"];
        /* Set the array to track if we are vobsub (picture sub) */
        if ([anObject intValue] != 0)
        {
            int sourceSubtitleIndex;
            bool isPictureSub = FALSE;
            if (rowIndex == 0)
            {
                sourceSubtitleIndex = [anObject intValue] - 2;
            }
            else
            {
                sourceSubtitleIndex = [anObject intValue] - 1;
            }
            
            if (rowIndex == 0 && [anObject intValue] == 1)// we are Foreign Launguage Search, which is inherently bitmap
            {
                isPictureSub = TRUE;
            }
            else
            {
                hb_subtitle_t        * subtitle;
                hb_subtitle_config_t   sub_config;
                subtitle = (hb_subtitle_t *) hb_list_item( fTitle->list_subtitle, sourceSubtitleIndex );
                sub_config = subtitle->config;
                if (subtitle->format == PICTURESUB)
                {
                    isPictureSub = TRUE;
                }
            }
            if (isPictureSub == TRUE)
            {
                [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:1] forKey:@"subtitleSourceTrackisPictureSub"];
            }
            else
            {
                [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:0] forKey:@"subtitleSourceTrackisPictureSub"];
                /* if we are not picture sub, then we must be a text sub, handbrake does not support burning in text subs */
                [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackBurned"];
            }
        }
     }
    else if ([[aTableColumn identifier] isEqualToString:@"forced"])
    {
        [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:[anObject intValue]] forKey:@"subtitleTrackForced"];   
    }
    else if ([[aTableColumn identifier] isEqualToString:@"burned"])
    {
        [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:[anObject intValue]] forKey:@"subtitleTrackBurned"];   
        /* now we need to make sure no other tracks are set to burned if we have set burned*/
        if ([anObject intValue] == 1)
        {
            int i = 0;
            NSEnumerator *enumerator = [subtitleArray objectEnumerator];
            id tempObject;
            while ( tempObject = [enumerator nextObject] )  
            {
                if (i != rowIndex )
                {
                    [tempObject setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackBurned"];
                }
            i++;
            }
        }
    }
    else if ([[aTableColumn identifier] isEqualToString:@"default"])
    {
        [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:[anObject intValue]] forKey:@"subtitleTrackDefault"];   
        /* now we need to make sure no other tracks are set to default */
        if ([anObject intValue] == 1)
        {
            int i = 0;
            NSEnumerator *enumerator = [subtitleArray objectEnumerator];
            id tempObject;
            while ( tempObject = [enumerator nextObject] )  
            {
                if (i != rowIndex)
                {
                    [tempObject setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackDefault"];
                }
            i++;
            }
        }
        
    }
    
    
    /* now lets do a bit of logic to add / remove tracks as necessary via the "None" track (index 0) */
    if ([[aTableColumn identifier] isEqualToString:@"track"])
    {
 
        /* since mp4 only supports burned in vobsubs (bitmap) we need to make sure burned in is specified */
        if (container == HB_MUX_MP4 && [anObject intValue] != 0)
        {
            /* so, if isPictureSub = TRUE and we are mp4, we now have to A) set burned-in to 1 and b) remove any other
             * tracks specified that are burned in */
            if ([[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackisPictureSub"] intValue] == 1)
            {
                [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:1] forKey:@"subtitleTrackBurned"];
            }
        }
        
         
         /* We use the track popup index number (presumes index 0 is "None" which is ignored and only used to remove tracks if need be)
         * to determine whether to 1 modify an existing track, 2. add a new empty "None" track or 3. remove an existing track.
         */

        if ([anObject intValue] != 0 && rowIndex == [subtitleArray count] - 1) // if we have a last track which != "None"
        {
            /* add a new empty None track */
            [self addSubtitleTrack];
        }
        else if ([anObject intValue] == 0 && rowIndex != ([subtitleArray count] -1))// if this track is none and not the last track displayed
        {
            /* we know the user chose to remove this track by setting it to None, so remove it from the array */
            [subtitleArray removeObjectAtIndex: rowIndex];
        }
        
        
        
    }
    
    [aTableView reloadData];
}


/* Gives fine grained control over the final drawing of the widget, including widget status via the controlling array */
- (void)tableView:(NSTableView *)aTableView willDisplayCell:(id)aCell forTableColumn:(NSTableColumn *)aTableColumn row:(NSInteger)rowIndex
{
    /* we setup whats displayed given the column identifier */
    if ([[aTableColumn identifier] isEqualToString:@"track"])
    {
        /* Set the index of the recorded source track here */
        [aCell selectItemAtIndex:[[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackNum"] intValue]];
        /* now that we have actually selected our track, we can grok the titleOfSelectedItem for that track */
        [[subtitleArray objectAtIndex:rowIndex] setObject:[[aTableColumn dataCellForRow:rowIndex] titleOfSelectedItem] forKey:@"subtitleSourceTrackName"];
        
     }
    else
    {
        
        [aCell setAlignment:NSCenterTextAlignment];
        /* If the Track is None, we disable the other cells as None is an empty track */
        if ([[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackNum"] intValue] == 0)
        {
            [aCell setState:0];
            [aCell setEnabled:NO];
        }
        else
        {
            /* Since we have a valid track, we go ahead and enable the rest of the widgets and set them according to the controlling array */
            [aCell setEnabled:YES];
        }
        
        if ([[aTableColumn identifier] isEqualToString:@"forced"])
        {
            [aCell setState:[[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleTrackForced"] intValue]];
        }
        else if ([[aTableColumn identifier] isEqualToString:@"burned"])
        {
            [aCell setState:[[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleTrackBurned"] intValue]];
            /* Disable the "Burned-In" checkbox if a) the track is "None", b) the subtitle track is text (we do not support burning in
             * text subs, or c) we are mp4 and the track is a vobsub (picture sub) */
            if ([[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackNum"] intValue] == 0 ||
                [[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackisPictureSub"] intValue] == 0 ||
                (container == HB_MUX_MP4 && [[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackisPictureSub"] intValue] == 1))
            {
                [aCell setEnabled:NO];
            }
            else
            {
                [aCell setEnabled:YES];
            }
        }
        else if ([[aTableColumn identifier] isEqualToString:@"default"])
        {
            [aCell setState:[[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleTrackDefault"] intValue]];   
        }
        
    }
    
    BOOL useMp4VobsubDelete = YES;
    if (useMp4VobsubDelete == YES)
    {
        if (container == HB_MUX_MP4)
        {
            /* now remove any other tracks that are set as burned and are picturesubs */
            int i = 0;
            int removedTracks = 0;
            NSEnumerator *enumerator = [subtitleArray objectEnumerator];
            id tempObject;
            NSMutableArray *tempArrayToDelete = [NSMutableArray array];
            BOOL removeTrack = NO; 
            while ( tempObject = [enumerator nextObject] )  
            {
                
                    if ([[tempObject objectForKey:@"subtitleSourceTrackisPictureSub"] intValue] == 1)
                    {
                        /* if this is the first vobsub mark it. if not, remove it */
                        if (removeTrack == NO)
                        {
                            /* make sure that this is set to be burned in */
                            [tempObject setObject:[NSNumber numberWithInt:1] forKey:@"subtitleTrackBurned"];
                            removeTrack = YES;
                        }
                        else
                        {
                            [tempArrayToDelete addObject:tempObject];
                            removedTracks ++;
                        }
                    }
                
                i++;
            }
            /* check to see if there are tracks to remove from the array */
            if ([tempArrayToDelete count] > 0)
            {
                /* Popup a warning that hb only support one pic sub being burned in with mp4 */
                int status;
                NSBeep();
                status = NSRunAlertPanel(@"More than one vobsub is not supported in an mp4...",@"Your first vobsub track will now be used.", @"OK", nil, nil);
                [NSApp requestUserAttention:NSCriticalRequest];
                
                [subtitleArray removeObjectsInArray:tempArrayToDelete];
                [aTableView reloadData];
            }
        }
    }

}


@end
