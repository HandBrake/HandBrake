// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceAudioTrack.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An audio track from the source video.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Scan
{
    /// <summary>
    /// An audio track from the source video.
    /// </summary>
    public class SourceAudioTrack
    {
        /// <summary>
        /// Gets or sets the track number (1-based).
        /// </summary>
        public int TrackNumber { get; set; }

        /// <summary>
        /// Gets or sets the bit rate.
        /// </summary>
        public int BitRate { get; set; }

        /// <summary>
        /// Gets or sets the channel layout.
        /// </summary>
        public long ChannelLayout { get; set; }

        /// <summary>
        /// Gets or sets the description.
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Gets or sets the language.
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Gets or sets the language code.
        /// </summary>
        public string LanguageCode { get; set; }

        /// <summary>
        /// Gets or sets the sample rate.
        /// </summary>
        public int SampleRate { get; set; }

        /// <summary>
        /// Gets or sets the codec.
        /// </summary>
        public int Codec { get; set; }

        /// <summary>
        /// Parameters for the codec.
        /// </summary>
        public int CodecParam { get; set; }

        public string CodecName { get; set; }

        public long LFECount { get; set; }

        public string ChannelLayoutName { get; set; }

        public int ChannelCount { get; set; }

        public AudioAttributes Attributes { get; set; }

        public string Name { get; set; }
    }
}