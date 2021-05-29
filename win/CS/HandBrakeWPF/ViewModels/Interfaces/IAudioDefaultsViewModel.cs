// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IAudioDefaultsViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IAudioViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Services.Encode.Model.Models;

    public interface IAudioDefaultsViewModel : IViewModelBase
    {
        AudioBehaviours AudioBehaviours { get; }

        bool IsApplied { get; }

        void Setup(AudioBehaviours behaviours, OutputFormat outputformat);

        void RefreshTask(OutputFormat outputFormat);

        bool ShowWindow();
    }
}
