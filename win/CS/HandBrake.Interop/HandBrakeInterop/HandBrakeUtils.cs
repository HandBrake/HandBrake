// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HandBrakeUtils.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the HandBrakeUtils type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

using System.Runtime.InteropServices;

namespace HandBrake.Interop
{
	using System;
	using System.Collections.Generic;

	using HandBrake.Interop.HbLib;
	using HandBrake.Interop.Model;
	using HandBrake.Interop.Model.Encoding;
	using HandBrake.Interop.SourceData;

	/// <summary>
	/// HandBrake Interop Utilities
	/// </summary>
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

		/// <summary>
		/// Register the logger.
		/// </summary>
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
		/// <param name="encoderName">The encoder to query.</param>
		/// <returns>The number of audio samples used per frame for the given
		/// audio encoder.</returns>
		internal static int GetAudioSamplesPerFrame(string encoderName)
		{
			switch (encoderName)
			{
				case "faac":
				case "ffaac":
				case "copy:aac":
				case "vorbis":
					return 1024;
				case "lame":
				case "copy:mp3":
					return 1152;
				case "ffac3":
				case "copy":
				case "copy:ac3":
				case "copy:dts":
				case "copy:dtshd":
					return 1536;
			}

			// Unknown encoder; make a guess.
			return 1536;
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

				HBAudioEncoder audioEncoder = Encoders.GetAudioEncoder(encoding.Encoder);

				if (audioEncoder.IsPassthrough)
				{
					// Input bitrate is in bits/second.
					audioBitrate = track.Bitrate / 8;
				}
				else if (encoding.EncodeRateType == AudioEncodeRateType.Quality)
				{
					// Can't predict size of quality targeted audio encoding.
					audioBitrate = 0;
				}
				else
				{
					int outputBitrate;
					if (encoding.Bitrate > 0)
					{
						outputBitrate = encoding.Bitrate;
					}
					else
					{
						outputBitrate = Encoders.GetDefaultBitrate(
							audioEncoder,
							encoding.SampleRateRaw == 0 ? track.SampleRate : encoding.SampleRateRaw,
							Encoders.SanitizeMixdown(Encoders.GetMixdown(encoding.Mixdown), audioEncoder, track.ChannelLayout));
					}

					// Output bitrate is in kbps.
					audioBitrate = outputBitrate * 1000 / 8;
				}

				audioBytes += (long)(lengthSeconds * audioBitrate);

				// Audio overhead
				audioBytes += encoding.SampleRateRaw * ContainerOverheadPerFrame / samplesPerFrame;
			}

			return audioBytes;
		}
	}
}
