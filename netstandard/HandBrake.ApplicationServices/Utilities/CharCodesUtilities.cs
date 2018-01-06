// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CharCodesUtilities.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Char Codes
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Utilities
{
    using System.Collections.Generic;

    /// <summary>
    /// Char Codes
    /// </summary>
    public class CharCodesUtilities
    {
        /// <summary>
        /// Get a command subset of character codes.
        /// </summary>
        /// <returns>
        /// A String List of Character codes.
        /// </returns>
        public static List<string> GetCharacterCodes()
        {
            return new List<string>
                {
                    "ANSI_X3.4-1968",
                    "ANSI_X3.4-1986",
                    "ANSI_X3.4",
                    "ANSI_X3.110-1983",
                    "ANSI_X3.110",
                    "ASCII",
                    "ECMA-114",
                    "ECMA-118",
                    "ECMA-128",
                    "ECMA-CYRILLIC",
                    "IEC_P27-1",
                    "ISO-8859-1",
                    "ISO-8859-2",
                    "ISO-8859-3",
                    "ISO-8859-4",
                    "ISO-8859-5",
                    "ISO-8859-6",
                    "ISO-8859-7",
                    "ISO-8859-8",
                    "ISO-8859-9",
                    "ISO-8859-9E",
                    "ISO-8859-10",
                    "ISO-8859-11",
                    "ISO-8859-13",
                    "ISO-8859-14",
                    "ISO-8859-15",
                    "ISO-8859-16",
                    "UTF-7",
                    "UTF-8",
                    "UTF-16",
                    "UTF-32"
                };
        }
    }
}
