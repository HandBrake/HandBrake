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
    using HandBrake.Worker.Logging;
    using HandBrake.Worker.Routing;
    using HandBrake.Worker.Services;
    using HandBrake.Worker.Services.Interfaces;
    using HandBrake.Worker.Utilities;

    public class Program
    {
        private static readonly ITokenService TokenService = new TokenService();

        private static ApiRouter router;
        private static ManualResetEvent manualResetEvent = new ManualResetEvent(false);

        public static void Main(string[] args)
        {
            AppDomain.CurrentDomain.ProcessExit += CurrentDomain_ProcessExit;

            Portable.Initialise();

            if (!Portable.IsProcessIsolationEnabled())
            {
                Console.WriteLine("Worker is disabled in portable.ini");
                Console.WriteLine("Press 'Enter' to exit");
                Console.Read();
                return;
            }

            int port = 8037; // Default Port;
            string token;
            
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
                        TokenService.RegisterToken(token);
                    }
                }
            }

            if (!TokenService.IsTokenSet())
            {
                ConsoleOutput.WriteLine("# HandBrake Worker", ConsoleColor.DarkYellow);
                ConsoleOutput.WriteLine("*** Please note, this application should not be run standalone. To run the GUI, please use 'HandBrake.exe' *** ", ConsoleColor.Red);
                Console.WriteLine();
            }

            ConsoleOutput.WriteLine("Worker: Starting HandBrake Engine ...", ConsoleColor.White, true);
            router = new ApiRouter();
            router.TerminationEvent += Router_TerminationEvent;

            ConsoleOutput.WriteLine(string.Format("Worker: Starting Web Server on port {0} ...", port), ConsoleColor.White, true);

            Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers = RegisterApiHandlers();
            HttpServer webServer = new HttpServer(apiHandlers, port, TokenService);
            if (webServer.Run())
            {
                ConsoleOutput.WriteLine("Worker: Server Started", ConsoleColor.White, true);
                manualResetEvent.WaitOne();
                webServer.Stop();
            }
            else
            {
                ConsoleOutput.WriteLine("Worker is exiting ...");
            }
        }

        private static void CurrentDomain_ProcessExit(object sender, EventArgs e)
        {
            HandBrakeUtils.DisposeGlobal();
        }

        private static Dictionary<string, Func<HttpListenerRequest, string>> RegisterApiHandlers()
        {
            Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers =
                new Dictionary<string, Func<HttpListenerRequest, string>>();

            // Process Handling
            apiHandlers.Add("Shutdown", ShutdownServer);
            apiHandlers.Add("IsTokenSet", IsTokenSet);
            apiHandlers.Add("RegisterToken", RegisterToken);
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

        private static string RegisterToken(HttpListenerRequest request)
        {
            string requestPostData = HttpUtilities.GetRequestPostData(request);
            if (!string.IsNullOrEmpty(requestPostData))
            {
                TokenService.RegisterToken(requestPostData);
                return true.ToString();
            }

            return false.ToString();
        }

        private static string IsTokenSet(HttpListenerRequest arg)
        {
            return TokenService.IsTokenSet().ToString();
        }

        private static void Router_TerminationEvent(object sender, EventArgs e)
        {
            Console.WriteLine("Worker: Termination event received. ");
            manualResetEvent.Set();
            Environment.Exit(0);
        }
        
        private static string ShutdownServer(HttpListenerRequest request)
        {
            manualResetEvent.Set();
            return "Server Terminated";
        }
    }
}

