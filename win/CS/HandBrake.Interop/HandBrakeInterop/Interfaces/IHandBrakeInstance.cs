// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IHandBrakeInstance.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Interface for HandBrakeInstance
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interfaces
{
    using System;
    using System.Collections.Generic;
    using System.Windows.Media.Imaging;

    using HandBrake.Interop.EventArgs;
    using HandBrake.Interop.Model;
    using HandBrake.Interop.SourceData;

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
        /// Gets the number of previews created during scan.
        /// </summary>
        int PreviewCount { get; }

        /// <summary>
        /// Gets the list of titles on this instance.
        /// </summary>
        List<Title> Titles { get; }

        #endregion

        #region Public Methods

        /// <summary>
        /// Initializes this instance.
        /// </summary>
        /// <param name="verbosity">The code for the logging verbosity to use.</param>
        void Initialize(int verbosity);

        /// <summary>
        /// Calculates the video bitrate for the given job and target size.
        /// </summary>
        /// <param name="job">
        /// The encode job.
        /// </param>
        /// <param name="sizeMB">
        /// The target size in MB.
        /// </param>
        /// <param name="overallSelectedLengthSeconds">
        /// The currently selected encode length. Used in preview
        /// for calculating bitrate when the target size would be wrong.
        /// </param>
        /// <returns>
        /// The video bitrate in kbps.
        /// </returns>
        int CalculateBitrate(EncodeJob job, int sizeMB, double overallSelectedLengthSeconds = 0);

        /// <summary>
        /// Gives estimated file size (in MB) of the given job and video bitrate.
        /// </summary>
        /// <param name="job">
        /// The encode job.
        /// </param>
        /// <param name="videoBitrate">
        /// The video bitrate to be used (kbps).
        /// </param>
        /// <returns>
        /// The estimated file size (in MB) of the given job and video bitrate.
        /// </returns>
        double CalculateFileSize(EncodeJob job, int videoBitrate);

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
        /// <returns>
        /// An image with the requested preview.
        /// </returns>
        BitmapImage GetPreview(EncodeJob job, int previewNumber);

        /// <summary>
        /// Gets the final size for a given encode job.
        /// </summary>
        /// <param name="job">
        /// The encode job to use.
        /// </param>
        /// <param name="width">
        /// The storage width.
        /// </param>
        /// <param name="height">
        /// The storage height.
        /// </param>
        /// <param name="parWidth">
        /// The pixel aspect X number.
        /// </param>
        /// <param name="parHeight">
        /// The pixel aspect Y number.
        /// </param>
        void GetSize(EncodeJob job, out int width, out int height, out int parWidth, out int parHeight);

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
        void StartEncode(EncodeJob jobToStart);

        /// <summary>
        /// Starts an encode with the given job.
        /// </summary>
        /// <param name="job">
        /// The job to start.
        /// </param>
        /// <param name="preview">
        /// True if this is a preview encode.
        /// </param>
        /// <param name="previewNumber">
        /// The preview number to start the encode at (0-based).
        /// </param>
        /// <param name="previewSeconds">
        /// The number of seconds in the preview.
        /// </param>
        /// <param name="overallSelectedLengthSeconds">
        /// The currently selected encode length. Used in preview
        /// for calculating bitrate when the target size would be wrong.
        /// </param>
        void StartEncode(
            EncodeJob job, bool preview, int previewNumber, int previewSeconds, double overallSelectedLengthSeconds);

        /// <summary>
        /// Starts scanning the given path.
        /// </summary>
        /// <param name="path">
        /// The path to the video to scan.
        /// </param>
        /// <param name="previewCount">
        /// The number of preview images to make.
        /// </param>
        void StartScan(string path, int previewCount);

        /// <summary>
        /// Starts a scan of the given path.
        /// </summary>
        /// <param name="path">
        /// The path of the video to scan.
        /// </param>
        /// <param name="previewCount">
        /// The number of previews to make on each title.
        /// </param>
        /// <param name="titleIndex">
        /// The title index to scan (1-based, 0 for all titles).
        /// </param>
        void StartScan(string path, int previewCount, int titleIndex);

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