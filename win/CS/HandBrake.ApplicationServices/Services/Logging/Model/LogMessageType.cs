// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogMessageType.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The log message type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Logging.Model
{
    /// <summary>
    /// The log message type.
    /// </summary>
    public enum LogMessageType
    {
        scanJson,
        encodeJson,
        anamorphicJson,
        progressJson,
        libhb,
    }
}
