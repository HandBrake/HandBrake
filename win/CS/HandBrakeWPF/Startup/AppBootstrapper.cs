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
    using System;
    using System.Collections.Generic;
    using System.Reflection;

    using Autofac;

    using Caliburn.Micro;

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
    using HandBrakeWPF.ViewModels.Interfaces;

    using IEncode = Services.Encode.Interfaces.IEncode;
    using IWindowManager = Services.Interfaces.IWindowManager;
    using LibEncode = Services.Encode.LibEncode;

    /// <summary>
    /// The Castle Bootstrapper
    /// </summary>
    public class AppBootstrapper : BootstrapperBase
    {
        private IContainer container;

        /// <summary>
        /// Initializes a new instance of the <see cref="AppBootstrapper"/> class.
        /// </summary>
        public AppBootstrapper()
        {
            this.Initialize();
        }

        /// <summary>
        /// Configure AutoFac
        /// </summary>
        protected override void Configure()
        {
            ContainerBuilder builder = new ContainerBuilder();

            builder.RegisterType<Caliburn.Micro.WindowManager>().As<Caliburn.Micro.IWindowManager>();
            builder.RegisterInstance(new EventAggregator()).As<IEventAggregator>();

            // Services
            builder.RegisterType<UpdateService>().As<IUpdateService>().SingleInstance();
            builder.RegisterType<LibScan>().As<IScan>().SingleInstance();
            builder.RegisterType<LibEncode>().As<IEncode>().SingleInstance();
            builder.RegisterType<PrePostActionService>().As<IPrePostActionService>().SingleInstance();
            builder.RegisterType<UserSettingService>().As<IUserSettingService>().SingleInstance();
            builder.RegisterType<PresetService>().As<IPresetService>().SingleInstance();
            builder.RegisterType<QueueService>().As<IQueueService>().SingleInstance();
            builder.RegisterType<LogService>().As<Services.Logging.Interfaces.ILog>().SingleInstance();
            builder.RegisterType<Services.WindowManager>().As<IWindowManager>().SingleInstance();
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

            base.Configure();
        }

        /// <summary>
        /// The on startup.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        protected override void OnStartup(object sender, System.Windows.StartupEventArgs e)
        {
            this.DisplayRootViewForAsync<IShellViewModel>();
        }

        /// <summary>
        /// Get an Instance of a service
        /// </summary>
        /// <param name="service">
        /// The service.
        /// </param>
        /// <param name="key">
        /// The key.
        /// </param>
        /// <returns>
        /// The Service Requested
        /// </returns>
        protected override object GetInstance(Type service, string key)
        {
            if (string.IsNullOrEmpty(key) && container.IsRegistered(service))
            {
                return container.Resolve(service);
            }

            if (!string.IsNullOrEmpty(key) && container.IsRegisteredWithKey(key, service))
            {
                return container.ResolveKeyed(key, service);
            }

            throw new InvalidOperationException("Could not locate any instances.");
        }

        /// <summary>
        /// Get all instances of a service
        /// </summary>
        /// <param name="service">
        /// The service.
        /// </param>
        /// <returns>
        /// A collection of instances of the requested service type.
        /// </returns>
        protected override IEnumerable<object> GetAllInstances(Type service)
        {
            return this.container.Resolve(typeof(IEnumerable<>).MakeGenericType(service)) as IEnumerable<object>;
        }

        /// <summary>
        /// Build Up
        /// </summary>
        /// <param name="instance">
        /// The instance.
        /// </param>
        protected override void BuildUp(object instance)
        {
            this.container.InjectProperties(instance);
        }
    }
}
