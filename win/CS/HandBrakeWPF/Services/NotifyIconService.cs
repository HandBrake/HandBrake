// --------------------------------------------------------------------------------------------------------------------
// <copyright file="NotifyIconService.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System.Drawing;
    using System.Windows.Forms;
    using System.Windows.Input;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Properties;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Queue.Interfaces;
    using HandBrakeWPF.ViewModels.Interfaces;

    using Action = System.Action;
    using MouseEventArgs = System.Windows.Forms.MouseEventArgs;

    public class NotifyIconService : INotifyIconService
    {
        private Action actionCallback;
        private NotifyIcon notifyIcon;

        public void Setup(Icon icon)
        {
            this.notifyIcon = new NotifyIcon();
            this.notifyIcon.Icon = icon;

            this.notifyIcon.ContextMenuStrip = this.BuildAppMenu();
            this.notifyIcon.MouseClick += this.NotifyIcon_MouseClick;
            this.notifyIcon.MouseDown += this.NotifyIcon_MouseDown;
        }

        private void NotifyIcon_MouseDown(object sender, MouseEventArgs e)
        {
            // Update the Context Menu
            this.notifyIcon.ContextMenuStrip = this.BuildAppMenu();
        }

        private void NotifyIcon_MouseClick(object sender, MouseEventArgs e)
        {
            // Only open the window on Left click. Right click has menu.
            if (e.Button == MouseButtons.Left)
            {
                if (this.actionCallback != null)
                {
                    this.actionCallback();
                }
            }
        }

        private ContextMenuStrip BuildAppMenu()
        {
            ContextMenuStrip contextMenuStrip = new ContextMenuStrip();

            contextMenuStrip.Items.Add(Resources.MainView_OpenHandBrake, null, (sender, args) => this.RunAction()); // Open Log Window
            contextMenuStrip.Items.Add(new ToolStripSeparator());

            var queueService = IoCHelper.Get<IQueueService>();
            if (queueService.IsEncoding)
            {
                contextMenuStrip.Items.Add(Resources.MainView_Pause, null, (sender, args) => this.PauseEncode());
                contextMenuStrip.Items.Add(Resources.MainView_Stop, null, (sender, args) => this.StopEncode()); // Open Log Window
                contextMenuStrip.Items.Add(new ToolStripSeparator());
            }

            if (queueService.IsPaused)
            {
                contextMenuStrip.Items.Add(Resources.MainView_StartEncode, null, (sender, args) => this.RestartEncode());
                contextMenuStrip.Items.Add(new ToolStripSeparator());
            }

            contextMenuStrip.Items.Add(Resources.MainView_ActivityLog, null, (sender, args) => new ProcessShortcutCommand(new KeyGesture(Key.L, ModifierKeys.Control)).Execute(null)); // Open Log Window
            contextMenuStrip.Items.Add(Resources.MainView_ShowQueue, null, (sender, args) => new ProcessShortcutCommand(new KeyGesture(Key.Q, ModifierKeys.Control)).Execute(null)); // Open Queue Window

            return contextMenuStrip;
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

        private void RunAction()
        {
            if (this.actionCallback != null)
            {
                this.actionCallback();
            }
        }

        private void StopEncode()
        {
            var vm = IoCHelper.Get<IMainViewModel>();
            vm.StopEncode();
        }

        private void PauseEncode()
        {
            var vm = IoCHelper.Get<IMainViewModel>();
            vm.PauseEncode();
        }

        private void RestartEncode()
        {
            var vm = IoCHelper.Get<IMainViewModel>();
            vm.StartEncode();
        }
    }
}
