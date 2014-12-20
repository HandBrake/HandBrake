// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TitleList.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The title list.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Json.Scan
{
    using System.Collections.Generic;

    /// <summary>
    /// The title list.
    /// </summary>
    internal class TitleList
    {
        /// <summary>
        /// Gets or sets the angle count.
        /// </summary>
        public int AngleCount { get; set; }

        /// <summary>
        /// Gets or sets the audio list.
        /// </summary>
        public List<AudioList> AudioList { get; set; }

        /// <summary>
        /// Gets or sets the chapter list.
        /// </summary>
        public List<ChapterList> ChapterList { get; set; }

        /// <summary>
        /// Gets or sets the color.
        /// </summary>
        public Color Color { get; set; }

        /// <summary>
        /// Gets or sets the cropping values
        /// </summary>
        public List<int> Crop { get; set; }

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
        public MetaData MetaData { get; set; }

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
        public List<SubtitleList> SubtitleList { get; set; }

        /// <summary>
        /// Gets or sets the type.
        /// </summary>
        public int Type { get; set; }

        /// <summary>
        /// Gets or sets the video codec.
        /// </summary>
        public string VideoCodec { get; set; }
    }
}