/*  IScan.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services.Interfaces
{
    using System;

    using HandBrake.ApplicationServices.Parsing;

    /// <summary>
    /// The IScan Interface
    /// </summary>
    public interface IScan
    {
        /// <summary>
        /// Scan has Started
        /// </summary>
        event EventHandler ScanStared;

        /// <summary>
        /// Scan has completed
        /// </summary>
        event EventHandler ScanCompleted;

        /// <summary>
        /// Scan process has changed to a new title
        /// </summary>
        event ScanService.ScanProgessStatus ScanStatusChanged;

        /// <summary>
        /// Gets a value indicating whether IsScanning.
        /// </summary>
        bool IsScanning { get; }

        /// <summary>
        /// Gets the Souce Data.
        /// </summary>
        DVD SouceData { get; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        string ActivityLog { get; }

        /// <summary>
        /// Scan a Source Path.
        /// Title 0: scan all
        /// </summary>
        /// <param name="sourcePath">Path to the file to scan</param>
        /// <param name="title">int title number. 0 for scan all</param>
        void Scan(string sourcePath, int title);

        /// <summary>
        /// Kill the scan
        /// </summary>
        void Stop();
    }
}