﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeInstance.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A wrapper for a HandBrake instance.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Interop
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Imaging;
    using System.IO;
    using System.Linq;
    using System.Runtime.ExceptionServices;
    using System.Runtime.InteropServices;
    using System.Timers;

    using HandBrake.ApplicationServices.Interop.EventArgs;
    using HandBrake.ApplicationServices.Interop.Factories;
    using HandBrake.ApplicationServices.Interop.HbLib;
    using HandBrake.ApplicationServices.Interop.Helpers;
    using HandBrake.ApplicationServices.Interop.Interfaces;
    using HandBrake.ApplicationServices.Interop.Json.Encode;
    using HandBrake.ApplicationServices.Interop.Json.Scan;
    using HandBrake.ApplicationServices.Interop.Json.Shared;
    using HandBrake.ApplicationServices.Interop.Json.State;
    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Interop.Model.Preview;
    using HandBrake.ApplicationServices.Services.Logging;
    using HandBrake.ApplicationServices.Services.Logging.Interfaces;
    using HandBrake.ApplicationServices.Services.Logging.Model;

    using Newtonsoft.Json;

    using Size = HandBrake.ApplicationServices.Interop.Model.Size;

    /// <summary>
    /// A wrapper for a HandBrake instance.
    /// </summary>
    public class HandBrakeInstance : IHandBrakeInstance, IDisposable
    {
        /// <summary>
        /// The number of MS between status polls when scanning.
        /// </summary>
        private const double ScanPollIntervalMs = 250;

        /// <summary>
        /// The number of MS between status polls when encoding.
        /// </summary>
        private const double EncodePollIntervalMs = 250;

        private readonly ILog log = LogService.GetLogger();

        /// <summary>
        /// The native handle to the HandBrake instance.
        /// </summary>
        private IntPtr hbHandle;

        /// <summary>
        /// The number of previews created during scan.
        /// </summary>
        private int previewCount;

        /// <summary>
        /// The timer to poll for scan status.
        /// </summary>
        private Timer scanPollTimer;

        /// <summary>
        /// The timer to poll for encode status.
        /// </summary>
        private Timer encodePollTimer;

        /// <summary>
        /// The list of titles on this instance.
        /// </summary>
        private JsonScanObject titles;

        /// <summary>
        /// The raw JSON for the titles list.
        /// </summary>
        private string titlesJson;

        /// <summary>
        /// The index of the default title.
        /// </summary>
        private int featureTitle;

        /// <summary>
        /// A value indicating whether this object has been disposed or not.
        /// </summary>
        private bool disposed;

        /// <summary>
        /// Finalizes an instance of the HandBrakeInstance class.
        /// </summary>
        ~HandBrakeInstance()
        {
            this.Dispose(false);
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
        /// Gets the handle.
        /// </summary>
        internal IntPtr Handle
        {
            get
            {
                return this.hbHandle;
            }
        }

        /// <summary>
        /// Gets the number of previews created during scan.
        /// </summary>
        public int PreviewCount
        {
            get
            {
                return this.previewCount;
            }
        }

        /// <summary>
        /// Gets the list of titles on this instance.
        /// </summary>
        public JsonScanObject Titles
        {
            get
            {
                return this.titles;
            }
        }

        /// <summary>
        /// Gets the raw JSON for the list of titles on this instance.
        /// </summary>
        public string TitlesJson
        {
            get
            {
                return this.titlesJson;
            }
        }

        /// <summary>
        /// Gets the index of the default title.
        /// </summary>
        public int FeatureTitle
        {
            get
            {
                return this.featureTitle;
            }
        }

        /// <summary>
        /// Gets the HandBrake version string.
        /// </summary>
        public string Version
        {
            get
            {
                var versionPtr = HBFunctions.hb_get_version(this.hbHandle);
                return Marshal.PtrToStringAnsi(versionPtr);
            }
        }

        /// <summary>
        /// Gets the HandBrake build number.
        /// </summary>
        public int Build
        {
            get
            {
                return HBFunctions.hb_get_build(this.hbHandle);
            }
        }

        /// <summary>
        /// Initializes this instance.
        /// </summary>
        /// <param name="verbosity">
        /// The code for the logging verbosity to use.
        /// </param>
        public void Initialize(int verbosity)
        {
            HandBrakeUtils.EnsureGlobalInit();

            HandBrakeUtils.RegisterLogger();
            this.hbHandle = HBFunctions.hb_init(verbosity, update_check: 0);
        }

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
        /// The minimum duration of a title to show up on the scan.
        /// </param>
        /// <param name="titleIndex">
        /// The title index to scan (1-based, 0 for all titles).
        /// </param>
        public void StartScan(string path, int previewCount, TimeSpan minDuration, int titleIndex)
        {
            this.previewCount = previewCount;

            IntPtr pathPtr = InteropUtilities.ToUtf8PtrFromString(path);
            HBFunctions.hb_scan(this.hbHandle, pathPtr, titleIndex, previewCount, 1, (ulong)(minDuration.TotalSeconds * 90000));
            Marshal.FreeHGlobal(pathPtr);

            this.scanPollTimer = new Timer();
            this.scanPollTimer.Interval = ScanPollIntervalMs;

            // Lambda notation used to make sure we can view any JIT exceptions the method throws
            this.scanPollTimer.Elapsed += (o, e) =>
            {
                try
                {
                    this.PollScanProgress();
                }
                catch (Exception exc)
                {
                    Debug.WriteLine(exc);
                    this.log.LogMessage(exc.ToString(), LogMessageType.API, LogLevel.Error);
                }
            };
            this.scanPollTimer.Start();
        }

        /// <summary>
        /// Stops an ongoing scan.
        /// </summary>
        [HandleProcessCorruptedStateExceptions]
        public void StopScan()
        {
            HBFunctions.hb_scan_stop(this.hbHandle);
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
        /// <param name="deinterlace">
        /// True to enable basic deinterlace of preview images.
        /// </param>
        /// <returns>
        /// An image with the requested preview.
        /// </returns>
        [HandleProcessCorruptedStateExceptions]
        public Bitmap GetPreview(PreviewSettings settings, int previewNumber, bool deinterlace)
        {
            SourceTitle title = this.Titles.TitleList.FirstOrDefault(t => t.Index == settings.TitleNumber);

            // Create the Expected Output Geometry details for libhb.
            hb_geometry_settings_s uiGeometry = new hb_geometry_settings_s
            {
                crop = new[] { settings.Cropping.Top, settings.Cropping.Bottom, settings.Cropping.Left, settings.Cropping.Right },
                itu_par = 0,
                keep = (int)AnamorphicFactory.KeepSetting.HB_KEEP_WIDTH + (settings.KeepDisplayAspect ? 0x04 : 0), // TODO Keep Width?
                maxWidth = settings.MaxWidth,
                maxHeight = settings.MaxHeight,
                mode = (int)(hb_anamorphic_mode_t)settings.Anamorphic,
                modulus = settings.Modulus ?? 16,
                geometry = new hb_geometry_s
                {
                    height = settings.Height,
                    width = settings.Width,
                    par = settings.Anamorphic != Anamorphic.Custom
                        ? new hb_rational_t { den = title.Geometry.PAR.Den, num = title.Geometry.PAR.Num }
                        : new hb_rational_t { den = settings.PixelAspectY, num = settings.PixelAspectX }
                }
            };

            // Fetch the image data from LibHb
            IntPtr resultingImageStuct = HBFunctions.hb_get_preview2(this.hbHandle, settings.TitleNumber, previewNumber, ref uiGeometry, deinterlace ? 1 : 0);
            hb_image_s image = InteropUtilities.ToStructureFromPtr<hb_image_s>(resultingImageStuct);

            // Copy the filled image buffer to a managed array.
            int stride_width = image.plane[0].stride;
            int stride_height = image.plane[0].height_stride;
            int imageBufferSize = stride_width * stride_height;  // int imageBufferSize = outputWidth * outputHeight * 4;

            byte[] managedBuffer = new byte[imageBufferSize];
            Marshal.Copy(image.plane[0].data, managedBuffer, 0, imageBufferSize);

            var bitmap = new Bitmap(image.width, image.height);

            BitmapData bitmapData = bitmap.LockBits(new Rectangle(0, 0, image.width, image.height), ImageLockMode.WriteOnly, PixelFormat.Format32bppRgb);

            IntPtr ptr = bitmapData.Scan0; // Pointer to the first pixel.
            for (int i = 0; i < image.height; i++)
            {
                try
                {
                    Marshal.Copy(managedBuffer, i * stride_width, ptr, stride_width);
                    ptr = IntPtr.Add(ptr, image.width * 4);
                }
                catch (Exception exc)
                {
                    Debug.WriteLine(exc); // In theory, this will allow a partial image display if this happens. TODO add better logging of this.
                }
            }

            bitmap.UnlockBits(bitmapData);

            // Close the image so we don't leak memory.
            IntPtr nativeJobPtrPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));
            Marshal.WriteIntPtr(nativeJobPtrPtr, resultingImageStuct);
            HBFunctions.hb_image_close(nativeJobPtrPtr);
            Marshal.FreeHGlobal(nativeJobPtrPtr);

            return bitmap;
        }

        /// <summary>
        /// Determines if DRC can be applied to the given track with the given encoder.
        /// </summary>
        /// <param name="trackNumber">The track Number.</param>
        /// <param name="encoder">The encoder to use for DRC.</param>
        /// <param name="title">The title.</param>
        /// <returns>True if DRC can be applied to the track with the given encoder.</returns>
        public bool CanApplyDrc(int trackNumber, HBAudioEncoder encoder, int title)
        {
            return HBFunctions.hb_audio_can_apply_drc2(this.hbHandle, title, trackNumber, encoder.Id) > 0;
        }

        /// <summary>
        /// Starts an encode with the given job.
        /// </summary>
        /// <param name="encodeObject">
        /// The encode Object.
        /// </param>
        [HandleProcessCorruptedStateExceptions]
        public void StartEncode(JsonEncodeObject encodeObject)
        {
            JsonSerializerSettings settings = new JsonSerializerSettings
            {
                NullValueHandling = NullValueHandling.Ignore,
            };

            string encode = JsonConvert.SerializeObject(encodeObject, Formatting.Indented, settings);
            this.StartEncode(encode);
        }

        /// <summary>
        /// Starts an encode with the given job JSON.
        /// </summary>
        /// <param name="encodeJson">The JSON for the job to start.</param>
        [HandleProcessCorruptedStateExceptions]
        public void StartEncode(string encodeJson)
        {
            HBFunctions.hb_add_json(this.hbHandle, InteropUtilities.ToUtf8PtrFromString(encodeJson));
            HBFunctions.hb_start(this.hbHandle);

            this.encodePollTimer = new Timer();
            this.encodePollTimer.Interval = EncodePollIntervalMs;

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
        [HandleProcessCorruptedStateExceptions]
        public void PauseEncode()
        {
            HBFunctions.hb_pause(this.hbHandle);
        }

        /// <summary>
        /// Resumes a paused encode.
        /// </summary>
        [HandleProcessCorruptedStateExceptions]
        public void ResumeEncode()
        {
            HBFunctions.hb_resume(this.hbHandle);
        }

        /// <summary>
        /// Stops the current encode.
        /// </summary>
        [HandleProcessCorruptedStateExceptions]
        public void StopEncode()
        {
            HBFunctions.hb_stop(this.hbHandle);

            // Also remove all jobs from the queue (in case we stopped a 2-pass encode)
            var currentJobs = new List<IntPtr>();

            int jobs = HBFunctions.hb_count(this.hbHandle);
            for (int i = 0; i < jobs; i++)
            {
                currentJobs.Add(HBFunctions.hb_job(this.hbHandle, 0));
            }

            foreach (IntPtr job in currentJobs)
            {
                HBFunctions.hb_rem(this.hbHandle, job);
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
            Marshal.WriteIntPtr(handlePtr, this.hbHandle);
            HBFunctions.hb_close(handlePtr);
            Marshal.FreeHGlobal(handlePtr);

            this.disposed = true;
        }

        /// <summary>
        /// Checks the status of the ongoing scan.
        /// </summary>
        [HandleProcessCorruptedStateExceptions]
        private void PollScanProgress()
        {
            IntPtr json = HBFunctions.hb_get_state_json(this.hbHandle);
            string statusJson = Marshal.PtrToStringAnsi(json);
            this.log.LogMessage(statusJson, LogMessageType.Progress, LogLevel.Trace);
            JsonState state = null;
            if (!string.IsNullOrEmpty(statusJson))
            {
                state = JsonConvert.DeserializeObject<JsonState>(statusJson);
            }

            TaskState taskState = state != null ? TaskState.FromRepositoryValue(state.State) : null;

            if (taskState != null && (taskState == TaskState.Scanning || taskState == TaskState.Searching))
            {
                if (this.ScanProgress != null && state.Scanning != null)
                {
                    this.ScanProgress(this, new ScanProgressEventArgs(state.Scanning.Progress, state.Scanning.Preview, state.Scanning.PreviewCount, state.Scanning.Title, state.Scanning.TitleCount));
                }
            }
            else if (taskState != null && taskState == TaskState.ScanDone)
            {
                this.scanPollTimer.Stop();

                var jsonMsg = HBFunctions.hb_get_title_set_json(this.hbHandle);
                this.titlesJson = InteropUtilities.ToStringFromUtf8Ptr(jsonMsg);
                this.log.LogMessage(this.titlesJson, LogMessageType.Progress, LogLevel.Trace);

                if (string.IsNullOrEmpty(this.titlesJson))
                {
                    this.log.LogMessage("Scan Error: No Scan Data Returned.", LogMessageType.API, LogLevel.Error);
                }
                else
                {
                    this.titles = JsonConvert.DeserializeObject<JsonScanObject>(this.titlesJson);
                    if (this.titles != null)
                    {
                        this.featureTitle = this.titles.MainFeature;
                    }
                }

                if (this.ScanCompleted != null)
                {
                    this.ScanCompleted(this, new System.EventArgs());
                }
            }
        }

        /// <summary>
        /// Checks the status of the ongoing encode.
        /// </summary>
        [HandleProcessCorruptedStateExceptions]
        private void PollEncodeProgress()
        {
            IntPtr json = HBFunctions.hb_get_state_json(this.hbHandle);
            string statusJson = Marshal.PtrToStringAnsi(json);

            this.log.LogMessage(statusJson, LogMessageType.Progress, LogLevel.Trace);

            JsonState state = JsonConvert.DeserializeObject<JsonState>(statusJson);

            TaskState taskState = state != null ? TaskState.FromRepositoryValue(state.State) : null;

            if (taskState != null && (taskState == TaskState.Working || taskState == TaskState.Muxing || taskState == TaskState.Searching))
            {
                if (this.EncodeProgress != null)
                {
                    var progressEventArgs = new EncodeProgressEventArgs(state.Working.Progress, state.Working.Rate, state.Working.RateAvg, new TimeSpan(state.Working.Hours, state.Working.Minutes, state.Working.Seconds),
                        state.Working.PassID, state.Working.Pass, state.Working.PassCount, taskState.Code);

                    this.EncodeProgress(this, progressEventArgs);
                }
            }
            else if (taskState != null && taskState == TaskState.WorkDone)
            {
                this.encodePollTimer.Stop();

                if (this.EncodeCompleted != null)
                {
                    this.EncodeCompleted(
                        this,
                        new EncodeCompletedEventArgs(state.WorkDone.Error != (int)hb_error_code.HB_ERROR_NONE));
                }
            }
        }
    }
}
