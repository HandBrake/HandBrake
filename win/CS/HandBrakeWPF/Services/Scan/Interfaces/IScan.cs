// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IScan.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Encode Progess Status
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan.Interfaces
{
    using System;
    using System.Windows.Media.Imaging;

    using HandBrake.ApplicationServices.Model;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Scan.EventArgs;
    using HandBrakeWPF.Services.Scan.Model;

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
        event EventHandler ScanStarted;

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
        /// <param name="postAction">
        /// The post Action.
        /// </param>
        /// <param name="configuration">
        /// The configuraiton.
        /// </param>
        void Scan(string sourcePath, int title, Action<bool, Source> postAction, HBConfiguration configuration);

        /// <summary>
        /// Cancel the current scan.
        /// </summary>
        void Cancel();

        /// <summary>
        /// Get a Preview image for the current job and preview number.
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        /// <param name="preview">
        /// The preview.
        /// </param>
        /// <param name="configuration">
        /// The configuration.
        /// </param>
        /// <returns>
        /// The <see cref="BitmapImage"/>.
        /// </returns>
        BitmapImage GetPreview(EncodeTask task, int preview, HBConfiguration configuration);

        /// <summary>
        /// Kill the scan
        /// </summary>
        void Stop();
    }
}