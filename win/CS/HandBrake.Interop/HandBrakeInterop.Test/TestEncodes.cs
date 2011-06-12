using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using HandBrake.Interop;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace HandBrakeInterop.Test
{
	[TestClass]
	public class TestEncodes
	{
		public const string OutputVideoDirectoryName = "OutputVideos";
		private static readonly string OutputVideoDirectory = Path.Combine(Environment.CurrentDirectory, OutputVideoDirectoryName);

		private ManualResetEvent resetEvent = new ManualResetEvent(false);

		[ClassInitialize]
		public static void Init(TestContext context)
		{
			EnsureOutputVideoDirectoryExists();

			FileInfo[] files = new DirectoryInfo(OutputVideoDirectory).GetFiles();
			foreach (FileInfo file in files)
			{
				file.Delete();
			}
		}

		[TestMethod]
		public void Normal()
		{
			this.RunJob("Normal");
		}

		private void RunJob(string jobName)
		{
			this.resetEvent.Reset();

			EncodeJob job = EncodeJobsPersist.GetJob("Normal");

			if (job.SourceType == SourceType.VideoFolder)
			{
				job.SourcePath = Path.Combine(Environment.CurrentDirectory, Path.GetFileName(job.SourcePath));
			}

			if (job.SourceType == SourceType.File)
			{
				job.SourcePath = Path.Combine(Environment.CurrentDirectory, Path.GetFileName(job.SourcePath));
			}

			string extension;
			if (job.EncodingProfile.OutputFormat == OutputFormat.Mkv)
			{
				extension = ".mkv";
			}
			else
			{
				extension = ".mp4";
			}

			job.OutputPath = Path.Combine(OutputVideoDirectory, jobName + extension);

			var instance = new HandBrakeInstance();
			instance.Initialize(0);
			instance.ScanCompleted += (sender, e) =>
			{
				this.resetEvent.Set();
			};

			instance.StartScan(job.SourcePath, 10);
			this.resetEvent.WaitOne();

			this.resetEvent.Reset();
			instance.EncodeCompleted += (sender, e) =>
			{
				Assert.IsFalse(e.Error);
				this.resetEvent.Set();
			};

			instance.StartEncode(job);
			this.resetEvent.WaitOne();

			Assert.IsTrue(File.Exists(job.OutputPath));

			var fileInfo = new FileInfo(job.OutputPath);
			Assert.IsTrue(fileInfo.Length > 1024);
		}

		private static void EnsureOutputVideoDirectoryExists()
		{
			if (!Directory.Exists(OutputVideoDirectory))
			{
				Directory.CreateDirectory(OutputVideoDirectory);
			}
		}
	}
}
