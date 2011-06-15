/*  IScan.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services.Interfaces
{
    using System;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Parsing;

    /// <summary>
    /// Encode Progess Status
    /// </summary>
    /// <param name="sender">
    /// The sender.
    /// </param>
    /// <param name="e">
    /// The EncodeProgressEventArgs.
    /// </param>
    public delegate void ScanProgessStatus(object sender, ScanProgressEventArgs e);

    /// <summary>
    /// Encode Progess Status
    /// </summary>
    /// <param name="sender">
    /// The sender.
    /// </param>
    /// <param name="e">
    /// The ScanCompletedEventArgs.
    /// </param>
    public delegate void ScanCompletedStatus(object sender, ScanCompletedEventArgs e);

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
        event ScanCompletedStatus ScanCompleted;

        /// <summary>
        /// Scan process has changed to a new title
        /// </summary>
        event ScanProgessStatus ScanStatusChanged;

        /// <summary>
        /// Gets a value indicating whether IsScanning.
        /// </summary>
        bool IsScanning { get; }

        /// <summary>
        /// Gets the Souce Data.
        /// </summary>
        Source SouceData { get; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        string ActivityLog { get; }

        /// <summary>
        /// Scan a Source Path.
        /// Title 0: scan all
        /// </summary>
        /// <param name="sourcePath">
        /// Path to the file to scan
        /// </param>
        /// <param name="title">
        /// int title number. 0 for scan all
        /// </param>
        /// <param name="previewCount">
        /// The preview Count.
        /// </param>
        void Scan(string sourcePath, int title, int previewCount);

        /// <summary>
        /// Kill the scan
        /// </summary>
        void Stop();
    }
}