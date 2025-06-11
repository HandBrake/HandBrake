// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Video.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Encode
{
    /// <summary>
    /// The video.
    /// </summary>
    public class Video
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Video"/> class.
        /// </summary>
        public Video()
        {
            this.QSV = new QSV();
        }

        /// <summary>
        /// Gets or sets the codec.
        /// </summary>
        public string Encoder { get; set; }

        /// <summary>
        /// Gets or sets the level.
        /// </summary>
        public string Level { get; set; }

        /// <summary>
        /// Gets or sets the bitrate for the encode.
        /// </summary>
        public int? Bitrate { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether multi pass.
        /// </summary>
        public bool MultiPass { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether Turbo Analysis Pass. For x264/5
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
        public decimal? Quality { get; set; }

        /// <summary>
        /// Gets or sets the tune.
        /// </summary>
        public string Tune { get; set; }

        /// <summary>
        /// Gets or sets the qsv.
        /// </summary>
        public QSV QSV { get; set; }

        /// <summary>
        /// HB_DECODE_SUPPORT constants in common.h
        /// </summary>
        public uint HardwareDecode { get; set; }
    }
}