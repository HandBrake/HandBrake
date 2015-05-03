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
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Services.Encode.Model;

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
        /// Initializes a new instance of the <see cref="PreviewSettings"/> class.
        /// </summary>
        /// <param name="task">The task.</param>
        public PreviewSettings(EncodeTask task)
        {
        }

        public Cropping Cropping { get; set; }

        public int MaxWidth { get; set; }

        public int MaxHeight { get; set; }

        public bool KeepDisplayAspect { get; set; }

        public int TitleNumber { get; set; }

        public Anamorphic Anamorphic { get; set; }

        public int? Modulus { get; set; }

        public int Width { get; set; }

        public int Height { get; set; }

        public int PixelAspectX { get; set; }

        public int PixelAspectY { get; set; }
    }
}
