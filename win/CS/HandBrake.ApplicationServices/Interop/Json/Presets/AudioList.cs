// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioList.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The audio list.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Json.Presets
{
    /// <summary>
    /// The audio list.
    /// </summary>
    public class AudioList
    {
        /// <summary>
        /// Gets or sets the audio bitrate.
        /// </summary>
        public string AudioBitrate { get; set; }

        /// <summary>
        /// Gets or sets the audio encoder.
        /// </summary>
        public string AudioEncoder { get; set; }

        /// <summary>
        /// Gets or sets the audio mixdown.
        /// </summary>
        public string AudioMixdown { get; set; }

        /// <summary>
        /// Gets or sets the audio samplerate.
        /// </summary>
        public string AudioSamplerate { get; set; }

        /// <summary>
        /// Gets or sets the audio track.
        /// </summary>
        public int AudioTrack { get; set; }

        /// <summary>
        /// Gets or sets the audio track drc slider.
        /// </summary>
        public double AudioTrackDRCSlider { get; set; }

        /// <summary>
        /// Gets or sets the audio track gain slider.
        /// </summary>
        public double AudioTrackGainSlider { get; set; }
    }
}
