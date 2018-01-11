﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="StringExtensions.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   String Extensions
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Extensions
{
    using System.Globalization;

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
            TextInfo textInfo = new CultureInfo(CultureInfo.CurrentCulture.Name, false).TextInfo;
            return textInfo.ToTitleCase(input.ToLower());
          
        }
    }
}
