// --------------------------------------------------------------------------------------------------------------------
// <copyright file="INotifyIconService.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services.Interfaces
{
    using System;
    using System.Drawing;


    public interface INotifyIconService
    {
        void Setup(Icon icon);

        void SetTooltip(string text);

        void SetVisibility(bool isVisible);

        void SetClickCallback(Action callback);
    }
}