// <copyright file="INotifyIconService.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>

namespace HandBrakeWPF.Services.Interfaces
{
    using System.Windows.Forms;

    public interface INotifyIconService
    {
        void RegisterNotifyIcon(NotifyIcon ni);

        void SetTooltip(string text);
    }
}