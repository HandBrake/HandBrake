// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeInstance.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A wrapper for a HandBrake instance.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Runtime.InteropServices;
    using System.Text.Json;
    using System.Timers;

    using HandBrake.Interop.Interop.HbLib;
    using HandBrake.Interop.Interop.Helpers;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Interfaces.EventArgs;
    using HandBrake.Interop.Interop.Interfaces.Model.Preview;
    using HandBrake.Interop.Interop.Json.Encode;
    using HandBrake.Interop.Interop.Json.Scan;
    using HandBrake.Interop.Interop.Json.State;
    using HandBrake.Interop.Utilities;

    public class HandBrakeInstance : IEncodeInstance, IScanInstance
    {
        private const double ScanPollIntervalMs = 250;
        private const double EncodePollIntervalMs = 250;
        private Timer scanPollTimer;
        private Timer encodePollTimer;
        private bool disposed;

        private JsonState lastProgressJson;
        private readonly object progressJsonLockObj = new object();
        private EncodeProgressEventArgs lastEncodeProgress = null;

        /// <summary>
        /// Finalizes an instance of the HandBrakeInstance class.
        /// </summary>
        ~HandBrakeInstance()
        {
            if (this.Handle != IntPtr.Zero)
            {
                this.Dispose(false);
            }
        }

        /// <summary>
        /// Fires for progress updates when scanning.
        /// </summary>
        public event EventHandler<ScanProgressEventArgs> ScanProgress;

        /// <summary>
        /// Fires when a scan has completed.
        /// </summary>
        public event EventHandler<System.EventArgs> ScanCompleted;

        /// <summary>
        /// Fires for progress updates when encoding.
        /// </summary>
        public event EventHandler<EncodeProgressEventArgs> EncodeProgress;

        /// <summary>
        /// Fires when an encode has completed.
        /// </summary>
        public event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

        /// <summary>
        /// Gets the number of previews created during scan.
        /// </summary>
        public int PreviewCount { get; private set; }

        /// <summary>
        /// Gets the list of titles on this instance.
        /// </summary>
        public JsonScanObject Titles { get; private set; }

        /// <summary>
        /// Gets the raw JSON for the list of titles on this instance.
        /// </summary>
        public string TitlesJson { get; private set; }

        /// <summary>
        /// Gets the index of the default title.
        /// </summary>
        public int FeatureTitle { get; private set; }

        /// <summary>
        /// Gets the HandBrake version string.
        /// </summary>
        public string Version => Marshal.PtrToStringAnsi(HBFunctions.hb_get_version(this.Handle));

        /// <summary>
        /// Gets the HandBrake build number.
        /// </summary>
        public int Build => HBFunctions.hb_get_build(this.Handle);

        public bool IsRemoteInstance => false;

        /// <summary>
        /// Gets the handle.
        /// </summary>
        internal IntPtr Handle { get; private set; }

        /// <summary>
        /// Initializes this instance.
        /// </summary>
        /// <param name="verbosity">
        /// The code for the logging verbosity to use.
        /// </param>
        /// <param name="noHardware">
        /// True disables hardware init.
        /// </param>
        public void Initialize(int verbosity, bool noHardware)
        {
            HandBrakeUtils.EnsureGlobalInit(noHardware);

            HandBrakeUtils.RegisterLogger();
            this.Handle = HBFunctions.hb_init(verbosity, update_check: 0);
        }

        /// <summary>
        /// Starts a scan of the given path.
        /// </summary>
        /// <param name="paths">
        /// The path of the video to scan.
        /// </param>
        /// <param name="previewCount">
        /// The number of previews to make on each title.
        /// </param>
        /// <param name="minDuration">
        /// The minimum duration of a title to show up on the scan.
        /// </param>
        /// <param name="titleIndex">
        /// The title index to scan (1-based, 0 for all titles).
        /// </param>
        /// <param name="excludedExtensions">
        /// A list of file extensions to exclude.
        /// These should be the extension name only. No .
        /// Case Insensitive.
        /// </param>
        /// <param name="hwDecode">
        /// Hardware decoding during scans.
        /// </param>
        public void StartScan(List<string> paths, int previewCount, TimeSpan minDuration, int titleIndex, List<string> excludedExtensions, int hwDecode, bool keepDuplicateTitles)
        {
            this.PreviewCount = previewCount;

            // File Exclusions
            NativeList excludedExtensionsNative = null;
            if (excludedExtensions != null && excludedExtensions.Count > 0)
            {
                excludedExtensionsNative = NativeList.CreateList();
                foreach (string extension in excludedExtensions)
                {
                    excludedExtensionsNative.Add(InteropUtilities.ToUtf8PtrFromString(extension));
                }
            }

            // Handle Scan Paths
            NativeList scanPathsList = NativeList.CreateList();
            foreach (string path in paths)
            {
                scanPathsList.Add(InteropUtilities.ToUtf8PtrFromString(path));
            }

            // Start the Scan
            IntPtr excludedExtensionsPtr = excludedExtensionsNative?.Ptr ?? IntPtr.Zero;
            HBFunctions.hb_scan(this.Handle, scanPathsList.Ptr, titleIndex, previewCount, 1, (ulong)(minDuration.TotalSeconds * 90000), 0, 0, excludedExtensionsPtr, hwDecode, Convert.ToInt32(keepDuplicateTitles));

            this.scanPollTimer = new Timer();
            this.scanPollTimer.Interval = ScanPollIntervalMs;

            // Lambda notation used to make sure we can view any JIT exceptions the method throws
            this.scanPollTimer.Elapsed += (o, e) =>
            {
                try
                {
                    this.PollScanProgress(excludedExtensionsNative);
                }
                catch (Exception exc)
                {
                    Debug.WriteLine(exc);
                    HandBrakeUtils.SendErrorEvent(exc.ToString());
                }
            };
            this.scanPollTimer.Start();
        }

        /// <summary>
        /// Stops an ongoing scan.
        /// </summary>
        public void StopScan()
        {
            this.scanPollTimer.Stop();
            HBFunctions.hb_scan_stop(this.Handle);
        }

        /// <summary>
        /// Gets an image for the given job and preview
        /// </summary>
        /// <remarks>
        /// Only incorporates sizing and aspect ratio into preview image.
        /// </remarks>
        /// <param name="settings">
        /// The encode job to preview.
        /// </param>
        /// <param name="previewNumber">
        /// The index of the preview to get (0-based).
        /// </param>
        /// <returns>
        /// An image with the requested preview.
        /// </returns>
        public RawPreviewData GetPreview(JsonEncodeObject settings, int previewNumber)
        {
            // Fetch the image data from LibHb
            string taskJson = JsonSerializer.Serialize(settings, JsonSettings.Options);
            IntPtr resultingImageStruct = HBFunctions.hb_get_preview3_json(this.Handle, previewNumber, taskJson);
            hb_image_s image = InteropUtilities.ToStructureFromPtr<hb_image_s>(resultingImageStruct);

            // Copy the filled image buffer to a managed array.
            int stride_width = image.plane[0].stride;
            int stride_height = image.plane[0].height;
            int imageBufferSize = stride_width * stride_height;

            byte[] managedBuffer = new byte[imageBufferSize];
            Marshal.Copy(image.plane[0].data, managedBuffer, 0, imageBufferSize);

            RawPreviewData preview = new RawPreviewData(managedBuffer, stride_width, stride_height, image.width, image.height);

            // Close the image so we don't leak memory.
            IntPtr nativeJobPtrPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));
            Marshal.WriteIntPtr(nativeJobPtrPtr, resultingImageStruct);
            HBFunctions.hb_image_close(nativeJobPtrPtr);
            Marshal.FreeHGlobal(nativeJobPtrPtr);

            return preview;
        }

        /// <summary>
        /// Starts an encode with the given job.
        /// </summary>
        /// <param name="encodeObject">
        /// The encode Object.
        /// </param>
        public void StartEncode(JsonEncodeObject encodeObject)
        {
            string encode = JsonSerializer.Serialize(encodeObject, JsonSettings.Options);
            this.StartEncode(encode);
        }

        /// <summary>
        /// Starts an encode with the given job JSON.
        /// </summary>
        /// <param name="encodeJson">The JSON for the job to start.</param>
        public void StartEncode(string encodeJson)
        {
            HBFunctions.hb_add_json(this.Handle, InteropUtilities.ToUtf8PtrFromString(encodeJson));
            HBFunctions.hb_start(this.Handle);

            this.encodePollTimer = new Timer();
            this.encodePollTimer.Interval = EncodePollIntervalMs;
            this.lastEncodeProgress = null;

            this.encodePollTimer.Elapsed += (o, e) =>
            {
                try
                {
                    this.PollEncodeProgress();
                }
                catch (Exception exc)
                {
                    Debug.WriteLine(exc);
                }
            };
            this.encodePollTimer.Start();
        }

        /// <summary>
        /// Pauses the current encode.
        /// </summary>
        public void PauseEncode()
        {
            HBFunctions.hb_pause(this.Handle);
        }

        /// <summary>
        /// Resumes a paused encode.
        /// </summary>
        public void ResumeEncode()
        {
            HBFunctions.hb_resume(this.Handle);
        }

        /// <summary>
        /// Stops the current encode.
        /// </summary>
        public void StopEncode()
        {
            JsonState state = GetProgress();
            TaskState taskState = TaskState.FromRepositoryValue(state?.State);
            if (taskState == TaskState.WorkDone)
            {
                // We got the stop event at a bad time. Don't do anything.
                return;
            }

            HBFunctions.hb_stop(this.Handle);

            // Also remove all jobs from the queue (in case we stopped a 2-pass encode)
            var currentJobs = new List<IntPtr>();

            int jobs = HBFunctions.hb_count(this.Handle);
            for (int i = 0; i < jobs; i++)
            {
                currentJobs.Add(HBFunctions.hb_job(this.Handle, 0));
            }

            foreach (IntPtr job in currentJobs)
            {
                HBFunctions.hb_rem(this.Handle, job);
            }
        }

        /// <summary>
        /// Checks the status of this instance
        /// </summary>
        /// <returns>
        /// The <see cref="JsonState"/>.
        /// </returns>
        public JsonState GetProgress()
        {
            lock (this.progressJsonLockObj)
            {
                return this.lastProgressJson ?? JsonState.CreateDummy();
            }
        }

        /// <summary>
        /// Frees any resources associated with this object.
        /// </summary>
        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Gets a value indicating whether the object is disposed.
        /// </summary>
        public bool IsDisposed
        {
            get
            {
                return this.disposed;
            }
        }

        /// <summary>
        /// Frees any resources associated with this object.
        /// </summary>
        /// <param name="disposing">
        /// True if managed objects as well as unmanaged should be disposed.
        /// </param>
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                // Free other state (managed objects).
            }

            // Free unmanaged objects.
            IntPtr handlePtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));
            Marshal.WriteIntPtr(handlePtr, this.Handle);
            HBFunctions.hb_close(handlePtr);
            Marshal.FreeHGlobal(handlePtr);

            this.disposed = true;
        }

        /// <summary>
        /// Checks the status of the ongoing scan.
        /// </summary>
        private void PollScanProgress(NativeList exclusionList)
        {
            JsonState state = null;
            lock (this.progressJsonLockObj)
            {
                IntPtr json = HBFunctions.hb_get_state_json(this.Handle);
                string statusJson = Marshal.PtrToStringAnsi(json);

                if (!string.IsNullOrEmpty(statusJson))
                {
                    state = JsonSerializer.Deserialize<JsonState>(statusJson, JsonSettings.Options);
                }

                this.lastProgressJson = state;
            }

            TaskState taskState = state != null ? TaskState.FromRepositoryValue(state.State) : null;

            if (taskState != null && (taskState == TaskState.Scanning || taskState == TaskState.Searching))
            {
                if (this.ScanProgress != null && state.Scanning != null)
                {
                    this.ScanProgress(this, new ScanProgressEventArgs(state.Scanning.Progress, state.Scanning.Preview, state.Scanning.PreviewCount, state.Scanning.Title, state.Scanning.TitleCount));
                }
            }
            else if (taskState != null && (taskState == TaskState.ScanDone))
            {
                this.scanPollTimer.Stop();

                var jsonMsg = HBFunctions.hb_get_title_set_json(this.Handle);
                this.TitlesJson = InteropUtilities.ToStringFromUtf8Ptr(jsonMsg);

                if (!string.IsNullOrEmpty(this.TitlesJson))
                {
                    this.Titles = JsonSerializer.Deserialize<JsonScanObject>(this.TitlesJson, JsonSettings.Options);
                    if (this.Titles != null)
                    {
                        this.FeatureTitle = this.Titles.MainFeature;
                    }
                }

                if (this.ScanCompleted != null)
                {
                    this.ScanCompleted(this, EventArgs.Empty);
                }

                // Memory Management for the exclusion list.
                try
                {
                    if (exclusionList != null)
                    {
                        for (int i = 0; i < exclusionList.Count; i++)
                        {
                            IntPtr item = exclusionList[i];
                            exclusionList.Remove(item);
                            InteropUtilities.FreeMemory(new List<IntPtr> { item });
                        }

                        exclusionList.Dispose();
                    }
                }
                catch (Exception ex)
                {
                    Debug.WriteLine(ex);
                }
            }
            else if (taskState != null && (taskState == TaskState.Idle))
            {
                this.scanPollTimer.Stop();
                this.Titles = null;

                if (this.ScanCompleted != null)
                {
                    this.ScanCompleted(this, EventArgs.Empty);
                }
            }
        }

        /// <summary>
        /// Checks the status of the ongoing encode.
        /// </summary>
        private void PollEncodeProgress()
        {
            JsonState state = null;
            lock (this.progressJsonLockObj)
            {
                IntPtr json = HBFunctions.hb_get_state_json(this.Handle);
                string statusJson = Marshal.PtrToStringAnsi(json);

                if (!string.IsNullOrEmpty(statusJson))
                {
                    state = JsonSerializer.Deserialize<JsonState>(statusJson, JsonSettings.Options);
                }

                this.lastProgressJson = state;
            }

            TaskState taskState = state != null ? TaskState.FromRepositoryValue(state.State) : null;

            if (taskState != null && (taskState == TaskState.Working || taskState == TaskState.Muxing || taskState == TaskState.Searching))
            {
                if (this.EncodeProgress != null)
                {
                    TimeSpan eta = TimeSpan.FromSeconds(state?.Working?.ETASeconds ?? 0);
                    var progressEventArgs = new EncodeProgressEventArgs(0, 0, 0, TimeSpan.MinValue, 0, 0, 0, taskState.Code);
                    if (taskState == TaskState.Muxing || state.Working == null)
                    {
                        progressEventArgs = new EncodeProgressEventArgs(100, 0, 0, TimeSpan.MinValue, 0, 0, 0, taskState.Code);
                    }
                    else
                    {
                        progressEventArgs = new EncodeProgressEventArgs(state.Working.Progress, state.Working.Rate, state.Working.RateAvg, eta, state.Working.PassID, state.Working.Pass, state.Working.PassCount, taskState.Code);
                    }

                    if (!ProgressIsEqual(progressEventArgs, this.lastEncodeProgress))
                    {
                        this.EncodeProgress(this, progressEventArgs);
                        this.lastEncodeProgress = progressEventArgs;
                    }
                }
            }
            else if (taskState != null && taskState == TaskState.WorkDone)
            {
                this.encodePollTimer.Stop();

                if (this.EncodeCompleted != null)
                {
                    this.EncodeCompleted(
                        this,
                        new EncodeCompletedEventArgs(state.WorkDone.Error));
                }
            }
        }

        /// <summary>
        /// Returns true if the two progress events are equal. Used to throttle unnecessary progress events.
        /// </summary>
        /// <param name="a">The first progress event.</param>
        /// <param name="b">The second progress event.</param>
        /// <returns>True if the progress events are equal.</returns>
        private static bool ProgressIsEqual(EncodeProgressEventArgs a, EncodeProgressEventArgs b)
        {
            if (a == null || b == null)
            {
                return false;
            }

            return a.FractionComplete == b.FractionComplete &&
                   a.CurrentFrameRate == b.CurrentFrameRate &&
                   a.AverageFrameRate == b.AverageFrameRate &&
                   a.EstimatedTimeLeft == b.EstimatedTimeLeft &&
                   a.PassId == b.PassId &&
                   a.Pass == b.Pass &&
                   a.PassCount == b.PassCount &&
                   a.StateCode == b.StateCode;
        }
    }
}
