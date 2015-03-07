// --------------------------------------------------------------------------------------------------------------------
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
    using System.Windows.Media.Imaging;

    using HandBrake.ApplicationServices.Interop.EventArgs;
    using HandBrake.ApplicationServices.Interop.Factories;
    using HandBrake.ApplicationServices.Interop.HbLib;
    using HandBrake.ApplicationServices.Interop.Helpers;
    using HandBrake.ApplicationServices.Interop.Interfaces;
    using HandBrake.ApplicationServices.Interop.Json.Encode;
    using HandBrake.ApplicationServices.Interop.Json.Scan;
    using HandBrake.ApplicationServices.Interop.Json.State;
    using HandBrake.ApplicationServices.Interop.Model;
    using HandBrake.ApplicationServices.Interop.Model.Encoding;
    using HandBrake.ApplicationServices.Interop.Model.Preview;

    using Newtonsoft.Json;

    using Geometry = HandBrake.ApplicationServices.Interop.Json.Anamorphic.Geometry;
    using Size = HandBrake.ApplicationServices.Interop.Model.Size;

    /// <summary>
    /// A wrapper for a HandBrake instance.
    /// </summary>
    internal class HandBrakeInstance : IHandBrakeInstance, IDisposable
    {
        /// <summary>
        /// The number of MS between status polls when scanning.
        /// </summary>
        private const double ScanPollIntervalMs = 200;

        /// <summary>
        /// The number of MS between status polls when encoding.
        /// </summary>
        private const double EncodePollIntervalMs = 200;

        /// <summary>
        /// The native handle to the HandBrake instance.
        /// </summary>
        private IntPtr hbHandle;

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
                return this.Handle;
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
        /// <param name="job">
        /// The encode job to preview.
        /// </param>
        /// <param name="previewNumber">
        /// The index of the preview to get (0-based).
        /// </param>
        /// <returns>
        /// An image with the requested preview.
        /// </returns>
        [HandleProcessCorruptedStateExceptions]
        public BitmapImage GetPreview(PreviewSettings job, int previewNumber)
        {
            TitleList title = this.Titles.TitleList.FirstOrDefault(t => t.Index == job.Title);
            Validate.NotNull(title, "GetPreview: Title should not have been null. This is probably a bug.");

            // Creat the Expected Output Geometry details for libhb.
            hb_geometry_settings_s uiGeometry = new hb_geometry_settings_s
            {
                crop = new[] { job.Cropping.Top, job.Cropping.Bottom, job.Cropping.Left, job.Cropping.Right }, 
                itu_par = 0, 
                keep = (int)AnamorphicFactory.KeepSetting.HB_KEEP_WIDTH + (job.KeepDisplayAspect ? 0x04 : 0), // TODO Keep Width?
                maxWidth = job.MaxWidth ?? 0, 
                maxHeight = job.MaxHeight ?? 0, 
                mode = (int)(hb_anamorphic_mode_t)job.Anamorphic, 
                modulus = job.Modulus ?? 16, 
                geometry = new hb_geometry_s
                {
                    height = job.Height ?? 0, 
                    width = job.Width ?? 0, 
                    par = job.Anamorphic != Anamorphic.Custom
                        ? new hb_rational_t { den = title.Geometry.PAR.Den, num = title.Geometry.PAR.Num }
                        : new hb_rational_t { den = job.PixelAspectY, num = job.PixelAspectX }
                }
            };

            // Sanatise the input.
            Geometry resultGeometry = AnamorphicFactory.CreateGeometry(job, new SourceVideoInfo(new Size(title.Geometry.Width, title.Geometry.Height), new Size(title.Geometry.PAR.Num, title.Geometry.PAR.Den)), AnamorphicFactory.KeepSetting.HB_KEEP_WIDTH); // TODO this keep isn't right.
            int width = resultGeometry.Width * resultGeometry.PAR.Num / resultGeometry.PAR.Den;
            int height = resultGeometry.Height;
            uiGeometry.geometry.height = resultGeometry.Height; // Prased the height now.
            int outputWidth = width;
            int outputHeight = height;
           
            // Fetch the image data from LibHb
            IntPtr resultingImageStuct = HBFunctions.hb_get_preview2(this.hbHandle, job.Title, previewNumber, ref uiGeometry, 0);
            hb_image_s image = InteropUtilities.ToStructureFromPtr<hb_image_s>(resultingImageStuct);

            // Copy the filled image buffer to a managed array.
            int stride_width = image.plane[0].stride;
            int stride_height = image.plane[0].height_stride;
            int imageBufferSize = stride_width * stride_height;  // int imageBufferSize = outputWidth * outputHeight * 4;

            byte[] managedBuffer = new byte[imageBufferSize];
            Marshal.Copy(image.plane[0].data, managedBuffer, 0, imageBufferSize);

            var bitmap = new Bitmap(outputWidth, outputHeight);
            BitmapData bitmapData = bitmap.LockBits(new Rectangle(0, 0, outputWidth, outputHeight), ImageLockMode.WriteOnly, PixelFormat.Format32bppRgb);

            IntPtr ptr = bitmapData.Scan0; // Pointer to the first pixel.
            for (int i = 0; i < image.height; i++)
            {
                try
                {
                    Marshal.Copy(managedBuffer, i * stride_width, ptr, stride_width);
                    ptr = IntPtr.Add(ptr, outputWidth * 4);
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

            // Create a Bitmap Image for display.
            using (var memoryStream = new MemoryStream())
            {
                try
                {
                    bitmap.Save(memoryStream, ImageFormat.Bmp);
                }
                finally
                {
                    bitmap.Dispose();
                }

                var wpfBitmap = new BitmapImage();
                wpfBitmap.BeginInit();
                wpfBitmap.CacheOption = BitmapCacheOption.OnLoad;
                wpfBitmap.StreamSource = memoryStream;
                wpfBitmap.EndInit();
                wpfBitmap.Freeze();

                return wpfBitmap;
            }
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
            HBFunctions.hb_add_json(this.hbHandle, InteropUtilities.ToUtf8PtrFromString(encode));
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
            JsonState state = JsonConvert.DeserializeObject<JsonState>(statusJson);

            if (state != null && state.State == NativeConstants.HB_STATE_SCANNING)
            {
                if (this.ScanProgress != null)
                {
                    this.ScanProgress(this, new ScanProgressEventArgs
                    {
                        Progress = state.Scanning.Progress, 
                        CurrentPreview = state.Scanning.Preview, 
                        Previews = state.Scanning.PreviewCount, 
                        CurrentTitle = state.Scanning.Title, 
                        Titles = state.Scanning.TitleCount
                    });
                }
            }
            else if (state != null && state.State == NativeConstants.HB_STATE_SCANDONE)
            {
                var jsonMsg = HBFunctions.hb_get_title_set_json(this.hbHandle);
                string scanJson = InteropUtilities.ToStringFromUtf8Ptr(jsonMsg);
                this.titles = JsonConvert.DeserializeObject<JsonScanObject>(scanJson);
                this.featureTitle = this.titles.MainFeature;

                this.scanPollTimer.Stop();

                if (this.ScanCompleted != null)
                {
                    this.ScanCompleted(this, new System.EventArgs());
                }
            }
        }

        /// <summary>
        /// Checks the status of the ongoing encode.
        /// </summary>
        /// <summary>
        /// Checks the status of the ongoing encode.
        /// </summary>
        [HandleProcessCorruptedStateExceptions]
        private void PollEncodeProgress()
        {
            IntPtr json = HBFunctions.hb_get_state_json(this.hbHandle);
            string statusJson = Marshal.PtrToStringAnsi(json);
            JsonState state = JsonConvert.DeserializeObject<JsonState>(statusJson);

            if (state != null && state.State == NativeConstants.HB_STATE_WORKING)
            {
                if (this.EncodeProgress != null)
                {
                    var progressEventArgs = new EncodeProgressEventArgs
                    {
                        FractionComplete = state.Working.Progress, 
                        CurrentFrameRate = state.Working.Rate, 
                        AverageFrameRate = state.Working.RateAvg, 
                        EstimatedTimeLeft = new TimeSpan(state.Working.Hours, state.Working.Minutes, state.Working.Seconds), 
                        Pass = 1, // TODO
                    };

                    this.EncodeProgress(this, progressEventArgs);
                }
            }
            else if (state != null && state.State == NativeConstants.HB_STATE_WORKDONE)
            {
                this.encodePollTimer.Stop();

                if (this.EncodeCompleted != null)
                {
                    this.EncodeCompleted(this, new EncodeCompletedEventArgs
                    {
                        Error = state.WorkDone.Error != (int)hb_error_code.HB_ERROR_NONE
                    });
                }
            }
        }
    }
}
