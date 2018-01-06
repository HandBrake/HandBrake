// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBContainer.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hb container.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Model.Encoding
{
    /// <summary>
    /// The hb container.
    /// </summary>
    public class HBContainer
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="HBContainer"/> class.
        /// </summary>
        /// <param name="defaultExtension">
        /// The default extension.
        /// </param>
        /// <param name="displayName">
        /// The display name.
        /// </param>
        /// <param name="id">
        /// The id.
        /// </param>
        /// <param name="shortName">
        /// The short name.
        /// </param>
        public HBContainer(string defaultExtension, string displayName, int id, string shortName)
        {
            this.DefaultExtension = defaultExtension;
            this.DisplayName = displayName;
            this.Id = id;
            this.ShortName = shortName;
        }

        /// <summary>
        /// Gets the default extension.
        /// </summary>
        public string DefaultExtension { get; private set; }

        /// <summary>
        /// Gets the display name.
        /// </summary>
        public string DisplayName { get; private set; }

        /// <summary>
        /// Gets the id.
        /// </summary>
        public int Id { get; private set; }

        /// <summary>
        /// Gets the short name.
        /// </summary>
        public string ShortName { get; private set; }
    }
}