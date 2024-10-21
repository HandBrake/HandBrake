/* lang.c

   Copyright (c) 2003-2024 HandBrake Team
   This file is part of the HandBrake source code
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License v2.
   For full terms see the file COPYING file or visit http://www.gnu.org/licenses/gpl-2.0.html
 */

#include "handbrake/lang.h"
#include <string.h>
#include <ctype.h>

// Fake iso639 entry to deal with selection of "any" language
static iso639_lang_t lang_any =
  { "Any", "", "yy", "any" };

static const iso639_lang_t languages[] =
{ { "Unknown", "", "", "und" },
  { "Afar", "Qafar", "aa", "aar" },
  { "Abkhaz", "Аԥсуа бызшәа", "ab", "abk" },
  { "Afrikaans", "Afrikaans", "af", "afr" },
  { "Akan", "Akan", "ak", "aka" },
  { "Albanian", "shqip", "sq", "sqi", "alb" },
  { "Amharic", "አማርኛ", "am", "amh" },
  { "Arabic", "العربية", "ar", "ara" },
  { "Aragonese", "aragonés", "an", "arg" },
  { "Armenian", "Հայերեն", "hy", "hye", "arm" },
  { "Assamese", "অসমীয়া", "as", "asm" },
  { "Avar", "магӏарул мацӏ", "av", "ava" },
  { "Avestan", "", "ae", "ave" },
  { "Aymara", "Aymar aru", "ay", "aym" },
  { "Azerbaijani", "Azərbaycan­ılı", "az", "aze" },
  { "Bashkir", "Башҡорт", "ba", "bak" },
  { "Bambara", "Bamana", "bm", "bam" },
  { "Basque", "euskara", "eu", "eus", "baq" },
  { "Belarusian", "Беларуская", "be", "bel" },
  { "Bengali", "Bangla", "bn", "ben" },
  { "Bihari", "", "bh", "bih" },
  { "Bislama", "Bislama", "bi", "bis" },
  { "Bosnian", "bosanski", "bs", "bos" },
  { "Breton", "brezhoneg", "br", "bre" },
  { "Bulgarian", "български", "bg", "bul" },
  { "Burmese", "ဗမာ", "my", "mya", "bur" },
  { "Catalan", "català", "ca", "cat" },
  { "Chamorro", "Finu' Chamorro", "ch", "cha" },
  { "Chechen", "Нохчийн Мотт", "ce", "che" },
  { "Chinese", "中文", "zh", "zho", "chi" },
  { "Church Slavonic", "", "cu", "chu" },
  { "Chuvash", "Чӑвашла", "cv", "chv" },
  { "Cornish", "kernewek", "kw", "cor" },
  { "Corsican", "Corsu", "co", "cos" },
  { "Cree", "", "cr", "cre" },
  { "Croatian", "hrvatski", "hr", "hrv" },
  // Deprecated version of Croatian code 'scr'
  // When used, HB will record the latest iso639_2 'hrv' as the code in titles
  // and write the correct code to the output since the line above that has
  // no 2b variant will be used when generating output.
  { "Croatian", "hrvatski", NULL, "hrv", "scr" },
  { "Czech", "čeština", "cs", "ces", "cze" },
  { "Danish", "dansk", "da", "dan" },
  { "Divehi", "ދިވެހިބަސް", "dv", "div" },
  { "Dutch", "Nederlands", "nl", "nld", "dut" },
  { "Dzongkha", "རྫོང་ཁ", "dz", "dzo" },
  { "English", "English", "en", "eng" },
  { "Esperanto", "esperanto", "eo", "epo" },
  { "Estonian", "eesti", "et", "est" },
  { "Ewe", "eʋegbe", "ee", "ewe" },
  { "Faroese", "føroyskt", "fo", "fao" },
  { "Fijian", "Na Vosa Vakaviti", "fj", "fij" },
  { "Finnish", "suomi", "fi", "fin" },
  { "French", "Francais", "fr", "fra", "fre" },
  { "Filipino", "Wikang Filipino", NULL, "fil" },
  { "Western Frisian", "Frysk", "fy", "fry" },
  { "Fulah", "Fulah", "ff", "ful" },
  { "Georgian", "ქართული", "ka", "kat", "geo" },
  { "German", "Deutsch", "de", "deu", "ger" },
  { "Gaelic (Scots)", "Gàidhlig", "gd", "gla" },
  { "Galician", "galego", "gl", "glg" },
  { "Manx", "Gaelg", "gv", "glv" },
  { "Greek, Modern", "Ελληνικά", "el", "ell", "gre" },
  { "Greenlandic", "Kalaallisut", "kl", "kal" },
  { "Guarani", "Avañe’ẽ", "gn", "grn" },
  { "Gujarati", "ગુજરાતી", "gu", "guj" },
  { "Haitian", "", "ht", "hat" },
  { "Hausa", "Hausa", "ha", "hau" },
  { "Hebrew", "עברית", "he", "heb", "", "iw" },
  { "Herero", "Otjiherero", "hz", "her" },
  { "Hindi", "हिंदी", "hi", "hin" },
  { "Hiri Motu", "", "ho", "hmo" },
  { "Hungarian", "magyar", "hu", "hun" },
  { "Igbo", "Igbo", "ig", "ibo" },
  { "Icelandic", "íslenska", "is", "isl", "ice" },
  { "Ido", "", "io", "ido" },
  { "Inuktitut", "Inuktitut", "iu", "iku" },
  { "Interlingue", "", "ie", "ile" },
  { "Interlingua", "", "ia", "ina" },
  { "Indonesian", "Bahasa Indonesia", "id", "ind", "", "in" },
  { "Inupiaq", "", "ik", "ipk" },
  { "Irish", "Gaeilge", "ga", "gle" },
  { "Italian", "italiano", "it", "ita" },
  { "Javanese", "Basa Jawa", "jv", "jav", "", "jw" },
  { "Japanese", "日本語", "ja", "jpn" },
  { "Kannada", "", "kn", "kan" },
  { "Kashmiri", "کٲشُر", "ks", "kas" },
  { "Kanuri", "Kanuri", "kr", "kau" },
  { "Kazakh", "қазақ тілі", "kk", "kaz" },
  { "Central Khmer", "ភាសាខ្មែរ", "km", "khm" },
  { "Kikuyu", "Gikuyu", "ki", "kik" },
  { "Kinyarwanda", "Kinyarwanda", "rw", "kin" },
  { "Kirghiz", "قىرعىزچا", "ky", "kir" },
  { "Komi", "Коми кыв", "kv", "kom" },
  { "Kongo", "Kikongo", "kg", "kon" },
  { "Korean", "한국어", "ko", "kor" },
  { "Kuanyama", "Oshikwanyama", "kj", "kua" },
  { "Kurdish", "کوردی", "ku", "kur" },
  { "Lao", "ລາວ", "lo", "lao" },
  { "Latin", "lingua latīna", "la", "lat" },
  { "Latvian", "latviešu", "lv", "lav" },
  { "Limburgish", "Limburgs", "li", "lim" },
  { "Lingala", "lingála", "ln", "lin" },
  { "Lithuanian", "lietuvių", "lt", "lit" },
  { "Luxembourgish", "Lëtzebuergesch", "lb", "ltz" },
  { "Luba-Katanga", "Tshiluba", "lu", "lub" },
  { "Ganda", "Luganda", "lg", "lug" },
  { "Macedonian", "македонски", "mk", "mkd", "mac" },
  { "Marshallese", "Kajin M̧ajeļ", "mh", "mah" },
  { "Malayalam", "മലയാളം", "ml", "mal" },
  { "Maori", "Reo Māori", "mi", "mri", "mao" },
  { "Marathi", "मराठी", "mr", "mar" },
  { "Malay", "Bahasa Melayu", "ms", "msa", "may" },
  { "Malagasy", "Malagasy", "mg", "mlg" },
  { "Maltese", "Malti", "mt", "mlt" },
  { "Moldavian", "limba moldovenească", "mo", "mol" },
  { "Mongolian", "Монгол хэл", "mn", "mon" },
  { "Nauruan", "dorerin Naoero", "na", "nau" },
  { "Navajo", "Diné bizaad", "nv", "nav" },
  { "Ndebele, South", "Nrebele", "nr", "nbl" },
  { "Ndebele, North", "isiNdebele", "nd", "nde" },
  { "Ndonga", "Oshindonga", "ng", "ndo" },
  { "Nepali", "नेपाली", "ne", "nep" },
  { "Norwegian Nynorsk", "nynorsk", "nn", "nno" },
  { "Norwegian Bokmål", "norsk bokmål", "nb", "nob" },
  { "Norwegian", "norsk", "no", "nor" },
  { "Nuosu", "ꆈꌠ꒿", "ii", "iii" },
  { "Chewa", "Nyanja", "ny", "nya" },
  { "Occitan (post 1500); Provençal", "Occitan", "oc", "oci" },
  { "Ojibwe", "ᐊᓂᔑᓈᐯᒧᐎᓐ", "oj", "oji" },
  { "Oriya", "ଓଡ଼ିଆ", "or", "ori" },
  { "Oromo", "Oromoo", "om", "orm" },
  { "Ossetian; Ossetic", "Ирон æвзаг", "os", "oss" },
  { "Persian", "فارسی", "fa", "fas", "per" },
  { "Pali", "Pāli", "pi", "pli" },
  { "Polish", "polski", "pl", "pol" },
  { "Portuguese", "Portugues", "pt", "por" },
  { "Punjabi", "ਪੰਜਾਬੀ", "pa", "pan" },
  { "Pashto", "پښتو", "ps", "pus" },
  { "Quechua", "runasimi", "qu", "que" },
  { "Romanian", "română", "ro", "ron", "rum" },
  { "Romansh", "rumantsch", "rm", "roh" },
  { "Rundi", "Ikirundi", "rn", "run" },
  { "Russian", "русский", "ru", "rus" },
  { "Samoan", "Gagana fa'a Sāmoa", "sm", "smo" },
  { "Sango", "Sängö", "sg", "sag" },
  { "Sanskrit", "संस्कृत", "sa", "san" },
  { "Sardinian", "sardu", "sc", "srd" },
  { "Serbian", "srpski", "sr", "srp", "scc" },
  { "Sinhala", "සිංහල", "si", "sin" },
  { "Slovak", "slovenčina", "sk", "slk", "slo" },
  { "Slovenian", "slovenščina", "sl", "slv" },
  { "Northern Sami", "davvisámegiella", "se", "sme" },
  { "Shona", "chiShona", "sn", "sna" },
  { "Sindhi", "سنڌي", "sd", "snd" },
  { "Somali", "Soomaali", "so", "som" },
  { "Sotho, Southern", "Sesotho", "st", "sot" },
  { "Spanish", "español", "es", "spa" },
  { "Swati", "Siswati", "ss", "ssw" },
  { "Sundanese", "Basa Sunda", "su", "sun" },
  { "Swahili", "Kiswahili", "sw", "swa" },
  { "Swedish", "svenska", "sv", "swe" },
  { "Tahitian", "Reo Tahiti", "ty", "tah" },
  { "Tajik", "Тоҷикӣ", "tg", "tgk" },
  { "Tamil", "தமிழ்", "ta", "tam" },
  { "Tatar", "Татар", "tt", "tat" },
  { "Telugu", "తెలుగు", "te", "tel" },
  { "Tagalog", "Katagalugan", "tl", "tgl" },
  { "Thai", "ไทย", "th", "tha" },
  { "Tibetan", "བོད་ཡིག", "bo", "bod", "tib" },
  { "Tigrinya", "ትግርኛ", "ti", "tir" },
  { "Tonga (Tonga Islands)", "lea fakatonga", "to", "ton" },
  { "Tswana", "Setswana", "tn", "tsn" },
  { "Tsonga", "Xitsonga", "ts", "tso" },
  { "Turkmen", "Türkmen dili", "tk", "tuk" },
  { "Turkish", "Türkçe", "tr", "tur" },
  { "Twi", "", "tw", "twi" },
  { "Ukrainian", "українська", "uk", "ukr" },
  { "Urdu", "اُردو", "ur", "urd" },
  { "Uyghur", "ئۇيغۇرچە", "ug", "uig" },
  { "Uzbek", "O'zbekcha", "uz", "uzb" },
  { "Venda", "Tshivenḓa", "ve", "ven" },
  { "Vietnamese", "Tiếng Việt", "vi", "vie" },
  { "Volapük", "Volapük", "vo", "vol" },
  { "Welsh", "Cymraeg", "cy", "cym", "wel" },
  { "Walloon", "Walon", "wa", "wln" },
  { "Wolof", "Wolof", "wo", "wol" },
  { "Xhosa", "isiXhosa", "xh", "xho" },
  { "Yiddish", "ייִדיש", "yi", "yid", "", "ji" },
  { "Yoruba", "Èdè Yorùbá", "yo", "yor" },
  { "Zhuang", "Vahcuengh", "za", "zha" },
  { "Zulu", "isiZulu", "zu", "zul" },
  { NULL, NULL, NULL } };

static const int lang_count = sizeof(languages) / sizeof(languages[0]);

int lang_lookup_index( const char * str )
{
    int             ii = 0;
    const iso639_lang_t * lang;

    // We use fake lang_any iso639 to designate a match for "Any" language
    if ((lang_any.iso639_1    && !strcasecmp(lang_any.iso639_1,    str)) ||
        (lang_any.iso639      && !strcasecmp(lang_any.iso639,      str)) ||
        (lang_any.iso639_2    && !strcasecmp(lang_any.iso639_2,    str)) ||
        (lang_any.iso639_2b   && !strcasecmp(lang_any.iso639_2b,   str)) ||
        (lang_any.eng_name    && !strcasecmp(lang_any.eng_name,    str)) ||
        (lang_any.native_name && !strcasecmp(lang_any.native_name, str)))
    {
        return -1;
    }

    for (ii = 0; languages[ii].eng_name; ii++)
    {
        lang = &languages[ii];
        if ((lang->iso639_1    && !strcasecmp(lang->iso639_1,    str)) ||
            (lang->iso639      && !strcasecmp(lang->iso639,      str)) ||
            (lang->iso639_2    && !strcasecmp(lang->iso639_2,    str)) ||
            (lang->iso639_2b   && !strcasecmp(lang->iso639_2b,   str)) ||
            (lang->eng_name    && !strcasecmp(lang->eng_name,    str)) ||
            (lang->native_name && !strcasecmp(lang->native_name, str)))
        {
            return ii;
        }
    }

    return -1;
}

const iso639_lang_t * lang_lookup( const char * str )
{
    return lang_for_index(lang_lookup_index(str));
}

const iso639_lang_t * lang_for_index( int index )
{
    if (index == -1)
        return &lang_any;

    if (index < 0 || index >= lang_count)
        return NULL;

    return &languages[index];
}

iso639_lang_t * lang_for_code( int code )
{
    char code_string[2];
    iso639_lang_t * lang;

    code_string[0] = tolower( ( code >> 8 ) & 0xFF );
    code_string[1] = tolower( code & 0xFF );

    if ((lang_any.iso639_1 && !strncmp(lang_any.iso639_1, code_string, 2)) ||
        (lang_any.iso639   && !strncmp(lang_any.iso639,   code_string, 2)))
    {
        return &lang_any;
    }
    for (lang = (iso639_lang_t*) languages; lang->eng_name; lang++)
    {
        if ((lang->iso639_1 && !strncmp( lang->iso639_1, code_string, 2)) ||
            (lang->iso639   && !strncmp(lang->iso639,    code_string, 2)))
        {
            return lang;
        }
    }

    // Not found, return "Unknown"
    return (iso639_lang_t*) languages;
}

iso639_lang_t * lang_for_code2( const char *code )
{
    char code_string[4];
    iso639_lang_t * lang;

    code_string[0] = tolower( code[0] );
    code_string[1] = tolower( code[1] );
    code_string[2] = tolower( code[2] );
    code_string[3] = 0;

    if ((lang_any.iso639_2  && !strcmp(lang_any.iso639_2,  code_string)) ||
        (lang_any.iso639_2b && !strcmp(lang_any.iso639_2b, code_string)))
    {
        return &lang_any;
    }

    for( lang = (iso639_lang_t*) languages; lang->eng_name; lang++ )
    {
        if ((lang->iso639_2  && !strcmp(lang->iso639_2,  code_string)) ||
            (lang->iso639_2b && !strcmp(lang->iso639_2b, code_string)))
        {
            return lang;
        }
    }

    // Not found, return "Unknown"
    return (iso639_lang_t*) languages;
}

int lang_to_code(const iso639_lang_t *lang)
{
    int code = 0;

    if (lang && strlen(lang->iso639_1) >= 2)
    {
        code = (lang->iso639_1[0] << 8) | lang->iso639_1[1];
    }

    return code;
}

iso639_lang_t * lang_for_english( const char * english )
{
    iso639_lang_t * lang;

    if (!strcmp(lang_any.eng_name, english))
    {
        return &lang_any;
    }
    for (lang = (iso639_lang_t*) languages; lang->eng_name; lang++)
    {
        if (!strcmp(lang->eng_name, english))
        {
            return lang;
        }
    }

    // Not found, return "Unknown"
    return (iso639_lang_t*) languages;
}

const iso639_lang_t* lang_get_any(void)
{
    return &lang_any;
}

const iso639_lang_t* lang_get_next(const iso639_lang_t *last)
{
    if (last == NULL || last == &lang_any)
    {
        return (const iso639_lang_t*)languages;
    }
    if (last <  languages ||                // out of bounds
        last >= languages + lang_count - 2) // last valid language
    {
        return NULL;
    }
    return ++last;
}

