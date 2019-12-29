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

    using HandBrake.Worker.Registration;
    using HandBrake.Worker.Routing;

    public class Program
    {
        private static ApiRouter router;
        private static ManualResetEvent manualResetEvent = new ManualResetEvent(false);
        private static ConnectionRegistrar registrar = new ConnectionRegistrar();

        public static void Main(string[] args)
        {
            int port = 8037; // Default Port;
            int verbosity = 1;

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

                    if (argument.StartsWith("--verbosity"))
                    {
                        string value = argument.TrimStart("--port=".ToCharArray());
                        if (int.TryParse(value, out var verbosityVal))
                        {
                            verbosity = verbosityVal;
                        }
                    }
                }
            }

            Console.WriteLine("Starting HandBrake Engine ...");
            router = new ApiRouter();
            router.Initialise(verbosity);

            Console.WriteLine("Starting Web Server ...");
            Console.WriteLine("Using Port: {0}", port);
            Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers = RegisterApiHandlers();
            HttpServer webServer = new HttpServer(apiHandlers, port);
            webServer.Run();

            Console.WriteLine("Web Server Started");

            manualResetEvent.WaitOne();

            webServer.Stop();
        }

        public static Dictionary<string, Func<HttpListenerRequest, string>> RegisterApiHandlers()
        {
            Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers =
                new Dictionary<string, Func<HttpListenerRequest, string>>();

            // Worker APIs
            apiHandlers.Add("Pair", registrar.Pair);
            apiHandlers.Add("GetToken", registrar.GetToken);
            apiHandlers.Add("Shutdown", ShutdownServer);

            // Logging
            apiHandlers.Add("GetAllLogMessages", router.GetAllLogMessages);
            apiHandlers.Add("GetLogMessagesFromIndex", router.GetLogMessagesFromIndex);
            apiHandlers.Add("ResetLogging", router.ResetLogging);

            // HandBrake APIs
            apiHandlers.Add("Version", router.GetVersionInfo); 
            apiHandlers.Add("StartEncode", router.StartEncode);
            apiHandlers.Add("PauseEncode", router.PauseEncode);
            apiHandlers.Add("ResumeEncode", router.ResumeEncode);
            apiHandlers.Add("StopEncode", router.StopEncode);
            apiHandlers.Add("PollEncodeProgress", router.PollEncodeProgress);
            apiHandlers.Add("SetConfiguration", router.SetConfiguration);
            
            return apiHandlers;
        }

        public static string ShutdownServer(HttpListenerRequest request)
        {
            manualResetEvent.Set();
            return "Server Terminated";
        }
    }
}

