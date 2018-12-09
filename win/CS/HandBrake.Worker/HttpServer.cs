// --------------------------------------------------------------------------------------------------------------------
// <copyright file="HttpServer.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   This is a service worker for the HandBrake app. It allows us to run encodes / scans in a seperate process easily.
//   All API's expose the ApplicationServices models as JSON.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Net;
    using System.Text;
    using System.Threading;

    public class HttpServer
    {
        private readonly HttpListener httpListener = new HttpListener();
        private readonly Dictionary<string, Func<HttpListenerRequest, string>> apiHandlers;

        public HttpServer(Dictionary<string, Func<HttpListenerRequest, string>> apiCalls, int port)
        {
            if (!HttpListener.IsSupported)
            {
                throw new NotSupportedException("HttpListener not supported on this computer.");
            }

            // Store the Handlers
            this.apiHandlers = new Dictionary<string, Func<HttpListenerRequest, string>>(apiCalls);

            Console.WriteLine(Environment.NewLine + "Available APIs: ");
            foreach (KeyValuePair<string, Func<HttpListenerRequest, string>> api in apiCalls)
            {
                string url = string.Format("http://localhost:{0}/{1}/", port, api.Key);
                this.httpListener.Prefixes.Add(url);
                Console.WriteLine(url);
            }

            Console.WriteLine(Environment.NewLine);
              
            this.httpListener.Start();
        }

        public void Run()
        {
            ThreadPool.QueueUserWorkItem((o) =>
            {
                try
                {
                    while (this.httpListener.IsListening)
                    {
                        ThreadPool.QueueUserWorkItem(
                            (c) =>
                                {
                                    var context = c as HttpListenerContext;
                                    if (context == null)
                                    {
                                        return;
                                    }

                                    try
                                    {
                                        string path = context.Request.RawUrl.TrimStart('/').TrimEnd('/');

                                        if (this.apiHandlers.TryGetValue(path, out var actionToPerform))
                                        {
                                            string rstr = actionToPerform(context.Request);
                                            byte[] buf = Encoding.UTF8.GetBytes(rstr);
                                            context.Response.ContentLength64 = buf.Length;
                                            context.Response.OutputStream.Write(buf, 0, buf.Length);
                                        }
                                        else
                                        {
                                            string rstr = "Error, There is a missing API handler.";
                                            byte[] buf = Encoding.UTF8.GetBytes(rstr);
                                            context.Response.ContentLength64 = buf.Length;
                                            context.Response.OutputStream.Write(buf, 0, buf.Length);
                                        }
                                    }
                                    catch (Exception exc)
                                    {
                                        Debug.WriteLine(exc);
                                    }
                                    finally
                                    {
                                        context?.Response.OutputStream.Close();
                                    }
                                },
                            this.httpListener.GetContext());
                    }
                }
                catch (Exception exc)
                {
                    Debug.WriteLine(exc);
                }
            });
        }

        public void Stop()
        {
            this.httpListener.Stop();
            this.httpListener.Close();
        }
    }
}