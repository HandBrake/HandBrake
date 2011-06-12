namespace HandBrake.Interop
{
	using System;
	using System.Collections.Generic;
	using System.Linq;
	using HandBrake.SourceData;

	public static class HandBrakeUtils
	{
		/// <summary>
		/// Estimated overhead in bytes for each frame in output container.
		/// </summary>
		internal const int ContainerOverheadPerFrame = 6;

		/// <summary>
		/// The callback for log messages from HandBrake.
		/// </summary>
		private static LoggingCallback loggingCallback;

		/// <summary>
		/// The callback for error messages from HandBrake.
		/// </summary>
		private static LoggingCallback errorCallback;

		/// <summary>
		/// Fires when HandBrake has logged a message.
		/// </summary>
		public static event EventHandler<MessageLoggedEventArgs> MessageLogged;

		/// <summary>
		/// Fires when HandBrake has logged an error.
		/// </summary>
		public static event EventHandler<MessageLoggedEventArgs> ErrorLogged;

		public static void RegisterLogger()
		{
			// Register the logger if we have not already
			if (loggingCallback == null)
			{
				// Keep the callback as a member to prevent it from being garbage collected.
				loggingCallback = new LoggingCallback(LoggingHandler);
				errorCallback = new LoggingCallback(ErrorHandler);
				HBFunctions.hb_register_logger(loggingCallback);
				HBFunctions.hb_register_error_handler(errorCallback);
			}
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
				// This error happens in normal operations. Log it as a message.
				if (message == "dvd: ifoOpen failed")
				{
					if (MessageLogged != null)
					{
						MessageLogged(null, new MessageLoggedEventArgs { Message = message });
					}

					return;
				}

				if (ErrorLogged != null)
				{
					ErrorLogged(null, new MessageLoggedEventArgs { Message = message });
				}

				System.Diagnostics.Debug.WriteLine("ERROR: " + message);
			}
		}

		/// <summary>
		/// Gets the default mixdown for the given audio encoder and channel layout.
		/// </summary>
		/// <param name="encoder">The output codec to be used.</param>
		/// <param name="layout">The input channel layout.</param>
		/// <returns>The default mixdown for the given codec and channel layout.</returns>
		public static Mixdown GetDefaultMixdown(AudioEncoder encoder, int layout)
		{
			int defaultMixdown = HBFunctions.hb_get_default_mixdown(Converters.AudioEncoderToNative(encoder), layout);
			return Converters.NativeToMixdown(defaultMixdown);
		}

		/// <summary>
		/// Gets the bitrate limits for the given audio codec, sample rate and mixdown.
		/// </summary>
		/// <param name="encoder">The audio encoder used.</param>
		/// <param name="sampleRate">The sample rate used (Hz).</param>
		/// <param name="mixdown">The mixdown used.</param>
		/// <returns>Limits on the audio bitrate for the given settings.</returns>
		public static Limits GetBitrateLimits(AudioEncoder encoder, int sampleRate, Mixdown mixdown)
		{
			if (mixdown == Mixdown.Auto)
			{
				throw new ArgumentException("Mixdown cannot be Auto.");			
			}

			int low = 0;
			int high = 0;

			HBFunctions.hb_get_audio_bitrate_limits(Converters.AudioEncoderToNative(encoder), sampleRate, Converters.MixdownToNative(mixdown), ref low, ref high);

			return new Limits { Low = low, High = high };
		}

		/// <summary>
		/// Sanitizes a mixdown given the output codec and input channel layout.
		/// </summary>
		/// <param name="mixdown">The desired mixdown.</param>
		/// <param name="encoder">The output encoder to be used.</param>
		/// <param name="layout">The input channel layout.</param>
		/// <returns>A sanitized mixdown value.</returns>
		public static Mixdown SanitizeMixdown(Mixdown mixdown, AudioEncoder encoder, int layout)
		{
			int sanitizedMixdown = HBFunctions.hb_get_best_mixdown(Converters.AudioEncoderToNative(encoder), layout, Converters.MixdownToNative(mixdown));
			return Converters.NativeToMixdown(sanitizedMixdown);
		}

		/// <summary>
		/// Sanitizes an audio bitrate given the output codec, sample rate and mixdown.
		/// </summary>
		/// <param name="audioBitrate">The desired audio bitrate.</param>
		/// <param name="encoder">The output encoder to be used.</param>
		/// <param name="sampleRate">The output sample rate to be used.</param>
		/// <param name="mixdown">The mixdown to be used.</param>
		/// <returns>A sanitized audio bitrate.</returns>
		public static int SanitizeAudioBitrate(int audioBitrate, AudioEncoder encoder, int sampleRate, Mixdown mixdown)
		{
			return HBFunctions.hb_get_best_audio_bitrate(Converters.AudioEncoderToNative(encoder), audioBitrate, sampleRate, Converters.MixdownToNative(mixdown));
		}

		/// <summary>
		/// Gets the total number of seconds on the given encode job.
		/// </summary>
		/// <param name="job">The encode job to query.</param>
		/// <param name="title">The title being encoded.</param>
		/// <returns>The total number of seconds of video to encode.</returns>
		internal static double GetJobLengthSeconds(EncodeJob job, Title title)
		{
			switch (job.RangeType)
			{
				case VideoRangeType.Chapters:
					TimeSpan duration = TimeSpan.Zero;
					for (int i = job.ChapterStart; i <= job.ChapterEnd; i++)
					{
						duration += title.Chapters[i - 1].Duration;
					}

					return duration.TotalSeconds;
				case VideoRangeType.Seconds:
					return job.SecondsEnd - job.SecondsStart;
				case VideoRangeType.Frames:
					return (job.FramesEnd - job.FramesStart) / title.Framerate;
			}

			return 0;
		}

		/// <summary>
		/// Gets the number of audio samples used per frame for the given audio encoder.
		/// </summary>
		/// <param name="encoder">The encoder to query.</param>
		/// <returns>The number of audio samples used per frame for the given
		/// audio encoder.</returns>
		internal static int GetAudioSamplesPerFrame(AudioEncoder encoder)
		{
			switch (encoder)
			{
				case AudioEncoder.Faac:
				case AudioEncoder.Vorbis:
					return 1024;
				case AudioEncoder.Lame:
					return 1152;
				case AudioEncoder.Ac3:
				case AudioEncoder.Passthrough:
				case AudioEncoder.Ac3Passthrough:
				case AudioEncoder.DtsPassthrough:
					return 1536;
			}

			System.Diagnostics.Debug.Assert(true, "Audio encoder unrecognized.");
			return 0;
		}

		/// <summary>
		/// Gets the size in bytes for the audio with the given parameters.
		/// </summary>
		/// <param name="job">The encode job.</param>
		/// <param name="lengthSeconds">The length of the encode in seconds.</param>
		/// <param name="title">The title to encode.</param>
		/// <param name="outputTrackList">The list of tracks to encode.</param>
		/// <returns>The size in bytes for the audio with the given parameters.</returns>
		internal static long GetAudioSize(EncodeJob job, double lengthSeconds, Title title, List<Tuple<AudioEncoding, int>> outputTrackList)
		{
			long audioBytes = 0;

			foreach (Tuple<AudioEncoding, int> outputTrack in outputTrackList)
			{
				AudioEncoding encoding = outputTrack.Item1;
				AudioTrack track = title.AudioTracks[outputTrack.Item2 - 1];

				int samplesPerFrame = HandBrakeUtils.GetAudioSamplesPerFrame(encoding.Encoder);
				int audioBitrate;

				if (encoding.Encoder == AudioEncoder.Passthrough ||
					encoding.Encoder == AudioEncoder.Ac3Passthrough ||
					encoding.Encoder == AudioEncoder.DtsPassthrough)
				{
					// Input bitrate is in bits/second.
					audioBitrate = track.Bitrate / 8;
				}
				else
				{
					// Output bitrate is in kbps.
					audioBitrate = encoding.Bitrate * 1000 / 8;
				}

				audioBytes += (long)(lengthSeconds * audioBitrate);

				// Audio overhead
				audioBytes += encoding.SampleRateRaw * ContainerOverheadPerFrame / samplesPerFrame;
			}

			return audioBytes;
		}
	}
}
