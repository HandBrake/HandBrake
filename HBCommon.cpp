/* $Id: HBCommon.cpp,v 1.6 2003/08/24 21:56:03 titer Exp $ */

#include "HBCommon.h"
#include "HBFifo.h"
#include "HBMpegDemux.h"
#include "HBPictureWin.h"
#include "HBWindow.h"

#include <Application.h>

#include <dvdread/ifo_types.h>
#include <dvdplay/dvdplay.h>
#include <dvdplay/info.h>
#include <dvdplay/state.h>
#include <dvdplay/nav.h>

extern "C" {
#include <mpeg2dec/mpeg2.h>
}

void Log( char * log, ... )
{
    char * string = (char*) malloc( 1024 );
    
    /* Show the time */
    time_t _now = time( NULL );
    struct tm * now = localtime( &_now );
    sprintf( string, "[%02d:%02d:%02d] ",
             now->tm_hour, now->tm_min, now->tm_sec );

    /* Convert the message to a string */
    va_list args;
    va_start( args, log );
    int ret = vsnprintf( string + 11, 1011, log, args );
    va_end( args );
    
    /* Add the end of line */
    string[ret+11] = '\n';
    string[ret+12] = '\0';
    
    /* Send this to the be_app */
    /* We do this so we are sure that only one message is printed
       at a time */
    BMessage * message = new BMessage( PRINT_MESSAGE );
    message->AddPointer( "string", string );
    be_app->PostMessage( message );
    delete message;
}

void Status( char * text, float pos, int mode )
{
    char * textCopy = strdup( text );
    BMessage * message = new BMessage( CHANGE_STATUS );
    message->AddPointer( "text", textCopy );
    message->AddFloat( "pos", pos );
    message->AddInt32( "mode", mode );
    be_app->PostMessage( message );
    delete message;
}

/* Get a readable language description from the code */
iso639_lang_t languages[] =
{ { "Afar", "", "aa" },
  { "Abkhazian", "", "ab" },
  { "Afrikaans", "", "af" },
  { "Albanian", "", "sq" },
  { "Amharic", "", "am" },
  { "Arabic", "", "ar" },
  { "Armenian", "", "hy" },
  { "Assamese", "", "as" },
  { "Avestan", "", "ae" },
  { "Aymara", "", "ay" },
  { "Azerbaijani", "", "az" },
  { "Bashkir", "", "ba" },
  { "Basque", "", "eu" },
  { "Belarusian", "", "be" },
  { "Bengali", "", "bn" },
  { "Bihari", "", "bh" },
  { "Bislama", "", "bi" },
  { "Bosnian", "", "bs" },
  { "Breton", "", "br" },
  { "Bulgarian", "", "bg" },
  { "Burmese", "", "my" },
  { "Catalan", "", "ca" },
  { "Chamorro", "", "ch" },
  { "Chechen", "", "ce" },
  { "Chinese", "", "zh" },
  { "Church Slavic", "", "cu" },
  { "Chuvash", "", "cv" },
  { "Cornish", "", "kw" },
  { "Corsican", "", "co" },
  { "Czech", "", "cs" },
  { "Danish", "Dansk", "da" },
  { "Dutch", "Nederlands", "nl" },
  { "Dzongkha", "", "dz" },
  { "English", "English", "en" },
  { "Esperanto", "", "eo" },
  { "Estonian", "", "et" },
  { "Faroese", "", "fo" },
  { "Fijian", "", "fj" },
  { "Finnish", "Suomi", "fi" },
  { "French", "Francais", "fr" },
  { "Frisian", "", "fy" },
  { "Georgian", "", "ka" },
  { "German", "Deutsch", "de" },
  { "Gaelic (Scots)", "", "gd" },
  { "Irish", "", "ga" },
  { "Gallegan", "", "gl" },
  { "Manx", "", "gv" },
  { "Greek, Modern ()", "", "el" },
  { "Guarani", "", "gn" },
  { "Gujarati", "", "gu" },
  { "Hebrew", "", "he" },
  { "Herero", "", "hz" },
  { "Hindi", "", "hi" },
  { "Hiri Motu", "", "ho" },
  { "Hungarian", "Magyar", "hu" },
  { "Icelandic", "Islenska", "is" },
  { "Inuktitut", "", "iu" },
  { "Interlingue", "", "ie" },
  { "Interlingua", "", "ia" },
  { "Indonesian", "", "id" },
  { "Inupiaq", "", "ik" },
  { "Italian", "Italiano", "it" },
  { "Javanese", "", "jv" },
  { "Japanese", "", "ja" },
  { "Kalaallisut (Greenlandic)", "", "kl" },
  { "Kannada", "", "kn" },
  { "Kashmiri", "", "ks" },
  { "Kazakh", "", "kk" },
  { "Khmer", "", "km" },
  { "Kikuyu", "", "ki" },
  { "Kinyarwanda", "", "rw" },
  { "Kirghiz", "", "ky" },
  { "Komi", "", "kv" },
  { "Korean", "", "ko" },
  { "Kuanyama", "", "kj" },
  { "Kurdish", "", "ku" },
  { "Lao", "", "lo" },
  { "Latin", "", "la" },
  { "Latvian", "", "lv" },
  { "Lingala", "", "ln" },
  { "Lithuanian", "", "lt" },
  { "Letzeburgesch", "", "lb" },
  { "Macedonian", "", "mk" },
  { "Marshall", "", "mh" },
  { "Malayalam", "", "ml" },
  { "Maori", "", "mi" },
  { "Marathi", "", "mr" },
  { "Malay", "", "ms" },
  { "Malagasy", "", "mg" },
  { "Maltese", "", "mt" },
  { "Moldavian", "", "mo" },
  { "Mongolian", "", "mn" },
  { "Nauru", "", "na" },
  { "Navajo", "", "nv" },
  { "Ndebele, South", "", "nr" },
  { "Ndebele, North", "", "nd" },
  { "Ndonga", "", "ng" },
  { "Nepali", "", "ne" },
  { "Norwegian", "Norsk", "no" },
  { "Norwegian Nynorsk", "", "nn" },
  { "Norwegian Bokmål", "", "nb" },
  { "Chichewa; Nyanja", "", "ny" },
  { "Occitan (post 1500); Provençal", "", "oc" },
  { "Oriya", "", "or" },
  { "Oromo", "", "om" },
  { "Ossetian; Ossetic", "", "os" },
  { "Panjabi", "", "pa" },
  { "Persian", "", "fa" },
  { "Pali", "", "pi" },
  { "Polish", "", "pl" },
  { "Portuguese", "Portugues", "pt" },
  { "Pushto", "", "ps" },
  { "Quechua", "", "qu" },
  { "Raeto-Romance", "", "rm" },
  { "Romanian", "", "ro" },
  { "Rundi", "", "rn" },
  { "Russian", "", "ru" },
  { "Sango", "", "sg" },
  { "Sanskrit", "", "sa" },
  { "Serbian", "", "sr" },
  { "Croatian", "Hrvatski", "hr" },
  { "Sinhalese", "", "si" },
  { "Slovak", "", "sk" },
  { "Slovenian", "", "sl" },
  { "Northern Sami", "", "se" },
  { "Samoan", "", "sm" },
  { "Shona", "", "sn" },
  { "Sindhi", "", "sd" },
  { "Somali", "", "so" },
  { "Sotho, Southern", "", "st" },
  { "Spanish", "Espanol", "es" },
  { "Sardinian", "", "sc" },
  { "Swati", "", "ss" },
  { "Sundanese", "", "su" },
  { "Swahili", "", "sw" },
  { "Swedish", "Svenska", "sv" },
  { "Tahitian", "", "ty" },
  { "Tamil", "", "ta" },
  { "Tatar", "", "tt" },
  { "Telugu", "", "te" },
  { "Tajik", "", "tg" },
  { "Tagalog", "", "tl" },
  { "Thai", "", "th" },
  { "Tibetan", "", "bo" },
  { "Tigrinya", "", "ti" },
  { "Tonga (Tonga Islands)", "", "to" },
  { "Tswana", "", "tn" },
  { "Tsonga", "", "ts" },
  { "Turkish", "", "tr" },
  { "Turkmen", "", "tk" },
  { "Twi", "", "tw" },
  { "Uighur", "", "ug" },
  { "Ukrainian", "", "uk" },
  { "Urdu", "", "ur" },
  { "Uzbek", "", "uz" },
  { "Vietnamese", "", "vi" },
  { "Volapük", "", "vo" },
  { "Welsh", "", "cy" },
  { "Wolof", "", "wo" },
  { "Xhosa", "", "xh" },
  { "Yiddish", "", "yi" },
  { "Yoruba", "", "yo" },
  { "Zhuang", "", "za" },
  { "Zulu", "", "zu" },
  { NULL, NULL, NULL } };

char * LanguageForCode( int code )
{
    char codeString[2];
    codeString[0] = ( code >> 8 ) & 0xFF;
    codeString[1] = code & 0xFF;
    
    iso639_lang_t * lang;
    for( lang = languages; lang->engName; lang++ )
    {
        if( !strncmp( lang->iso639_1, codeString, 2 ) )
        {
            if( *lang->nativeName )
                return lang->nativeName;

            return lang->engName;
        }
    }
    
    return "Unknown";
}

HBVolumeInfo::HBVolumeInfo( char * name, char * device )
    : BMenuItem( "", new BMessage( VOLUME_SELECTED ) )
{
    fInitOK    = false;
    fName      = strdup( name );
    fDevice    = strdup( device );
    fTitleList = new BList();

    SetLabel( fName );
    
    /* Open the device */
    dvdplay_ptr vmg;
    vmg = dvdplay_open( device, NULL, NULL );
    if( !vmg )
    {
        Log( "VolumeInfo::DetectTitles: dvdplay_open() failed (%s)",
             device );
        return;
    }
    
    /* Detect titles */
    HBTitleInfo * titleInfo;
    for( int i = 0; i < dvdplay_title_nr( vmg ); i++ )
    {
        Log( "HBVolumeInfo : new title (%d)", i + 1 );
        
        char statusText[128]; memset( statusText, 0, 128 );
        snprintf( statusText, 128,
                  "Checking DVD volumes (%s, title %d)...",
                  fName, i + 1 );
        Status( statusText, 0.0, ENABLE_DETECTING );

        titleInfo = new HBTitleInfo( vmg, i + 1, device );
        
        if( !titleInfo->InitCheck() )
        {
            delete titleInfo;
            continue;
        }
        
        fTitleList->AddItem( titleInfo );
    }
    
    dvdplay_close( vmg );
    
    if( fTitleList->CountItems() > 0 )
    {
        fInitOK = true;
    }

}

HBVolumeInfo::~HBVolumeInfo()
{
    free( fName );
    free( fDevice );
    
    HBTitleInfo * titleInfo;
    while( ( titleInfo = (HBTitleInfo*) fTitleList->ItemAt( 0 ) ) )
    {
        fTitleList->RemoveItem( titleInfo );
        delete titleInfo;
    }
    delete fTitleList;
}

bool HBVolumeInfo::InitCheck()
{
    return fInitOK;
}

HBTitleInfo::HBTitleInfo( dvdplay_ptr vmg, int index, char * device )
    : BMenuItem( "", new BMessage( TITLE_SELECTED ) )
{
    fInitOK         = false;
    fDevice         = strdup( device );
    fIndex          = index;

    fAudioInfoList1 = new BList();
    fAudioInfoList2 = new BList();
    fPSFifo         = NULL;
    fMpeg2Fifo      = NULL;
    fRawFifo        = NULL;
    fMpeg4Fifo      = NULL;

    dvdplay_start( vmg, fIndex );
    
    /* Length */
    fLength = dvdplay_title_time( vmg );
    
    /* Discard titles under 60 seconds */
    if( fLength < 60 )
    {
        Log( "HBTitleInfo : skipping title %d (length = %lld sec)",
             index, fLength );
        return;
    }

    char label[1024]; memset( label, 0, 1024 );
    sprintf( label, "Title %d (%02lld:%02lld:%02lld)",
             index, fLength / 3600, ( fLength % 3600 ) / 60,
             fLength % 60 );
    SetLabel( label );
    
    /* Detect languages */
    int audio_nr, audio;
    dvdplay_audio_info( vmg, &audio_nr, &audio );
    
    audio_attr_t * attr;
    HBAudioInfo * audioInfo;
    for( int i = 0; i < audio_nr; i++ )
    {
        int id = dvdplay_audio_id( vmg, i );
        if( id > 0 )
        {
            Log( "HBTitleInfo : new language (%x)", id );
            attr = dvdplay_audio_attr( vmg, i );
            audioInfo = new HBAudioInfo( id, LanguageForCode( attr->lang_code ) );
            fAudioInfoList1->AddItem( audioInfo );
        }
    }
    
    /* Discard titles with no audio tracks */
    if( !fAudioInfoList1->CountItems() )
    {
        Log( "HBTitleInfo : skipping title %d (no audio track)", index );
        return;
    }
    
    /* Add a dummy 'None' language */
    audioInfo = new HBAudioInfo( 0, "None" );
    fAudioInfoList1->AddItem( audioInfo );
    
    /* Duplicate the audio list */
    for( int i = 0; i < fAudioInfoList1->CountItems(); i++ )
    {
        audioInfo = (HBAudioInfo*) fAudioInfoList1->ItemAt( i );
        fAudioInfoList2->AddItem( new HBAudioInfo( audioInfo ) );
    }
    
    /* Decode a few pictures so the user can crop/resize it */
    int titleFirst = dvdplay_title_first ( vmg );
    int titleEnd   = dvdplay_title_end( vmg );

    /* Kludge : libdvdplay wants we to read a first block before seeking */
    uint8_t dummyBuf[2048];
    dvdplay_read( vmg, dummyBuf, 1 );

    for( int i = 0; i < 10; i++ )
    {
        dvdplay_seek( vmg, ( i + 1 ) * ( titleEnd - titleFirst ) / 11 ) ;
        if( !DecodeFrame( vmg, i ) )
        {
            Log( "HBTitleInfo::HBTitleInfo : could not decode frame %d", i );
            return;
        }
    }
    
    fPictureWin = new HBPictureWin( this );

    fInitOK = true;
}

HBTitleInfo::~HBTitleInfo()
{
    HBAudioInfo * audioInfo;
    
    while( ( audioInfo = (HBAudioInfo*) fAudioInfoList1->ItemAt( 0 ) ) )
    {
        fAudioInfoList1->RemoveItem( audioInfo );
        delete audioInfo;
    }
    delete fAudioInfoList1;
    
    while( ( audioInfo = (HBAudioInfo*) fAudioInfoList2->ItemAt( 0 ) ) )
    {
        fAudioInfoList2->RemoveItem( audioInfo );
        delete audioInfo;
    }
    delete fAudioInfoList2;
}

bool HBTitleInfo::DecodeFrame( dvdplay_ptr vmg, int i )
{
    /* Init libmpeg2 */
    mpeg2dec_t         * handle = mpeg2_init();
    const mpeg2_info_t * info   = mpeg2_info( handle );
    mpeg2_state_t        state;
    
    BList    * esBufferList = NULL;
    HBBuffer * psBuffer     = NULL;
    HBBuffer * esBuffer     = NULL;
    
    for( ;; )
    {
        state = mpeg2_parse( handle );
        
        if( state == STATE_BUFFER )
        {
            /* Free the previous buffer */
            if( esBuffer )
            {
                delete esBuffer;
                esBuffer = NULL;
            }
            
            /* Get a new one */
            while( !esBuffer )
            {
                while( !esBufferList )
                {
                    psBuffer = new HBBuffer( DVD_VIDEO_LB_LEN );
                    if( dvdplay_read( vmg, psBuffer->fData, 1 ) != 1 )
                    {
                        Log( "dvdplay_read failed" );
                    }
                    esBufferList = PStoES( psBuffer );
                }

                esBuffer = (HBBuffer*) esBufferList->ItemAt( 0 );
                esBufferList->RemoveItem( esBuffer );
                if( !esBufferList->CountItems() )
                {
                    delete esBufferList;
                    esBufferList = NULL;
                }
                
                if( esBuffer->fStreamId != 0xE0 )
                {
                    delete esBuffer;
                    esBuffer = NULL;
                }
            }
            
            /* Feed libmpeg2 */
            mpeg2_buffer( handle, esBuffer->fData,
                          esBuffer->fData + esBuffer->fSize );
        }
        else if( state == STATE_SEQUENCE )
        {
            /* Get size & framerate info */
            fInWidth     = info->sequence->width;
            fInHeight    = info->sequence->height;
            fPixelWidth  = info->sequence->pixel_width;
            fPixelHeight = info->sequence->pixel_height;
            fPictures[i] = (uint8_t*) malloc( 3 * fInWidth * fInHeight / 2 );
            fRate        = 27000000;
            fScale       = info->sequence->frame_period;
        }
        else if( ( state == STATE_SLICE || state == STATE_END ) &&
                 ( info->display_fbuf ) &&
                 ( info->display_picture->flags & PIC_MASK_CODING_TYPE )
                     == PIC_FLAG_CODING_TYPE_I )
        {
            /* Copy it */
            /* TODO : make libmpeg2 write directly in our buffer */
            memcpy( fPictures[i],
                    info->display_fbuf->buf[0],
                    fInWidth * fInHeight );
            memcpy( fPictures[i] + fInWidth * fInHeight,
                    info->display_fbuf->buf[1],
                    fInWidth * fInHeight / 4 );
            memcpy( fPictures[i] + 5 * fInWidth * fInHeight / 4,
                    info->display_fbuf->buf[2],
                    fInWidth * fInHeight / 4 );
            break;
        }
        else if( state == STATE_INVALID )
        {
            /* Reset libmpeg2 */
            mpeg2_close( handle );
            handle = mpeg2_init();
        }
    }
    
    mpeg2_close( handle );
    
    return true;
}

bool HBTitleInfo::InitCheck()
{
    return fInitOK;
}

/* Audio track */
HBAudioInfo::HBAudioInfo( int id, char * description )
    : BMenuItem( "", new BMessage( LANGUAGE_SELECTED ) )
{
    fId            = id;
    fOutSampleRate = 44100;
    
    fAc3Fifo = NULL;
    fRawFifo = NULL;
    fMp3Fifo = NULL;
    
    SetLabel( description );
}

HBAudioInfo::HBAudioInfo( HBAudioInfo * audioInfo )
    : BMenuItem( "", new BMessage( LANGUAGE_SELECTED ) )
{
    fId            = audioInfo->fId;
    fInSampleRate  = audioInfo->fInSampleRate;
    fOutSampleRate = audioInfo->fOutSampleRate;
    fInBitrate     = audioInfo->fInBitrate;
    fOutBitrate    = audioInfo->fOutBitrate;
    
    fAc3Fifo = NULL;
    fRawFifo = NULL;
    fMp3Fifo = NULL;
    
    SetLabel( audioInfo->Label() );
}

HBAudioInfo::~HBAudioInfo()
{
}
