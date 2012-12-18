// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeInstance.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A wrapper for a HandBrake instance.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
	using System;
	using System.Collections.Generic;
	using System.Globalization;
	using System.IO;
	using System.Linq;
	using System.Runtime.InteropServices;
	using System.Windows.Media.Imaging;

	using HandBrake.Interop.HbLib;
	using HandBrake.Interop.Interfaces;
	using HandBrake.Interop.Model;
	using HandBrake.Interop.Model.Encoding;
	using HandBrake.Interop.SourceData;

	/// <summary>
	/// A wrapper for a HandBrake instance.
	/// </summary>
	public class HandBrakeInstance : IHandBrakeInstance, IDisposable
	{
		/// <summary>
		/// The modulus for picture size when auto-sizing dimensions.
		/// </summary>
		private const int PictureAutoSizeModulus = 2;

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
		/// Lock for creation of handbrake instances;
		/// </summary>
		private static object instanceCreationLock = new object();

		/// <summary>
		/// True if a handbrake instance has been created.
		/// </summary>
		private static bool globalInitialized;

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
		/// The current encode job for this instance.
		/// </summary>
		private EncodeJob currentJob;

		/// <summary>
		/// True if the current job is scanning for subtitles.
		/// </summary>
		private bool subtitleScan;

		/// <summary>
		/// The index of the default title.
		/// </summary>
		private int featureTitle;

		/// <summary>
		/// A list of native memory locations allocated by this instance.
		/// </summary>
		private List<IntPtr> encodeAllocatedMemory;

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
		public event EventHandler<EventArgs> ScanCompleted;

		/// <summary>
		/// Fires for progress updates when encoding.
		/// </summary>
		public event EventHandler<EncodeProgressEventArgs> EncodeProgress;

		/// <summary>
		/// Fires when an encode has completed.
		/// </summary>
		public event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

		/// <summary>
		/// Gets the list of titles on this instance.
		/// </summary>
		public List<Title> Titles
		{
			get
			{
				return this.titles;
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
		/// Initializes this instance.
		/// </summary>
		/// <param name="verbosity">The code for the logging verbosity to use.</param>
		public void Initialize(int verbosity)
		{
			lock (instanceCreationLock)
			{
				if (!globalInitialized)
				{
					globalInitialized = true;
				}
			}

			HandBrakeUtils.RegisterLogger();
			this.hbHandle = HBFunctions.hb_init(verbosity, update_check: 0);
		}

		/// <summary>
		/// Starts scanning the given path.
		/// </summary>
		/// <param name="path">The path to the video to scan.</param>
		/// <param name="previewCount">The number of preview images to make.</param>
		/// <param name="minDuration">The minimum duration of a title to show up on the scan.</param>
		public void StartScan(string path, int previewCount, TimeSpan minDuration)
		{
			this.StartScan(path, previewCount, minDuration, 0);
		}

		public void StartScan(string path, int previewCount)
		{
			this.StartScan(path, previewCount, TimeSpan.FromSeconds(10), 0);
		}

		/// <summary>
		/// Starts a scan of the given path.
		/// </summary>
		/// <param name="path">The path of the video to scan.</param>
		/// <param name="previewCount">The number of previews to make on each title.</param>
		/// <param name="titleIndex">The title index to scan (1-based, 0 for all titles).</param>
		public void StartScan(string path, int previewCount, int titleIndex)
		{
			this.StartScan(path, previewCount, TimeSpan.Zero, titleIndex);
		}

		/// <summary>
		/// Stops an ongoing scan.
		/// </summary>
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
		/// <param name="job">The encode job to preview.</param>
		/// <param name="previewNumber">The index of the preview to get (0-based).</param>
		/// <returns>An image with the requested preview.</returns>
		public BitmapImage GetPreview(EncodeJob job, int previewNumber)
		{
			IntPtr nativeJobPtr = HBFunctions.hb_job_init_by_index(this.hbHandle, this.GetTitleIndex(job.Title));
			var nativeJob = InteropUtilities.ReadStructure<hb_job_s>(nativeJobPtr);

			List<IntPtr> allocatedMemory = this.ApplyJob(ref nativeJob, job);
			
			// There are some problems with getting previews with deinterlacing. Disabling for now.
			nativeJob.deinterlace = 0;

			int outputWidth = nativeJob.width;
			int outputHeight = nativeJob.height;
			int imageBufferSize = outputWidth * outputHeight * 4;
			IntPtr nativeBuffer = Marshal.AllocHGlobal(imageBufferSize);
			allocatedMemory.Add(nativeBuffer);
			HBFunctions.hb_get_preview(this.hbHandle, ref nativeJob, previewNumber, nativeBuffer);

			// We've used the job to get the preview. Clean up the job.
			InteropUtilities.CloseJob(nativeJobPtr);

			// Copy the filled image buffer to a managed array.
			byte[] managedBuffer = new byte[imageBufferSize];
			Marshal.Copy(nativeBuffer, managedBuffer, 0, imageBufferSize);

			// We've copied the data out of unmanaged memory. Clean up that memory now.
			InteropUtilities.FreeMemory(allocatedMemory);

			var bitmap = new System.Drawing.Bitmap(outputWidth, outputHeight);
			System.Drawing.Imaging.BitmapData bitmapData = bitmap.LockBits(new System.Drawing.Rectangle(0, 0, outputWidth, outputHeight), System.Drawing.Imaging.ImageLockMode.WriteOnly, System.Drawing.Imaging.PixelFormat.Format32bppRgb);

			IntPtr ptr = bitmapData.Scan0;

			for (int i = 0; i < nativeJob.height; i++)
			{
				Marshal.Copy(managedBuffer, i * nativeJob.width * 4, ptr, nativeJob.width * 4);
				ptr = IntPtr.Add(ptr, bitmapData.Stride);
			}

			bitmap.UnlockBits(bitmapData);

			using (var memoryStream = new MemoryStream())
			{
				try
				{
					bitmap.Save(memoryStream, System.Drawing.Imaging.ImageFormat.Bmp);
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
		/// Calculates the video bitrate for the given job and target size.
		/// </summary>
		/// <param name="job">The encode job.</param>
		/// <param name="sizeMB">The target size in MB.</param>
		/// <param name="overallSelectedLengthSeconds">The currently selected encode length. Used in preview
		/// for calculating bitrate when the target size would be wrong.</param>
		/// <returns>The video bitrate in kbps.</returns>
		public int CalculateBitrate(EncodeJob job, int sizeMB, double overallSelectedLengthSeconds = 0)
		{
			long availableBytes = ((long) sizeMB) * 1024 * 1024;

			EncodingProfile profile = job.EncodingProfile;
			Title title = this.GetTitle(job.Title);

			double lengthSeconds = overallSelectedLengthSeconds > 0 ? overallSelectedLengthSeconds : HandBrakeUtils.GetJobLengthSeconds(job, title);
			lengthSeconds += 1.5;

			double outputFramerate;
			if (profile.Framerate == 0)
			{
				outputFramerate = title.Framerate;
			}
			else
			{
				// Not sure what to do for VFR here hb_calc_bitrate never handled it...
				//   just use the peak for now.
				outputFramerate = profile.Framerate;
			}

			long frames = (long)(lengthSeconds * outputFramerate);

			availableBytes -= frames * HandBrakeUtils.ContainerOverheadPerFrame;

			List<Tuple<AudioEncoding, int>> outputTrackList = this.GetOutputTracks(job, title);
			availableBytes -= HandBrakeUtils.GetAudioSize(job, lengthSeconds, title, outputTrackList);

			if (availableBytes < 0)
			{
				return 0;
			}

			// Video bitrate is in kilobits per second, or where 1 kbps is 1000 bits per second.
			// So 1 kbps is 125 bytes per second.
			return (int)(availableBytes / (125 * lengthSeconds));
		}

		/// <summary>
		/// Gives estimated file size (in MB) of the given job and video bitrate.
		/// </summary>
		/// <param name="job">The encode job.</param>
		/// <param name="videoBitrate">The video bitrate to be used (kbps).</param>
		/// <returns>The estimated file size (in MB) of the given job and video bitrate.</returns>
		public double CalculateFileSize(EncodeJob job, int videoBitrate)
		{
			long totalBytes = 0;

			EncodingProfile profile = job.EncodingProfile;
			Title title = this.GetTitle(job.Title);

			double lengthSeconds = HandBrakeUtils.GetJobLengthSeconds(job, title);
			lengthSeconds += 1.5;

			double outputFramerate;
			if (profile.Framerate == 0)
			{
				outputFramerate = title.Framerate;
			}
			else
			{
				// Not sure what to do for VFR here hb_calc_bitrate never handled it...
				//   just use the peak for now.
				outputFramerate = profile.Framerate;
			}

			long frames = (long)(lengthSeconds * outputFramerate);

			totalBytes += (long)(lengthSeconds * videoBitrate * 125);
			totalBytes += frames * HandBrakeUtils.ContainerOverheadPerFrame;

			List<Tuple<AudioEncoding, int>> outputTrackList = this.GetOutputTracks(job, title);
			totalBytes += HandBrakeUtils.GetAudioSize(job, lengthSeconds, title, outputTrackList);

			return (double)totalBytes / 1024 / 1024;
		}

		/// <summary>
		/// Starts an encode with the given job.
		/// </summary>
		/// <param name="jobToStart">The job to start.</param>
		public void StartEncode(EncodeJob jobToStart)
		{
			this.StartEncode(jobToStart, false, 0, 0, 0);
		}

		/// <summary>
		/// Starts an encode with the given job.
		/// </summary>
		/// <param name="job">The job to start.</param>
		/// <param name="preview">True if this is a preview encode.</param>
		/// <param name="previewNumber">The preview number to start the encode at (0-based).</param>
		/// <param name="previewSeconds">The number of seconds in the preview.</param>
		/// <param name="overallSelectedLengthSeconds">The currently selected encode length. Used in preview
		/// for calculating bitrate when the target size would be wrong.</param>
		public void StartEncode(EncodeJob job, bool preview, int previewNumber, int previewSeconds, double overallSelectedLengthSeconds)
		{
			EncodingProfile profile = job.EncodingProfile;
			this.currentJob = job;

			IntPtr nativeJobPtr = HBFunctions.hb_job_init_by_index(this.hbHandle, this.GetTitleIndex(job.Title));
			var nativeJob = InteropUtilities.ReadStructure<hb_job_s>(nativeJobPtr);

			this.encodeAllocatedMemory = this.ApplyJob(ref nativeJob, job, preview, previewNumber, previewSeconds, overallSelectedLengthSeconds);

			this.subtitleScan = false;
			if (job.Subtitles.SourceSubtitles != null)
			{
				foreach (SourceSubtitle subtitle in job.Subtitles.SourceSubtitles)
				{
					if (subtitle.TrackNumber == 0)
					{
						this.subtitleScan = true;
						break;
					}
				}
			}

			string x264Options = profile.X264Options ?? string.Empty;
			IntPtr originalX264Options = Marshal.StringToHGlobalAnsi(x264Options);
			this.encodeAllocatedMemory.Add(originalX264Options);

			if (!string.IsNullOrEmpty(profile.X264Profile))
			{
				nativeJob.x264_profile = Marshal.StringToHGlobalAnsi(profile.X264Profile);
				this.encodeAllocatedMemory.Add(nativeJob.x264_profile);
			}

			if (!string.IsNullOrEmpty(profile.X264Preset))
			{
				nativeJob.x264_preset = Marshal.StringToHGlobalAnsi(profile.X264Preset);
				this.encodeAllocatedMemory.Add(nativeJob.x264_preset);
			}

			if (profile.X264Tunes != null && profile.X264Tunes.Count > 0)
			{
				nativeJob.x264_tune = Marshal.StringToHGlobalAnsi(string.Join(",", profile.X264Tunes));
				this.encodeAllocatedMemory.Add(nativeJob.x264_tune);
			}

			if (!string.IsNullOrEmpty(job.EncodingProfile.H264Level))
			{
				nativeJob.h264_level = Marshal.StringToHGlobalAnsi(job.EncodingProfile.H264Level);
				this.encodeAllocatedMemory.Add(nativeJob.h264_level);
			}

			if (this.subtitleScan)
			{
				// If we need to scan subtitles, enqueue a pre-processing job to do that.
				nativeJob.pass = -1;
				nativeJob.indepth_scan = 1;

				nativeJob.advanced_opts = IntPtr.Zero;

				HBFunctions.hb_add(this.hbHandle, ref nativeJob);
			}

			nativeJob.indepth_scan = 0;

			if (job.EncodingProfile.TwoPass)
			{
				// First pass. Apply turbo options if needed.
				nativeJob.pass = 1;
				string firstPassAdvancedOptions = x264Options;
				if (job.EncodingProfile.TurboFirstPass)
				{
					if (firstPassAdvancedOptions == string.Empty)
					{
						firstPassAdvancedOptions = TurboX264Opts;
					}
					else
					{
						firstPassAdvancedOptions += ":" + TurboX264Opts;
					}
				}

				nativeJob.advanced_opts = Marshal.StringToHGlobalAnsi(firstPassAdvancedOptions);
				this.encodeAllocatedMemory.Add(nativeJob.advanced_opts);

				HBFunctions.hb_add(this.hbHandle, ref nativeJob);

				// Second pass. Apply normal options.
				nativeJob.pass = 2;
				nativeJob.advanced_opts = originalX264Options;

				HBFunctions.hb_add(this.hbHandle, ref nativeJob);
			}
			else
			{
				// One pass job.
				nativeJob.pass = 0;
				nativeJob.advanced_opts = originalX264Options;

				HBFunctions.hb_add(this.hbHandle, ref nativeJob);
			}

			HBFunctions.hb_start(this.hbHandle);

			// Should be safe to clean up the job we started with; a copy is in the queue now.
			InteropUtilities.CloseJob(nativeJobPtr);

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
			HBFunctions.hb_pause(this.hbHandle);
		}

		/// <summary>
		/// Resumes a paused encode.
		/// </summary>
		public void ResumeEncode()
		{
			HBFunctions.hb_resume(this.hbHandle);
		}

		/// <summary>
		/// Stops the current encode.
		/// </summary>
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
		/// Gets the final size for a given encode job.
		/// </summary>
		/// <param name="job">The encode job to use.</param>
		/// <param name="width">The storage width.</param>
		/// <param name="height">The storage height.</param>
		/// <param name="parWidth">The pixel aspect X number.</param>
		/// <param name="parHeight">The pixel aspect Y number.</param>
		public void GetSize(EncodeJob job, out int width, out int height, out int parWidth, out int parHeight)
		{
			Title title = this.GetTitle(job.Title);

			if (job.EncodingProfile.Anamorphic == Anamorphic.None)
			{
				Size storageDimensions = CalculateNonAnamorphicOutput(job.EncodingProfile, title);

				width = storageDimensions.Width;
				height = storageDimensions.Height;

				parWidth = 1;
				parHeight = 1;

				return;
			}

			IntPtr nativeJobPtr = HBFunctions.hb_job_init_by_index(this.hbHandle, this.GetTitleIndex(title));
			var nativeJob = InteropUtilities.ReadStructure<hb_job_s>(nativeJobPtr);

			List<IntPtr> allocatedMemory = this.ApplyJob(ref nativeJob, job);
			InteropUtilities.FreeMemory(allocatedMemory);

			InteropUtilities.CloseJob(nativeJobPtr);

			// During the ApplyJob call, it modified nativeJob to have the correct width, height and PAR.
			// We use those for the size.
			width = nativeJob.width;
			height = nativeJob.height;
			parWidth = nativeJob.anamorphic.par_width;
			parHeight = nativeJob.anamorphic.par_height;
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
			HBFunctions.hb_close(handlePtr);
			Marshal.FreeHGlobal(handlePtr);
		}

		/// <summary>
		/// Calculates the output size for a non-anamorphic job.
		/// </summary>
		/// <param name="profile">The encoding profile for the job.</param>
		/// <param name="title">The title being encoded.</param>
		/// <returns>The dimensions of the final encode.</returns>
		private static Size CalculateNonAnamorphicOutput(EncodingProfile profile, Title title)
		{
			int sourceWidth = title.Resolution.Width;
			int sourceHeight = title.Resolution.Height;

			int width = profile.Width;
			int height = profile.Height;

			Cropping crop;
			switch (profile.CroppingType)
			{
				case CroppingType.Automatic:
					crop = title.AutoCropDimensions;
					break;
				case CroppingType.Custom:
					crop = profile.Cropping;
					break;
				default:
					crop = new Cropping();
					break;
			}

			sourceWidth -= crop.Left;
			sourceWidth -= crop.Right;

			sourceHeight -= crop.Top;
			sourceHeight -= crop.Bottom;

			double croppedAspectRatio = ((double)sourceWidth * title.ParVal.Width) / (sourceHeight * title.ParVal.Height);

			if (width == 0)
			{
				width = sourceWidth;
			}

			if (profile.MaxWidth > 0 && width > profile.MaxWidth)
			{
				width = profile.MaxWidth;
			}

			if (height == 0)
			{
				height = sourceHeight;
			}

			if (profile.MaxHeight > 0 && height > profile.MaxHeight)
			{
				height = profile.MaxHeight;
			}

			if (profile.KeepDisplayAspect)
			{
				if ((profile.Width == 0 && profile.Height == 0) || profile.Width == 0)
				{
					width = (int)((double)height * croppedAspectRatio);
					if (profile.MaxWidth > 0 && width > profile.MaxWidth)
					{
						width = profile.MaxWidth;
						height = (int)((double)width / croppedAspectRatio);
						height = GetNearestValue(height, PictureAutoSizeModulus);
					}

					width = GetNearestValue(width, PictureAutoSizeModulus);
				}
				else if (profile.Height == 0)
				{
					height = (int)((double)width / croppedAspectRatio);
					if (profile.MaxHeight > 0 && height > profile.MaxHeight)
					{
						height = profile.MaxHeight;
						width = (int)((double)height * croppedAspectRatio);
						width = GetNearestValue(width, PictureAutoSizeModulus);
					}

					height = GetNearestValue(height, PictureAutoSizeModulus);
				}
			}

			return new Size(width, height);
		}

		/// <summary>
		/// Gets the closest value to the given number divisible by the given modulus.
		/// </summary>
		/// <param name="number">The number to approximate.</param>
		/// <param name="modulus">The modulus.</param>
		/// <returns>The closest value to the given number divisible by the given modulus.</returns>
		private static int GetNearestValue(int number, int modulus)
		{
			return modulus * ((number + modulus / 2) / modulus);
		}

		/// <summary>
		/// Starts a scan of the given path.
		/// </summary>
		/// <param name="path">The path of the video to scan.</param>
		/// <param name="previewCount">The number of previews to make on each title.</param>
		/// <param name="minDuration">The minimum duration of a title to show up on the scan.</param>
		/// <param name="titleIndex">The title index to scan (1-based, 0 for all titles).</param>
		private void StartScan(string path, int previewCount, TimeSpan minDuration, int titleIndex)
		{
			this.previewCount = previewCount;
			HBFunctions.hb_scan(this.hbHandle, path, titleIndex, previewCount, 1, (ulong)(minDuration.TotalSeconds * 90000));
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
		/// Checks the status of the ongoing scan.
		/// </summary>
		private void PollScanProgress()
		{
			var state = new hb_state_s();
			HBFunctions.hb_get_state(this.hbHandle, ref state);

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

				IntPtr titleSetPtr = HBFunctions.hb_get_title_set(this.hbHandle);
				hb_title_set_s titleSet = InteropUtilities.ReadStructure<hb_title_set_s>(titleSetPtr);
				this.originalTitles = titleSet.list_title.ToList<hb_title_s>();

				foreach (hb_title_s title in this.originalTitles)
				{
					var newTitle = this.ConvertTitle(title);
					this.titles.Add(newTitle);
				}

				if (this.originalTitles.Count > 0)
				{
					this.featureTitle = titleSet.feature;
				}
				else
				{
					this.featureTitle = 0;
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
			HBFunctions.hb_get_state(this.hbHandle, ref state);

			if (state.state == NativeConstants.HB_STATE_WORKING)
			{
				if (this.EncodeProgress != null)
				{
					int pass = 1;
					int rawJobNumber = state.param.working.job_cur;

					if (this.currentJob.EncodingProfile.TwoPass)
					{
						if (this.subtitleScan)
						{
							switch (rawJobNumber)
							{
								case 1:
									pass = -1;
									break;
								case 2:
									pass = 1;
									break;
								case 3:
									pass = 2;
									break;
								default:
									break;
							}
						}
						else
						{
							switch (rawJobNumber)
							{
								case 1:
									pass = 1;
									break;
								case 2:
									pass = 2;
									break;
								default:
									break;
							}
						}
					}
					else
					{
						if (this.subtitleScan)
						{
							switch (rawJobNumber)
							{
								case 1:
									pass = -1;
									break;
								case 2:
									pass = 1;
									break;
								default:
									break;
							}
						}
						else
						{
							pass = 1;
						}
					}

					var progressEventArgs = new EncodeProgressEventArgs
					{
						FractionComplete = state.param.working.progress,
						CurrentFrameRate = state.param.working.rate_cur,
						AverageFrameRate = state.param.working.rate_avg,
						EstimatedTimeLeft = new TimeSpan(state.param.working.hours, state.param.working.minutes, state.param.working.seconds),
						Pass = pass
					};

					this.EncodeProgress(this, progressEventArgs);
				}
			}
			else if (state.state == NativeConstants.HB_STATE_WORKDONE)
			{
				InteropUtilities.FreeMemory(this.encodeAllocatedMemory);
				this.encodePollTimer.Stop();

				if (this.EncodeCompleted != null)
				{
					this.EncodeCompleted(this, new EncodeCompletedEventArgs { Error = state.param.workdone.error > 0 });
				}
			}
		}

		/// <summary>
		/// Applies the encoding job to the native memory structure and returns a list of memory
		/// locations allocated during this.
		/// </summary>
		/// <param name="nativeJob">The native structure to apply to job info to.</param>
		/// <param name="job">The job info to apply.</param>
		/// <returns>The list of memory locations allocated for the job.</returns>
		private List<IntPtr> ApplyJob(ref hb_job_s nativeJob, EncodeJob job)
		{
			return this.ApplyJob(ref nativeJob, job, false, 0, 0, 0);
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
		/// <param name="overallSelectedLengthSeconds">The currently selected encode length. Used in preview
		/// for calculating bitrate when the target size would be wrong.</param>
		/// <returns>The list of memory locations allocated for the job.</returns>
		private List<IntPtr> ApplyJob(ref hb_job_s nativeJob, EncodeJob job, bool preview, int previewNumber, int previewSeconds, double overallSelectedLengthSeconds)
		{
			var allocatedMemory = new List<IntPtr>();
			Title title = this.GetTitle(job.Title);
			hb_title_s originalTitle = this.GetOriginalTitle(job.Title);

			EncodingProfile profile = job.EncodingProfile;

			if (preview)
			{
				nativeJob.start_at_preview = previewNumber + 1;
				nativeJob.seek_points = this.previewCount;

				// There are 90,000 PTS per second.
				nativeJob.pts_to_stop = previewSeconds * 90000;
			}
			else
			{
				switch (job.RangeType)
				{
					case VideoRangeType.Chapters:

						if (job.ChapterStart > 0 && job.ChapterEnd > 0)
						{
							nativeJob.chapter_start = job.ChapterStart;
							nativeJob.chapter_end = job.ChapterEnd;
						}
						else
						{
							nativeJob.chapter_start = 1;
							nativeJob.chapter_end = title.Chapters.Count;
						}

						break;
					case VideoRangeType.Seconds:
						if (job.SecondsStart < 0 || job.SecondsEnd < 0 || job.SecondsStart >= job.SecondsEnd)
						{
							throw new ArgumentException("Seconds range " + job.SecondsStart + "-" + job.SecondsEnd + " is invalid.", "job");
						}

						// For some reason "pts_to_stop" actually means the number of pts to stop AFTER the start point.
						nativeJob.pts_to_start = (int)(job.SecondsStart * 90000);
						nativeJob.pts_to_stop = (int)((job.SecondsEnd - job.SecondsStart) * 90000);
						break;
					case VideoRangeType.Frames:
						if (job.FramesStart < 0 || job.FramesEnd < 0 || job.FramesStart >= job.FramesEnd)
						{
							throw new ArgumentException("Frames range " + job.FramesStart + "-" + job.FramesEnd + " is invalid.", "job");
						}

						// "frame_to_stop" actually means the number of frames total to encode AFTER the start point.
						nativeJob.frame_to_start = job.FramesStart;
						nativeJob.frame_to_stop = job.FramesEnd - job.FramesStart;
						break;
				}
			}

			// Chapter markers
			nativeJob.chapter_markers = profile.IncludeChapterMarkers ? 1 : 0;

			List<IntPtr> nativeChapters = nativeJob.list_chapter.ToIntPtrList();

			if (!preview && profile.IncludeChapterMarkers)
			{
				int numChapters = title.Chapters.Count;

				if (job.UseDefaultChapterNames)
				{
					for (int i = 0; i < numChapters; i++)
					{
						if (i < nativeChapters.Count)
						{
							HBFunctions.hb_chapter_set_title(nativeChapters[i], "Chapter " + (i + 1));
						}
					}
				}
				else
				{
					for (int i = 0; i < numChapters; i++)
					{
						if (i < nativeChapters.Count && i < job.CustomChapterNames.Count)
						{
							HBFunctions.hb_chapter_set_title(nativeChapters[i], job.CustomChapterNames[i]);
						}
					}
				}
			}

			Cropping crop = GetCropping(profile, title);

			nativeJob.crop[0] = crop.Top;
			nativeJob.crop[1] = crop.Bottom;
			nativeJob.crop[2] = crop.Left;
			nativeJob.crop[3] = crop.Right;

			var filterList = new List<IntPtr>();

			// FILTERS: These must be added in the correct order since we cannot depend on the automatic ordering in hb_add_filter . Ordering is determined
			// by the order they show up in the filters enum.

			// Detelecine
			if (profile.Detelecine != Detelecine.Off)
			{
				string settings = null;
				if (profile.Detelecine == Detelecine.Custom)
				{
					settings = profile.CustomDetelecine;
				}

				this.AddFilter(filterList, (int)hb_filter_ids.HB_FILTER_DETELECINE, settings, allocatedMemory);
			}

			// Decomb
			if (profile.Decomb != Decomb.Off)
			{
				string settings = null;
				switch (profile.Decomb)
				{
					case Decomb.Default:
						break;
					case Decomb.Fast:
						settings = "7:2:6:9:1:80";
						break;
					case Decomb.Bob:
						settings = "455";
						break;
					case Decomb.Custom:
						settings = profile.CustomDecomb;
						break;
					default:
						break;
				}

				this.AddFilter(filterList, (int)hb_filter_ids.HB_FILTER_DECOMB, settings, allocatedMemory);
			}

			// Deinterlace
			if (profile.Deinterlace != Deinterlace.Off)
			{
				nativeJob.deinterlace = 1;
				string settings = null;

				switch (profile.Deinterlace)
				{
					case Deinterlace.Fast:
						settings = "0";
						break;
					case Deinterlace.Slow:
						settings = "1";
						break;
					case Deinterlace.Slower:
						settings = "3";
						break;
					case Deinterlace.Bob:
						settings = "15";
						break;
					case Deinterlace.Custom:
						settings = profile.CustomDeinterlace;
						break;
					default:
						break;
				}

				this.AddFilter(filterList, (int)hb_filter_ids.HB_FILTER_DEINTERLACE, settings, allocatedMemory);
			}
			else
			{
				nativeJob.deinterlace = 0;
			}

			// VFR
			if (profile.Framerate == 0)
			{
				if (profile.ConstantFramerate)
				{
					// CFR with "Same as Source". Use the title rate
					nativeJob.cfr = 1;
					nativeJob.vrate = originalTitle.rate;
					nativeJob.vrate_base = originalTitle.rate_base;
				}
				else
				{
					// Pure VFR "Same as Source"
					nativeJob.cfr = 0;
				}
			}
			else
			{
				// Specified framerate
				if (profile.ConstantFramerate)
				{
					// Mark as pure CFR
					nativeJob.cfr = 1;
				}
				else
				{
					// Mark as peak framerate
					nativeJob.cfr = 2;
				}

				nativeJob.vrate = 27000000;
				nativeJob.vrate_base = Converters.FramerateToVrate(profile.Framerate);
			}

			string vfrSettings = string.Format(CultureInfo.InvariantCulture, "{0}:{1}:{2}", nativeJob.cfr, nativeJob.vrate, nativeJob.vrate_base);
			this.AddFilter(filterList, (int) hb_filter_ids.HB_FILTER_VFR, vfrSettings, allocatedMemory);

			// Deblock
			if (profile.Deblock > 0)
			{
				this.AddFilter(filterList, (int)hb_filter_ids.HB_FILTER_DEBLOCK, profile.Deblock.ToString(CultureInfo.InvariantCulture), allocatedMemory);
			}

			// Denoise
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

				this.AddFilter(filterList, (int)hb_filter_ids.HB_FILTER_DENOISE, settings, allocatedMemory);
			}

			// Crop/scale
			int width = profile.Width;
			int height = profile.Height;

			int cropHorizontal = crop.Left + crop.Right;
			int cropVertical = crop.Top + crop.Bottom;

			if (width == 0)
			{
				width = title.Resolution.Width - cropHorizontal;
			}

			if (profile.MaxWidth > 0 && width > profile.MaxWidth)
			{
				width = profile.MaxWidth;
			}

			if (height == 0)
			{
				height = title.Resolution.Height - cropVertical;
			}

			if (profile.MaxHeight > 0 && height > profile.MaxHeight)
			{
				height = profile.MaxHeight;
			}

			// The job width can sometimes start not clean, due to interference from
			// preview generation. We reset it here to allow good calculations.
			nativeJob.width = width;
			nativeJob.height = height;

			nativeJob.grayscale = profile.Grayscale ? 1 : 0;

			switch (profile.Anamorphic)
			{
				case Anamorphic.None:
					nativeJob.anamorphic.mode = 0;

					Size outputSize = CalculateNonAnamorphicOutput(profile, title);
					width = outputSize.Width;
					height = outputSize.Height;

					nativeJob.anamorphic.keep_display_aspect = profile.KeepDisplayAspect ? 1 : 0;

					nativeJob.width = width;
					nativeJob.height = height;

					nativeJob.maxWidth = profile.MaxWidth;
					nativeJob.maxHeight = profile.MaxHeight;

					break;
				case Anamorphic.Strict:
					nativeJob.anamorphic.mode = 1;

					nativeJob.anamorphic.par_width = title.ParVal.Width;
					nativeJob.anamorphic.par_height = title.ParVal.Height;
					break;
				case Anamorphic.Loose:
					nativeJob.anamorphic.mode = 2;

					nativeJob.modulus = profile.Modulus;

					nativeJob.width = width;

					nativeJob.maxWidth = profile.MaxWidth;

					nativeJob.anamorphic.par_width = title.ParVal.Width;
					nativeJob.anamorphic.par_height = title.ParVal.Height;
					break;
				case Anamorphic.Custom:
					nativeJob.anamorphic.mode = 3;

					nativeJob.modulus = profile.Modulus;

					if (profile.UseDisplayWidth)
					{
						if (profile.KeepDisplayAspect)
						{
							int cropWidth = title.Resolution.Width - cropHorizontal;
							int cropHeight = title.Resolution.Height - cropVertical;

							double displayAspect = ((double)(cropWidth * title.ParVal.Width)) / (cropHeight * title.ParVal.Height);
							int displayWidth = profile.DisplayWidth;

							if (profile.Height > 0)
							{
								displayWidth = (int)((double)profile.Height * displayAspect);
							}
							else if (displayWidth > 0)
							{
								height = (int)((double)displayWidth / displayAspect);
							}
							else
							{
								displayWidth = (int)((double)cropHeight * displayAspect);
							}

							nativeJob.anamorphic.dar_width = displayWidth;
							nativeJob.anamorphic.dar_height = height;
							nativeJob.anamorphic.keep_display_aspect = 1;
						}
						else
						{
							nativeJob.anamorphic.dar_width = profile.DisplayWidth;
							nativeJob.anamorphic.dar_height = height;
							nativeJob.anamorphic.keep_display_aspect = profile.KeepDisplayAspect ? 1 : 0;
						}
					}
					else
					{
						nativeJob.anamorphic.par_width = profile.PixelAspectX;
						nativeJob.anamorphic.par_height = profile.PixelAspectY;
						nativeJob.anamorphic.keep_display_aspect = 0;
					}

					nativeJob.width = width;
					nativeJob.height = height;

					nativeJob.maxWidth = profile.MaxWidth;
					nativeJob.maxHeight = profile.MaxHeight;

					break;
				default:
					break;
			}

			// Need to fix up values before adding crop/scale filter
			if (profile.Anamorphic != Anamorphic.None)
			{
				int anamorphicWidth = 0, anamorphicHeight = 0, anamorphicParWidth = 0, anamorphicParHeight = 0;

				HBFunctions.hb_set_anamorphic_size(ref nativeJob, ref anamorphicWidth, ref anamorphicHeight, ref anamorphicParWidth, ref anamorphicParHeight);
				nativeJob.width = anamorphicWidth;
				nativeJob.height = anamorphicHeight;
				nativeJob.anamorphic.par_width = anamorphicParWidth;
				nativeJob.anamorphic.par_height = anamorphicParHeight;
			}

			string cropScaleSettings = string.Format(
				CultureInfo.InvariantCulture,
				"{0}:{1}:{2}:{3}:{4}:{5}",
				nativeJob.width,
				nativeJob.height,
				nativeJob.crop[0],
				nativeJob.crop[1],
				nativeJob.crop[2],
				nativeJob.crop[3]);
			this.AddFilter(filterList, (int) hb_filter_ids.HB_FILTER_CROP_SCALE, cropScaleSettings, allocatedMemory);

			// Construct final filter list
			NativeList filterListNative = InteropUtilities.CreateIntPtrList(filterList);
			nativeJob.list_filter = filterListNative.ListPtr;
			allocatedMemory.AddRange(filterListNative.AllocatedMemory);



			HBVideoEncoder videoEncoder = Encoders.VideoEncoders.FirstOrDefault(e => e.ShortName == profile.VideoEncoder);
			if (videoEncoder == null)
			{
				throw new ArgumentException("Video encoder " + profile.VideoEncoder + " not recognized.");
			}

			nativeJob.vcodec = videoEncoder.Id;



			// areBframes
			// color_matrix
			List<hb_audio_s> titleAudio = originalTitle.list_audio.ToList<hb_audio_s>();
			
			var audioList = new List<hb_audio_s>();
			int numTracks = 0;

			List<Tuple<AudioEncoding, int>> outputTrackList = this.GetOutputTracks(job, title);

			if (!string.IsNullOrEmpty(profile.AudioEncoderFallback))
			{
				HBAudioEncoder audioEncoder = Encoders.GetAudioEncoder(profile.AudioEncoderFallback);
				if (audioEncoder == null)
				{
					throw new ArgumentException("Unrecognized fallback audio encoder: " + profile.AudioEncoderFallback);
				}

				nativeJob.acodec_fallback = Encoders.GetAudioEncoder(profile.AudioEncoderFallback).Id;
			}

			nativeJob.acodec_copy_mask = (int)NativeConstants.HB_ACODEC_ANY;

			foreach (Tuple<AudioEncoding, int> outputTrack in outputTrackList)
			{
				audioList.Add(this.ConvertAudioBack(outputTrack.Item1, titleAudio[outputTrack.Item2 - 1], numTracks++, allocatedMemory));
			}

			NativeList nativeAudioList = InteropUtilities.ConvertListBack<hb_audio_s>(audioList);
			nativeJob.list_audio = nativeAudioList.ListPtr;
			allocatedMemory.AddRange(nativeAudioList.AllocatedMemory);

			// Create a new empty list
			int totalSubtitles = 0;
			if (job.Subtitles != null)
			{
				if (job.Subtitles.SourceSubtitles != null)
				{
					totalSubtitles += job.Subtitles.SourceSubtitles.Count;
				}

				if (job.Subtitles.SrtSubtitles != null)
				{
					totalSubtitles += job.Subtitles.SrtSubtitles.Count;
				}
			}

			NativeList nativeSubtitleList = InteropUtilities.CreateNativeList(totalSubtitles + 2);
			nativeJob.list_subtitle = nativeSubtitleList.ListPtr;
			allocatedMemory.AddRange(nativeSubtitleList.AllocatedMemory);

			if (job.Subtitles != null)
			{
				if (job.Subtitles.SourceSubtitles != null && job.Subtitles.SourceSubtitles.Count > 0)
				{
					List<hb_subtitle_s> titleSubtitles = originalTitle.list_subtitle.ToList<hb_subtitle_s>();

					foreach (SourceSubtitle sourceSubtitle in job.Subtitles.SourceSubtitles)
					{
						if (sourceSubtitle.TrackNumber == 0)
						{
							// Use subtitle search.
							nativeJob.select_subtitle_config.force = sourceSubtitle.Forced ? 1 : 0;
							nativeJob.select_subtitle_config.default_track = sourceSubtitle.Default ? 1 : 0;

							if (!sourceSubtitle.BurnedIn)
							{
								nativeJob.select_subtitle_config.dest = hb_subtitle_config_s_subdest.PASSTHRUSUB;
							}

							nativeJob.indepth_scan = 1;
						}
						else
						{
							// Use specified subtitle.
							hb_subtitle_s nativeSubtitle = titleSubtitles[sourceSubtitle.TrackNumber - 1];
							var subtitleConfig = new hb_subtitle_config_s();

							subtitleConfig.force = sourceSubtitle.Forced ? 1 : 0;
							subtitleConfig.default_track = sourceSubtitle.Default ? 1 : 0;

							bool supportsBurn = nativeSubtitle.source == hb_subtitle_s_subsource.VOBSUB || nativeSubtitle.source == hb_subtitle_s_subsource.SSASUB || nativeSubtitle.source == hb_subtitle_s_subsource.PGSSUB;
							if (supportsBurn && sourceSubtitle.BurnedIn)
							{
								subtitleConfig.dest = hb_subtitle_config_s_subdest.RENDERSUB;
							}
							else
							{
								subtitleConfig.dest = hb_subtitle_config_s_subdest.PASSTHRUSUB;
							}

							int subtitleAddSucceded = HBFunctions.hb_subtitle_add(ref nativeJob, ref subtitleConfig, sourceSubtitle.TrackNumber - 1);
							if (subtitleAddSucceded == 0)
							{
								System.Diagnostics.Debug.WriteLine("Subtitle add failed");
							}
						}
					}
				}

				if (job.Subtitles.SrtSubtitles != null)
				{
					foreach (SrtSubtitle srtSubtitle in job.Subtitles.SrtSubtitles)
					{
						var subtitleConfig = new hb_subtitle_config_s();

						subtitleConfig.src_codeset = srtSubtitle.CharacterCode;
						subtitleConfig.src_filename = srtSubtitle.FileName;
						subtitleConfig.offset = srtSubtitle.Offset;
						subtitleConfig.default_track = srtSubtitle.Default ? 1 : 0;

						int srtAddSucceded = HBFunctions.hb_srt_add(ref nativeJob, ref subtitleConfig, srtSubtitle.LanguageCode);
						if (srtAddSucceded == 0)
						{
							System.Diagnostics.Debug.WriteLine("SRT add failed");
						}
					}
				}
			}

			if (profile.OutputFormat == Container.Mp4)
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
            nativeJob.opencl_support = profile.OpenCLGSupport ? 1 : 0;
            nativeJob.uvd_support = profile.UVDSupport ? 1 : 0;

			if (title.AngleCount > 1)
			{
				nativeJob.angle = job.Angle;
			}

			switch (profile.VideoEncodeRateType)
			{
				case VideoEncodeRateType.ConstantQuality:
					nativeJob.vquality = (float)profile.Quality;
					nativeJob.vbitrate = 0;
					break;
				case VideoEncodeRateType.AverageBitrate:
					nativeJob.vquality = -1;
					nativeJob.vbitrate = profile.VideoBitrate;
					break;
				case VideoEncodeRateType.TargetSize:
					nativeJob.vquality = -1;
					nativeJob.vbitrate = this.CalculateBitrate(job, profile.TargetSize, overallSelectedLengthSeconds);
					break;
				default:
					break;
			}

			// frames_to_skip

			return allocatedMemory;
		}

		/// <summary>
		/// Gets a list of encodings and target track indices (1-based).
		/// </summary>
		/// <param name="job">The encode job</param>
		/// <param name="title">The title the job is meant to encode.</param>
		/// <returns>A list of encodings and target track indices (1-based).</returns>
		private List<Tuple<AudioEncoding, int>> GetOutputTracks(EncodeJob job, Title title)
		{
			var list = new List<Tuple<AudioEncoding, int>>();

			foreach (AudioEncoding encoding in job.EncodingProfile.AudioEncodings)
			{
				if (encoding.InputNumber == 0)
				{
					// Add this encoding for all chosen tracks
					foreach (int chosenTrack in job.ChosenAudioTracks)
					{
						// In normal cases we'll never have a chosen audio track that doesn't exist but when batch encoding
						// we just choose the first audio track without checking if it exists.
						if (chosenTrack <= title.AudioTracks.Count)
						{
							list.Add(new Tuple<AudioEncoding, int>(encoding, chosenTrack));
						}
					}
				}
				else if (encoding.InputNumber <= job.ChosenAudioTracks.Count)
				{
					// Add this encoding for the specified track, if it exists
					int trackNumber = job.ChosenAudioTracks[encoding.InputNumber - 1];

					// In normal cases we'll never have a chosen audio track that doesn't exist but when batch encoding
					// we just choose the first audio track without checking if it exists.
					if (trackNumber <= title.AudioTracks.Count)
					{
						list.Add(new Tuple<AudioEncoding, int>(encoding, trackNumber));
					}
				}
			}

			return list;
		}

		/// <summary>
		/// Adds a filter to the given filter list.
		/// </summary>
		/// <param name="filterList">The list to add the filter to.</param>
		/// <param name="filterType">The type of filter.</param>
		/// <param name="settings">Settings for the filter.</param>
		/// <param name="allocatedMemory">The list of allocated memory.</param>
		private void AddFilter(List<IntPtr> filterList, int filterType, string settings, List<IntPtr> allocatedMemory)
		{
			IntPtr settingsNativeString = Marshal.StringToHGlobalAnsi(settings);
			hb_filter_object_s filter = InteropUtilities.ReadStructure<hb_filter_object_s>(HBFunctions.hb_filter_init(filterType));
			filter.settings = settingsNativeString;

			IntPtr filterPtr = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(hb_filter_object_s)));
			Marshal.StructureToPtr(filter, filterPtr, false);

			filterList.Add(filterPtr);
			allocatedMemory.Add(settingsNativeString);
			allocatedMemory.Add(filterPtr);
		}

		/// <summary>
		/// Gets the title, given the 1-based title number.
		/// </summary>
		/// <param name="titleNumber">The number of the title (1-based).</param>
		/// <returns>The requested Title.</returns>
		private Title GetTitle(int titleNumber)
		{
			return this.Titles.SingleOrDefault(title => title.TitleNumber == titleNumber);
		}

		/// <summary>
		/// Gets the 1-based title index of the given title.
		/// </summary>
		/// <param name="titleNumber">The 1-based title title number.</param>
		/// <returns>The 1-based title index.</returns>
		private int GetTitleIndex(int titleNumber)
		{
			Title title = this.GetTitle(titleNumber);
			return this.GetTitleIndex(title);
		}

		/// <summary>
		/// Gets the 1-based title index of the given title.
		/// </summary>
		/// <param name="title">The title to look up</param>
		/// <returns>The 1-based title index of the given title.</returns>
		private int GetTitleIndex(Title title)
		{
			return this.Titles.IndexOf(title) + 1;
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
		/// <param name="outputTrack">The output track number (0-based).</param>
		/// <param name="allocatedMemory">The collection of allocated memory.</param>
		/// <returns>The resulting native audio structure.</returns>
		private hb_audio_s ConvertAudioBack(AudioEncoding encoding, hb_audio_s baseStruct, int outputTrack, List<IntPtr> allocatedMemory)
		{
			hb_audio_s nativeAudio = baseStruct;
			HBAudioEncoder encoder = Encoders.GetAudioEncoder(encoding.Encoder);

			nativeAudio.config.output.track = outputTrack;
			nativeAudio.config.output.codec = (uint)encoder.Id;

			if (!encoder.IsPassthrough)
			{
				if (encoding.SampleRateRaw == 0)
				{
					nativeAudio.config.output.samplerate = nativeAudio.config.input.samplerate;
				}
				else
				{
					nativeAudio.config.output.samplerate = encoding.SampleRateRaw;
				}

				HBMixdown mixdown = Encoders.GetMixdown(encoding.Mixdown);
				nativeAudio.config.output.mixdown = mixdown.Id;

				if (encoding.EncodeRateType == AudioEncodeRateType.Bitrate)
				{
					// Disable quality targeting.
					nativeAudio.config.output.quality = -1;

					if (encoding.Bitrate == 0)
					{
						// Bitrate of 0 means auto: choose the default for this codec, sample rate and mixdown.
						nativeAudio.config.output.bitrate = HBFunctions.hb_get_default_audio_bitrate(
							nativeAudio.config.output.codec,
							nativeAudio.config.output.samplerate,
							nativeAudio.config.output.mixdown);
					}
					else
					{
						nativeAudio.config.output.bitrate = encoding.Bitrate;
					}
				}
				else if (encoding.EncodeRateType == AudioEncodeRateType.Quality)
				{
					// Bitrate of -1 signals quality targeting.
					nativeAudio.config.output.bitrate = -1;
					nativeAudio.config.output.quality = encoding.Quality;
				}

				// If this encoder supports compression level, pass it in.
				if (encoder.SupportsCompression)
				{
					nativeAudio.config.output.compression_level = encoding.Compression;
				}

				nativeAudio.config.output.dynamic_range_compression = encoding.Drc;
				nativeAudio.config.output.gain = encoding.Gain;
			}

			if (!string.IsNullOrEmpty(encoding.Name))
			{
				IntPtr encodingNamePtr = Marshal.StringToHGlobalAnsi(encoding.Name);
				nativeAudio.config.output.name = encodingNamePtr;
				allocatedMemory.Add(encodingNamePtr);
			}

			nativeAudio.padding = new byte[MarshalingConstants.AudioPaddingBytes];

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
				Playlist = title.playlist,
				Resolution = new Size(title.width, title.height),
				ParVal = new Size(title.pixel_aspect_width, title.pixel_aspect_height),
				Duration = TimeSpan.FromSeconds(title.duration / 90000),
				AutoCropDimensions = new Cropping
				{
					Top = title.crop[0],
					Bottom = title.crop[1],
					Left = title.crop[2],
					Right = title.crop[3]
				},
				AspectRatio = title.aspect,
				AngleCount = title.angle_count,
				VideoCodecName = title.video_codec_name,
				Framerate = ((double)title.rate) / title.rate_base,
				FramerateNumerator = title.rate,
				FramerateDenominator = title.rate_base
			};

			switch (title.type)
			{
				case hb_title_type_anon.HB_STREAM_TYPE:
					newTitle.InputType = InputType.Stream;
					break;
				case hb_title_type_anon.HB_DVD_TYPE:
					newTitle.InputType = InputType.Dvd;
					break;
				case hb_title_type_anon.HB_BD_TYPE:
					newTitle.InputType = InputType.Bluray;
					break;
			}

			int currentSubtitleTrack = 1;
			List<hb_subtitle_s> subtitleList = title.list_subtitle.ToList<hb_subtitle_s>();
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

				switch (subtitle.source)
				{
					case hb_subtitle_s_subsource.CC608SUB:
						newSubtitle.SubtitleSource = SubtitleSource.CC608;
						break;
					case hb_subtitle_s_subsource.CC708SUB:
						newSubtitle.SubtitleSource = SubtitleSource.CC708;
						break;
					case hb_subtitle_s_subsource.SRTSUB:
						newSubtitle.SubtitleSource = SubtitleSource.SRT;
						break;
					case hb_subtitle_s_subsource.SSASUB:
						newSubtitle.SubtitleSource = SubtitleSource.SSA;
						break;
					case hb_subtitle_s_subsource.TX3GSUB:
						newSubtitle.SubtitleSource = SubtitleSource.TX3G;
						break;
					case hb_subtitle_s_subsource.UTF8SUB:
						newSubtitle.SubtitleSource = SubtitleSource.UTF8;
						break;
					case hb_subtitle_s_subsource.VOBSUB:
						newSubtitle.SubtitleSource = SubtitleSource.VobSub;
						break;
					case hb_subtitle_s_subsource.PGSSUB:
						newSubtitle.SubtitleSource = SubtitleSource.PGS;
						break;
					default:
						break;
				}

				newTitle.Subtitles.Add(newSubtitle);

				currentSubtitleTrack++;
			}

			int currentAudioTrack = 1;
			List<hb_audio_s> audioList = title.list_audio.ToList<hb_audio_s>();
			foreach (hb_audio_s audio in audioList)
			{
				var newAudio = new AudioTrack
				{
					TrackNumber = currentAudioTrack,
					Codec = Converters.NativeToAudioCodec(audio.config.input.codec),
					CodecId = audio.config.input.codec,
					Language = audio.config.lang.simple,
					LanguageCode = audio.config.lang.iso639_2,
					Description = audio.config.lang.description,
					ChannelLayout = audio.config.input.channel_layout,
					SampleRate = audio.config.input.samplerate,
					Bitrate = audio.config.input.bitrate
				};

				newTitle.AudioTracks.Add(newAudio);

				currentAudioTrack++;
			}

			List<hb_chapter_s> chapterList = title.list_chapter.ToList<hb_chapter_s>();
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

		/// <summary>
		/// Gets the cropping to use for the given encoding profile and title.
		/// </summary>
		/// <param name="profile">The encoding profile to use.</param>
		/// <param name="title">The title being encoded.</param>
		/// <returns>The cropping to use for the encode.</returns>
		private static Cropping GetCropping(EncodingProfile profile, Title title)
		{
			Cropping crop;
			switch (profile.CroppingType)
			{
				case CroppingType.Automatic:
					crop = title.AutoCropDimensions;
					break;
				case CroppingType.Custom:
					crop = profile.Cropping;
					break;
				default:
					crop = new Cropping();
					break;
			}
			return crop;
		}
	}
}
