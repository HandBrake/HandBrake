// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakePresetService.cs" company="HandBrake Project (https://handbrake.fr)">
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
    using System.ComponentModel;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text.Json;

    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Helpers;
    using HandBrake.Interop.Interop.Interfaces.Model.Presets;
    using HandBrake.Interop.Interop.Json.Presets;
    using HandBrake.Interop.Utilities;

    /// <summary>
    /// The hand brake preset service.
    /// </summary>
    public class HandBrakePresetService
    {
        /// <summary>
        /// The get built in presets.
        /// Requires an hb_init to have been invoked.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static IList<HBPresetCategory> GetBuiltInPresets()
        {
            IntPtr presets = HBFunctions.hb_presets_builtin_get_json();
            string presetJson = Marshal.PtrToStringAnsi(presets);
            IList<HBPresetCategory> presetList = JsonSerializer.Deserialize<IList<HBPresetCategory>>(presetJson, JsonSettings.Options);

            return presetList;
        }

        public static PresetTransportContainer GetPresetsFromFile(string filename)
        {
            IntPtr presetStringPointer = HBFunctions.hb_presets_read_file_json(InteropUtilities.ToUtf8PtrFromString(filename));
            PresetTransportContainer container = GetPresets(presetStringPointer);
            
            return container;
        }

        public static PresetTransportContainer UpgradePresets(string filename)
        {
            IntPtr presetStringPointer = HBFunctions.hb_presets_read_file_json(InteropUtilities.ToUtf8PtrFromString(filename));
            PresetTransportContainer container = GetPresets(presetStringPointer);

            // If Presets are of a different version, try upgrade them for the user.
            PresetVersion presetVersion = GetCurrentPresetVersion();
            if (container.VersionMajor != presetVersion.Major || container.VersionMinor != presetVersion.Minor || container.VersionMicro != presetVersion.Micro)
            {
                IntPtr outputPtr = IntPtr.Zero;
                HBFunctions.hb_presets_import_json(presetStringPointer, ref outputPtr);
                container = GetPresets(outputPtr);
            }

            return container;
        }

        private static PresetTransportContainer GetPresets(IntPtr outputPtr)
        {
            string presetJson = Marshal.PtrToStringUTF8(outputPtr);
            if (!string.IsNullOrEmpty(presetJson))
            {
                // Check to see if we have a list of presets.
                if (presetJson.StartsWith("["))
                {
                    presetJson = "{ \"PresetList\":" + presetJson + " } ";
                }

                PresetTransportContainer preset = JsonSerializer.Deserialize<PresetTransportContainer>(presetJson, JsonSettings.Options);

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
            string preset = JsonSerializer.Serialize(container, JsonSettings.Options);
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

            HBFunctions.hb_presets_current_version(major, minor, micro);

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
