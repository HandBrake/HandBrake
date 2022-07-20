// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IPresetManagerViewModel.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the IPresetManagerViewModel type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels.Interfaces
{
    using System;

    using HandBrakeWPF.Services.Presets.Model;

    public interface IPresetManagerViewModel : IViewModelBase
    {
        bool IsOpen { get; set; }

        void SetupWindow(Action<Preset> callback);
    }
}
