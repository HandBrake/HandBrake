// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Language.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Represents a language.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model
{
    /// <summary>
    /// Represents a language.
    /// </summary>
    public class Language
    {
        /// <summary>
        /// Gets or sets the english name of the language.
        /// </summary>
        public string EnglishName { get; set; }

        /// <summary>
        /// Gets or sets the native name of the language.
        /// </summary>
        public string NativeName { get; set; }

        /// <summary>
        /// Gets or sets the language code.
        /// </summary>
        public string Code { get; set; }

        /// <summary>
        /// Gets the display string for the language.
        /// </summary>
        public string Display
        {
            get
            {
                if (!string.IsNullOrEmpty(this.NativeName) && this.NativeName != this.EnglishName)
                {
                    return this.EnglishName + " (" + this.NativeName + ")";
                }

                return this.EnglishName;
            }
        }
    }
}
