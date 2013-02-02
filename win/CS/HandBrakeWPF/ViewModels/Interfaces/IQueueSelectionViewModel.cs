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
    using System.Collections.Generic;
    using System.ComponentModel;

    using HandBrake.ApplicationServices.Parsing;

    using HandBrakeWPF.Model;

    /// <summary>
    /// The Add Preset View Model
    /// </summary>
    public interface IQueueSelectionViewModel
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
        /// <param name="sourceName">
        /// The source Name.
        /// </param>
        void Setup(Source scannedSource, string sourceName);
    }
}
