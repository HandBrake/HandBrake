// --------------------------------------------------------------------------------------------------------------------
// <copyright file="JsonSettings.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the JsonSettings type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Json
{
    using System.Text.Json;

    public class JsonSettings
    {
        public static JsonSerializerOptions Options = new JsonSerializerOptions
                                                            {
                                                                IgnoreNullValues = true,
                                                                WriteIndented = true,
                                                                PropertyNameCaseInsensitive = true
                                                            };
    }
}
