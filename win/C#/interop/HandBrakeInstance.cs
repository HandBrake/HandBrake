namespace HandBrake.Interop
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Timers;
    using System.Threading;
    using System.Windows.Media.Imaging;
    using HandBrake.SourceData;
    using HandBrake.Interop;

    /// <summary>
    /// A wrapper for a HandBrake instance.
    /// </summary>
    public class HandBrakeInstance : IDisposable
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
        /// X264 options to add for a turbo first pass.
        /// </summary>
        private const string TurboX264Opts = "ref=1:subme=2:me=dia:analyse=none:trellis=0:no-fast-pskip=0:8x8dct=0:weightb=0";

        /// <summary>
        /// The native handle to the HandBrake instance.
        /// </summary>
        private IntPtr hbHandle;

        /// <summary>
        /// The timer to poll for scan status.
        /// </summary>
        private System.Timers.Timer scanPollTimer;

        /// <summary>
        /// The timer to poll for encode status.
        /// </summary>
        private System.Timers.Timer encodePollTimer;

        /// <summary>
        /// The list of original titles in native structure form.
        /// </summary>
        private List<hb_title_s> originalTitles;

        /// <summary>
        /// The list of titles on this instance.
        /// </summary>
        private List<Title> titles;

        /// <summary>
        /// A list of native memory locations allocated by this instance.
        /// </summary>
        private List<IntPtr> encodeAllocatedMemory;

        /// <summary>
        /// The callback for log messages from HandBrake.
        /// </summary>
        private static LoggingCallback loggingCallback;

        /// <summary>
        /// The callback for error messages from HandBrake.
        /// </summary>
        private static LoggingCallback errorCallback;

        /// <summary>
        /// Fires for progress updates when scanning.
        /// </summary>
        public event EventHandler<ScanProgressEventArgs> ScanProgress;

        /// <summary>
        /// Fires when a scan has completed.
        /// </summary>
        public event EventHandler<EventArgs> ScanCompleted;

        /// <summary>
        /// Fires for progress updates when encoding.
        /// </summary>
        public event EventHandler<EncodeProgressEventArgs> EncodeProgress;

        /// <summary>
        /// Fires when an encode has completed.
        /// </summary>
        public event EventHandler<EventArgs> EncodeCompleted;

        /// <summary>
        /// Fires when HandBrake has logged a message.
        /// </summary>
        public static event EventHandler<MessageLoggedEventArgs> MessageLogged;

        /// <summary>
        /// Fires when HandBrake has logged an error.
        /// </summary>
        public static event EventHandler<MessageLoggedEventArgs> ErrorLogged;

        /// <summary>
        /// Destructor.
        /// </summary>
        ~HandBrakeInstance()
        {
            this.Dispose(false);
        }

        /// <summary>
        /// The list of titles on this instance.
        /// </summary>
        public List<Title> Titles
        {
            get
            {
                return this.titles;
            }
        }

        /// <summary>
        /// Initializes this instance.
        /// </summary>
        /// <param name="verbosity"></param>
        public void Initialize(int verbosity)
        {
            // Register the logger if we have not already
            if (loggingCallback == null)
            {
                // Keep the callback as a member to prevent it from being garbage collected.
                loggingCallback = new LoggingCallback(HandBrakeInstance.LoggingHandler);
                errorCallback = new LoggingCallback(HandBrakeInstance.ErrorHandler);
                HbLib.hb_register_logger(loggingCallback);
                HbLib.hb_register_error_handler(errorCallback);
            }

            this.hbHandle = HbLib.hb_init(verbosity, update_check: 0);
        }

        /// <summary>
        /// Handles log messages from HandBrake.
        /// </summary>
        /// <param name="message">The log message (including newline).</param>
        public static void LoggingHandler(string message)
        {
            if (!string.IsNullOrEmpty(message))
            {
                string[] messageParts = message.Split(new string[] { "\n" }, StringSplitOptions.RemoveEmptyEntries);

                if (messageParts.Length > 0)
                {
                    if (MessageLogged != null)
                    {
                        MessageLogged(null, new MessageLoggedEventArgs { Message = messageParts[0] });
                    }

                    System.Diagnostics.Debug.WriteLine(messageParts[0]);
                }
            }
        }

        /// <summary>
        /// Handles errors from HandBrake.
        /// </summary>
        /// <param name="message">The error message.</param>
        public static void ErrorHandler(string message)
        {
            if (!string.IsNullOrEmpty(message))
            {
                if (ErrorLogged != null)
                {
                    ErrorLogged(null, new MessageLoggedEventArgs { Message = message });
                }

                System.Diagnostics.Debug.WriteLine("ERROR: " + message);
            }
        }

        /// <summary>
        /// Starts scanning the given path.
        /// </summary>
        /// <param name="path">The path to the video to scan.</param>
        /// <param name="previewCount">The number of preview images to make.</param>
        public void StartScan(string path, int previewCount)
        {
            this.StartScan(path, previewCount, 0);
        }

        /// <summary>
        /// Starts a scan of the given path.
        /// </summary>
        /// <param name="path">The path of the video to scan.</param>
        /// <param name="previewCount">The number of previews to make on each title.</param>
        /// <param name="titleIndex">The title index to scan (1-based, 0 for all titles).</param>
        public void StartScan(string path, int previewCount, int titleIndex)
        {
            HbLib.hb_scan(hbHandle, path, titleIndex, previewCount, 1);
            this.scanPollTimer = new System.Timers.Timer();
            this.scanPollTimer.Interval = ScanPollIntervalMs;

            // Lambda notation used to make sure we can view any JIT exceptions the method throws
            this.scanPollTimer.Elapsed += (o, e) =>
            {
                this.PollScanProgress();
            };
            this.scanPollTimer.Start();
        }

        /// <summary>
        /// Gets an image for the given job and preview
        /// </summary>
        /// <remarks>
        /// Only incorporates sizing and aspect ratio into preview image.
        /// </remarks>
        /// <param name="job">The encode job to preview.</param>
        /// <param name="previewNumber">The index of the preview to get (0-based).</param>
        /// <returns>An image with the requested preview.</returns>
        public BitmapImage GetPreview(EncodeJob job, int previewNumber)
        {
            hb_title_s title = this.GetOriginalTitle(job.Title);

            hb_job_s nativeJob = InteropUtilities.ReadStructure<hb_job_s>(title.job);
            List<IntPtr> allocatedMemory = this.ApplyJob(ref nativeJob, job, false, 0, 0);
            
            // Create a new job pointer from our modified job object
            IntPtr newJob = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(hb_job_s)));
            Marshal.StructureToPtr(nativeJob, newJob, false);
            allocatedMemory.Add(newJob);

            int outputWidth = nativeJob.width;
            int outputHeight = nativeJob.height;
            int imageBufferSize = outputWidth * outputHeight * 4;
            IntPtr nativeBuffer = Marshal.AllocHGlobal(imageBufferSize);
            allocatedMemory.Add(nativeBuffer);
            HbLib.hb_set_job(this.hbHandle, job.Title, ref nativeJob);
            HbLib.hb_get_preview_by_index(this.hbHandle, job.Title, previewNumber, nativeBuffer);

            // Copy the filled image buffer to a managed array.
            byte[] managedBuffer = new byte[imageBufferSize];
            Marshal.Copy(nativeBuffer, managedBuffer, 0, imageBufferSize);

            InteropUtilities.FreeMemory(allocatedMemory);

            System.Drawing.Bitmap bitmap = new System.Drawing.Bitmap(outputWidth, outputHeight);
            System.Drawing.Imaging.BitmapData bitmapData = bitmap.LockBits(new System.Drawing.Rectangle(0, 0, outputWidth, outputHeight), System.Drawing.Imaging.ImageLockMode.WriteOnly, System.Drawing.Imaging.PixelFormat.Format32bppRgb);

            IntPtr ptr = bitmapData.Scan0;

            for (int i = 0; i < nativeJob.height; i++)
            {
                Marshal.Copy(managedBuffer, i * nativeJob.width * 4, ptr, nativeJob.width * 4);
                ptr = IntPtr.Add(ptr, bitmapData.Stride);
            }

            bitmap.UnlockBits(bitmapData);
            //bitmap.Save(@"d:\docs\test_" + previewNumber + ".png", System.Drawing.Imaging.ImageFormat.Png);

            using (MemoryStream memoryStream = new MemoryStream())
            {
                bitmap.Save(memoryStream, System.Drawing.Imaging.ImageFormat.Bmp);
                bitmap.Dispose();

                BitmapImage wpfBitmap = new BitmapImage();
                wpfBitmap.BeginInit();
                wpfBitmap.CacheOption = BitmapCacheOption.OnLoad;
                wpfBitmap.StreamSource = memoryStream;
                wpfBitmap.EndInit();

                return wpfBitmap;
            }
        }

        /// <summary>
        /// Starts an encode with the given job.
        /// </summary>
        /// <param name="job">The job to start.</param>
        public void StartEncode(EncodeJob job)
        {
            this.StartEncode(job, false, 0, 0);
        }

        /// <summary>
        /// Starts an encode with the given job.
        /// </summary>
        /// <param name="job">The job to start.</param>
        /// <param name="preview">True if this is a preview encode.</param>
        /// <param name="previewNumber">The preview number to start the encode at (0-based).</param>
        /// <param name="previewSeconds">The number of seconds in the preview.</param>
        public void StartEncode(EncodeJob job, bool preview, int previewNumber, int previewSeconds)
        {
            hb_job_s nativeJob = InteropUtilities.ReadStructure<hb_job_s>(this.GetOriginalTitle(job.Title).job);
            this.encodeAllocatedMemory = this.ApplyJob(ref nativeJob, job, preview, previewNumber, previewSeconds);

            if (!preview && job.EncodingProfile.IncludeChapterMarkers)
            {
                Title title = this.GetTitle(job.Title);
                int numChapters = title.Chapters.Count;

                if (job.UseDefaultChapterNames)
                {
                    for (int i = 0; i < numChapters; i++)
                    {
                        HbLib.hb_set_chapter_name(this.hbHandle, job.Title, i + 1, "Chapter " + (i + 1));
                    }
                }
                else
                {
                    for (int i = 0; i < numChapters; i++)
                    {
                        HbLib.hb_set_chapter_name(this.hbHandle, job.Title, i + 1, job.CustomChapterNames[i]);
                    }
                }
            }

            HbLib.hb_add(this.hbHandle, ref nativeJob);

            if (job.EncodingProfile.TwoPass)
            {
                nativeJob.pass = 2;

                string x264Opts = job.EncodingProfile.X264Options ?? string.Empty;
                nativeJob.x264opts = Marshal.StringToHGlobalAnsi(x264Opts);
                this.encodeAllocatedMemory.Add(nativeJob.x264opts);

                HbLib.hb_add(this.hbHandle, ref nativeJob);
            }

            HbLib.hb_start(this.hbHandle);

            this.encodePollTimer = new System.Timers.Timer();
            this.encodePollTimer.Interval = EncodePollIntervalMs;

            this.encodePollTimer.Elapsed += (o, e) =>
            {
                this.PollEncodeProgress();
            };
            this.encodePollTimer.Start();
        }

        /// <summary>
        /// Pauses the current encode.
        /// </summary>
        public void PauseEncode()
        {
            HbLib.hb_pause(this.hbHandle);
        }

        /// <summary>
        /// Resumes a paused encode.
        /// </summary>
        public void ResumeEncode()
        {
            HbLib.hb_resume(this.hbHandle);
        }

        /// <summary>
        /// Stops the current encode.
        /// </summary>
        public void StopEncode()
        {
            HbLib.hb_stop(this.hbHandle);

            // Also remove all jobs from the queue (in case we stopped a 2-pass encode)
            var currentJobs = new List<IntPtr>();

            int jobs = HbLib.hb_count(this.hbHandle);
            for (int i = 0; i < jobs; i++)
            {
                currentJobs.Add(HbLib.hb_job(this.hbHandle, 0));
            }

            foreach (IntPtr job in currentJobs)
            {
                HbLib.hb_rem(this.hbHandle, job);
            }
        }

        /// <summary>
        /// Gets the final size when using Anamorphic for a given encode job.
        /// </summary>
        /// <param name="job">The encode job to use.</param>
        /// <param name="width">The storage width.</param>
        /// <param name="height">The storage height.</param>
        /// <param name="parWidth">The pixel aspect X number.</param>
        /// <param name="parHeight">The pixel aspect Y number.</param>
        public void GetAnamorphicSize(EncodeJob job, out int width, out int height, out int parWidth, out int parHeight)
        {
            hb_job_s nativeJob = InteropUtilities.ReadStructure<hb_job_s>(this.GetOriginalTitle(job.Title).job);
            List<IntPtr> allocatedMemory = this.ApplyJob(ref nativeJob, job, false, 0, 0);

            int refWidth = 0;
            int refHeight = 0;
            int refParWidth = 0;
            int refParHeight = 0;
            HbLib.hb_set_job(this.hbHandle, job.Title, ref nativeJob);
            HbLib.hb_set_anamorphic_size_by_index(this.hbHandle, job.Title, ref refWidth, ref refHeight, ref refParWidth, ref refParHeight);
            //HbLib.hb_set_anamorphic_size(ref nativeJob, ref refWidth, ref refHeight, ref refParWidth, ref refParHeight);
            InteropUtilities.FreeMemory(allocatedMemory);

            width = refWidth;
            height = refHeight;
            parWidth = refParWidth;
            parHeight = refParHeight;
        }

        /// <summary>
        /// Frees any resources associated with this object.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// Call before app shutdown. Performs global cleanup.
        /// </summary>
        public static void DisposeGlobal()
        {
            HbLib.hb_global_close();
        }

        /// <summary>
        /// Frees any resources associated with this object.
        /// </summary>
        /// <param name="disposing">True if managed objects as well as unmanaged should be disposed.</param>
        protected virtual void Dispose(bool disposing)
        {
            if (disposing)
            {
                // Free other state (managed objects).
            }

            // Free unmanaged objects.
            IntPtr handlePtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));
            Marshal.WriteIntPtr(handlePtr, this.hbHandle);
            HbLib.hb_close(handlePtr);
            Marshal.FreeHGlobal(handlePtr);
        }

        /// <summary>
        /// Checks the status of the ongoing scan.
        /// </summary>
        private void PollScanProgress()
        {
            hb_state_s state = new hb_state_s();
            HbLib.hb_get_state(this.hbHandle, ref state);

            if (state.state == NativeConstants.HB_STATE_SCANNING)
            {
                if (this.ScanProgress != null)
                {
                    int currentTitle = state.param.scanning.title_cur;
                    int totalTitles = state.param.scanning.title_count;
                    this.ScanProgress(this, new ScanProgressEventArgs { CurrentTitle = currentTitle, Titles = totalTitles });
                }
            }
            else if (state.state == NativeConstants.HB_STATE_SCANDONE)
            {
                this.titles = new List<Title>();

                IntPtr listPtr = HbLib.hb_get_titles(this.hbHandle);
                this.originalTitles = InteropUtilities.ConvertList<hb_title_s>(listPtr);

                foreach (hb_title_s title in this.originalTitles)
                {
                    var newTitle = this.ConvertTitle(title);
                    this.titles.Add(newTitle);
                }

                this.scanPollTimer.Stop();

                if (this.ScanCompleted != null)
                {
                    this.ScanCompleted(this, new EventArgs());
                }
            }
        }

        /// <summary>
        /// Checks the status of the ongoing encode.
        /// </summary>
        private void PollEncodeProgress()
        {
            hb_state_s state = new hb_state_s();
            HbLib.hb_get_state(this.hbHandle, ref state);

            if (state.state == NativeConstants.HB_STATE_WORKING)
            {
                if (this.EncodeProgress != null)
                {
                    var progressEventArgs = new EncodeProgressEventArgs
                    {
                        FractionComplete = state.param.working.progress,
                        CurrentFrameRate = state.param.working.rate_cur,
                        AverageFrameRate = state.param.working.rate_avg,
                        EstimatedTimeLeft = new TimeSpan(state.param.working.hours, state.param.working.minutes, state.param.working.seconds),
                        Pass = state.param.working.job_cur
                    };

                    this.EncodeProgress(this, progressEventArgs);
                }
            }
            else if (state.state == NativeConstants.HB_STATE_MUXING)
            {
                //System.Diagnostics.Debug.WriteLine("Muxing...");
            }
            else if (state.state == NativeConstants.HB_STATE_WORKDONE)
            {
                InteropUtilities.FreeMemory(this.encodeAllocatedMemory);
                this.encodePollTimer.Stop();

                if (this.EncodeCompleted != null)
                {
                    this.EncodeCompleted(this, new EventArgs());
                }
            }
        }

        /// <summary>
        /// Applies the encoding job to the native memory structure and returns a list of memory
        /// locations allocated during this.
        /// </summary>
        /// <param name="nativeJob">The native structure to apply to job info to.</param>
        /// <param name="job">The job info to apply.</param>
        /// <param name="preview">True if this is a preview encode.</param>
        /// <param name="previewNumber">The preview number (0-based) to encode.</param>
        /// <param name="previewSeconds">The number of seconds in the preview.</param>
        /// <returns>The list of memory locations allocated for the job.</returns>
        private List<IntPtr> ApplyJob(ref hb_job_s nativeJob, EncodeJob job, bool preview, int previewNumber, int previewSeconds)
        {
            var allocatedMemory = new List<IntPtr>();
            Title title = this.GetTitle(job.Title);
            hb_title_s originalTitle = this.GetOriginalTitle(job.Title);

            EncodingProfile profile = job.EncodingProfile;

            if (preview)
            {
                nativeJob.start_at_preview = previewNumber + 1;
                nativeJob.seek_points = 10;

                // There are 90,000 PTS per second.
                nativeJob.pts_to_stop = previewSeconds * 90000;
            }
            else if (job.ChapterStart > 0 && job.ChapterEnd > 0)
            {
                nativeJob.chapter_start = job.ChapterStart;
                nativeJob.chapter_end = job.ChapterEnd;
            }
            else
            {
                nativeJob.chapter_start = 1;
                nativeJob.chapter_end = title.Chapters.Count;
            }

            nativeJob.chapter_markers = profile.IncludeChapterMarkers ? 1 : 0;

            Cropping crop;

            if (profile.CustomCropping)
            {
                crop = profile.Cropping;
            }
            else
            {
                crop = title.AutoCropDimensions;
            }

            nativeJob.crop[0] = crop.Top;
            nativeJob.crop[1] = crop.Bottom;
            nativeJob.crop[2] = crop.Left;
            nativeJob.crop[3] = crop.Right;

            List<IntPtr> filterList = new List<IntPtr>();
            if (profile.Deinterlace != Deinterlace.Off)
            {
                nativeJob.deinterlace = 1;
                string settings = null;

                switch (profile.Deinterlace)
                {
                    case Deinterlace.Fast:
                        settings = "-1";
                        break;
                    case Deinterlace.Slow:
                        settings = "2";
                        break;
                    case Deinterlace.Slower:
                        settings = "0";
                        break;
                    case Deinterlace.Custom:
                        settings = profile.CustomDeinterlace;
                        break;
                    default:
                        break;
                }

                this.AddFilter(filterList, NativeConstants.HB_FILTER_DEINTERLACE, settings, allocatedMemory);
                //filterList.Add(HbLib.hb_get_filter_object(NativeConstants.HB_FILTER_DEINTERLACE, settings));
            }
            else
            {
                nativeJob.deinterlace = 0;
            }

            if (profile.Detelecine != Detelecine.Off)
            {
                string settings = null;
                if (profile.Detelecine == Detelecine.Custom)
                {
                    settings = profile.CustomDetelecine;
                }

                this.AddFilter(filterList, NativeConstants.HB_FILTER_DETELECINE, settings, allocatedMemory);
                //filterList.Add(HbLib.hb_get_filter_object(NativeConstants.HB_FILTER_DETELECINE, settings));
            }

            if (profile.Decomb != Decomb.Off)
            {
                string settings = null;
                if (profile.Decomb == Decomb.Custom)
                {
                    settings = profile.CustomDecomb;
                }

                this.AddFilter(filterList, NativeConstants.HB_FILTER_DECOMB, settings, allocatedMemory);
                //filterList.Add(HbLib.hb_get_filter_object(NativeConstants.HB_FILTER_DECOMB, settings));
            }

            if (profile.Deblock > 0)
            {
                this.AddFilter(filterList, NativeConstants.HB_FILTER_DEBLOCK, profile.Deblock.ToString(), allocatedMemory);
                //filterList.Add(HbLib.hb_get_filter_object(NativeConstants.HB_FILTER_DEBLOCK, profile.Deblock.ToString()));
            }

            if (profile.Denoise != Denoise.Off)
            {
                string settings = null;
                switch (profile.Denoise)
                {
                    case Denoise.Weak:
                        settings = "2:1:2:3";
                        break;
                    case Denoise.Medium:
                        settings = "3:2:2:3";
                        break;
                    case Denoise.Strong:
                        settings = "7:7:5:5";
                        break;
                    case Denoise.Custom:
                        settings = profile.CustomDenoise;
                        break;
                    default:
                        break;
                }

                this.AddFilter(filterList, NativeConstants.HB_FILTER_DENOISE, settings, allocatedMemory);
                //filterList.Add(HbLib.hb_get_filter_object(NativeConstants.HB_FILTER_DENOISE, settings));
            }

            NativeList filterListNative = InteropUtilities.CreateIntPtrList(filterList);
            nativeJob.filters = filterListNative.ListPtr;
            allocatedMemory.AddRange(filterListNative.AllocatedMemory);

            int width = profile.Width;
            int height = profile.Height;

            if (width == 0)
            {
                width = title.Resolution.Width;
            }

            if (profile.MaxWidth > 0 && width > profile.MaxWidth)
            {
                width = profile.MaxWidth;
            }

            if (height == 0)
            {
                height = title.Resolution.Height;
            }

            if (profile.MaxHeight > 0 && height > profile.MaxHeight)
            {
                height = profile.MaxHeight;
            }

            nativeJob.grayscale = profile.Grayscale ? 1 : 0;

            switch (profile.Anamorphic)
            {
                case Anamorphic.None:
                    nativeJob.anamorphic.mode = 0;

                    if (profile.KeepDisplayAspect)
                    {
                        if (profile.Width == 0 && profile.Height == 0 || profile.Width == 0)
                        {
                            width = (int)((double)height * this.GetTitle(job.Title).AspectRatio);
                        }
                        else if (profile.Height == 0)
                        {
                            height = (int)((double)width / this.GetTitle(job.Title).AspectRatio);
                        }
                    }

                    nativeJob.anamorphic.keep_display_aspect = profile.KeepDisplayAspect ? 1 : 0;
                    break;
                case Anamorphic.Strict:
                    nativeJob.anamorphic.mode = 1;
                    break;
                case Anamorphic.Loose:
                    nativeJob.anamorphic.mode = 2;
                    break;
                case Anamorphic.Custom:
                    nativeJob.anamorphic.mode = 3;

                    nativeJob.modulus = profile.Modulus;

                    if (profile.UseDisplayWidth)
                    {
                        if (profile.KeepDisplayAspect)
                        {
                            height = (int)((double)profile.DisplayWidth / this.GetTitle(job.Title).AspectRatio);
                        }

                        nativeJob.anamorphic.dar_width = profile.DisplayWidth;
                        nativeJob.anamorphic.dar_height = height;
                        nativeJob.anamorphic.keep_display_aspect = profile.KeepDisplayAspect ? 1 : 0;
                    }
                    else
                    {
                        nativeJob.anamorphic.par_width = profile.PixelAspectX;
                        nativeJob.anamorphic.par_height = profile.PixelAspectY;
                        nativeJob.anamorphic.keep_display_aspect = 0;
                    }
                    break;
                default:
                    break;
            }

            nativeJob.width = width;
            nativeJob.height = height;

            nativeJob.maxWidth = profile.MaxWidth;
            nativeJob.maxHeight = profile.MaxHeight;

            switch (profile.VideoEncoder)
            {
                case VideoEncoder.X264:
                    nativeJob.vcodec = NativeConstants.HB_VCODEC_X264;
                    break;
                case VideoEncoder.Theora:
                    nativeJob.vcodec = NativeConstants.HB_VCODEC_THEORA;
                    break;
                case VideoEncoder.FFMpeg:
                    nativeJob.vcodec = NativeConstants.HB_VCODEC_FFMPEG;
                    break;
                default:
                    break;
            }

            if (profile.VideoEncodeRateType == VideoEncodeRateType.ConstantQuality)
            {
                nativeJob.vquality = (float)profile.Quality;
                nativeJob.vbitrate = 0;
            }
            else if (profile.VideoEncodeRateType == VideoEncodeRateType.AverageBitrate)
            {
                nativeJob.vquality = -1;
                nativeJob.vbitrate = profile.VideoBitrate;
            }

            // vrate
            // vrate_base
            // vfr
            // cfr
            // areBframes
            // color_matrix
            List<hb_audio_s> titleAudio = InteropUtilities.ConvertList<hb_audio_s>(originalTitle.list_audio);
            
            List<hb_audio_s> audioList = new List<hb_audio_s>();
            int numTracks = 0;
            foreach (AudioEncoding encoding in profile.AudioEncodings)
            {
                if (encoding.InputNumber == 0)
                {
                    // Add this encoding for all chosen tracks
                    foreach (int chosenTrack in job.ChosenAudioTracks)
                    {
                        if (titleAudio.Count >= chosenTrack)
                        {
                            audioList.Add(ConvertAudioBack(encoding, titleAudio[chosenTrack - 1], chosenTrack, numTracks++));
                        }
                    }
                }
                else if (encoding.InputNumber <= job.ChosenAudioTracks.Count)
                {
                    // Add this encoding for the specified track, if it exists
                    int trackNumber = job.ChosenAudioTracks[encoding.InputNumber - 1];
                    audioList.Add(ConvertAudioBack(encoding, titleAudio[trackNumber - 1], trackNumber, numTracks++));
                }
            }

            NativeList nativeAudioList = InteropUtilities.ConvertListBack<hb_audio_s>(audioList);
            nativeJob.list_audio = nativeAudioList.ListPtr;
            allocatedMemory.AddRange(nativeAudioList.AllocatedMemory);

            List<hb_subtitle_s> subtitleList = new List<hb_subtitle_s>();

            if (job.Subtitles != null)
            {
                if (job.Subtitles.SourceSubtitles != null && job.Subtitles.SourceSubtitles.Count > 0)
                {
                    List<hb_subtitle_s> titleSubtitles = InteropUtilities.ConvertList<hb_subtitle_s>(originalTitle.list_subtitle);

                    foreach (SourceSubtitle sourceSubtitle in job.Subtitles.SourceSubtitles)
                    {
                        if (sourceSubtitle.TrackNumber == 0)
                        {
                            // Use subtitle search.
                            nativeJob.select_subtitle_config.force = sourceSubtitle.Forced ? 1 : 0;
                            nativeJob.select_subtitle_config.default_track = sourceSubtitle.Default ? 1 : 0;

                            if (!sourceSubtitle.BurnedIn && profile.OutputFormat == OutputFormat.Mkv)
                            {
                                nativeJob.select_subtitle_config.dest = hb_subtitle_config_s_subdest.PASSTHRUSUB;
                            }

                            nativeJob.indepth_scan = 1;
                        }
                        else
                        {
                            // Use specified subtitle.
                            hb_subtitle_s nativeSubtitle = titleSubtitles[sourceSubtitle.TrackNumber - 1];
                            nativeSubtitle.config.force = sourceSubtitle.Forced ? 1 : 0;
                            nativeSubtitle.config.default_track = sourceSubtitle.Default ? 1 : 0;

                            if (!sourceSubtitle.BurnedIn && profile.OutputFormat == OutputFormat.Mkv && nativeSubtitle.format == hb_subtitle_s_subtype.PICTURESUB)
                            {
                                nativeSubtitle.config.dest = hb_subtitle_config_s_subdest.PASSTHRUSUB;
                            }

                            subtitleList.Add(nativeSubtitle);
                        }
                    }
                }

                if (job.Subtitles.SrtSubtitles != null)
                {
                    foreach (SrtSubtitle srtSubtitle in job.Subtitles.SrtSubtitles)
                    {
                        hb_subtitle_s nativeSubtitle = new hb_subtitle_s();
                        nativeSubtitle.id = subtitleList.Count << 8 | 0xFF;
                        nativeSubtitle.iso639_2 = srtSubtitle.LanguageCode;
                        nativeSubtitle.lang = LanguageCodes.Decode(srtSubtitle.LanguageCode);
                        nativeSubtitle.source = hb_subtitle_s_subsource.SRTSUB;
                        nativeSubtitle.format = hb_subtitle_s_subtype.TEXTSUB;

                        nativeSubtitle.config.src_codeset = srtSubtitle.CharacterCode;
                        nativeSubtitle.config.src_filename = srtSubtitle.FileName;
                        nativeSubtitle.config.offset = srtSubtitle.Offset;
                        nativeSubtitle.config.dest = hb_subtitle_config_s_subdest.PASSTHRUSUB;
                        nativeSubtitle.config.default_track = srtSubtitle.Default ? 1 : 0;

                        subtitleList.Add(nativeSubtitle);
                    }
                }
            }

            NativeList nativeSubtitleList = InteropUtilities.ConvertListBack<hb_subtitle_s>(subtitleList);
            nativeJob.list_subtitle = nativeSubtitleList.ListPtr;
            allocatedMemory.AddRange(nativeSubtitleList.AllocatedMemory);

            if (profile.OutputFormat == OutputFormat.Mp4)
            {
                nativeJob.mux = NativeConstants.HB_MUX_MP4;
            }
            else
            {
                nativeJob.mux = NativeConstants.HB_MUX_MKV;
            }

            nativeJob.file = job.OutputPath;

            nativeJob.largeFileSize = profile.LargeFile ? 1 : 0;
            nativeJob.mp4_optimize = profile.Optimize ? 1 : 0;
            nativeJob.ipod_atom = profile.IPod5GSupport ? 1 : 0;

            string x264Options = profile.X264Options ?? string.Empty;
            if (profile.TwoPass)
            {
                nativeJob.pass = 1;

                if (profile.TurboFirstPass)
                {
                    if (x264Options == string.Empty)
                    {
                        x264Options = TurboX264Opts;
                    }
                    else
                    {
                        x264Options += ":" + TurboX264Opts;
                    }
                }
            }

            nativeJob.x264opts = Marshal.StringToHGlobalAnsi(x264Options);
            allocatedMemory.Add(nativeJob.x264opts);

            // indepth_scan

            if (title.AngleCount > 1)
            {
                nativeJob.angle = job.Angle;
            }

            // frames_to_skip

            return allocatedMemory;
        }

        /// <summary>
        /// Adds a filter to the given filter list.
        /// </summary>
        /// <param name="filterList">The filter list to add to.</param>
        /// <param name="filterType">The type of filter.</param>
        /// <param name="settings">Settings for the filter.</param>
        /// <param name="allocatedMemory">The list of allocated memory.</param>
        private void AddFilter(List<IntPtr> filterList, int filterType, string settings, List<IntPtr> allocatedMemory)
        {
            IntPtr settingsNativeString = Marshal.StringToHGlobalAnsi(settings);
            filterList.Add(HbLib.hb_get_filter_object(filterType, settingsNativeString));

            allocatedMemory.Add(settingsNativeString);
        }

        /// <summary>
        /// Gets the title, given the 1-based index.
        /// </summary>
        /// <param name="titleIndex">The index of the title (1-based).</param>
        /// <returns>The requested Title.</returns>
        private Title GetTitle(int titleIndex)
        {
            return this.Titles.SingleOrDefault(title => title.TitleNumber == titleIndex);
        }

        /// <summary>
        /// Gets the native title object from the title index.
        /// </summary>
        /// <param name="titleIndex">The index of the title (1-based).</param>
        /// <returns>Gets the native title object for the given index.</returns>
        private hb_title_s GetOriginalTitle(int titleIndex)
        {
            List<hb_title_s> matchingTitles = this.originalTitles.Where(title => title.index == titleIndex).ToList();
            if (matchingTitles.Count == 0)
            {
                throw new ArgumentException("Could not find specified title.");
            }

            if (matchingTitles.Count > 1)
            {
                throw new ArgumentException("Multiple titles matched.");
            }

            return matchingTitles[0];
        }

        /// <summary>
        /// Applies an audio encoding to a native audio encoding base structure.
        /// </summary>
        /// <param name="encoding">The encoding to apply.</param>
        /// <param name="baseStruct">The base native structure.</param>
        /// <param name="track"></param>
        /// <param name="outputTrack"></param>
        /// <returns>The resulting native audio structure.</returns>
        private hb_audio_s ConvertAudioBack(AudioEncoding encoding, hb_audio_s baseStruct, int track, int outputTrack)
        {
            hb_audio_s nativeAudio = baseStruct;

            //nativeAudio.config.input.track = track;
            nativeAudio.config.output.track = outputTrack;

            switch (encoding.Encoder)
            {
                case AudioEncoder.Ac3Passthrough:
                    nativeAudio.config.output.codec = NativeConstants.HB_ACODEC_AC3;
                    break;
                case AudioEncoder.DtsPassthrough:
                    nativeAudio.config.output.codec = NativeConstants.HB_ACODEC_DCA;
                    break;
                case AudioEncoder.Faac:
                    nativeAudio.config.output.codec = NativeConstants.HB_ACODEC_FAAC;
                    break;
                case AudioEncoder.Lame:
                    nativeAudio.config.output.codec = NativeConstants.HB_ACODEC_LAME;
                    break;
                case AudioEncoder.Vorbis:
                    nativeAudio.config.output.codec = NativeConstants.HB_ACODEC_VORBIS;
                    break;
                default:
                    break;
            }

            nativeAudio.config.output.bitrate = encoding.Bitrate;
            nativeAudio.config.output.dynamic_range_compression = 0.0;

            switch (encoding.Mixdown)
            {
                case Mixdown.DolbyProLogicII:
                    nativeAudio.config.output.mixdown = NativeConstants.HB_AMIXDOWN_DOLBYPLII;
                    break;
                case Mixdown.DolbySurround:
                    nativeAudio.config.output.mixdown = NativeConstants.HB_AMIXDOWN_DOLBY;
                    break;
                case Mixdown.Mono:
                    nativeAudio.config.output.mixdown = NativeConstants.HB_AMIXDOWN_MONO;
                    break;
                case Mixdown.SixChannelDiscrete:
                    nativeAudio.config.output.mixdown = NativeConstants.HB_AMIXDOWN_6CH;
                    break;
                case Mixdown.Stereo:
                    nativeAudio.config.output.mixdown = NativeConstants.HB_AMIXDOWN_STEREO;
                    break;
                default:
                    break;
            }

            if (encoding.SampleRate != null)
            {
                nativeAudio.config.output.samplerate = (int)(double.Parse(encoding.SampleRate) * 1000);
            }

            nativeAudio.padding = new byte[24600];

            return nativeAudio;
        }

        /// <summary>
        /// Converts a native title to a Title object.
        /// </summary>
        /// <param name="title">The native title structure.</param>
        /// <returns>The managed Title object.</returns>
        private Title ConvertTitle(hb_title_s title)
        {
            var newTitle = new Title
            {
                TitleNumber = title.index,
                Resolution = new Size(title.width, title.height),
                ParVal = new Size(title.pixel_aspect_width, title.pixel_aspect_height),
                Duration = TimeSpan.FromSeconds(((double)title.duration) / 90000),
                AutoCropDimensions = new Cropping
                {
                    Top = title.crop[0],
                    Bottom = title.crop[1],
                    Left = title.crop[2],
                    Right = title.crop[3]
                },
                AspectRatio = title.aspect,
                AngleCount = title.angle_count
            };

            int currentSubtitleTrack = 1;
            List<hb_subtitle_s> subtitleList = InteropUtilities.ConvertList<hb_subtitle_s>(title.list_subtitle);
            foreach (hb_subtitle_s subtitle in subtitleList)
            {
                var newSubtitle = new Subtitle
                {
                    TrackNumber = currentSubtitleTrack,
                    Language = subtitle.lang,
                    LanguageCode = subtitle.iso639_2
                };

                if (subtitle.format == hb_subtitle_s_subtype.PICTURESUB)
                {
                    newSubtitle.SubtitleType = SubtitleType.Picture;
                }
                else if (subtitle.format == hb_subtitle_s_subtype.TEXTSUB)
                {
                    newSubtitle.SubtitleType = SubtitleType.Text;
                }

                newTitle.Subtitles.Add(newSubtitle);

                currentSubtitleTrack++;
            }

            int currentAudioTrack = 1;
            List<hb_audio_s> audioList = InteropUtilities.ConvertList<hb_audio_s>(title.list_audio);
            foreach (hb_audio_s audio in audioList)
            {
                var newAudio = new AudioTrack
                {
                    TrackNumber = currentAudioTrack,
                    Language = audio.config.lang.simple,
                    LanguageCode = audio.config.lang.iso639_2,
                    Description = audio.config.lang.description
                };

                newTitle.AudioTracks.Add(newAudio);

                currentAudioTrack++;
            }

            List<hb_chapter_s> chapterList = InteropUtilities.ConvertList<hb_chapter_s>(title.list_chapter);
            foreach (hb_chapter_s chapter in chapterList)
            {
                var newChapter = new Chapter
                {
                    ChapterNumber = chapter.index,
                    Duration = TimeSpan.FromSeconds(((double)chapter.duration) / 90000)
                };

                newTitle.Chapters.Add(newChapter);
            }

            return newTitle;
        }
    }
}
