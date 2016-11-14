﻿// --------------------------------------------------------------------------------------------------------------------
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
    using HandBrake.ApplicationServices.Interop.Helpers;
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

        /// <summary>
        /// Gets a list of keys for custom settings for the filter.
        /// </summary>
        /// <param name="filter">The filter to look up.</param>
        /// <returns>The list of keys for custom settings for the filter.</returns>
        public static List<string> GetFilterKeys(int filter)
        {
            IntPtr ptr = HBFunctions.hb_filter_get_keys(filter);
            return InteropUtilities.ToStringListFromArrayPtr(ptr);
        }

        /// <summary>
        /// Gets the default settings for the filter.
        /// </summary>
        /// <param name="filter">The filter to look up.</param>
        /// <returns>The default settings for that filter.</returns>
        public static IDictionary<string, string> GetDefaultCustomSettings(int filter)
        {
            string presetName;

            List<HBPresetTune> presets = GetFilterPresets(filter);
            if (presets.Any(p => p.ShortName == "default"))
            {
                presetName = "default";
            }
            else if (presets.Any(p => p.ShortName == "medium"))
            {
                presetName = "medium";
            }
            else
            {
                return new Dictionary<string, string>();
            }

            IntPtr ptr = HBFunctions.hb_generate_filter_settings_json(filter, presetName, null, null);
            string result = Marshal.PtrToStringAnsi(ptr);
            return JsonConvert.DeserializeObject<Dictionary<string, string>>(result);
        }
    }
}
