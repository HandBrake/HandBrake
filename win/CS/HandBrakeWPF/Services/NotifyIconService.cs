// <copyright file="NotifyIconService.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>

namespace HandBrakeWPF.Services
{
    using System.Windows.Forms;

    using HandBrakeWPF.Services.Interfaces;

    public class NotifyIconService : INotifyIconService
    {
        private NotifyIcon notifyIcon;

        public void RegisterNotifyIcon(NotifyIcon ni)
        {
            this.notifyIcon = ni;
        }

        public void SetTooltip(string text)
        {
            if (this.notifyIcon != null)
            {
                this.notifyIcon.Text = text;
            }
        }
    }
}
