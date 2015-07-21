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

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Services.Encode;
    using HandBrake.ApplicationServices.Services.Encode.Interfaces;
    using HandBrake.ApplicationServices.Services.Scan;
    using HandBrake.ApplicationServices.Services.Scan.Interfaces;

    using HandBrakeWPF.Commands;
    using HandBrakeWPF.Commands.Interfaces;
    using HandBrakeWPF.Services;
    using HandBrakeWPF.Services.Interfaces;
    using HandBrakeWPF.Services.Presets;
    using HandBrakeWPF.Services.Presets.Interfaces;
    using HandBrakeWPF.Services.Queue;
    using HandBrakeWPF.Services.Queue.Interfaces;
    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// The Castle Bootstrapper
    /// </summary>
    public class AppBootstrapper : Bootstrapper<IShellViewModel>
    {
        private SimpleContainer container;

        /// <summary>
        /// Configure Castle Windsor
        /// </summary>
        protected override void Configure()
        {
            this.container = new SimpleContainer();

            this.container.Singleton<IWindowManager, WindowManager>();
            this.container.Singleton<IEventAggregator, EventAggregator>();

            // Services
            this.container.Singleton<IUpdateService, UpdateService>();
            this.container.Singleton<IScan, LibScan>();
            this.container.Singleton<IEncode, LibEncode>();
            this.container.Singleton<INotificationService, NotificationService>();
            this.container.Singleton<IPrePostActionService, PrePostActionService>();
            this.container.Singleton<IUserSettingService, UserSettingService>();
            this.container.Singleton<IPresetService, PresetService>();
            this.container.Singleton<IQueueProcessor, QueueProcessor>();

            // Commands
            this.container.Singleton<IAdvancedEncoderOptionsCommand, AdvancedEncoderOptionsCommand>();

            // Services and Shell Components
            this.container.Singleton<IErrorService, ErrorService>();
            this.container.Singleton<IErrorViewModel, ErrorViewModel>();
            this.container.Singleton<IMainViewModel, MainViewModel>();
            this.container.Singleton<IQueueViewModel, QueueViewModel>();
            this.container.PerRequest<IAddPresetViewModel, AddPresetViewModel>();
            this.container.Singleton<ILogViewModel, LogViewModel>();
            this.container.Singleton<IAboutViewModel, AboutViewModel>();
            this.container.Singleton<IOptionsViewModel, OptionsViewModel>();
            this.container.Singleton<ITitleSpecificViewModel, TitleSpecificViewModel>();
            this.container.Singleton<IQueueSelectionViewModel, QueueSelectionViewModel>();
            this.container.Singleton<ICountdownAlertViewModel, CountdownAlertViewModel>();
            this.container.Singleton<IMiniViewModel, MiniViewModel>();
            this.container.Singleton<IStaticPreviewViewModel, StaticPreviewViewModel>();


            // Tab Components
            this.container.Singleton<IAudioViewModel, AudioViewModel>();
            this.container.Singleton<IX264ViewModel, X264ViewModel>();
            this.container.Singleton<IAdvancedViewModel, AdvancedViewModel>();
            this.container.Singleton<IPictureSettingsViewModel, PictureSettingsViewModel>();
            this.container.Singleton<IChaptersViewModel, ChaptersViewModel>();
            this.container.Singleton<ISubtitlesViewModel, SubtitlesViewModel>();
            this.container.Singleton<IFiltersViewModel, FiltersViewModel>();
            this.container.Singleton<IVideoViewModel, VideoViewModel>();

            // Shell
            this.container.Singleton<IShellViewModel, ShellViewModel>();

            base.Configure();
        }

        /// <summary>
        /// Select Assemblies
        /// </summary>
        /// <returns>
        /// A List of Assembly objects
        /// </returns>
        protected override IEnumerable<Assembly> SelectAssemblies()
        {
            return AppDomain.CurrentDomain.GetAssemblies();
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
            var instance = container.GetInstance(service, key);
            if (instance != null)
            {
               // this.BuildUp(instance);
                return instance;
            }

            throw new InvalidOperationException("Could not locate any instances for: " + key);
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
            IEnumerable<object> instances = this.container.GetAllInstances(service);
            if (instances != null)
            {
                foreach (var item in instances)
                {
                //   this.BuildUp(item);
                }
            }

            return instances;
        }

        /// <summary>
        /// Build Up
        /// </summary>
        /// <param name="instance">
        /// The instance.
        /// </param>
        protected override void BuildUp(object instance)
        {
            //instance.GetType().GetProperties()
            //   .Where(property => property.CanWrite && property.PropertyType.IsPublic)
            //   .ForEach(property => property.SetValue(instance, this.container.GetInstance(property.PropertyType, property.Name), null));

            this.container.BuildUp(instance);
        }
    }
}
