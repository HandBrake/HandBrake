// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoProfile.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video profile.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models.Video
{
    using VideoProfileFactory = HandBrakeWPF.Services.Encode.Factories.VideoProfileFactory;

    /// <summary>
    /// The video profile.
    /// </summary>
    public class VideoProfile
    {
        /// <summary>
        /// An internal representation of the Auto Selection.
        /// </summary>
        public static VideoProfile Auto = new VideoProfile("Auto", "auto");

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoProfile"/> class.
        /// </summary>
        public VideoProfile()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoProfile"/> class.
        /// </summary>
        /// <param name="displayName">
        /// The display name.
        /// </param>
        /// <param name="shortName">
        /// The short name.
        /// </param>
        public VideoProfile(string displayName, string shortName)
        {
            this.DisplayName = VideoProfileFactory.GetDisplayName(displayName);
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
        public VideoProfile Clone()
        {
            return new VideoProfile(this.DisplayName, this.ShortName);
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
        protected bool Equals(VideoProfile other)
        {
            return string.Equals(this.DisplayName, other.DisplayName) && string.Equals(this.ShortName, other.ShortName);
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
            return this.Equals((VideoProfile)obj);
        }

        /// <summary>
        /// The get hash code.
        /// </summary>
        /// <returns>
        /// The <see cref="int"/>.
        /// </returns>
        public override int GetHashCode()
        {
            unchecked
            {
                return ((this.DisplayName != null ? this.DisplayName.GetHashCode() : 0) * 397) ^ (this.ShortName != null ? this.ShortName.GetHashCode() : 0);
            }
        }
    }
}
