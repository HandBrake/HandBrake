// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Video.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Json.Encode
{
    /// <summary>
    /// The video.
    /// </summary>
    public class Video
    {
        /// <summary>
        /// Gets or sets the codec.
        /// </summary>
        public int Codec { get; set; }

        /// <summary>
        /// Gets or sets the level.
        /// </summary>
        public string Level { get; set; }

        /// <summary>
        /// Gets or sets the bitrate for the encode.
        /// </summary>
        public int Bitrate { get; set; }

        /// <summary>
        /// Gets or sets the number of passes
        /// </summary>
        public int pass { get; set; }

        /// <summary>
        /// Gets or sets Turbo First Pass. For x264/5
        /// </summary>
        public bool Turbo { get; set; }

        /// <summary>
        /// Gets or sets the Colour Matrix Code
        /// </summary>
        public int ColorMatrixCode { get; set; }

        /// <summary>
        /// Gets or sets the options.
        /// </summary>
        public string Options { get; set; }

        /// <summary>
        /// Gets or sets the preset.
        /// </summary>
        public string Preset { get; set; }

        /// <summary>
        /// Gets or sets the profile.
        /// </summary>
        public string Profile { get; set; }

        /// <summary>
        /// Gets or sets the quality.
        /// </summary>
        public double Quality { get; set; }

        /// <summary>
        /// Gets or sets the tune.
        /// </summary>
        public string Tune { get; set; }
    }
}