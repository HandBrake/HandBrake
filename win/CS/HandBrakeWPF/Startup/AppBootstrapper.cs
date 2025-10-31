// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AppBootstrapper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Application Bootstrapper
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Startup
{
    using System;
    using System.Linq;
    using System.Reflection;

    using Microsoft.Extensions.DependencyInjection;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Logging;
    using HandBrakeWPF.Services.Logging.Interfaces;
    using HandBrakeWPF.Services.Presets;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Queue;
    using HandBrakeWPF.Services.Queue.Interfaces;
    using HandBrakeWPF.Services.Scan;
    using HandBrakeWPF.Services.Scan.Interfaces;

    using IEncode = Services.Encode.Interfaces.IEncode;
    using IWindowManager = Services.Interfaces.IWindowManager;
    using LibEncode = Services.Encode.LibEncode;

    public class AppBootstrapper
    {
        private IServiceProvider serviceProvider;

        public AppBootstrapper()
        {
            this.Configure();
        }

        protected void Configure()
        {
            var services = new ServiceCollection();

            // Services
            services.AddSingleton<IUpdateService, UpdateService>();
            services.AddSingleton<IScan, LibScan>();
            services.AddSingleton<IEncode, LibEncode>();
            services.AddSingleton<IPrePostActionService, PrePostActionService>();
            services.AddSingleton<IUserSettingService, UserSettingService>();
            services.AddSingleton<IPresetService, PresetService>();
            services.AddSingleton<IQueueService, QueueService>();
            services.AddSingleton<ILog, LogService>();
            services.AddSingleton<IWindowManager, WindowManager>();
            services.AddSingleton<INotifyIconService, NotifyIconService>();
            services.AddSingleton<IErrorService, ErrorService>();
            services.AddSingleton<ISystemService, SystemService>();
            services.AddSingleton<ILogInstanceManager, LogInstanceManager>();
            services.AddSingleton<IPortService, PortService>();
            services.AddSingleton<INotificationService, NotificationService>();

            // ViewModels
            Assembly assembly = typeof(AppBootstrapper).Assembly;
            var viewModelTypes = assembly.GetTypes()
                .Where(t => t.Name.EndsWith("ViewModel") && !t.IsAbstract && !t.IsInterface);

            foreach (var viewModelType in viewModelTypes)
            {
                var interfaces = viewModelType.GetInterfaces();
                if (interfaces.Any())
                {
                    foreach (var @interface in interfaces)
                    {
                        services.AddSingleton(@interface, viewModelType);
                    }
                }

                services.AddSingleton(viewModelType);
            }

            serviceProvider = services.BuildServiceProvider();

            IoCHelper.Setup(serviceProvider);
        }
    }
}
