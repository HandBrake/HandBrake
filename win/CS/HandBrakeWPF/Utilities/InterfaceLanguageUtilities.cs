// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InterfaceLanguageUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Helper methods for UI Language Support.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System.Collections.Generic;
    using System.Linq;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Properties;

    public class InterfaceLanguageUtilities
    {
        public static InterfaceLanguage DefaultEnglishLanguage = new InterfaceLanguage("en", "English");

        public static InterfaceLanguage UseSystemLanguage = new InterfaceLanguage(null, Resources.Language_UseSystem);

        public static List<InterfaceLanguage> GetUserInterfaceLanguages()
        {
            return new List<InterfaceLanguage>
                   {
                       UseSystemLanguage,
                       new InterfaceLanguage("pt-BR", "Brazilian Portuguese (Português do Brasil)"),
                       new InterfaceLanguage("zh-CN", "Simplified Chinese (简体中文)"),
                       new InterfaceLanguage("zh-TW", "Traditional Chinese (正體中文)"),
                       new InterfaceLanguage("co", "Corsican (Corsu)"),
                       DefaultEnglishLanguage,
                       new InterfaceLanguage("fr", "French (Français)"),
                       new InterfaceLanguage("de", "German (Deutsch)"),
                       new InterfaceLanguage("it", "Italian (Italiano)"),
                       new InterfaceLanguage("ja", "Japanese (日本語 (にほんご))"),
                       new InterfaceLanguage("ko", "Korean (한국어)"),
                       new InterfaceLanguage("fa-IR", "Persian (Iran) (فارسی)"),
                       new InterfaceLanguage("pl", "Polish (Polski)"),
                       new InterfaceLanguage("ru", "Russian (Русский)"),
                       new InterfaceLanguage("es", "Spanish (Español)"),
                       new InterfaceLanguage("th", "Thai (ไทย)"),
                       new InterfaceLanguage("tr", "Turkish (Türkçe)"),
                       new InterfaceLanguage("uk", "Ukrainian (Українська)"),
                   };
        }

        public static InterfaceLanguage FindInterfaceLanguage(string culture)
        {
            return GetUserInterfaceLanguages().FirstOrDefault(f => f.Culture == culture);
        }
    }
}
