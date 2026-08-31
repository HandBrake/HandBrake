// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IAudioAdvancedView.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IAudioAdvancedView type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using HandBrakeWPF.Model.Audio;
    using HandBrakeWPF.Services.Encode.Model.Models;

    public interface IAudioAdvancedViewModel
    {
        void UpdateTask(AudioTrack task);
        void UpdateTask(AudioBehaviourTrack task);
        bool? ShowDialog();
    }
}
