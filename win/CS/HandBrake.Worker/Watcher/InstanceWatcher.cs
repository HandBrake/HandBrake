// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InstanceWatcher.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the InstanceWatcher type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Watcher
{
    using System.Diagnostics;
    using System.Timers;

    using HandBrake.Worker.Logging;
    using HandBrake.Worker.Routing;

    public class InstanceWatcher
    {
        private readonly ApiRouter routerInstance;

        private Timer timer;

        public InstanceWatcher(ApiRouter routerInstance)
        {
            this.routerInstance = routerInstance;
        }

        public void Start(int ms)
        {
            if (this.timer != null)
            {
                this.timer.Stop();
            }

            this.timer = new Timer(ms) { AutoReset = true };
            this.timer.Start();
            this.timer.Elapsed += this.Timer_Elapsed;
        }

        private void Timer_Elapsed(object sender, ElapsedEventArgs e)
        {
            // Check if we are still attached to a UI. If not, terminate. 
            Process[] uiProcesses = Process.GetProcessesByName("HandBrake");
            if (uiProcesses.Length == 0)
            {
                ConsoleOutput.WriteLine("HandBrake.exe appears to no longer be running. Terminating child process ...");
                this.routerInstance.StopEncode(null);
                this.routerInstance.OnTerminationEvent();
            }
        }
    }
}
