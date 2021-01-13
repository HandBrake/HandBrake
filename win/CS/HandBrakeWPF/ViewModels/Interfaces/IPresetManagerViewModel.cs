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

    public interface IPresetManagerViewModel
    {
        bool IsOpen { get; set; }

        void SetupWindow(Action callback);
    }
}
