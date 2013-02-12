// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SrtSubtitle.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the SrtSubtitle type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    /// <summary>
    /// The srt subtitle.
    /// </summary>
    public class SrtSubtitle
    {
        #region Properties

        /// <summary>
        /// Gets or sets the character code.
        /// </summary>
        public string CharacterCode { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether default.
        /// </summary>
        public bool Default { get; set; }

        /// <summary>
        /// Gets or sets the file name.
        /// </summary>
        public string FileName { get; set; }

        /// <summary>
        /// Gets or sets the language code.
        /// </summary>
        public string LanguageCode { get; set; }

        /// <summary>
        /// Gets or sets the offset.
        /// </summary>
        public int Offset { get; set; }

        #endregion

        #region Public Methods

        /// <summary>
        /// The clone.
        /// </summary>
        /// <returns>
        /// The <see cref="SrtSubtitle"/>.
        /// </returns>
        public SrtSubtitle Clone()
        {
            return new SrtSubtitle
                       {
                           Default = this.Default, 
                           FileName = this.FileName, 
                           LanguageCode = this.LanguageCode, 
                           CharacterCode = this.CharacterCode, 
                           Offset = this.Offset
                       };
        }

        #endregion
    }
}