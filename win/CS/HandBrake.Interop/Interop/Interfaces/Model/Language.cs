// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Language.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Represents a language.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces.Model
{
    /// <summary>
    /// Represents a language.
    /// </summary>
    public class Language
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Language"/> class.
        /// </summary>
        /// <param name="englishName">
        /// The english name.
        /// </param>
        /// <param name="nativeName">
        /// The native name.
        /// </param>
        /// <param name="code">
        /// The code.
        /// </param>
        public Language(string englishName, string nativeName, string code)
        {
            this.EnglishName = englishName;
            this.NativeName = nativeName;
            this.Code = code;
        }

        /// <summary>
        /// Gets the english name of the language.
        /// </summary>
        public string EnglishName { get; private set; }

        /// <summary>
        /// Gets the native name of the language.
        /// </summary>
        public string NativeName { get; private set; }

        /// <summary>
        /// Gets the language code.
        /// </summary>
        public string Code { get; private set; }

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

        public string DisplayNative
        {
            get
            {
                if (!string.IsNullOrEmpty(this.NativeName) && this.NativeName != this.EnglishName)
                {
                    return this.NativeName + " (" + this.EnglishName + ")";
                }

                return !string.IsNullOrEmpty(this.NativeName) ? this.NativeName : this.EnglishName;
            }
        }

        /// <summary>
        /// Returns the display string for this language.
        /// </summary>
        /// <returns>The display string for this language.</returns>
        public override string ToString()
        {
            return this.Display;
        }

        protected bool Equals(Language other)
        {
            return this.Code == other.Code;
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
            {
                return false;
            }

            if (ReferenceEquals(this, obj))
            {
                return true;
            }

            if (obj.GetType() != this.GetType())
            {
                return false;
            }

            return Equals((Language)obj);
        }

        public override int GetHashCode()
        {
            return (this.Code != null ? this.Code.GetHashCode() : 0);
        }
    }
}
