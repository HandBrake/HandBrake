// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IQueueSelectionViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Queue Selection View Model Interface
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;

    using HandBrakeWPF.Model;
    using HandBrakeWPF.Model.Queue;
    using HandBrakeWPF.Services.Presets.Model;
    using HandBrakeWPF.Services.Scan.Model;

    /// <summary>
    /// The Add Preset View Model
    /// </summary>
    public interface IQueueSelectionViewModel : IViewModelBase
    {
        /// <summary>
        /// Gets the selected titles.
        /// </summary>
        BindingList<SelectionTitle> TitleList { get; }

        /// <summary>
        /// The setup.
        /// </summary>
        /// <param name="scannedSource">
        /// The scanned source.
        /// </param>
        /// <param name="addAction">
        /// The add To Queue action
        /// </param>
        /// <param name="preset">
        /// The preset.
        /// </param>
        void Setup(Source scannedSource, Action<IEnumerable<SelectionTitle>, QueueAddRangeLimit> addAction, Preset preset);
    }
}
