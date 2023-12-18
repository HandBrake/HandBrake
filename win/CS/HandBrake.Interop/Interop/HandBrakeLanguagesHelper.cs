// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeLanguagesHelper.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Contains utilities for converting language codes.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using System.Collections.Generic;
    using System.Linq;

    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Helpers;
    using HandBrake.Interop.Interop.Interfaces.Model;

    /// <summary>
    /// Contains utilities for converting language codes.
    /// </summary>
    public static class HandBrakeLanguagesHelper
    {
        public static string Any = "(Any)";
        public static Language AnyLanguage = new Language(Any, Any, "any");

        private static IList<Language> allLanguages;
        private static Dictionary<string, Language> allLanguagesDict;

        static HandBrakeLanguagesHelper()
        {
            allLanguages = InteropUtilities.ToListFromIterator<iso639_lang_t, Language>(
                HBFunctions.lang_get_next,
                HandBrakeUnitConversionHelpers.NativeToLanguage);

            allLanguagesDict = new Dictionary<string, Language>();
            foreach (Language lang in allLanguages)
            {
                allLanguagesDict.Add(lang.Code, lang);
            }
        }

        public static Dictionary<string, Language> AllLanguagesDict
        {
            get
            {
                return allLanguagesDict;
            }
        }


        public static IList<Language> AllLanguages
        {
            get
            {
                return allLanguages;
            }
        }

        public static IList<Language> AllLanguagesWithAny
        {
            get
            {
                List<Language> languages = new List<Language>() { AnyLanguage };
                languages.AddRange(AllLanguages);

                return languages;
            }
        }

        public static Language GetByName(string name)
        {
            return AllLanguages.FirstOrDefault(s => s.EnglishName == name || s.NativeName == name);
        }

        /// <summary>
        /// Gets the language object for the given code.
        /// </summary>
        /// <param name="code">The ISO-639-2 code for the language.</param>
        /// <returns>Object that describes the language.</returns>
        public static Language GetByCode(string code)
        {
            if (code == AnyLanguage.Code)
            {
                return AnyLanguage;
            }

            iso639_lang_t language = InteropUtilities.ToStructureFromPtr<iso639_lang_t>(HBFunctions.lang_for_code2(code));
            return HandBrakeUnitConversionHelpers.NativeToLanguage(language);
        }

        public static IList<Language> GetLanguageListByCode(IList<string> names)
        {
            List<Language> languages = new List<Language>();
            foreach (string name in names)
            {
                languages.Add(GetByCode(name));
            }

            return languages;
        }

        public static List<string> GetLanguageCodes(IEnumerable<Language> userLanguages)
        {
            // Translate to Iso Codes
            List<string> iso6392Codes = new List<string>();
            foreach (var item in userLanguages)
            {
                iso6392Codes.Add(item.Code);
            }

            return iso6392Codes;
        }

        public static List<string> GetIsoCodes()
        {
            return AllLanguages.Select(allLanguage => allLanguage.Code).ToList();
        }

        public static List<string> OrderIsoCodes(List<string> iso6392Codes, IList<Language> languages)
        {
            List<string> orderedSet = new List<string>();
            foreach (Language item in languages)
            {
                Language isoCode;
                if (AllLanguagesDict.TryGetValue(item.Code, out isoCode))
                {
                    orderedSet.Add(isoCode.Code);
                }
            }

            List<string> unorderedSet = iso6392Codes.Where(isoCode => !orderedSet.Contains(isoCode)).ToList();

            List<string> orderedList = orderedSet.Union(unorderedSet).ToList();

            return orderedList;
        }
    }
}
