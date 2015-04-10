// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogLevel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The log level.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Services.Logging.Model
{
    /// <summary>
    /// The log level.
    /// </summary>
    public enum LogLevel
    {
        /// <summary>
        /// The info.
        /// </summary>
        info,

        /// <summary>
        /// The warning.
        /// </summary>
        warning, 

        /// <summary>
        /// The error.
        /// </summary>
        error,      
        
        /// <summary>
        /// The debug.
        /// </summary>
        debug, 
    }
}
