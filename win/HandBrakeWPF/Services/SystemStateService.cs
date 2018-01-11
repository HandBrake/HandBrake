// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SystemStateService.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Functions related to the System State.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Services
{
    using System.Diagnostics;
    using System.Windows.Forms;
    using Caliburn.Micro;
    using HandBrake.Utilities.Interfaces;
    using HandBrake.Utilities;

    public class SystemStateService : ISystemStateService
    {
        public bool SupportsPowerStateChange => true;

        public bool SupportsLock => true;

        public bool SupportsHibernate => true;

        public bool SupportsLogOff => true;

        public void AllowSleep()
        {
            Win32.AllowSleep();
        }

        public void PreventSleep()
        {
            Win32.PreventSleep();
        }

        public void Hibernate()
        {
            Application.SetSuspendState(PowerState.Hibernate, true, true);
        }

        public void Lock()
        {
            Win32.LockWorkStation();
        }

        public void LogOff()
        {
            Win32.ExitWindowsEx(0, 0);
        }

        public void Quit()
        {
            Execute.OnUIThread(() => System.Windows.Application.Current.Shutdown());
        }

        public void Shutdown()
        {
            Process.Start("Shutdown", "-s -t 60");
        }

        public void Suspend()
        {
            Application.SetSuspendState(PowerState.Suspend, true, true);
        }
    }
}