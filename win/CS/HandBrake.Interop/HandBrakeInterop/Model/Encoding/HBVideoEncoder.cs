// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBVideoEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hb video encoder.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    /// <summary>
    /// The hb video encoder.
    /// </summary>
    public class HBVideoEncoder
    {
        #region Public Properties

        /// <summary>
        /// Gets or sets the compatible containers.
        /// </summary>
        public int CompatibleContainers { get; set; }

        /// <summary>
        /// Gets or sets the display name.
        /// </summary>
        public string DisplayName { get; set; }

        /// <summary>
        /// Gets or sets the id.
        /// </summary>
        public int Id { get; set; }

        /// <summary>
        /// Gets or sets the short name.
        /// </summary>
        public string ShortName { get; set; }

        #endregion
    }
}