// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeFilterHelpers.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HandBrakeFilterHelpers type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text.Json;

    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Helpers;
    using HandBrake.Interop.Interop.Interfaces.Model.Filters;
    using HandBrake.Interop.Interop.Json.Filters;
    using HandBrake.Interop.Utilities;

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
        /// The <see cref="List{T}"/>.
        /// </returns>
        public static List<HBPresetTune> GetFilterPresets(int filter)
        {
            if (filter == 0)
            {
                return new List<HBPresetTune>();
            }

            IntPtr ptr = HBFunctions.hb_filter_get_presets_json(filter);
            if (ptr == new IntPtr(0))
            {
                return new List<HBPresetTune>();
            }

            string result = Marshal.PtrToStringAnsi(ptr);
            List<PresetTune> list = JsonSerializer.Deserialize<List<PresetTune>>(result, JsonSettings.Options);

            return list.Select(item => new HBPresetTune(item.Name, item.Short_Name)).ToList();
        }

        public static HBPresetTune GetPreset(int filter, string shortName)
        {
            return GetFilterPresets(filter).FirstOrDefault(s => s.ShortName.Equals(shortName));
        }

        /// <summary>
        /// The get filter tunes.
        /// </summary>
        /// <param name="filter">
        /// The filter.
        /// </param>
        /// <returns>
        /// The <see cref="List{T}"/>.
        /// </returns>
        public static List<HBPresetTune> GetFilterTunes(int filter)
        {
            if (filter == 0)
            {
                return new List<HBPresetTune>();
            }

            IntPtr ptr = HBFunctions.hb_filter_get_tunes_json(filter);
            if (ptr == new IntPtr(0))
            {
                return new List<HBPresetTune>();
            }

            string result = Marshal.PtrToStringAnsi(ptr);
            List<PresetTune> list = JsonSerializer.Deserialize<List<PresetTune>>(result, JsonSettings.Options);

            return list.Select(item => new HBPresetTune(item.Name, item.Short_Name)).ToList();
        }

        public static HBPresetTune GetTune(int filter, string shortName)
        {
            return GetFilterTunes(filter).FirstOrDefault(s => s.ShortName.Equals(shortName));
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
        /// <returns>The default settings for that filter. Values can be strings or numbers.</returns>
        public static IDictionary<string, object> GetDefaultCustomSettings(int filter)
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
                return new Dictionary<string, object>();
            }

            IntPtr ptr = HBFunctions.hb_generate_filter_settings_json(filter, presetName, null, null);
            string result = Marshal.PtrToStringAnsi(ptr);
            return JsonSerializer.Deserialize<Dictionary<string, object>>(result, JsonSettings.Options);
        }

        public static string GenerateFilterSettingJson(int filterId, string preset, string tune, string custom)
        {
            IntPtr result = HBFunctions.hb_generate_filter_settings_json(filterId, preset, tune, custom);
            string unparsedJson = Marshal.PtrToStringAnsi(result);
            return unparsedJson;
        }
    }
}
