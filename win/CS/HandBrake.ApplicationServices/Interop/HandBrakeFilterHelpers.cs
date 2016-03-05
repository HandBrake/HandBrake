// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeFilterHelpers.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HandBrakeFilterHelpers type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;

    using HandBrake.ApplicationServices.Interop.HbLib;
    using HandBrake.ApplicationServices.Interop.Json.Filters;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;

    using Newtonsoft.Json;

    /// <summary>
    /// The hand brake filter helpers.
    /// </summary>
    public class HandBrakeFilterHelpers
    {
        /// <summary>
        /// The get filter presets.
        /// </summary>
        /// <param name="filter">
        /// The filter.
        /// </param>
        /// <returns>
        /// The <see cref="List"/>.
        /// </returns>
        public static List<HBPresetTune> GetFilterPresets(int filter)
        {
            IntPtr ptr = HBFunctions.hb_filter_get_presets_json(filter);
            string result = Marshal.PtrToStringAnsi(ptr);
            List<PresetTune> list = JsonConvert.DeserializeObject<List<PresetTune>>(result);

            return list.Select(item => new HBPresetTune(item.Name, item.Short_Name)).ToList();
        }

        /// <summary>
        /// The get filter tunes.
        /// </summary>
        /// <param name="filter">
        /// The filter.
        /// </param>
        /// <returns>
        /// The <see cref="List"/>.
        /// </returns>
        public static List<HBPresetTune> GetFilterTunes(int filter)
        {
            IntPtr ptr = HBFunctions.hb_filter_get_tunes_json(filter);
            string result = Marshal.PtrToStringAnsi(ptr);
            List<PresetTune> list = JsonConvert.DeserializeObject<List<PresetTune>>(result);

            return list.Select(item => new HBPresetTune(item.Name, item.Short_Name)).ToList();
        }
    }
}
