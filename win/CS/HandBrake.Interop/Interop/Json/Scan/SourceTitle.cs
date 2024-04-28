// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceTitle.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The title list.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Scan
{
    using System.Collections.Generic;
    using System.Text.Json.Serialization;

    using HandBrake.Interop.Interop.Json.Shared;

    /// <summary>
    /// The title list.
    /// </summary>
    public class SourceTitle
    {
        /// <summary>
        /// Gets or sets the angle count.
        /// </summary>
        public int AngleCount { get; set; }

        /// <summary>
        /// Gets or sets the audio list.
        /// </summary>
        public List<SourceAudioTrack> AudioList { get; set; }

        /// <summary>
        /// Gets or sets the chapter list.
        /// </summary>
        public List<SourceChapter> ChapterList { get; set; }

        /// <summary>
        /// Gets or sets the color.
        /// </summary>
        public Color Color { get; set; }

        /// <summary>
        /// Gets or sets the input file container.
        /// </summary>
        public string Container { get; set; }

        /// <summary>
        /// Gets or sets the cropping values
        /// </summary>
        public List<int> Crop { get; set; }

        /// <summary>
        /// Gets or sets the cropping values
        /// </summary>
        public List<int> LooseCrop { get; set; }

        /// <summary>
        /// Gets or sets the duration.
        /// </summary>
        public Duration Duration { get; set; }

        /// <summary>
        /// Gets or sets the frame rate.
        /// </summary>
        public FrameRate FrameRate { get; set; }

        /// <summary>
        /// Gets or sets the geometry.
        /// </summary>
        public Geometry Geometry { get; set; }

        /// <summary>
        /// Gets or sets the index.
        /// </summary>
        public int Index { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether interlace detected.
        /// </summary>
        public bool InterlaceDetected { get; set; }

        /// <summary>
        /// Gets or sets the meta data.
        /// </summary>
        public Dictionary<string, string> MetaData { get; set; }

        /// <summary>
        /// Gets or sets the name.
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the path.
        /// </summary>
        public string Path { get; set; }

        /// <summary>
        /// Gets or sets the playlist.
        /// </summary>
        public int Playlist { get; set; }

        /// <summary>
        /// Gets or sets the subtitle list.
        /// </summary>
        public List<SourceSubtitleTrack> SubtitleList { get; set; }

        /// <summary>
        /// Gets or sets the type.
        ///  HB_DVD_TYPE = 0, HB_BD_TYPE, HB_STREAM_TYPE, HB_FF_STREAM_TYPE
        /// </summary>
        public int Type { get; set; }

        /// <summary>
        /// Gets or sets the video codec.
        /// </summary>
        public string VideoCodec { get; set; }

        public DVConfigRecord DolbyVisionConfigurationRecord { get; set; }

        [JsonPropertyName("HDR10+")]
        public int? HDR10plus { get; set; }

        public MasteringDisplayColorVolume MasteringDisplayColorVolume { get; set; }
    } 
}