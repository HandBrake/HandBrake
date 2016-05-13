// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoLevel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video level.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models.Video
{
    using VideoLevelFactory = HandBrakeWPF.Services.Encode.Factories.VideoLevelFactory;

    /// <summary>
    /// The video level.
    /// </summary>
    public class VideoLevel
    {
        /// <summary>
        /// An internal representation of the Auto Selection.
        /// </summary>
        public static VideoLevel Auto = new VideoLevel("Auto", "auto");

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoLevel"/> class.
        /// </summary>
        public VideoLevel()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoLevel"/> class.
        /// </summary>
        /// <param name="displayName">
        /// The display name.
        /// </param>
        /// <param name="shortName">
        /// The short name.
        /// </param>
        public VideoLevel(string displayName, string shortName)
        {
            this.DisplayName = VideoLevelFactory.GetDisplayName(displayName);
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
        /// The <see cref="VideoProfile"/>.
        /// </returns>
        public VideoLevel Clone()
        {
            return new VideoLevel(this.DisplayName, this.ShortName);
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
        protected bool Equals(VideoLevel other)
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
            return this.Equals((VideoLevel)obj);
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
