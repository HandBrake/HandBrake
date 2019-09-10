// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LanguageUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Language Utilities
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Utilities
{
    using System.Collections.Generic;
    using System.Linq;

    /// <summary>
    /// Language Utilities
    /// </summary>
    public class LanguageUtilities
    {
        /// <summary>
        /// The language map.
        /// </summary>
        private static IDictionary<string, string> languageMap;

        /// <summary>
        /// Map languages and their iso639_2 value into a IDictionary
        /// </summary>
        /// <returns>A Dictionary containing the language and iso code</returns>
        public static IDictionary<string, string> MapLanguages()
        {
            if (languageMap != null)
            {
                return languageMap;
            }

            languageMap = new Dictionary<string, string>
                          {
                              { "(Any)", "any" },
                              { "(Unknown)", "und" },
                              { "Afar", "aar" },
                              { "Abkhazian", "abk" },
                              { "Afrikaans", "afr" },
                              { "Akan", "aka" },
                              { "Albanian", "sqi" },
                              { "Amharic", "amh" },
                              { "Arabic", "ara" },
                              { "Aragonese", "arg" },
                              { "Armenian", "hye" },
                              { "Assamese", "asm" },
                              { "Avaric", "ava" },
                              { "Avestan", "ave" },
                              { "Aymara", "aym" },
                              { "Azerbaijani", "aze" },
                              { "Bashkir", "bak" },
                              { "Bambara", "bam" },
                              { "Basque", "eus" },
                              { "Belarusian", "bel" },
                              { "Bengali", "ben" },
                              { "Bihari", "bih" },
                              { "Bislama", "bis" },
                              { "Bosnian", "bos" },
                              { "Breton", "bre" },
                              { "Bulgarian", "bul" },
                              { "Burmese", "mya" },
                              { "Catalan", "cat" },
                              { "Chamorro", "cha" },
                              { "Chechen", "che" },
                              { "Chinese", "zho" },
                              { "Church Slavic", "chu" },
                              { "Chuvash", "chv" },
                              { "Cornish", "cor" },
                              { "Corsican", "cos" },
                              { "Cree", "cre" },
                              { "Czech", "ces" },
                              { "Dansk", "dan" },
                              { "Divehi", "div" },
                              { "Nederlands", "nld" },
                              { "Dzongkha", "dzo" },
                              { "English", "eng" },
                              { "Esperanto", "epo" },
                              { "Estonian", "est" },
                              { "Ewe", "ewe" },
                              { "Faroese", "fao" },
                              { "Fijian", "fij" },
                              { "Suomi", "fin" },
                              { "Francais", "fra" },
                              { "Western Frisian", "fry" },
                              { "Fulah", "ful" },
                              { "Georgian", "kat" },
                              { "Deutsch", "deu" },
                              { "Gaelic (Scots)", "gla" },
                              { "Irish", "gle" },
                              { "Galician", "glg" },
                              { "Manx", "glv" },
                              { "Greek, Modern", "ell" },
                              { "Guarani", "grn" },
                              { "Gujarati", "guj" },
                              { "Haitian", "hat" },
                              { "Hausa", "hau" },
                              { "Hebrew", "heb" },
                              { "Herero", "her" },
                              { "Hindi", "hin" },
                              { "Hiri Motu", "hmo" },
                              { "Magyar", "hun" },
                              { "Igbo", "ibo" },
                              { "Islenska", "isl" },
                              { "Ido", "ido" },
                              { "Sichuan Yi", "iii" },
                              { "Inuktitut", "iku" },
                              { "Interlingue", "ile" },
                              { "Interlingua", "ina" },
                              { "Indonesian", "ind" },
                              { "Inupiaq", "ipk" },
                              { "Italiano", "ita" },
                              { "Javanese", "jav" },
                              { "Japanese", "jpn" },
                              { "Kalaallisut", "kal" },
                              { "Kannada", "kan" },
                              { "Kashmiri", "kas" },
                              { "Kanuri", "kau" },
                              { "Kazakh", "kaz" },
                              { "Central Khmer", "khm" },
                              { "Kikuyu", "kik" },
                              { "Kinyarwanda", "kin" },
                              { "Kirghiz", "kir" },
                              { "Komi", "kom" },
                              { "Kongo", "kon" },
                              { "Korean", "kor" },
                              { "Kuanyama", "kua" },
                              { "Kurdish", "kur" },
                              { "Lao", "lao" },
                              { "Latin", "lat" },
                              { "Latvian", "lav" },
                              { "Limburgan", "lim" },
                              { "Lingala", "lin" },
                              { "Lithuanian", "lit" },
                              { "Luxembourgish", "ltz" },
                              { "Luba-Katanga", "lub" },
                              { "Ganda", "lug" },
                              { "Macedonian", "mkd" },
                              { "Marshallese", "mah" },
                              { "Malayalam", "mal" },
                              { "Maori", "mri" },
                              { "Marathi", "mar" },
                              { "Malay", "msa" },
                              { "Malagasy", "mlg" },
                              { "Maltese", "mlt" },
                              { "Moldavian", "mol" },
                              { "Mongolian", "mon" },
                              { "Nauru", "nau" },
                              { "Navajo", "nav" },
                              { "Ndebele, South", "nbl" },
                              { "Ndebele, North", "nde" },
                              { "Ndonga", "ndo" },
                              { "Nepali", "nep" },
                              { "Norwegian Nynorsk", "nno" },
                              { "Norwegian Bokmål", "nob" },
                              { "Norsk", "nor" },
                              { "Chichewa; Nyanja", "nya" },
                              { "Occitan", "oci" },
                              { "Ojibwa", "oji" },
                              { "Oriya", "ori" },
                              { "Oromo", "orm" },
                              { "Ossetian", "oss" },
                              { "Panjabi", "pan" },
                              { "Persian", "fas" },
                              { "Pali", "pli" },
                              { "Polish", "pol" },
                              { "Portuguese", "por" },
                              { "Pushto", "pus" },
                              { "Quechua", "que" },
                              { "Romansh", "roh" },
                              { "Romanian", "ron" },
                              { "Rundi", "run" },
                              { "Russian", "rus" },
                              { "Sango", "sag" },
                              { "Sanskrit", "san" },
                              { "Serbian", "srp" },
                              { "Hrvatski", "hrv" },
                              { "Sinhala", "sin" },
                              { "Slovak", "slk" },
                              { "Slovenian", "slv" },
                              { "Northern Sami", "sme" },
                              { "Samoan", "smo" },
                              { "Shona", "sna" },
                              { "Sindhi", "snd" },
                              { "Somali", "som" },
                              { "Sotho Southern", "sot" },
                              { "Espanol", "spa" },
                              { "Sardinian", "srd" },
                              { "Swati", "ssw" },
                              { "Sundanese", "sun" },
                              { "Swahili", "swa" },
                              { "Svenska", "swe" },
                              { "Tahitian", "tah" },
                              { "Tamil", "tam" },
                              { "Tatar", "tat" },
                              { "Telugu", "tel" },
                              { "Tajik", "tgk" },
                              { "Tagalog", "tgl" },
                              { "Thai", "tha" },
                              { "Tibetan", "bod" },
                              { "Tigrinya", "tir" },
                              { "Tonga", "ton" },
                              { "Tswana", "tsn" },
                              { "Tsonga", "tso" },
                              { "Turkmen", "tuk" },
                              { "Turkish", "tur" },
                              { "Twi", "twi" },
                              { "Uighur", "uig" },
                              { "Ukrainian", "ukr" },
                              { "Urdu", "urd" },
                              { "Uzbek", "uzb" },
                              { "Venda", "ven" },
                              { "Vietnamese", "vie" },
                              { "Volapük", "vol" },
                              { "Welsh", "cym" },
                              { "Walloon", "wln" },
                              { "Wolof", "wol" },
                              { "Xhosa", "xho" },
                              { "Yiddish", "yid" },
                              { "Yoruba", "yor" },
                              { "Zhuang", "zha" },
                              { "Zulu", "zul" }
                          };

            return languageMap;
        }

        /// <summary>
        /// The get language codes.
        /// </summary>
        /// <param name="userLanguages">
        /// The user languages.
        /// </param>
        /// <returns>
        /// The <see cref="List{T}"/>.
        /// </returns>
        public static List<string> GetLanguageCodes(IEnumerable<string> userLanguages)
        {
            // Translate to Iso Codes
            List<string> iso6392Codes = new List<string>();
            foreach (var item in userLanguages)
            {
                string isoCode;
                if (MapLanguages().TryGetValue(item, out isoCode))
                {
                    iso6392Codes.Add(isoCode);
                }
            }

            return iso6392Codes;
        }

        /// <summary>
        /// The get language names.
        /// </summary>
        /// <param name="languageCodes">
        /// The language codes.
        /// </param>
        /// <returns>
        /// The <see cref="List{T}"/>.
        /// </returns>
        public static List<string> GetLanguageNames(IList<string> languageCodes)
        {
            // Translate to Iso Codes
            List<string> names = new List<string>();
            foreach (var item in languageCodes)
            {
                KeyValuePair<string, string> name = MapLanguages().FirstOrDefault(v => v.Value == item);  // Slightly inefficient but small set anyway so not a big issue.
                names.Add(name.Key);
            }

            return names;
        }

        /// <summary>
        /// The get iso codes.
        /// </summary>
        /// <returns>
        /// The <see cref="List{T}"/>.
        /// </returns>
        public static List<string> GetIsoCodes()
        {
           return MapLanguages().Values.ToList();
        }

        /// <summary>
        /// The get language code.
        /// </summary>
        /// <param name="language">
        /// The language.
        /// </param>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static string GetLanguageCode(string language)
        {
            if (string.IsNullOrEmpty(language))
            {
                return null;
            }

            string isoCode;
            if (MapLanguages().TryGetValue(language, out isoCode))
            {
                return isoCode;
            }

            return null;
        }
    }
}
