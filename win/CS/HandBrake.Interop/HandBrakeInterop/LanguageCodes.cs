// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LanguageCodes.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Contains utilities for converting language codes.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
    using System.Collections.Generic;
    using System.Linq;

    /// <summary>
	/// Contains utilities for converting language codes.
	/// </summary>
	public static class LanguageCodes
	{
		/// <summary>
		/// The map of language codes to friendly names.
		/// </summary>
		private static Dictionary<string, string> languageMap;

		/// <summary>
		/// Gets the map of language codes to friendly names.
		/// </summary>
		private static Dictionary<string, string> LanguageMap
		{
			get
			{
				if (languageMap == null)
				{
					languageMap = new Dictionary<string, string>
					{
						{"und", "Unspecified"},
						{"eng", "English"},
						{"deu", "Deutsch"},
						{"fra", "Français"},
						{"spa", "Español"},
						{"rus", "Russian"},
						{"aar", "Afar"},
						{"abk", "Abkhazian"},
						{"afr", "Afrikaans"},
						{"aka", "Akan"},
						{"sqi", "Albanian"},
						{"amh", "Amharic"},
						{"ara", "Arabic"},
						{"arg", "Aragonese"},
						{"hye", "Armenian"},
						{"asm", "Assamese"},
						{"ava", "Avaric"},
						{"ave", "Avestan"},
						{"aym", "Aymara"},
						{"aze", "Azerbaijani"},
						{"bak", "Bashkir"},
						{"bam", "Bambara"},
						{"eus", "Basque"},
						{"bel", "Belarusian"},
						{"ben", "Bengali"},
						{"bih", "Bihari"},
						{"bis", "Bislama"},
						{"bos", "Bosnian"},
						{"bre", "Breton"},
						{"bul", "Bulgarian"},
						{"mya", "Burmese"},
						{"cat", "Catalan"},
						{"cha", "Chamorro"},
						{"che", "Chechen"},
						{"zho", "Chinese"},
						{"chu", "Church Slavic"},
						{"chv", "Chuvash"},
						{"cor", "Cornish"},
						{"cos", "Corsican"},
						{"cre", "Cree"},
						{"ces", "Czech"},
						{"dan", "Dansk"},
						{"div", "Divehi"},
						{"nld", "Nederlands"},
						{"dzo", "Dzongkha"},
						{"epo", "Esperanto"},
						{"est", "Estonian"},
						{"ewe", "Ewe"},
						{"fao", "Faroese"},
						{"fij", "Fijian"},
						{"fin", "Suomi"},
						{"fry", "Western Frisian"},
						{"ful", "Fulah"},
						{"kat", "Georgian"},
						{"gla", "Gaelic (Scots)"},
						{"gle", "Irish"},
						{"glg", "Galician"},
						{"glv", "Manx"},
						{"ell", "Greek Modern"},
						{"grn", "Guarani"},
						{"guj", "Gujarati"},
						{"hat", "Haitian"},
						{"hau", "Hausa"},
						{"heb", "Hebrew"},
						{"her", "Herero"},
						{"hin", "Hindi"},
						{"hmo", "Hiri Motu"},
						{"hun", "Magyar"},
						{"ibo", "Igbo"},
						{"isl", "Islenska"},
						{"ido", "Ido"},
						{"iii", "Sichuan Yi"},
						{"iku", "Inuktitut"},
						{"ile", "Interlingue"},
						{"ina", "Interlingua"},
						{"ind", "Indonesian"},
						{"ipk", "Inupiaq"},
						{"ita", "Italiano"},
						{"jav", "Javanese"},
						{"jpn", "Japanese"},
						{"kal", "Kalaallisut"},
						{"kan", "Kannada"},
						{"kas", "Kashmiri"},
						{"kau", "Kanuri"},
						{"kaz", "Kazakh"},
						{"khm", "Central Khmer"},
						{"kik", "Kikuyu"},
						{"kin", "Kinyarwanda"},
						{"kir", "Kirghiz"},
						{"kom", "Komi"},
						{"kon", "Kongo"},
						{"kor", "Korean"},
						{"kua", "Kuanyama"},
						{"kur", "Kurdish"},
						{"lao", "Lao"},
						{"lat", "Latin"},
						{"lav", "Latvian"},
						{"lim", "Limburgan"},
						{"lin", "Lingala"},
						{"lit", "Lithuanian"},
						{"ltz", "Luxembourgish"},
						{"lub", "Luba-Katanga"},
						{"lug", "Ganda"},
						{"mkd", "Macedonian"},
						{"mah", "Marshallese"},
						{"mal", "Malayalam"},
						{"mri", "Maori"},
						{"mar", "Marathi"},
						{"msa", "Malay"},
						{"mlg", "Malagasy"},
						{"mlt", "Maltese"},
						{"mol", "Moldavian"},
						{"mon", "Mongolian"},
						{"nau", "Nauru"},
						{"nav", "Navajo"},
						{"nbl", "Ndebele, South"},
						{"nde", "Ndebele, North"},
						{"ndo", "Ndonga"},
						{"nep", "Nepali"},
						{"nno", "Norwegian Nynorsk"},
						{"nob", "Norwegian Bokmål"},
						{"nor", "Norsk"},
						{"nya", "Chichewa; Nyanja"},
						{"oci", "Occitan"},
						{"oji", "Ojibwa"},
						{"ori", "Oriya"},
						{"orm", "Oromo"},
						{"oss", "Ossetian"},
						{"pan", "Panjabi"},
						{"fas", "Persian"},
						{"pli", "Pali"},
						{"pol", "Polish"},
						{"por", "Portugues"},
						{"pus", "Pushto"},
						{"que", "Quechua"},
						{"roh", "Romansh"},
						{"ron", "Romanian"},
						{"run", "Rundi"},
						{"sag", "Sango"},
						{"san", "Sanskrit"},
						{"srp", "Serbian"},
						{"hrv", "Hrvatski"},
						{"sin", "Sinhala"},
						{"slk", "Slovak"},
						{"slv", "Slovenian"},
						{"sme", "Northern Sami"},
						{"smo", "Samoan"},
						{"sna", "Shona"},
						{"snd", "Sindhi"},
						{"som", "Somali"},
						{"sot", "Sotho Southern"},
						{"srd", "Sardinian"},
						{"ssw", "Swati"},
						{"sun", "Sundanese"},
						{"swa", "Swahili"},
						{"swe", "Svenska"},
						{"tah", "Tahitian"},
						{"tam", "Tamil"},
						{"tat", "Tatar"},
						{"tel", "Telugu"},
						{"tgk", "Tajik"},
						{"tgl", "Tagalog"},
						{"tha", "Thai"},
						{"bod", "Tibetan"},
						{"tir", "Tigrinya"},
						{"ton", "Tonga"},
						{"tsn", "Tswana"},
						{"tso", "Tsonga"},
						{"tuk", "Turkmen"},
						{"tur", "Turkish"},
						{"twi", "Twi"},
						{"uig", "Uighur"},
						{"ukr", "Ukrainian"},
						{"urd", "Urdu"},
						{"uzb", "Uzbek"},
						{"ven", "Venda"},
						{"vie", "Vietnamese"},
						{"vol", "Volapük"},
						{"cym", "Welsh"},
						{"wln", "Walloon"},
						{"wol", "Wolof"},
						{"xho", "Xhosa"},
						{"yid", "Yiddish"},
						{"yor", "Yoruba"},
						{"zha", "Zhuang"},
						{"zul", "Zulu"}
					};
				}

				return languageMap;
			}
		}

		/// <summary>
		/// Gets a list of all languages.
		/// </summary>
		public static IList<Language> Languages
		{
			get
			{
			    return LanguageMap.Keys.Select(languageCode => new Language(languageCode)).ToList();
			}
		}

		/// <summary>
		/// Gives the friendly name of the language with the given code.
		/// </summary>
		/// <param name="languageCode">The language code.</param>
		/// <returns>The friendly name of the language.</returns>
		public static string Decode(string languageCode)
		{
			if (LanguageMap.ContainsKey(languageCode))
			{
				return LanguageMap[languageCode];
			}

			return "Unknown";
		}
	}
}
