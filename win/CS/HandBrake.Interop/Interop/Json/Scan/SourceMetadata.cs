// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceMetadata.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The meta data.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Json.Scan
{
    using System.Collections.Generic;

    /// <summary>
    /// The meta data.
    /// </summary>
    public class SourceMetadata
    {
        public Dictionary<string, string> Metadata { get; set; }
    }
}