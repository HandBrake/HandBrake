// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PreviewSettings.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The preview settings.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Preview
{
    using Encoding;

    /// <summary>
    /// The preview settings.
    /// </summary>
    public class PreviewSettings
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PreviewSettings"/> class.
        /// </summary>
        public PreviewSettings()
        {
        }

        /// <summary>
        /// Gets or sets the cropping.
        /// </summary>
        public Cropping Cropping { get; set; }

        /// <summary>
        /// Gets or sets the max width.
        /// </summary>
        public int MaxWidth { get; set; }

        /// <summary>
        /// Gets or sets the max height.
        /// </summary>
        public int MaxHeight { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether keep display aspect.
        /// </summary>
        public bool KeepDisplayAspect { get; set; }

        /// <summary>
        /// Gets or sets the title number.
        /// </summary>
        public int TitleNumber { get; set; }

        /// <summary>
        /// Gets or sets the anamorphic.
        /// </summary>
        public Anamorphic Anamorphic { get; set; }

        /// <summary>
        /// Gets or sets the modulus.
        /// </summary>
        public int? Modulus { get; set; }

        /// <summary>
        /// Gets or sets the width.
        /// </summary>
        public int Width { get; set; }

        /// <summary>
        /// Gets or sets the height.
        /// </summary>
        public int Height { get; set; }

        /// <summary>
        /// Gets or sets the pixel aspect x.
        /// </summary>
        public int PixelAspectX { get; set; }

        /// <summary>
        /// Gets or sets the pixel aspect y.
        /// </summary>
        public int PixelAspectY { get; set; }
    }
}
