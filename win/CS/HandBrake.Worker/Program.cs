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

    public class Program
    {
        /*
         * TODO
         *   Support for connecting via sockets.
         *   All methods will return a json state object response.
         */

        private static ApiRouter router;
        private static ManualResetEvent manualResetEvent = new ManualResetEvent(false);

        public static void Main(string[] args)
        {
            int port = 8036; // Default Port;
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

            // Console.ReadKey(); // Block from closing.
            manualResetEvent.WaitOne();

            webServer.Stop();
        }

        public static Dictionary<string, Func<HttpListenerRequest, string>> RegisterApiHandlers()
        {
            Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers =
                new Dictionary<string, Func<HttpListenerRequest, string>>();

            apiHandlers.Add("Version", router.GetVersionInfo); 
            apiHandlers.Add("StartEncode", router.StartEncode);
            apiHandlers.Add("PauseEncode", router.PauseEncode);
            apiHandlers.Add("ResumeEncode", router.ResumeEncode);
            apiHandlers.Add("StopEncode", router.StopEncode);
            apiHandlers.Add("PollEncodeProgress", router.PollEncodeProgress);
            apiHandlers.Add("SetConfiguration", router.SetConfiguration);
            apiHandlers.Add("Shutdown", ShutdownServer);

            // Logging
            apiHandlers.Add("ConfigureLogging", router.ConfigureLogging);
            apiHandlers.Add("GetFullLog", router.GetFullLog);
            apiHandlers.Add("GetLatestLogIndex", router.GetLatestLogIndex);
            apiHandlers.Add("GetLogFromIndex", router.GetLogFromIndex);

            return apiHandlers;
        }

        public static string ShutdownServer(HttpListenerRequest request)
        {
            manualResetEvent.Set();
            return "Server Terminated";
        }
    }
}

