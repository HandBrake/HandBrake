// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioEncoding.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the AudioEncoding type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    /// <summary>
    /// Audio Encoding
    /// </summary>
    public class AudioEncoding
    {
        /// <summary>
        /// Gets or sets InputNumber.
        /// </summary>
        public int InputNumber { get; set; }

        /// <summary>
        /// Gets or sets Encoder.
        /// </summary>
        public AudioEncoder Encoder { get; set; }

        /// <summary>
        /// Gets or sets Bitrate.
        /// </summary>
        public int Bitrate { get; set; }

        /// <summary>
        /// Gets or sets Mixdown.
        /// </summary>
        public Mixdown Mixdown { get; set; }

        /// <summary>
        /// Gets or sets SampleRate.
        /// </summary>
        public string SampleRate { get; set; }

        /// <summary>
        /// Gets or sets Drc.
        /// </summary>
        public double Drc { get; set; }
    }
}
