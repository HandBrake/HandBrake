﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IHandBrakeInstance.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Interface for HandBrakeInstance
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop.Interfaces
{
    using System;
    using System.Collections.Generic;

    using HandBrake.Interop.Interop.Interfaces.EventArgs;
    using HandBrake.Interop.Interop.Interfaces.Model.Preview;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.Scan;

    /// <summary>
    /// The Interface for HandBrakeInstance
    /// </summary>
    public interface IHandBrakeInstance : IEncodeInstance
    {
        #region Events

        /// <summary>
        /// Fires when a scan has completed.
        /// </summary>
        event EventHandler<System.EventArgs> ScanCompleted;

        /// <summary>
        /// Fires for progress updates when scanning.
        /// </summary>
        event EventHandler<ScanProgressEventArgs> ScanProgress;

        #endregion

        #region Properties

        /// <summary>
        /// Gets the index of the default title.
        /// </summary>
        int FeatureTitle { get; }

        /// <summary>
        /// Gets the list of titles on this instance.
        /// </summary>
        JsonScanObject Titles { get; }

        /// <summary>
        /// Gets the HandBrake version string.
        /// </summary>
        string Version { get; }

        /// <summary>
        /// Gets the HandBrake build number.
        /// </summary>
        int Build { get; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Gets an image for the given job and preview
        /// </summary>
        /// <remarks>
        /// Only incorporates sizing and aspect ratio into preview image.
        /// </remarks>
        /// <param name="job">
        /// The encode job to preview.
        /// </param>
        /// <param name="previewNumber">
        /// The index of the preview to get (0-based).
        /// </param>
        /// <returns>
        /// An image with the requested preview.
        /// </returns>
        RawPreviewData GetPreview(JsonEncodeObject job, int previewNumber);

        /// <summary>
        /// Starts a scan of the given path.
        /// </summary>
        /// <param name="path">
        /// The path of the video to scan.
        /// </param>
        /// <param name="previewCount">
        /// The number of previews to make on each title.
        /// </param>
        /// <param name="minDuration">
        /// The min Duration.
        /// </param>
        /// <param name="titleIndex">
        /// The title Index.
        /// </param>
        /// <param name="excludedExtensions">
        /// A list of file extensions to exclude.
        /// These should be the extension name only. No .
        /// Case Insensitive.
        /// </param>
        void StartScan(string path, int previewCount, TimeSpan minDuration, int titleIndex, List<string> excludedExtensions);

        /// <summary>
        /// Stop any running scans
        /// </summary>
        void StopScan();

        #endregion
    }
}