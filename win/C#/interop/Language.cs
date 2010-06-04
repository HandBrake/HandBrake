// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Language.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Represents a language.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
    /// <summary>
    /// Represents a language.
    /// </summary>
    public class Language
    {
        /// <summary>
        /// Initializes a new instance of the Language class.
        /// </summary>
        /// <param name="code">The code for the langauge.</param>
        public Language(string code)
        {
            this.Code = code;
        }

        /// <summary>
        /// Gets the friendly name of the language.
        /// </summary>
        public string Name
        {
            get
            {
                return LanguageCodes.Decode(this.Code);
            }
        }

        /// <summary>
        /// Gets or sets the language code.
        /// </summary>
        public string Code { get; set; }
    }
}
