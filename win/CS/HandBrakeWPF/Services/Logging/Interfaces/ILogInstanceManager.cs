﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ILogInstanceManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ILogInstanceManager type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Logging.Interfaces
{
    using System;
    using System.Collections.Generic;

    public interface ILogInstanceManager
    {
        event EventHandler NewLogInstanceRegistered;
        
        string ApplicationAndScanLog { get; }

        ILog MasterLogInstance { get; }

        /// <summary>
        /// Register an ILog instance.
        /// </summary>
        /// <param name="filename">
        /// This is the key associated with the log instance.
        /// </param>
        /// <param name="log">
        /// The ILog instance
        /// </param>
        /// <param name="isMaster">
        /// True indicates it's the log instance for the parent handbrake process.
        /// </param>
        void RegisterLoggerInstance(string filename, ILog log, bool isMaster);

        /// <summary>
        /// Gets a list of files without their associated ILog instances.
        /// </summary>
        /// <returns>List of filenames being logged</returns>
        List<string> GetLogFiles();

        /// <summary>
        /// Get the ILog instance for a given filename key
        /// </summary>
        /// <param name="filename">The key of the log instance</param>
        /// <returns>An ILog instance or null if invalid key</returns>
        ILog GetLogInstance(string filename);
    }
}
