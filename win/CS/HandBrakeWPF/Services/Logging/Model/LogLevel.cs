// --------------------------------------------------------------------------------------------------------------------
// <copyright file="LogLevel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The log level.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging.Model
{
    /// <summary>
    /// The log level.
    /// </summary>
    public enum LogLevel
    {
        /// <summary>
        /// The info.
        /// </summary>
        Info,

        /// <summary>
        /// The warning.
        /// </summary>
        Warning, 

        /// <summary>
        /// The error.
        /// </summary>
        Error,      
        
        /// <summary>
        /// The debug.
        /// </summary>
        Debug,
        
        /// <summary>
        /// Trace Level Logging.
        /// </summary>
        Trace, 
    }
}
