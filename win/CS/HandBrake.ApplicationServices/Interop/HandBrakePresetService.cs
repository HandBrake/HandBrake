﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakePresetService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The hand brake preset service.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Runtime.InteropServices;

    using HandBrake.ApplicationServices.Interop.HbLib;
    using HandBrake.ApplicationServices.Interop.Helpers;
    using HandBrake.ApplicationServices.Interop.Json.Presets;
    using HandBrake.ApplicationServices.Services.Logging;
    using HandBrake.ApplicationServices.Services.Logging.Interfaces;
    using HandBrake.ApplicationServices.Services.Logging.Model;

    using Newtonsoft.Json;

    /// <summary>
    /// The hand brake preset service.
    /// </summary>
    public class HandBrakePresetService
    {
        private static readonly ILog log = LogService.GetLogger();

        /// <summary>
        /// The get built in presets.
        /// Requires an hb_init to have been invoked.
        /// </summary>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static IList<PresetCategory> GetBuiltInPresets()
        {
            IntPtr presets = HBFunctions.hb_presets_builtin_get_json();
            string presetJson = Marshal.PtrToStringAnsi(presets);

            log.LogMessage(presetJson, LogMessageType.API, LogLevel.Debug);

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
        public static PresetTransportContainer GetPresetFromFile(string filename)
        {
            IntPtr presetStringPointer = HBFunctions.hb_presets_read_file_json(InteropUtilities.ToUtf8PtrFromString(filename));
            string presetJson = Marshal.PtrToStringAnsi(presetStringPointer);
            log.LogMessage(presetJson, LogMessageType.API, LogLevel.Debug);

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
            string preset = JsonConvert.SerializeObject(container, Formatting.Indented);
            using (StreamWriter writer = new StreamWriter(filename))
            {
                writer.Write(preset);
            }
        }
    }
}
