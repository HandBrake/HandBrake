// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AddToQueueQualitySweep.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands.DebugTools
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Windows;
    using System.Windows.Input;

    using HandBrakeWPF.Commands.DebugTools.Model;
    using HandBrakeWPF.Model.Video;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Queue.Model;
    using HandBrakeWPF.Utilities.FileDialogs;
    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

    internal class QueueExtractResultDataCommand : ICommand
    {
        private readonly IErrorService errorService;
        private readonly QueueViewModel queueViewModel;

        public QueueExtractResultDataCommand(IQueueViewModel queueViewModel, IErrorService errorService)
        {
            this.errorService = errorService;

            // Don't want to pollute the API for debug features.
            this.queueViewModel = (QueueViewModel)queueViewModel;
        }

        public bool CanExecute(object parameter)
        {
            return true;
        }

        public void Execute(object parameter)
        {
            if (!this.queueViewModel.QueueTasks.Any(s => s.Status == QueueItemStatus.Completed))
            {
                return;
            }
            
            StringBuilder content = this.CreateCsvHeader();

            foreach (QueueTask task in this.queueViewModel.QueueTasks)
            {
                if (task.Status == QueueItemStatus.Completed)
                {
                    ProcessedLog logdata = ProcessLog(task.Statistics.CompletedActivityLogPath);

                    content.AppendLine(
                        string.Format("{0}, {1}, {2}, {3}, {4}, {5}", 
                            task.Task.Source, 
                            task.Task.Destination, 
                            task.Statistics.FinalFileSizeInMegaBytes, 
                            Math.Round(task.Statistics.EncodingSpeed, 2),
                            logdata.VideoAvgBitrate,
                            task.Task.VideoEncodeRateType == VideoEncodeRateType.ConstantQuality ? task.Task.Quality : task.Task.VideoBitrate));
                }
            }

            // Output the File 
            SaveFileDialog saveFileDialog = new SaveFileDialog
                                            {
                                                Filter = "csv|*.csv",
                                                CheckPathExists = true,
                                                AddExtension = true,
                                                DefaultExt = ".csv",
                                            };

            bool? result = saveFileDialog.ShowDialog();
            if (result.HasValue && result.Value)
            {
                using (StreamWriter writer = new StreamWriter(saveFileDialog.FileName))
                {
                    writer.WriteLine(content.ToString());
                }
            }

            this.errorService.ShowMessageBox(
                Resources.Debug_QueueExportDataDone,
                Resources.Notice,
                MessageBoxButton.OK,
                MessageBoxImage.Information);
        }

        public event EventHandler CanExecuteChanged;

        private StringBuilder CreateCsvHeader()
        {
            StringBuilder content = new StringBuilder();
            content.AppendLine("source, destination, destination filesize (MB), encode fps, average output bitrate, quality");

            return content;
        }

        private ProcessedLog ProcessLog(string logPath)
        {
            if (!File.Exists(logPath))
            {
                return null;
            }

            ProcessedLog logData = new ProcessedLog();

            using (StreamReader reader = new StreamReader(logPath))
            {
                string line;
                while ((line = reader.ReadLine()) != null)
                {
                    // Video Track 
                    if (line.Contains("mux: track 0"))
                    {
                        List<string> muxedTrack = GetResultRegex(@"track ([0-9.]*), ([0-9.]*) frames, ([0-9.]*) bytes, ([0-9.]*) kbps, fifo", line);

                        if (muxedTrack.Count >= 5 && decimal.TryParse(muxedTrack[4], out decimal averageBitrate))
                        {
                            logData.VideoAvgBitrate = averageBitrate;
                        }
                    }
                }
            }


            return logData;
        }

        private List<string> GetResultRegex(string regex, string line)
        {
            List<string> groups = new List<string>();
            Match match = Regex.Match(line, regex, RegexOptions.IgnoreCase);

            if (match.Success)
            {
                foreach (var group in match.Groups)
                {
                    groups.Add(((Group)group).Value);
                }

                return groups;
            }

            return null;
        }
    }
}
