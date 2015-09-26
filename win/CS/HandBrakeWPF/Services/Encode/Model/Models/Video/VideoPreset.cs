// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoPreset.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The video preset.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Encode.Model.Models.Video
{
    using VideoPresetFactory = HandBrakeWPF.Services.Encode.Factories.VideoPresetFactory;

    /// <summary>
    /// The video preset.
    /// </summary>
    public class VideoPreset
    {
        /// <summary>
        /// A built-in version of the "None" object.
        /// </summary>
        public static VideoPreset None = new VideoPreset("None", "none");

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoPreset"/> class.
        /// </summary>
        public VideoPreset()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="VideoPreset"/> class.
        /// </summary>
        /// <param name="displayName">
        /// The display name.
        /// </param>
        /// <param name="shortName">
        /// The short name.
        /// </param>
        public VideoPreset(string displayName, string shortName)
        {
            this.DisplayName = VideoPresetFactory.GetDisplayName(displayName);
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
        public VideoPreset Clone()
        {
            return new VideoPreset(this.DisplayName, this.ShortName);
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
        protected bool Equals(VideoPreset other)
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

            return this.Equals((VideoPreset)obj);
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
