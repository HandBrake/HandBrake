// --------------------------------------------------------------------------------------------------------------------
// <copyright file="JsonSettings.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the JsonSettings type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Utilities
{
    using System.Text.Json;
    using System.Text.Json.Serialization;

    public class JsonSettings
    {
        /// <summary>
        /// For consistent use of JSON settings between this library and the consumers.
        /// </summary>
        public static JsonSerializerOptions Options = new JsonSerializerOptions
                                                            {
                                                                DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
                                                                WriteIndented = true,
                                                                PropertyNameCaseInsensitive = true
                                                            };
    }
}
