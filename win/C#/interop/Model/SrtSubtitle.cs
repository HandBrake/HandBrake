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
    /// A Srt Subtitle
    /// </summary>
    public class SrtSubtitle
    {
        /// <summary>
        /// Gets or sets a value indicating whether Default.
        /// </summary>
        public bool Default { get; set; }

        /// <summary>
        /// Gets or sets FileName.
        /// </summary>
        public string FileName { get; set; }

        /// <summary>
        /// Gets or sets LanguageCode.
        /// </summary>
        public string LanguageCode { get; set; }

        /// <summary>
        /// Gets or sets CharacterCode.
        /// </summary>
        public string CharacterCode { get; set; }

        /// <summary>
        /// Gets or sets Offset.
        /// </summary>
        public int Offset { get; set; }

        /// <summary>
        /// Close the SRT Subtitle object
        /// </summary>
        /// <returns>
        /// a SrtSubtitle
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
    }
}
