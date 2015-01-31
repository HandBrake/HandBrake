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

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Encode.Model;

    using HandBrakeWPF.Services.Presets.Model;

    /// <summary>
    /// The Main Window View Model
    /// </summary>
    public interface IMainViewModel
    {
        /// <summary>
        /// Sets SelectedPreset.
        /// </summary>
        Preset SelectedPreset { set; }

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
        bool AddToQueue();

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
        void EditQueueJob(EncodeTask task);

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
    }
}