// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Program.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The application entry point.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF
{
    using HandBrakeWPF.Startup;
    using System;

    using HandBrakeWPF.Helpers;

    using Microsoft.Extensions.DependencyInjection;
    using Microsoft.Extensions.Hosting;

    public static class Program
    {
        [STAThread]
        public static void Main(string[] args)
        {
            var builder = Host.CreateDefaultBuilder().ConfigureServices((_, services) =>
            {
                services.Configure();
            });
            using var host = builder.Build();
            IoCHelper.Setup(host.Services);
            OnApplicationStarted(host.Services);
            OnApplicationStopped(host.Services);
            host.Start();
        }

        private static void OnApplicationStarted(IServiceProvider serviceProvider)
        {
            var applicationLifetime = serviceProvider.GetRequiredService<IHostApplicationLifetime>();

            applicationLifetime.ApplicationStarted.Register(() =>
            {
                var app = serviceProvider.GetRequiredService<App>();
                app.Exit += (_, _) => applicationLifetime.StopApplication();
                app.Run();
            });
        }

        private static void OnApplicationStopped(IServiceProvider serviceProvider)
        {
            var applicationLifetime = serviceProvider.GetRequiredService<IHostApplicationLifetime>();

            applicationLifetime.ApplicationStopped.Register(() =>
            {
                var app = serviceProvider.GetRequiredService<App>();

                app.Dispatcher.Invoke(() =>
                {
                    app.Shutdown();
                });
            });
        }
    }
}