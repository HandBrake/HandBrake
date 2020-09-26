// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Program.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Manage the HandBrake Worker Process Service.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker
{
    using System;
    using System.Collections.Generic;
    using System.Net;
    using System.Threading;

    using HandBrake.Interop.Interop;
    using HandBrake.Worker.Routing;

    public class Program
    {
        private static ApiRouter router;
        private static ManualResetEvent manualResetEvent = new ManualResetEvent(false);

        public static void Main(string[] args)
        {
            AppDomain.CurrentDomain.ProcessExit += CurrentDomain_ProcessExit;

            int port = 8037; // Default Port;
            string token = null;
            
            if (args.Length != 0)
            {
                foreach (string argument in args)
                {
                    if (argument.StartsWith("--port"))
                    {
                        string value = argument.TrimStart("--port=".ToCharArray());
                        if (int.TryParse(value, out var parsedPort))
                        {
                            port = parsedPort;
                        }
                    }

                    if (argument.StartsWith("--token"))
                    {
                        token = argument.TrimStart("--token=".ToCharArray());
                    }
                }
            }
            
            Console.WriteLine("Worker: Starting HandBrake Engine ...");
            router = new ApiRouter();
            router.TerminationEvent += Router_TerminationEvent;
            
            Console.WriteLine("Worker: Starting Web Server on port {0} ...", port);
            Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers = RegisterApiHandlers();
            HttpServer webServer = new HttpServer(apiHandlers, port, token);
            if (webServer.Run())
            {
                Console.WriteLine("Worker: Server Started");
                manualResetEvent.WaitOne();
                webServer.Stop();
            }
            else
            {
                Console.WriteLine("Worker: Failed to start. Exiting ...");
            }
        }

        private static void CurrentDomain_ProcessExit(object sender, System.EventArgs e)
        {
            HandBrakeUtils.DisposeGlobal();
        }

        private static Dictionary<string, Func<HttpListenerRequest, string>> RegisterApiHandlers()
        {
            Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers =
                new Dictionary<string, Func<HttpListenerRequest, string>>();

            // Process Handling
            apiHandlers.Add("Shutdown", ShutdownServer);
            apiHandlers.Add("GetInstanceToken", router.GetInstanceToken);
            apiHandlers.Add("Version", router.GetVersionInfo);

            // Logging
            apiHandlers.Add("GetAllLogMessages", router.GetAllLogMessages);
            apiHandlers.Add("GetLogMessagesFromIndex", router.GetLogMessagesFromIndex);
            apiHandlers.Add("ResetLogging", router.ResetLogging);

            // HandBrake APIs
            apiHandlers.Add("StartEncode", router.StartEncode);
            apiHandlers.Add("PauseEncode", router.PauseEncode);
            apiHandlers.Add("ResumeEncode", router.ResumeEncode);
            apiHandlers.Add("StopEncode", router.StopEncode);
            apiHandlers.Add("PollEncodeProgress", router.PollEncodeProgress);
            
            return apiHandlers;
        }

        private static void Router_TerminationEvent(object sender, EventArgs e)
        {
            ShutdownServer(null);
        }
        
        private static string ShutdownServer(HttpListenerRequest request)
        {
            manualResetEvent.Set();
            return "Server Terminated";
        }
    }
}

