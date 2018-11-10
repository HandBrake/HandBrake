// --------------------------------------------------------------------------------------------------------------------
// <copyright file="StringExtensions.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   String Extensions
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Extensions
{
    using System.Globalization;
    using System.Text.RegularExpressions;

    public static class StringExtensions
    { 
        /// <summary>
        /// Change the input string to title case
        /// </summary>
        /// <param name="input">the input string</param>
        /// <returns>the input string in title case</returns>
        public static string ToTitleCase(this string input)
        {
            TextInfo textInfo = new CultureInfo(CultureInfo.CurrentCulture.Name, false).TextInfo;
            return textInfo.ToTitleCase(input.ToLower());       
        }

        public static int ToInt(this string input)
        {
            if (int.TryParse(input, out int value))
            {
                return value;
            }

            return 0;
        }

        public static string RegexReplace(this string input, string pattern, string repacelement)
        {
            return Regex.Replace(input, pattern, repacelement, RegexOptions.IgnoreCase);
        }
    }
}
