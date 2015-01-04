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
    using System.Text;

    /// <summary>
    /// String Extensions
    /// </summary>
    public static class StringExtensions
    {
        /// <summary>
        /// Change the input string to title case
        /// </summary>
        /// <param name="input">the input string</param>
        /// <returns>the input string in title case</returns>
        public static string ToTitleCase(this string input)
        {
            string[] tokens = input.Split(' ');
            StringBuilder sb = new StringBuilder(input.Length);
            foreach (string s in tokens)
            {
                if (!string.IsNullOrEmpty(s))
                {
                    sb.Append(s[0].ToString().ToUpper());
                    sb.Append(s.Substring(1).ToLower());
                    sb.Append(" ");
                }
            }

            return sb.ToString().Trim();
        }
    }
}
