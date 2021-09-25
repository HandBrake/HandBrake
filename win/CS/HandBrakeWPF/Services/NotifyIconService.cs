// <copyright file="NotifyIconService.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>

namespace HandBrakeWPF.Services
{
    using System;
    using System.Drawing;
    using System.Windows.Forms;

    using HandBrakeWPF.Services.Interfaces;

    public class NotifyIconService : INotifyIconService
    {
        private Action actionCallback;
        private NotifyIcon notifyIcon;

        public void Setup(Icon icon)
        {
            this.notifyIcon = new NotifyIcon();
            this.notifyIcon.Icon = icon;

            this.notifyIcon.Click += this.NotifyIcon_Click;
        }

        public void SetTooltip(string text)
        {
            if (this.notifyIcon != null)
            {
                this.notifyIcon.Text = text;
            }
        }

        public void SetVisibility(bool isVisible)
        {
            this.notifyIcon.Visible = isVisible;
        }

        public void SetClickCallback(Action callback)
        {
            this.actionCallback = callback;
        }

        private void NotifyIcon_Click(object sender, System.EventArgs e)
        {
            if (this.actionCallback != null)
            {
                this.actionCallback();
            }
        }
    }
}
