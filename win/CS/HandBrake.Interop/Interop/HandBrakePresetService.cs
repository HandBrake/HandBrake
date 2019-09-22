// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakePresetService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hand brake preset service.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Runtime.InteropServices;

    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.HbLib.Wrappers.Interfaces;
    using HandBrake.Interop.Interop.Helpers;
    using HandBrake.Interop.Interop.Json.Presets;
    using HandBrake.Interop.Interop.Model;
    using HandBrake.Interop.Interop.Providers;
    using HandBrake.Interop.Interop.Providers.Interfaces;

    using Newtonsoft.Json;

    /// <summary>
    /// The hand brake preset service.
    /// </summary>
    public class HandBrakePresetService
    {
        private static IHbFunctions hbFunctions;

        static HandBrakePresetService()
        {
            IHbFunctionsProvider hbFunctionsProvider = new HbFunctionsProvider();
            hbFunctions = hbFunctionsProvider.GetHbFunctionsWrapper();
        }

        /// <summary>
        /// The get built in presets.
        /// Requires an hb_init to have been invoked.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static IList<PresetCategory> GetBuiltInPresets()
        {
            IntPtr presets = hbFunctions.hb_presets_builtin_get_json();
            string presetJson = Marshal.PtrToStringAnsi(presets);
            IList<PresetCategory> presetList = JsonConvert.DeserializeObject<IList<PresetCategory>>(presetJson);

            return presetList;
        }

        /// <summary>
        /// The get preset from file.
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        /// <returns>
        /// The <see cref="PresetCategory"/>.
        /// </returns>
        public static PresetTransportContainer GetPresetsFromFile(string filename)
        {
            IntPtr presetStringPointer = hbFunctions.hb_presets_read_file_json(InteropUtilities.ToUtf8PtrFromString(filename));
            string presetJson = Marshal.PtrToStringAnsi(presetStringPointer);

            if (!string.IsNullOrEmpty(presetJson))
            {
                // Check to see if we have a list of presets.
                if (presetJson.StartsWith("["))
                {
                    presetJson = "{ \"PresetList\":" + presetJson + " } ";
                }

                PresetTransportContainer preset = JsonConvert.DeserializeObject<PresetTransportContainer>(presetJson);

                return preset;
            }

            return null;
        }

        /// <summary>
        /// The export preset.
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        /// <param name="container">
        /// The container.
        /// </param>
        public static void ExportPreset(string filename, PresetTransportContainer container)
        {
            string preset = JsonConvert.SerializeObject(container, Formatting.Indented, new JsonSerializerSettings { NullValueHandling = NullValueHandling.Ignore });
            using (StreamWriter writer = new StreamWriter(filename))
            {
                writer.Write(preset);
            }
        }

        public static PresetVersion GetCurrentPresetVersion()
        {
            IntPtr major = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(int)));
            IntPtr minor = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(int)));
            IntPtr micro = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(int)));

            hbFunctions.hb_presets_current_version(major, minor, micro);

            int majorVersion = Marshal.ReadInt32(major);
            int minorVersion = Marshal.ReadInt32(minor);
            int microVersion = Marshal.ReadInt32(micro);

            Marshal.FreeHGlobal(major);
            Marshal.FreeHGlobal(minor);
            Marshal.FreeHGlobal(micro);

            return new PresetVersion(majorVersion, minorVersion, microVersion);
        }
    }
}
