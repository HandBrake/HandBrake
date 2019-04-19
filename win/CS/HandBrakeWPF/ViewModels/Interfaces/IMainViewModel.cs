// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IMainViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Main Window View Model
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using System.Windows;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Services.Queue.Model;

    using EncodeTask = HandBrakeWPF.Services.Encode.Model.EncodeTask;

    /// <summary>
    /// The Main Window View Model
    /// </summary>
    public interface IMainViewModel
    {
        /// <summary>
        /// The preset select.
        /// </summary>
        /// <param name="tag">
        /// The tag.
        /// </param>
        void PresetSelect(object tag);

        /// <summary>
        /// Shutdown the Application
        /// </summary>
        void ExitApplication();

        /// <summary>
        /// Open the Log Window
        /// </summary>
        void OpenLogWindow();

        /// <summary>
        /// Open the Queue Window.
        /// </summary>
        void OpenQueueWindow();

        /// <summary>
        /// Add the current task to the queue.
        /// </summary>
        /// <returns>
        /// True if added, false if error
        /// </returns>
        AddQueueError AddToQueue();
        void AddToQueueWithErrorHandling();
        void AddAllToQueue();
        void AddSelectionToQueue();

        /// <summary>
        /// The launch help.
        /// </summary>
        void LaunchHelp();

        /// <summary>
        /// The select source window.
        /// </summary>
        void SelectSourceWindow();

        /// <summary>
        /// File Scan
        /// </summary>
        void FileScan();

        /// <summary>
        /// Folder Scan
        /// </summary>
        void FolderScan();

        /// <summary>
        /// Stop an Encode.
        /// </summary>
        void StopEncode();

        /// <summary>
        /// Start an Encode
        /// </summary>
        void StartEncode();

        /// <summary>
        /// The start scan.
        /// </summary>
        /// <param name="filename">
        /// The filename.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        void StartScan(string filename, int title);

        /// <summary>
        /// Edit a Queue Task
        /// </summary>
        /// <param name="task">
        /// The task.
        /// </param>
        void EditQueueJob(QueueTask task);

        /// <summary>
        /// Shutdown this View
        /// </summary>
        void Shutdown();

        /// <summary>
        /// The files dropped on window.
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        void FilesDroppedOnWindow(DragEventArgs e);

        /// <summary>
        /// Handle Tab Switching
        /// </summary>
        /// <param name="i">The Tab Number</param>
        void SwitchTab(int i);

        /// <summary>
        /// Browse for and set a destination file.
        /// </summary>
        void BrowseDestination();
    }
}