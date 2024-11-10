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
                       new InterfaceLanguage("bg", "Bulgarian (Български)"),
                       new InterfaceLanguage("ca", "Catalan (Català)"),
                       new InterfaceLanguage("co", "Corsican (Corsu)"),
                       new InterfaceLanguage("cs-CZ", "Czech (čeština)"),
                       new InterfaceLanguage("de", "German (Deutsch)"),
                       DefaultEnglishLanguage,
                       new InterfaceLanguage("el", "Greek (Ελληνικά)"),
                       new InterfaceLanguage("es", "Spanish (Español)"),
                       new InterfaceLanguage("et", "Estonian (Eesti)"),
                       new InterfaceLanguage("eu", "Basque (Euskara)"),
                       new InterfaceLanguage("fa-IR", "Persian (Iran) (فارسی)"),
                       new InterfaceLanguage("fi", "Finnish (Suomi)"),
                       new InterfaceLanguage("fr", "French (Français)"),
                       new InterfaceLanguage("it", "Italian (Italiano)"),
                       new InterfaceLanguage("ja", "Japanese (日本語 (にほんご))"),
                       new InterfaceLanguage("ko", "Korean (한국어)"),
                       new InterfaceLanguage("nl", "Dutch (Nederlands)"),
                       new InterfaceLanguage("pl", "Polish (Polski)"),
                       new InterfaceLanguage("pt-BR", "Brazilian Portuguese (Português do Brasil)"),
                       new InterfaceLanguage("ru", "Russian (Русский)"),
                       new InterfaceLanguage("sv", "Swedish (Svenska)"),
                       new InterfaceLanguage("th", "Thai (ไทย)"),
                       new InterfaceLanguage("tr", "Turkish (Türkçe)"),
                       new InterfaceLanguage("uk", "Ukrainian (Українська)"),
                       new InterfaceLanguage("zh-Hans", "Simplified Chinese (简体中文)"),
                       new InterfaceLanguage("zh-Hant", "Traditional Chinese (正體中文)"),
                   };
        }

        public static InterfaceLanguage FindInterfaceLanguage(string culture)
        {
            return GetUserInterfaceLanguages().FirstOrDefault(f => f.Culture == culture);
        }
    }
}
