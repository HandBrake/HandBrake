// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AppBootstrapper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Castle Bootstrapper
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Startup
{
    using System.Reflection;

    using Autofac;

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
        private IContainer container;

        public AppBootstrapper()
        {
            this.Configure();
        }

        protected void Configure()
        {
            ContainerBuilder builder = new ContainerBuilder();

            // Services
            builder.RegisterType<UpdateService>().As<IUpdateService>().SingleInstance();
            builder.RegisterType<LibScan>().As<IScan>().SingleInstance();
            builder.RegisterType<LibEncode>().As<IEncode>().SingleInstance();
            builder.RegisterType<PrePostActionService>().As<IPrePostActionService>().SingleInstance();
            builder.RegisterType<UserSettingService>().As<IUserSettingService>().SingleInstance();
            builder.RegisterType<PresetService>().As<IPresetService>().SingleInstance();
            builder.RegisterType<QueueService>().As<IQueueService>().SingleInstance();
            builder.RegisterType<LogService>().As<ILog>().SingleInstance();
            builder.RegisterType<WindowManager>().As<IWindowManager>().SingleInstance();
            builder.RegisterType<NotifyIconService>().As<INotifyIconService>().SingleInstance();
            builder.RegisterType<ErrorService>().As<IErrorService>().SingleInstance();
            builder.RegisterType<SystemService>().As<ISystemService>().SingleInstance();
            builder.RegisterType<LogInstanceManager>().As<ILogInstanceManager>().SingleInstance();
            builder.RegisterType<PortService>().As<IPortService>().SingleInstance();
            builder.RegisterType<NotificationService>().As<INotificationService>().SingleInstance();

            // ViewModels
            Assembly assembly = typeof(AppBootstrapper).Assembly;
            builder.RegisterAssemblyTypes(assembly).Where(t => t.Name.EndsWith("ViewModel")).AsImplementedInterfaces().SingleInstance();

            container = builder.Build();

            IoCHelper.Setup(container);
        }
    }
}
