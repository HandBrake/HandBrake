// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HBVideoEncoder.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hb video encoder.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Model.Encoding
{
    using System.Collections.Generic;

    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.HbLib.Wrappers.Interfaces;
    using HandBrake.Interop.Interop.Helpers;
    using HandBrake.Interop.Interop.Providers;
    using HandBrake.Interop.Interop.Providers.Interfaces;

    /// <summary>
    /// The hb video encoder.
    /// </summary>
    public class HBVideoEncoder
    {
        private IHbFunctions hbFunctions;

        /// <summary>
        /// Initializes a new instance of the <see cref="HBVideoEncoder"/> class.
        /// </summary>
        /// <param name="compatibleContainers">
        /// The compatible containers.
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
        public HBVideoEncoder(int compatibleContainers, string displayName, int id, string shortName)
        {
            IHbFunctionsProvider hbFunctionsProvider = new HbFunctionsProvider();
            hbFunctions = hbFunctionsProvider.GetHbFunctionsWrapper();

            this.CompatibleContainers = compatibleContainers;
            this.DisplayName = displayName;
            this.Id = id;
            this.ShortName = shortName;
        }

        /// <summary>
        /// Gets the compatible containers.
        /// </summary>
        public int CompatibleContainers { get; private set; }

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

        /// <summary>
        /// Gets the list of presets this encoder supports. (null if the encoder doesn't support presets)
        /// </summary>
        public List<string> Presets
        {
            get
            {
                return InteropUtilities.ToStringListFromArrayPtr(hbFunctions.hb_video_encoder_get_presets(this.Id));
            }
        }

        /// <summary>
        /// Gets the list of tunes this encoder supports. (null if the encoder doesn't support tunes)
        /// </summary>
        public List<string> Tunes
        {
            get
            {
                return InteropUtilities.ToStringListFromArrayPtr(hbFunctions.hb_video_encoder_get_tunes(this.Id));
            }
        }

        /// <summary>
        /// Gets the list of profiles this encoder supports. (null if the encoder doesn't support profiles)
        /// </summary>
        public List<string> Profiles
        {
            get
            {
                return InteropUtilities.ToStringListFromArrayPtr(hbFunctions.hb_video_encoder_get_profiles(this.Id));
            }
        }

        /// <summary>
        /// Gets the list of levels this encoder supports. (null if the encoder doesn't support levels)
        /// </summary>
        public List<string> Levels
        {
            get
            {
                return InteropUtilities.ToStringListFromArrayPtr(hbFunctions.hb_video_encoder_get_levels(this.Id));
            }
        } 
    }
}