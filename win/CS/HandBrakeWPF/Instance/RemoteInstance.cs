// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RemoteInstance.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Implementation of IEncodeInstance that works with a remote process rather than locally in-process.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Instance
{
    using System;
    using System.Diagnostics;

    using HandBrake.Interop.Interop.EventArgs;
    using HandBrake.Interop.Interop.Interfaces;
    using HandBrake.Interop.Interop.Json.Encode;

    public class RemoteInstance : IEncodeInstance
    {
        private Process workerProcess;

        public event EventHandler<EncodeCompletedEventArgs> EncodeCompleted;

        public event EventHandler<EncodeProgressEventArgs> EncodeProgress;

        public void PauseEncode()
        {
            throw new NotImplementedException();
        }

        public void ResumeEncode()
        {
            throw new NotImplementedException();
        }

        public void StartEncode(JsonEncodeObject jobToStart)
        {
            throw new NotImplementedException();
        }

        public void StopEncode()
        {
            throw new NotImplementedException();
        }

        protected virtual void OnEncodeCompleted(EncodeCompletedEventArgs e)
        {
            this.EncodeCompleted?.Invoke(this, e);
        }

        protected virtual void OnEncodeProgress(EncodeProgressEventArgs e)
        {
            this.EncodeProgress?.Invoke(this, e);
        }

        private void StartServer()
        {
            if (this.workerProcess == null || this.workerProcess.HasExited)
            {
                this.workerProcess = new Process();

                // TODO Take default port from preferences, then find a usable port thereafter.
                this.workerProcess.StartInfo =
                    new ProcessStartInfo("HandBrake.Worker.exe", "--port=8080")
                        {
                            WindowStyle = ProcessWindowStyle.Normal
                        };

                this.workerProcess.Start();
                this.workerProcess.Exited += this.WorkerProcess_Exited;
            }
        }

        private void WorkerProcess_Exited(object sender, EventArgs e)
        {
            Debug.WriteLine("Worker Process has exited");
        }

        private void StopServer()
        {
            if (this.workerProcess != null && !this.workerProcess.HasExited)
            {
                this.workerProcess.Kill();
            }
        }
    }
}
