// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IHandBrakeInstance.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Interface for HandBrakeInstance
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop.Interfaces
{
    using System;

    using HandBrake.ApplicationServices.Interop.EventArgs;
    using HandBrake.ApplicationServices.Interop.Json.Encode;
    using HandBrake.ApplicationServices.Interop.Json.Scan;
    using HandBrake.ApplicationServices.Interop.Model.Preview;

    /// <summary>
    /// The Interface for HandBrakeInstance
    /// </summary>
    public interface IHandBrakeInstance
    {
        #region Events

        /// <summary>
        /// Fires when an encode has completed.
        /// </summary>
        event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

        /// <summary>
        /// Fires for progress updates when encoding.
        /// </summary>
        event EventHandler<EncodeProgressEventArgs> EncodeProgress;

        /// <summary>
        /// Fires when a scan has completed.
        /// </summary>
        event EventHandler<EventArgs> ScanCompleted;

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
        /// Initializes this instance.
        /// </summary>
        /// <param name="verbosity">
        /// The code for the logging verbosity to use.
        /// </param>
        void Initialize(int verbosity);

        /// <summary>
        /// Frees any resources associated with this object.
        /// </summary>
        void Dispose();

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
        /// <param name="deinterlace">
        /// True to enable basic deinterlace of preview images.
        /// </param>
        /// <returns>
        /// An image with the requested preview.
        /// </returns>
        RawPreviewData GetPreview(PreviewSettings job, int previewNumber, bool deinterlace);

        /// <summary>
        /// Pauses the current encode.
        /// </summary>
        void PauseEncode();

        /// <summary>
        /// Resumes a paused encode.
        /// </summary>
        void ResumeEncode();

        /// <summary>
        /// Starts an encode with the given job.
        /// </summary>
        /// <param name="jobToStart">
        /// The job to start.
        /// </param>
        void StartEncode(JsonEncodeObject jobToStart);

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
        void StartScan(string path, int previewCount, TimeSpan minDuration, int titleIndex);

        /// <summary>
        /// Stops the current encode.
        /// </summary>
        void StopEncode();

        /// <summary>
        /// Stop any running scans
        /// </summary>
        void StopScan();

        #endregion
    }
}