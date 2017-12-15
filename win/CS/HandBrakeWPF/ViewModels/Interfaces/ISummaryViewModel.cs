// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ISummaryViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ISummaryViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using System;

    using HandBrakeWPF.EventArgs;
    using HandBrakeWPF.Services.Encode.Model.Models;

    public interface ISummaryViewModel : ITabInterface
    {
        event EventHandler<OutputFormatChangedEventArgs> OutputFormatChanged;
        void SetContainer(OutputFormat container);
        void UpdateDisplayedInfo();

        void PreviousPreview();
        void NextPreview();
    }
}