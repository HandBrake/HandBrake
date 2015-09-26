// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoTune.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video tune.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models.Video
{
    using VideoTuneFactory = HandBrakeWPF.Services.Encode.Factories.VideoTuneFactory;

    /// <summary>
    /// The video tune.
    /// </summary>
    public class VideoTune
    {
        /// <summary>
        /// Static object to represent "None" 
        /// </summary>
        public static VideoTune None = new VideoTune("None", "none");

        /// <summary>
        /// Static object to represent "None" 
        /// </summary>
        public static VideoTune FastDecode = new VideoTune("Fast Decode", "fastdecode");

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoTune"/> class.
        /// </summary>
        public VideoTune()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoTune"/> class.
        /// </summary>
        /// <param name="displayName">
        /// The display name.
        /// </param>
        /// <param name="shortName">
        /// The short name.
        /// </param>
        public VideoTune(string displayName, string shortName)
        {
            this.DisplayName = VideoTuneFactory.GetDisplayName(displayName);
            this.ShortName = shortName;
        }

        /// <summary>
        /// Gets or sets the display name.
        /// </summary>
        public string DisplayName { get; set; }

        /// <summary>
        /// Gets or sets the short name.
        /// </summary>
        public string ShortName { get; set; }

        /// <summary>
        /// The clone.
        /// </summary>
        /// <returns>
        /// The <see cref="HandBrakeWPF.Services.Encode.Model.Models.Video.VideoProfile"/>.
        /// </returns>
        public VideoTune Clone()
        {
            return new VideoTune(this.DisplayName, this.ShortName);
        }

        /// <summary>
        /// The equals.
        /// </summary>
        /// <param name="other">
        /// The other.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        protected bool Equals(HandBrakeWPF.Services.Encode.Model.Models.Video.VideoProfile other)
        {
            return string.Equals(this.DisplayName, other.DisplayName) && string.Equals(this.ShortName, other.ShortName);
        }

        /// <summary>
        /// The equals.
        /// </summary>
        /// <param name="other">
        /// The other.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        protected bool Equals(VideoTune other)
        {
            return string.Equals(this.ShortName, other.ShortName);
        }

        /// <summary>
        /// The equals.
        /// </summary>
        /// <param name="obj">
        /// The obj.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
            {
                return false;
            }
            if (ReferenceEquals(this, obj))
            {
                return true;
            }
            if (obj.GetType() != this.GetType())
            {
                return false;
            }
            return this.Equals((VideoTune)obj);
        }

        /// <summary>
        /// The get hash code.
        /// </summary>
        /// <returns>
        /// The <see cref="int"/>.
        /// </returns>
        public override int GetHashCode()
        {
            return (this.ShortName != null ? this.ShortName.GetHashCode() : 0);
        }
    }
}
