// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IScan.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Encode Progress Status
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Scan.Interfaces
{
    using System;
    using System.Collections.Generic;
    using System.Windows.Media.Imaging;

    using HandBrakeWPF.Services.Encode.Model;
    using HandBrakeWPF.Services.Scan.EventArgs;
    using HandBrakeWPF.Services.Scan.Model;

    /// <summary>
    /// Encode Progress Status
    /// </summary>
    /// <param name="sender">
    /// The sender.
    /// </param>
    /// <param name="e">
    /// The EncodeProgressEventArgs.
    /// </param>
    public delegate void ScanProgressStatus(object sender, ScanProgressEventArgs e);

    /// <summary>
    /// Encode Progress Status
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
    public interface IScan : IDisposable
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
        event ScanProgressStatus ScanStatusChanged;

        /// <summary>
        /// Gets a value indicating whether IsScanning.
        /// </summary>
        bool IsScanning { get; }

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
        void Scan(List<string> sourcePath, int title, Action<bool, Source> postAction);

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
        /// <param name="showCropBoundaries">Render crop boundary borders on the preview image.</param>
        /// <returns>
        /// The <see cref="BitmapImage"/>.
        /// </returns>
        BitmapImage GetPreview(EncodeTask task, int preview, bool showCropBoundaries);

        /// <summary>
        /// Kill the scan
        /// </summary>
        void Stop();
    }
}