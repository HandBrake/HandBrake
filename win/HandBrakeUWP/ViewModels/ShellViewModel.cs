// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ShellViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ViewModels
{
    using System;
    using HandBrake.Properties;
    using HandBrake.Services.Interfaces;
    using HandBrake.ViewModels.Interfaces;
    using Windows.ApplicationModel.DataTransfer;
    using Windows.UI.Xaml;

    public class ShellViewModel : ShellViewModelBase
    {
        public ShellViewModel(IErrorService errorService, IMainViewModel mainViewModel, IOptionsViewModel optionsViewModel)
            : base(errorService, mainViewModel, optionsViewModel)
        {
        }

        /// <summary>
        /// Something is dragged into the Window context, determine if it is a file, and allow linking.
        /// </summary>
        /// <param name="e">Drag Event Args.</param>
        public void DragEntered(DragEventArgs e)
        {
            if (e.DataView.Contains(StandardDataFormats.StorageItems))
            {
                // TODO: Filter by file type?
                e.AcceptedOperation = DataPackageOperation.Link;
                e.DragUIOverride.Caption = ResourcesUI.MainView_SourceOpen;
            }
        }

        /// <summary>
        /// The data dropped on window. Pass this through to the active view model.
        /// </summary>
        /// <param name="e">
        /// The DragEventArgs.
        /// </param>
        public async void DataDropped(DragEventArgs e)
        {
            if (e.DataView.Contains(StandardDataFormats.StorageItems))
            {
                var items = await e.DataView.GetStorageItemsAsync();
                if (items.Count > 0)
                {
                    this.MainViewModel.StartScan(items[0].Path, 0);
                }
            }
        }
    }
}