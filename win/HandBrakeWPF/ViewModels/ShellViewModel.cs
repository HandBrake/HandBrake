// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ShellViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ViewModels
{
    using System.IO;
    using System.Linq;
    using System.Windows;
    using HandBrake.Services.Interfaces;
    using HandBrake.ViewModels.Interfaces;

    public class ShellViewModel : ShellViewModelBase
    {
        public ShellViewModel(IErrorService errorService, IMainViewModel mainViewModel, IOptionsViewModel optionsViewModel)
            : base(errorService, mainViewModel, optionsViewModel)
        {
        }

        /// <summary>
        /// The files dropped on window. Pass this through to the active view model.
        /// </summary>
        /// <param name="e">
        /// The DragEventArgs.
        /// </param>
        public void FilesDroppedOnWindow(DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                string[] fileNames = e.Data.GetData(DataFormats.FileDrop, true) as string[];
                if (fileNames != null && fileNames.Any() && (File.Exists(fileNames[0]) || Directory.Exists(fileNames[0])))
                {
                    this.MainViewModel.StartScan(fileNames[0], 0);
                }
            }

            e.Handled = true;
        }
    }
}