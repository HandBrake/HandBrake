// --------------------------------------------------------------------------------------------------------------------
// <copyright file="RemoteBase.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A shared code base class for remote instances.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Instance
{
    using System;
    using System.Diagnostics;
    using System.Text;
    using System.Threading.Tasks;

    using HandBrakeWPF.Instance.Model;
    using HandBrakeWPF.Model.Options;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Utilities;

    public class RemoteBase : HttpRequestBase
    {
        internal readonly IUserSettingService userSettingService;
        internal readonly ILog logService;
        internal readonly IPortService portService;

        internal bool serverStarted;
        internal Process workerProcess;

        public RemoteBase(ILog logService, IUserSettingService userSettingService, IPortService portService)
        {
            this.logService = logService;
            this.userSettingService = userSettingService;
            this.portService = portService;
        }

        public bool IsRemoteInstance => true;

        public string Version
        {
            get
            {
                Task<ServerResponse> response = this.MakeHttpGetRequest("Version");
                response.Wait();

                if (!response.Result.WasSuccessful)
                {
                    return null;
                }

                return response.Result?.JsonResponse;
            }
        }

        public int Build
        {
            get
            {
                throw new NotImplementedException("This method is not implemented yet");
                return 0;
            }
        }

        public void Initialize(int verbosityLvl, bool noHardwareMode)
        {
            try
            {
                if (this.workerProcess == null || this.workerProcess.HasExited)
                {
                    var plainTextBytes = Encoding.UTF8.GetBytes(Guid.NewGuid().ToString());
                    this.base64Token = Convert.ToBase64String(plainTextBytes);
                    this.port = this.portService.GetOpenPort(userSettingService.GetUserSetting<int>(UserSettingConstants.ProcessIsolationPort));
                    this.serverUrl = string.Format("http://127.0.0.1:{0}/", this.port);

                    workerProcess = new Process
                    {
                        StartInfo =
                                        {
                                            FileName = "HandBrake.Worker.exe",
                                            Arguments =
                                                string.Format(" --port={0} --token={1} --pid={2}", port, this.base64Token, Process.GetCurrentProcess().Id),
                                            UseShellExecute = false,
                                            RedirectStandardOutput = true,
                                            RedirectStandardError = true,
                                            CreateNoWindow = true
                                        }
                    };
                    workerProcess.Exited += this.WorkerProcess_Exited;
                    workerProcess.OutputDataReceived += this.WorkerProcess_OutputDataReceived;
                    workerProcess.ErrorDataReceived += this.WorkerProcess_OutputDataReceived;

                    workerProcess.Start();
                    workerProcess.BeginOutputReadLine();
                    workerProcess.BeginErrorReadLine();

                    // Set Process Priority
                    switch ((ProcessPriority)this.userSettingService.GetUserSetting<int>(UserSettingConstants.ProcessPriorityInt))
                    {
                        case ProcessPriority.High:
                            workerProcess.PriorityClass = ProcessPriorityClass.High;
                            break;
                        case ProcessPriority.AboveNormal:
                            workerProcess.PriorityClass = ProcessPriorityClass.AboveNormal;
                            break;
                        case ProcessPriority.Normal:
                            workerProcess.PriorityClass = ProcessPriorityClass.Normal;
                            break;
                        case ProcessPriority.Low:
                            workerProcess.PriorityClass = ProcessPriorityClass.Idle;
                            break;
                        default:
                            workerProcess.PriorityClass = ProcessPriorityClass.BelowNormal;
                            break;
                    }

                    int maxAllowed = userSettingService.GetUserSetting<int>(UserSettingConstants.SimultaneousEncodes);
                    this.ServiceLogMessage(string.Format("Remote Process started with Process ID: {0} using port: {1}. Max Allowed Instances: {2}", this.workerProcess.Id, port, maxAllowed));
                }
            }
            catch (Exception e)
            {
                this.ServiceLogMessage("Unable to start worker process.");
                this.ServiceLogMessage(e.ToString());
            }
        }

        private void StopServer()
        {
            if (this.workerProcess != null && !this.workerProcess.HasExited)
            {
                this.workerProcess.Kill();
            }
        }

        public void Dispose()
        {
            try
            {
                if (this.workerProcess != null && !this.workerProcess.HasExited)
                {
                    this.workerProcess.Kill();
                }
            }
            catch (Exception exc)
            {
                Debug.WriteLine(exc); // Already dead.
            }

            this.workerProcess?.Dispose();
            this.workerProcess = null;
        }

        public bool IsServerRunning()
        {
            // Poll the server until it's started up. This allows us to prevent failures in upstream methods.
            if (this.serverStarted)
            {
                return this.serverStarted;
            }

            int count = 0;
            while (!this.serverStarted)
            {
                if (count > 10)
                {
                    logService.LogMessage("Unable to connect to the HandBrake Worker instance after 10 attempts. Try disabling this option in Tools -> Preferences -> Advanced.");
                    return false;
                }

                try
                {
                    var task = Task.Run(async () => await this.MakeHttpGetRequest("IsTokenSet"));
                    task.Wait(2000);

                    if (string.Equals(task.Result.JsonResponse, "True", StringComparison.CurrentCultureIgnoreCase))
                    {
                        this.serverStarted = true;
                        return true;
                    }
                }
                catch (Exception)
                {
                    // Do nothing. We'll try again. The service isn't ready yet.
                }
                finally
                {
                    count = count + 1;
                }
            }

            return true;
        }

        public void ServiceLogMessage(string text)
        {
            string time = DateTime.Now.ToString("HH:mm:ss", System.Globalization.DateTimeFormatInfo.InvariantInfo);
            logService.LogMessage(string.Format("[{0}] {1}", time, text));
        }

        private void WorkerProcess_Exited(object sender, EventArgs e)
        {
            this.ServiceLogMessage("Worker process exited!");
        }

        private void WorkerProcess_OutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            this.logService.LogMessage(e.Data);
        }
    }
}
