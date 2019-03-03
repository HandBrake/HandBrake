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
        public static List<InterfaceLanguage> GetUserInterfaceLangauges()
        {
            return new List<InterfaceLanguage>
                   {
                       new InterfaceLanguage(null, Resources.Language_UseSystem),
                       new InterfaceLanguage("en", "English"),
                       new InterfaceLanguage("de", "German"),
                       new InterfaceLanguage("zh", "Chinese"),
                   };
        }

        public static InterfaceLanguage FindInterfaceLanguage(string culture)
        {
            return GetUserInterfaceLangauges().FirstOrDefault(f => f.Culture == culture);
        }
    }
}
