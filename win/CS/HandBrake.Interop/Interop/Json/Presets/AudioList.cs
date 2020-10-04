// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioList.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio list.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Presets
{
    /// <summary>
    /// The audio list.
    /// </summary>
    public class AudioList
    {
        /// <summary>
        /// Gets or sets the audio bitrate.
        /// </summary>
        public int AudioBitrate { get; set; }

        /// <summary>
        /// Gets or sets the audio compression level.
        /// </summary>
        public double AudioCompressionLevel { get; set; }

        /// <summary>
        /// Gets or sets the audio dither method.
        /// </summary>
        public string AudioDitherMethod { get; set; }

        /// <summary>
        /// Gets or sets the audio encoder.
        /// </summary>
        public string AudioEncoder { get; set; }

        /// <summary>
        /// Gets or sets the audio mixdown. (ShortName)
        /// </summary>
        public string AudioMixdown { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether audio normalize mix level.
        /// </summary>
        public bool AudioNormalizeMixLevel { get; set; }

        /// <summary>
        /// Gets or sets the audio samplerate.
        /// </summary>
        public string AudioSamplerate { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether audio track quality enable.
        /// </summary>
        public bool AudioTrackQualityEnable { get; set; }

        /// <summary>
        /// Gets or sets the audio track quality.
        /// </summary>
        public double AudioTrackQuality { get; set; }

        /// <summary>
        /// Gets or sets the audio track gain slider.
        /// </summary>
        public double AudioTrackGainSlider { get; set; }

        /// <summary>
        /// Gets or sets the audio track drc slider.
        /// </summary>
        public double AudioTrackDRCSlider { get; set; }
    }
}