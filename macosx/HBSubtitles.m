/* $Id: HBSubtitles.m,v 1.35 2005/08/01 14:29:50 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#import "HBSubtitles.h"
#include "lang.h"
#include "hb.h"

@implementation HBSubtitles
- (id)init 
{
    self = [super init];
    if( self != nil )
    {
        fTitle = NULL;
    }
    
    /* setup our array of languages */
    const iso639_lang_t *lang;
    languagesArray = [[NSMutableArray alloc] init];
    for (lang = lang_get_next(NULL); lang != NULL; lang = lang_get_next(lang))
    {
        [languagesArray addObject:[NSArray arrayWithObjects:
                                   [NSString stringWithUTF8String:lang->eng_name],
                                   [NSString stringWithUTF8String:lang->iso639_2],
                                   nil]];
        if (!strcasecmp(lang->eng_name, "English"))
        {
            languagesArrayDefIndex = [languagesArray count] - 1;
        }
    }
   
    /* populate the charCodeArray */
    charCodeArray = [[NSMutableArray alloc] init];
    [charCodeArray addObject:@"ANSI_X3.4-1968"];
    [charCodeArray addObject:@"ANSI_X3.4-1986"];
    [charCodeArray addObject:@"ANSI_X3.4"];
    [charCodeArray addObject:@"ANSI_X3.110-1983"];
    [charCodeArray addObject:@"ANSI_X3.110"];
    [charCodeArray addObject:@"ASCII"];
    [charCodeArray addObject:@"ECMA-114"];
    [charCodeArray addObject:@"ECMA-118"];
    [charCodeArray addObject:@"ECMA-128"];
    [charCodeArray addObject:@"ECMA-CYRILLIC"];
    [charCodeArray addObject:@"IEC_P27-1"];
    [charCodeArray addObject:@"ISO-8859-1"];
    [charCodeArray addObject:@"ISO-8859-2"];
    [charCodeArray addObject:@"ISO-8859-3"];
    [charCodeArray addObject:@"ISO-8859-4"];
    [charCodeArray addObject:@"ISO-8859-5"];
    [charCodeArray addObject:@"ISO-8859-6"];
    [charCodeArray addObject:@"ISO-8859-7"];
    [charCodeArray addObject:@"ISO-8859-8"];
    [charCodeArray addObject:@"ISO-8859-9"];
    [charCodeArray addObject:@"ISO-8859-9E"];
    [charCodeArray addObject:@"ISO-8859-10"];
    [charCodeArray addObject:@"ISO-8859-11"];
    [charCodeArray addObject:@"ISO-8859-13"];
    [charCodeArray addObject:@"ISO-8859-14"];
    [charCodeArray addObject:@"ISO-8859-15"];
    [charCodeArray addObject:@"ISO-8859-16"];
    [charCodeArray addObject:@"UTF-7"];
    [charCodeArray addObject:@"UTF-8"];
    [charCodeArray addObject:@"UTF-16"];
    [charCodeArray addObject:@"UTF-16LE"];
    [charCodeArray addObject:@"UTF-16BE"];
    [charCodeArray addObject:@"UTF-32"];
    [charCodeArray addObject:@"UTF-32LE"];
    [charCodeArray addObject:@"UTF-32BE"];
    
    charCodeArrayDefIndex = 11;
    
    return self;
}


- (void)resetWithTitle:(hb_title_t *)title
{
    if (!title)
    {
        return;
    }
    fTitle = title;
    
    /* reset the subtitle source array */
    if (subtitleSourceArray)
    {
        [subtitleSourceArray release];
    }
    subtitleSourceArray = [[NSMutableArray alloc] init];
    
    /* now populate the array with the source subtitle track info */
    int i;
    hb_subtitle_t *subtitle;
    NSMutableArray *forcedSourceNamesArray = [[NSMutableArray alloc] init];
    for (i = 0; i < hb_list_count(fTitle->list_subtitle); i++)
    {
        subtitle = (hb_subtitle_t*)hb_list_item(fTitle->list_subtitle, i);
        
        /* Subtitle source features */
        int canBeBurnedIn       = hb_subtitle_can_burn(subtitle->source);
        int supportsForcedFlags = hb_subtitle_can_force(subtitle->source);
        /* Human-readable representation of subtitle->source */
        NSString *bitmapOrText  = subtitle->format == PICTURESUB ? @"Bitmap" : @"Text";
        NSString *subSourceName = [NSString stringWithUTF8String:hb_subsource_name(subtitle->source)];
        /* if the subtitle track can be forced, add its source name to the array */
        if (supportsForcedFlags &&
            [forcedSourceNamesArray containsObject:subSourceName] == NO)
        {
            [forcedSourceNamesArray addObject:subSourceName];
        }
        
        /* create a dictionary of source subtitle information to store in our array */
        NSMutableDictionary *newSubtitleSourceTrack = [[NSMutableDictionary alloc] init];
        /* Subtitle Source track name */
        [newSubtitleSourceTrack setObject:[NSString stringWithFormat:@"%d - %@ - (%@) (%@)",
                                           i, [NSString stringWithUTF8String:subtitle->lang],
                                           bitmapOrText,subSourceName]
                                   forKey:@"sourceTrackName"];
        /* Subtitle Source track number, type and features */
        [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:i]                   forKey:@"sourceTrackNum"];
        [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:subtitle->source]    forKey:@"sourceTrackType"];
        [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:canBeBurnedIn]       forKey:@"sourceTrackCanBeBurnedIn"];
        [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:supportsForcedFlags] forKey:@"sourceTrackSupportsForcedFlags"];
        [subtitleSourceArray addObject:newSubtitleSourceTrack];
        [newSubtitleSourceTrack autorelease];
    }
    
    /* now set the name of the Foreign Audio Search track */
    if ([forcedSourceNamesArray count])
    {
        [forcedSourceNamesArray sortUsingComparator:^(id obj1, id obj2)
         {
             return [((NSString*)obj1) compare:((NSString*)obj2)];
         }];
        NSString *tempString;
        NSString *tempList       = @"";
        NSEnumerator *enumerator = [forcedSourceNamesArray objectEnumerator];
        while (tempString = (NSString*)[enumerator nextObject])
        {
            if ([tempList length])
            {
                tempList = [tempList stringByAppendingString:@", "];
            }
            tempList = [tempList stringByAppendingString:tempString];
        }
        [foreignAudioSearchTrackName release];
        foreignAudioSearchTrackName = [[NSString stringWithFormat:@"Foreign Audio Search - (Bitmap) (%@)", tempList]
                                       retain];
    }
    else
    {
        [foreignAudioSearchTrackName release];
        foreignAudioSearchTrackName = [[NSString stringWithString:@"Foreign Audio Search - (Bitmap)"]
                                       retain];
    }
    [forcedSourceNamesArray release];
    
    /* reset the subtitle output array */
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
    /* Subtitle track forced state */
    [newSubtitleTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackForced"];
    /* Subtitle track burned state */
    [newSubtitleTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackBurned"];
    /* Subtitle track default state */
    [newSubtitleTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackDefault"];
    /* Subtitle Source track canBeBurnedIn */
    [newSubtitleTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleSourceTrackCanBeBurnedIn"];
    /* Subtitle Source track supportsForcedFlags */
    [newSubtitleTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleSourceTrackSupportsForcedFlags"];

    [newSubtitleTrack autorelease];
    return newSubtitleTrack;
}

- (void)createSubtitleSrtTrack:(NSString *)filePath
{
    /* Create a new entry for the subtitle source array so it shows up in our subtitle source list */
    NSString *displayname = [filePath lastPathComponent];// grok an appropriate display name from the srt subtitle */
    /* create a dictionary of source subtitle information to store in our array */
    NSMutableDictionary *newSubtitleSourceTrack = [[NSMutableDictionary alloc] init];
    /* Subtitle Source track popup index */
    [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:[subtitleSourceArray count]+1] forKey:@"sourceTrackNum"];
    /* Subtitle Source track name */
    [newSubtitleSourceTrack setObject:displayname forKey:@"sourceTrackName"];
    /* Subtitle Source track type (VobSub, Srt, etc.) */
    [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:SRTSUB] forKey:@"sourceTrackType"];
    [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:SRTSUB] forKey:@"subtitleSourceTrackType"];
    /* Subtitle Source file path */
    [newSubtitleSourceTrack setObject:filePath forKey:@"sourceSrtFilePath"];
    /* Subtitle Source track canBeBurnedIn */
    [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:0] forKey:@"sourceTrackCanBeBurnedIn"];
    /* Subtitle Source track supportsForcedFlags */
    [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:0] forKey:@"sourceTrackSupportsForcedFlags"];
    
    [subtitleSourceArray addObject:newSubtitleSourceTrack];
    [newSubtitleSourceTrack autorelease];
    
    /* Now create a new srt subtitle dictionary assuming the user wants to add it to their list 
     * Note: the subtitle array always has last entry for "None", so we need to replace its
     * position in the array and tack a "None" track back on the end of the list */
    [subtitleArray removeObjectAtIndex:[subtitleArray count] - 1];
    
    
    NSMutableDictionary *newSubtitleSrtTrack = [[NSMutableDictionary alloc] init];
    /* Subtitle Source track popup index */
    if ([subtitleArray count] == 0) // we now have an empty array so this will be our first track
    {
        [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:[subtitleSourceArray count] + 1] forKey:@"subtitleSourceTrackNum"];
    }
    else
    {
        [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:[subtitleSourceArray count]] forKey:@"subtitleSourceTrackNum"];
    }
    
    [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:SRTSUB] forKey:@"sourceTrackType"];
    [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:SRTSUB] forKey:@"subtitleSourceTrackType"];
    /* Subtitle Source track popup language */
    [newSubtitleSrtTrack setObject:displayname forKey:@"subtitleSourceTrackName"];
    /* Subtitle track forced state */
    [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackForced"];
    /* Subtitle track burned state */
    [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackBurned"];
    /* Subtitle track default state */
    [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackDefault"];
    /* Subtitle Source track canBeBurnedIn */
    [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleSourceTrackCanBeBurnedIn"];
    /* Subtitle Source track supportsForcedFlags */
    [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleSourceTrackSupportsForcedFlags"];
    
    /* now the srt only info, Language, Chart Code and offset */
    [newSubtitleSrtTrack setObject:filePath forKey:@"subtitleSourceSrtFilePath"];
    [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:languagesArrayDefIndex] forKey:@"subtitleTrackSrtLanguageIndex"];
    [newSubtitleSrtTrack setObject:[[languagesArray objectAtIndex:languagesArrayDefIndex] objectAtIndex:0] forKey:@"subtitleTrackSrtLanguageLong"];
    [newSubtitleSrtTrack setObject:[[languagesArray objectAtIndex:languagesArrayDefIndex] objectAtIndex:1] forKey:@"subtitleTrackSrtLanguageIso3"];
    
    [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:charCodeArrayDefIndex] forKey:@"subtitleTrackSrtCharCodeIndex"];
    [newSubtitleSrtTrack setObject:[charCodeArray objectAtIndex:charCodeArrayDefIndex] forKey:@"subtitleTrackSrtCharCode"];
                    
    [newSubtitleSrtTrack setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackSrtOffset"];
    
    
    [subtitleArray addObject:newSubtitleSrtTrack];
    [newSubtitleSrtTrack release];
    
    /* now add back the none track to the end of the array */
    [self addSubtitleTrack];
    
    
}

/* used to return the current subtitleArray to controller.m */
- (NSMutableArray*) getSubtitleArray
{
    return subtitleArray;
}

- (void)containerChanged:(int) newContainer
{
    container = newContainer;
}

- (void)setNewSubtitles:(NSMutableArray*) newSubtitleArray
{
    /* Note: we need to look for external subtitles so it can be added to the source array track.
     * Remember the source container subs are already loaded with resetTitle which is already called
     * so any external sub sources need to be added to our source subs here
     */
    
    int i = 0;
    NSEnumerator *enumerator = [newSubtitleArray objectEnumerator];
    id tempObject;
    while ( tempObject = [enumerator nextObject] )  
    {
        /* We have an srt track */
        if ([[tempObject objectForKey:@"subtitleSourceTrackType"] intValue] == SRTSUB)
        {
            NSString *filePath = [tempObject objectForKey:@"subtitleSourceSrtFilePath"];
            /* Start replicate the add new srt code above */
            /* Create a new entry for the subtitle source array so it shows up in our subtitle source list */
            NSString *displayname = [filePath lastPathComponent];// grok an appropriate display name from the srt subtitle */
            /* create a dictionary of source subtitle information to store in our array */
            NSMutableDictionary *newSubtitleSourceTrack = [[NSMutableDictionary alloc] init];
            /* Subtitle Source track popup index */
            [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:[subtitleSourceArray count]+1] forKey:@"sourceTrackNum"];
            /* Subtitle Source track name */
            [newSubtitleSourceTrack setObject:displayname forKey:@"sourceTrackName"];
            /* Subtitle Source track type (VobSub, Srt, etc.) */
            [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:SRTSUB] forKey:@"sourceTrackType"];
            [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:SRTSUB] forKey:@"subtitleSourceTrackType"];
            /* Subtitle Source file path */
            [newSubtitleSourceTrack setObject:filePath forKey:@"sourceSrtFilePath"];
            /* Subtitle Source track canBeBurnedIn */
            [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:0] forKey:@"sourceTrackCanBeBurnedIn"];
            /* Subtitle Source track supportsForcedFlags */
            [newSubtitleSourceTrack setObject:[NSNumber numberWithInt:0] forKey:@"sourceTrackSupportsForcedFlags"];
            
            [subtitleSourceArray addObject:newSubtitleSourceTrack];
            [newSubtitleSourceTrack autorelease];
            /* END replicate the add new srt code above */
        }
        i++;
    }
    
    
    /*Set the subtitleArray to the newSubtitleArray */
    [subtitleArray setArray:newSubtitleArray];
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
        [cellTrackPopup setControlSize:NSSmallControlSize];
        [cellTrackPopup setFont:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]]];
        
        
        /* Add our initial "None" track which we use to add source tracks or remove tracks.
         * "None" is always index 0.
         */
        [[cellTrackPopup menu] addItemWithTitle: @"None" action: NULL keyEquivalent: @""];
        
        /* Foreign Audio Search (index 1 in the popup) is only available for the first track */
        if (rowIndex == 0)
        {
            // TODO: hide the track when no force-able subtitles are present in the source
            [[cellTrackPopup menu] addItemWithTitle:foreignAudioSearchTrackName
                                             action:NULL
                                      keyEquivalent:@""];
        }
        
        int i;
        for(i = 0; i < [subtitleSourceArray count]; i++ )
        {
            [[cellTrackPopup menu] addItemWithTitle: [[subtitleSourceArray objectAtIndex:i] objectForKey: @"sourceTrackName"] action: NULL keyEquivalent: @""]; 
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
    /* These next three columns only apply to srt's. they are disabled for source subs */
    else if ([[aTableColumn identifier] isEqualToString:@"srt_lang"])
    {
        /* 'srt_lang' is a popup of commonly used languages to be matched to the source srt file */
        NSPopUpButtonCell *cellSrtLangPopup = [[NSPopUpButtonCell alloc] init];
        [cellSrtLangPopup autorelease];
        /* Set the Popups properties */
        [cellSrtLangPopup setControlSize:NSSmallControlSize];
        [cellSrtLangPopup setFont:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]]];
        /* list our languages as per the languagesArray */
        int i;
        for(i = 0; i < [languagesArray count]; i++ )
        {
            [[cellSrtLangPopup menu] addItemWithTitle: [[languagesArray objectAtIndex:i] objectAtIndex:0] action: NULL keyEquivalent: @""]; 
        }
        [aTableColumn setDataCell:cellSrtLangPopup]; 
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_charcode"])
    {
        /* 'srt_charcode' is a popup of commonly used character codes to be matched to the source srt file */
        NSPopUpButtonCell *cellSrtCharCodePopup = [[NSPopUpButtonCell alloc] init];
        [cellSrtCharCodePopup autorelease];
        /* Set the Popups properties */
        [cellSrtCharCodePopup setControlSize:NSSmallControlSize];
        [cellSrtCharCodePopup setFont:[NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]]];
        /* list our character codes, as per charCodeArray */
        
        int i;
        for(i = 0; i < [charCodeArray count]; i++ )
        {
            [[cellSrtCharCodePopup menu] addItemWithTitle: [charCodeArray objectAtIndex:i] action: NULL keyEquivalent: @""]; 
        }
        [aTableColumn setDataCell:cellSrtCharCodePopup];
        
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_offset"])
    {
        if ([[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleTrackSrtOffset"])
        {
            cellEntry = [NSString stringWithFormat:@"%d",[[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleTrackSrtOffset"] intValue]];
        }
        else
        {
            cellEntry = [NSString stringWithFormat:@"%d",0];
        }
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
            /* The first row has an additional track (Foreign Audio Search) */
            int sourceSubtitleIndex = [anObject intValue] - 1 - (rowIndex == 0);
            
            if(rowIndex == 0 && [anObject intValue] == 1)
            {
                /*
                 * we are foreign lang search, which is inherently bitmap
                 *
                 * since it can be either VOBSUB or PGS and the latter can't be
                 * passed through to MP4, we need to know whether there are any
                 * PGS tracks in the source - otherwise we can just set the
                 * source track type to VOBSUB
                 */
                int subtitleTrackType = VOBSUB;
                if ([foreignAudioSearchTrackName rangeOfString:
                     [NSString stringWithUTF8String:
                      hb_subsource_name(PGSSUB)]].location != NSNotFound)
                {
                    subtitleTrackType = PGSSUB;
                }
                // now set the track type
                [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:subtitleTrackType] forKey:@"subtitleSourceTrackType"];
                [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:1]                 forKey:@"subtitleSourceTrackCanBeBurnedIn"];
                [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:1]                 forKey:@"subtitleSourceTrackSupportsForcedFlags"];
                // foreign lang search is most useful when combined w/Forced Only - make it default
                [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:1]                 forKey:@"subtitleTrackForced"];
            }
            /* check to see if we are an srt, in which case set our file path and source track type kvp's*/
            else if ([[[subtitleSourceArray objectAtIndex:sourceSubtitleIndex] objectForKey:@"sourceTrackType"] intValue] == SRTSUB)
            {
                [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:SRTSUB]
                                                           forKey:@"subtitleSourceTrackType"];
                [[subtitleArray objectAtIndex:rowIndex] setObject:[[subtitleSourceArray objectAtIndex:sourceSubtitleIndex] objectForKey:@"sourceSrtFilePath"]
                                                           forKey:@"subtitleSourceSrtFilePath"];
            }
            else
            {
                [[subtitleArray objectAtIndex:rowIndex] setObject:[[subtitleSourceArray objectAtIndex:sourceSubtitleIndex] objectForKey:@"sourceTrackType"]
                                                           forKey:@"subtitleSourceTrackType"];
                [[subtitleArray objectAtIndex:rowIndex] setObject:[[subtitleSourceArray objectAtIndex:sourceSubtitleIndex] objectForKey:@"sourceTrackCanBeBurnedIn"]
                                                           forKey:@"subtitleSourceTrackCanBeBurnedIn"];
                [[subtitleArray objectAtIndex:rowIndex] setObject:[[subtitleSourceArray objectAtIndex:sourceSubtitleIndex] objectForKey:@"sourceTrackSupportsForcedFlags"]
                                                           forKey:@"subtitleSourceTrackSupportsForcedFlags"];
            } 
            
            if([[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackCanBeBurnedIn"] intValue] == 0)
            {
                /* the source track cannot be burned in, so uncheck the widget */
                [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackBurned"];
            }
             
            if([[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackSupportsForcedFlags"] intValue] == 0)
            {
                /* the source track does not support forced flags, so uncheck the widget */
                [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackForced"];
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
        if([anObject intValue] == 1)
        {
            /* Burned In and Default are mutually exclusive */
            [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackDefault"];
        }
        /* now we need to make sure no other tracks are set to burned if we have set burned */
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
        if([anObject intValue] == 1)
        {
            /* Burned In and Default are mutually exclusive */
            [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackBurned"];
        }   
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
    /* These next three columns only apply to srt's. they are disabled for source subs */
    else if ([[aTableColumn identifier] isEqualToString:@"srt_lang"])
    {
        
        [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:[anObject intValue]] forKey:@"subtitleTrackSrtLanguageIndex"];
        [[subtitleArray objectAtIndex:rowIndex] setObject:[[languagesArray objectAtIndex:[anObject intValue]] objectAtIndex:0] forKey:@"subtitleTrackSrtLanguageLong"];
        [[subtitleArray objectAtIndex:rowIndex] setObject:[[languagesArray objectAtIndex:[anObject intValue]] objectAtIndex:1] forKey:@"subtitleTrackSrtLanguageIso3"];
        
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_charcode"])
    {
        /* charCodeArray */
        [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:[anObject intValue]] forKey:@"subtitleTrackSrtCharCodeIndex"];
        [[subtitleArray objectAtIndex:rowIndex] setObject:[charCodeArray objectAtIndex:[anObject intValue]] forKey:@"subtitleTrackSrtCharCode"];
    }
    else if ([[aTableColumn identifier] isEqualToString:@"srt_offset"])
    {
        [[subtitleArray objectAtIndex:rowIndex] setObject:anObject forKey:@"subtitleTrackSrtOffset"];  
    } 
    
    
    /* now lets do a bit of logic to add / remove tracks as necessary via the "None" track (index 0) */
    if ([[aTableColumn identifier] isEqualToString:@"track"])
    {
        
        /* Since currently no quicktime based playback devices support soft vobsubs in mp4, we make sure "burned in" is specified
         * by default to avoid massive confusion and anarchy. However we also want to guard against multiple burned in subtitle tracks
         * as libhb would ignore all but the first one anyway. Plus it would probably be stupid. 
         */
        if ((container & HB_MUX_MASK_MP4) && ([anObject intValue] != 0))
        {
            if ([[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackType"] intValue] == VOBSUB)
            {
                /* lets see if there are currently any burned in subs specified */
                NSEnumerator *enumerator = [subtitleArray objectEnumerator];
                id tempObject;
                BOOL subtrackBurnedInFound = NO;
                while ( tempObject = [enumerator nextObject] )  
                {
                    if ([[tempObject objectForKey:@"subtitleTrackBurned"] intValue] == 1)
                    {
                        subtrackBurnedInFound = YES;
                    }
                }
                /* if we have no current vobsub set to burn it in ... burn it in by default */
                if (!subtrackBurnedInFound)
                {
                    [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:1] forKey:@"subtitleTrackBurned"];
                    /* Burned In and Default are mutually exclusive */
                    [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackDefault"];
                }
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
        else if ([anObject intValue] == 0 && rowIndex != ([subtitleArray count] -1))// if this track is set to "None" and not the last track displayed
        {
            /* we know the user chose to remove this track by setting it to None, so remove it from the array */
            /* However,if this is the first track we have to reset the selected index of the next track by + 1, since it will now become
             * the first track, which has to account for the extra "Foreign Language Search" index. */
            if (rowIndex == 0 && [[[subtitleArray objectAtIndex: 1] objectForKey: @"subtitleSourceTrackNum"] intValue] != 0)
            {
                /* get the index of the selection in row one (which is track two) */
                int trackOneSelectedIndex = [[[subtitleArray objectAtIndex: 1] objectForKey: @"subtitleSourceTrackNum"] intValue];
                /* increment the index of the subtitle menu item by one, to account for Foreign Language Search which is unique to the first track */
                [[subtitleArray objectAtIndex: 1] setObject:[NSNumber numberWithInt:trackOneSelectedIndex + 1] forKey:@"subtitleSourceTrackNum"];
            }
            /* now that we have made the adjustment for track one (index 0) go ahead and delete the track */
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
            /* Disable the "Forced Only" checkbox if a) the track is "None" or b) the subtitle track doesn't support forced flags */
            if (![[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackNum"] intValue] ||
                ![[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackSupportsForcedFlags"] intValue])
            {
                [aCell setEnabled:NO];
            }
            else
            {
                [aCell setEnabled:YES];
            }
        }
        else if ([[aTableColumn identifier] isEqualToString:@"burned"])
        {
            [aCell setState:[[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleTrackBurned"] intValue]];
            /*
             * Disable the "Burned In" checkbox if:
             * a) the track is "None" OR
             * b) the subtitle track can't be burned in OR
             * c) the subtitle track can't be passed through (e.g. PGS w/MP4)
             */
            if (![[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackNum"] intValue] ||
                ![[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackCanBeBurnedIn"] intValue] ||
                !hb_subtitle_can_pass([[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackType"] intValue], container))
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
            /*
             * Disable the "Default" checkbox if:
             * a) the track is "None" OR
             * b) the subtitle track can't be passed through (e.g. PGS w/MP4)
             */
            if (![[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackNum"] intValue] ||
                !hb_subtitle_can_pass([[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackType"] intValue], container))
            {
                [aCell setState:NSOffState];
                [aCell setEnabled:NO];
            }
            else
            {
                [aCell setState:[[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleTrackDefault"] intValue]];
                [aCell setEnabled:YES];
            }
        }
        /* These next three columns only apply to srt's. they are disabled for source subs */
        else if ([[aTableColumn identifier] isEqualToString:@"srt_lang"])
        {
            /* We have an srt file so set the track type (Source or SRT, and the srt file path ) kvp's*/
            if ([[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackType"] intValue] == SRTSUB)
            {
                [aCell setEnabled:YES];
                if([[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleTrackSrtLanguageIndex"])
                {
                    [aCell selectItemAtIndex:[[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleTrackSrtLanguageIndex"] intValue]];
                }
                else
                {
                    [aCell selectItemAtIndex:languagesArrayDefIndex]; // English
                    [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:languagesArrayDefIndex] forKey:@"subtitleTrackSrtLanguageIndex"];
                    [[subtitleArray objectAtIndex:rowIndex] setObject:[[languagesArray objectAtIndex:languagesArrayDefIndex] objectAtIndex:0] forKey:@"subtitleTrackSrtLanguageLong"];
                    [[subtitleArray objectAtIndex:rowIndex] setObject:[[languagesArray objectAtIndex:languagesArrayDefIndex] objectAtIndex:1] forKey:@"subtitleTrackSrtLanguageIso3"];
                    
                }
            }
            else
            {
                [aCell setEnabled:NO];
            }  
        }
        else if ([[aTableColumn identifier] isEqualToString:@"srt_charcode"])
        {
            /* We have an srt file so set the track type (Source or SRT, and the srt file path ) kvp's*/
            if ([[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackType"] intValue] == SRTSUB)
            {
                [aCell setEnabled:YES];
                if ([[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleTrackSrtCharCodeIndex"])
                {
                    [aCell selectItemAtIndex:[[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleTrackSrtCharCodeIndex"] intValue]];
                }
                else
                {
                    [aCell selectItemAtIndex:charCodeArrayDefIndex]; // ISO-8859-1
                    [[subtitleArray objectAtIndex:rowIndex] setObject:[NSNumber numberWithInt:charCodeArrayDefIndex] forKey:@"subtitleTrackSrtCharCodeIndex"];
                    [[subtitleArray objectAtIndex:rowIndex] setObject:[charCodeArray objectAtIndex:charCodeArrayDefIndex] forKey:@"subtitleTrackSrtCharCode"];
                }
            }
            else
            {
                [aCell setEnabled:NO];
            }   
        }
        else if ([[aTableColumn identifier] isEqualToString:@"srt_offset"])
        {
            if ([[[subtitleArray objectAtIndex:rowIndex] objectForKey:@"subtitleSourceTrackType"] intValue] == SRTSUB)
            {
                [aCell setEnabled:YES];
            }
            else
            {
                [aCell setEnabled:NO];
            }
        }
        
        /*
         * Let's check whether any subtitles in the list cannot be passed through.
         * Set the first of any such subtitles to burned-in, remove the others.
         */
        id tempObject;
        int subtitleTrackType;
        BOOL convertToBurnInUsed       = NO;
        NSMutableArray *tracksToDelete = [[NSMutableArray alloc] init];
        NSEnumerator *enumerator       = [subtitleArray objectEnumerator];
        /* convert any non-None incompatible tracks to burn-in or remove them */
        while ((tempObject = [enumerator nextObject]) &&
               [tempObject objectForKey:@"subtitleSourceTrackType"])
        {
            subtitleTrackType = [[tempObject objectForKey:@"subtitleSourceTrackType"] intValue];
            if (!hb_subtitle_can_pass(subtitleTrackType, container))
            {
                if (convertToBurnInUsed == NO)
                {
                    /* we haven't set any track to burned-in yet, so we can */
                    [tempObject setObject:[NSNumber numberWithInt:1] forKey:@"subtitleTrackBurned"];
                    convertToBurnInUsed = YES; //remove any additional tracks
                }
                else
                {
                    /* we already have a burned-in track, we must remove others */
                    [tracksToDelete addObject:tempObject];
                }
            }
        }
        /* if we converted a track to burned-in, unset it for tracks that support passthru */
        if (convertToBurnInUsed == YES)
        {
            enumerator = [subtitleArray objectEnumerator];
            while ((tempObject = [enumerator nextObject]) &&
                   [tempObject objectForKey:@"subtitleSourceTrackType"])
            {
                subtitleTrackType = [[tempObject objectForKey:@"subtitleSourceTrackType"] intValue];
                if (hb_subtitle_can_pass(subtitleTrackType, container))
                {
                    [tempObject setObject:[NSNumber numberWithInt:0] forKey:@"subtitleTrackBurned"];
                }
            }
        }
        /* this is where the actual removal takes place */
        if ([tracksToDelete count] > 0)
        {
            [subtitleArray removeObjectsInArray:tracksToDelete];
            [aTableView reloadData];
            /* this must be called after reloadData so as to not block the UI */
            [[NSAlert alertWithMessageText:@"Subtitle tack(s) removed"
                             defaultButton:@"OK"
                           alternateButton:nil
                               otherButton:nil
                 informativeTextWithFormat:@"%d subtitle %@ could neither be converted to burn-in nor passed through",
              [tracksToDelete count],
              [tracksToDelete count] > 1 ? @"tracks" : @"track"] runModal];
        }
        [tracksToDelete release];
    }
}


@end
