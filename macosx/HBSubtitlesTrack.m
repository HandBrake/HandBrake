/*  HBSubtitlesTrack.m

 This file is part of the HandBrake source code.
 Homepage: <http://handbrake.fr/>.
 It may be used under the terms of the GNU General Public License. */

#import "HBSubtitlesTrack.h"
#import "HBCodingUtilities.h"
#import "HBTitle.h"

#include "handbrake/common.h"
#include "handbrake/lang.h"

#define CHAR_CODE_DEFAULT_INDEX 28

static NSArray *charEncodingArray = nil;
static NSArray *_languagesArray = nil;

@interface HBSubtitlesTrack ()
@property (nonatomic, readwrite) BOOL validating;
@end

@implementation HBSubtitlesTrack

+ (void)initialize
{
    if ([HBSubtitlesTrack class] == self)
    {
        // populate the charCodeArray.
        charEncodingArray = @[@"ANSI_X3.4-1968", @"ANSI_X3.4-1986", @"ANSI_X3.4", @"ANSI_X3.110-1983", @"ANSI_X3.110",
                              @"ASCII", @"ECMA-114", @"ECMA-118", @"ECMA-128", @"ECMA-CYRILLIC", @"IEC_P27-1", @"ISO-8859-1",
                              @"ISO-8859-2", @"ISO-8859-3", @"ISO-8859-4", @"ISO-8859-5", @"ISO-8859-6", @"ISO-8859-7",
                              @"ISO-8859-8", @"ISO-8859-9", @"ISO-8859-9E", @"ISO-8859-10", @"ISO-8859-11", @"ISO-8859-13",
                              @"ISO-8859-14", @"ISO-8859-15", @"ISO-8859-16", @"UTF-7", @"UTF-8", @"UTF-16", @"UTF-16LE",
                              @"UTF-16BE", @"UTF-32", @"UTF-32LE", @"UTF-32BE"];

        // populate the languages array
        NSMutableArray *languages = [[NSMutableArray alloc] init];
        for (const iso639_lang_t *lang = lang_get_next(NULL); lang != NULL; lang = lang_get_next(lang))
        {
            [languages addObject:@(lang->eng_name)];
        }
        _languagesArray = [languages copy];
    }
}

- (instancetype)initWithTrackIdx:(NSUInteger)index
                       container:(int)container
                      dataSource:(id<HBSubtitlesTrackDataSource>)dataSource
                        delegate:(id<HBSubtitlesTrackDelegate>)delegate
{
    self = [super init];
    if (self)
    {
        _dataSource = dataSource;
        _sourceTrackIdx = index;
        _container = container;
        _title = [dataSource sourceTrackAtIndex:_sourceTrackIdx].title;

        [self validateSettings];

        _delegate = delegate;
    }

    return self;
}

- (void)validateSettings
{
    HBTitleSubtitlesTrack *sourceTrack = [_dataSource sourceTrackAtIndex:_sourceTrackIdx];
    self.type = sourceTrack.type;

    if (_sourceTrackIdx == 0)
    {
        return;
    }

    if (!hb_subtitle_can_burn(_type))
    {
        // the source track cannot be burned in, so uncheck the widget
        self.burnedIn = NO;
    }

    if (!hb_subtitle_can_force(_type))
    {
        // the source track does not support forced flags, so uncheck the widget
        self.forcedOnly = NO;
    }

    if (!hb_subtitle_can_pass(self.type, self.container))
    {
        self.burnedIn = YES;
    }

    if (_sourceTrackIdx == 1)
    {
        self.forcedOnly = YES;
        self.burnedIn = YES;
    }

    // check to see if we are an srt, in which case set our file path and source track type kvp's
    if (_type == IMPORTSRT || _type == IMPORTSSA)
    {
        self.fileURL = sourceTrack.fileURL;
        self.isoLanguage = @"eng";
        self.charCode = charEncodingArray[CHAR_CODE_DEFAULT_INDEX];
    }
    else
    {
        self.fileURL = nil;
        self.isoLanguage = nil;
        self.charCode = nil;
    }
}

#pragma mark - Track properties

- (void)setSourceTrackIdx:(NSUInteger)sourceTrackIdx
{
    if (sourceTrackIdx != _sourceTrackIdx)
    {
        [[self.undo prepareWithInvocationTarget:self] setSourceTrackIdx:_sourceTrackIdx];
    }

    NSUInteger oldIdx = _sourceTrackIdx;
    _sourceTrackIdx = sourceTrackIdx;

    if (!(self.undo.isUndoing || self.undo.isRedoing))
    {
        [self validateSettings];

        self.title = [self.dataSource defaultTitleForTrackAtIndex:_sourceTrackIdx];

        if (oldIdx != sourceTrackIdx)
        {
            [self.delegate track:self didChangeSourceFrom:oldIdx];
        }
    }
}

- (void)setType:(int)type
{
    if (type != _type)
    {
        [[self.undo prepareWithInvocationTarget:self] setType:_type];
    }
    _type = type;
}

- (void)setForcedOnly:(BOOL)forcedOnly
{
    if (forcedOnly != _forcedOnly)
    {
        [[self.undo prepareWithInvocationTarget:self] setForcedOnly:_forcedOnly];
    }
    _forcedOnly = forcedOnly;
}

- (void)setBurnedIn:(BOOL)burnedIn
{
    if (burnedIn != _burnedIn)
    {
        [[self.undo prepareWithInvocationTarget:self] setBurnedIn:_burnedIn];
    }
    _burnedIn = burnedIn;
    if (!(self.undo.isUndoing || self.undo.isRedoing) || !self.validating)
    {
        self.validating = YES;
        if (_burnedIn)
        {
            [self.delegate didSetBurnedInOption:self];
            self.def = NO;
        };
        self.validating = NO;
    }
}

- (BOOL)validateBurnedIn:(id *)ioValue error:(NSError * __autoreleasing *)outError
{
    BOOL retval = YES;

    if (nil != *ioValue)
    {
        BOOL value = [*ioValue boolValue];
        if (value && [self.delegate canSetBurnedInOption:self] == NO)
        {
            *ioValue = @NO;
        }
    }

    return retval;
}

- (void)setDef:(BOOL)def
{
    if (def != _def)
    {
        [[self.undo prepareWithInvocationTarget:self] setDef:_def];
    }
    _def = def;
    if (!(self.undo.isUndoing || self.undo.isRedoing) || !self.validating)
    {
        self.validating = YES;
        if (_def)
        {
            [self.delegate didSetDefaultOption:self];
            self.burnedIn = NO;
        }
        self.validating = NO;
    }
}

#pragma mark - External subtitles track only properties

- (void)setFileURL:(NSURL *)fileURL
{
    if (fileURL != _fileURL)
    {
        [[self.undo prepareWithInvocationTarget:self] setFileURL:_fileURL];
    }
    _fileURL = [fileURL copy];
}

- (void)setIsoLanguage:(NSString *)isoLanguage
{
    if (_isoLanguage != isoLanguage || (_isoLanguage && ![isoLanguage isEqualToString:_isoLanguage]))
    {
        [[self.undo prepareWithInvocationTarget:self] setIsoLanguage:_isoLanguage];
    }
    _isoLanguage = [isoLanguage copy];
}

- (void)setCharCode:(NSString *)charCode
{
    if (_charCode != charCode || (_charCode && ![charCode isEqualToString:_charCode]))
    {
        [[self.undo prepareWithInvocationTarget:self] setCharCode:_charCode];
    }
    _charCode = [charCode copy];
}

- (void)setOffset:(int)offset
{
    if (offset != _offset)
    {
        [[self.undo prepareWithInvocationTarget:self] setOffset:_offset];
    }
    _offset = offset;
}

#pragma mark -

- (NSArray *)sourceTracksArray
{
    return [self.dataSource sourceTracksArray];
}

- (BOOL)isExternal
{
    return self.type == IMPORTSRT || self.type == IMPORTSSA;
}

- (BOOL)isEnabled
{
    return self.sourceTrackIdx != 0;
}

- (BOOL)isForcedSupported
{
    return hb_subtitle_can_force(self.type) && self.isEnabled;
}

- (BOOL)canPassthru
{
    return hb_subtitle_can_pass(self.type, self.container) && self.isEnabled;
}

- (NSArray<NSString *> *)languages
{
    return _languagesArray;
}
- (NSArray<NSString *> *)encodings
{
    return charEncodingArray;
}

#pragma mark - KVO

+ (NSSet *)keyPathsForValuesAffectingValueForKey:(NSString *)key

{
    NSSet *retval = nil;

    if ([key isEqualToString: @"isExternal"])
    {
        retval = [NSSet setWithObjects: @"type", nil];
    }
    else if ([key isEqualToString: @"isEnabled"])
    {
        retval = [NSSet setWithObjects: @"sourceTrackIdx", nil];
    }
    else if ([key isEqualToString: @"canPassthru"] || [key isEqualToString: @"isForcedSupported"] )
    {
        retval = [NSSet setWithObjects: @"isEnabled", @"sourceTrackIdx", @"container", nil];
    }
    else
    {
        retval = [NSSet set];
    }

    return retval;
}

- (void)setNilValueForKey:(NSString *)key
{
    if ([key isEqualToString:@"offset"])
    {
        [self setValue:@0 forKey:key];
    }
}

#pragma mark - NSCopying

- (instancetype)copyWithZone:(NSZone *)zone
{
    HBSubtitlesTrack *copy = [[[self class] alloc] init];

    if (copy)
    {
        copy->_sourceTrackIdx = _sourceTrackIdx;
        copy->_type = _type;
        copy->_container = _container;

        copy->_forcedOnly = _forcedOnly;
        copy->_burnedIn = _burnedIn;
        copy->_def = _def;

        copy->_fileURL = [_fileURL copy];
        copy->_isoLanguage = [_isoLanguage copy];
        copy->_charCode = [_charCode copy];
        copy->_offset = _offset;

        copy->_title = [_title copy];
    }

    return copy;
}

#pragma mark - NSCoding

+ (BOOL)supportsSecureCoding
{
    return YES;
}

- (void)encodeWithCoder:(NSCoder *)coder
{
    [coder encodeInt:2 forKey:@"HBSubtitlesTrackVersion"];

    encodeInteger(_sourceTrackIdx);
    encodeInt(_type);
    encodeInt(_container);

    encodeBool(_forcedOnly);
    encodeBool(_burnedIn);
    encodeBool(_def);
    encodeObject(_title);

    encodeObject(_fileURL);
    encodeObject(_isoLanguage);
    encodeObject(_charCode);
    encodeInt(_offset);
}

- (instancetype)initWithCoder:(NSCoder *)decoder
{
    self = [super init];

    decodeInteger(_sourceTrackIdx); if (_sourceTrackIdx < 0) { goto fail; }
    decodeInt(_type); if (_type < VOBSUB || _type > DVBSUB) { goto fail; }
    decodeInt(_container); if (_container != HB_MUX_MP4 && _container != HB_MUX_MKV && _container != HB_MUX_WEBM) { goto fail; }

    decodeBool(_forcedOnly);
    decodeBool(_burnedIn);
    decodeBool(_def);
    decodeObject(_title, NSString);

    decodeObject(_fileURL, NSURL);
    decodeObject(_isoLanguage, NSString);
    decodeObject(_charCode, NSString);
    decodeInt(_offset);

    return self;

fail:
    return nil;
}

@end

#pragma mark - Value Transformers

@implementation HBIsoLanguageTransformer

+ (Class)transformedValueClass
{
    return [NSString class];
}

- (id)transformedValue:(id)value
{
    if ([value length])
    {
        iso639_lang_t *lang = lang_for_code2([value UTF8String]);
        if (lang)
        {
            return @(lang->eng_name);
        }
    }
    return nil;
}

+ (BOOL)allowsReverseTransformation
{
    return YES;
}

- (id)reverseTransformedValue:(id)value
{
    if ([value length])
    {
        iso639_lang_t *lang = lang_for_english([value UTF8String]);
        if (lang)
        {
            return @(lang->iso639_2);
        }
    }
    return nil;
}

@end
