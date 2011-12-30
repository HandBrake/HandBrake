// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CastleBootstrapper.cs" company="HandBrake Project (http://handbrake.fr)">
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
    using System.Linq;
    using System.Reflection;

    using Caliburn.Micro;

    using Castle.Core;
    using Castle.MicroKernel.Registration;
    using Castle.Windsor;

    using HandBrake.ApplicationServices;

    using ViewModels;
    using ViewModels.Interfaces;

    using HandBrakeWPF.Services;
    using HandBrakeWPF.Services.Interfaces;

    /// <summary>
    /// The Castle Bootstrapper
    /// </summary>
    public class CastleBootstrapper : Bootstrapper<IMainViewModel>
    {
        /// <summary>
        /// The Windsor Container
        /// </summary>
        private IWindsorContainer windsorContainer;

        /// <summary>
        /// Configure Castle Windsor
        /// </summary>
        protected override void Configure()
        {
            this.windsorContainer = new WindsorContainer();
            this.windsorContainer.Register(Component.For<IWindowManager>().ImplementedBy<WindowManager>());
            this.windsorContainer.Register(Component.For<IEventAggregator>().ImplementedBy<EventAggregator>());

            // Initialise the ApplicationServices IWindsorInstaller
            this.windsorContainer.Register(Component.For<IWindsorInstaller>().ImplementedBy<ServicesWindsorInstaller>());
            this.windsorContainer.Install(windsorContainer.ResolveAll<IWindsorInstaller>());

            // Shell
            this.windsorContainer.Register(Component.For<IErrorService>().ImplementedBy<ErrorService>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IErrorViewModel>().ImplementedBy<ErrorViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IMainViewModel>().ImplementedBy<MainViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IQueueViewModel>().ImplementedBy<QueueViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IAddPresetViewModel>().ImplementedBy<AddPresetViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IPreviewViewModel>().ImplementedBy<PreviewViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<ILogViewModel>().ImplementedBy<LogViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IAboutViewModel>().ImplementedBy<AboutViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IOptionsViewModel>().ImplementedBy<OptionsViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IUpdateVersionService>().ImplementedBy<UpdateVersionService>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IJobContextService>().ImplementedBy<JobContextService>().LifeStyle.Is(LifestyleType.Singleton));

            // Tab Components
            this.windsorContainer.Register(Component.For<IAudioViewModel>().ImplementedBy<AudioViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IAdvancedViewModel>().ImplementedBy<AdvancedViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IPictureSettingsViewModel>().ImplementedBy<PictureSettingsViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IChaptersViewModel>().ImplementedBy<ChaptersViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<ISubtitlesViewModel>().ImplementedBy<SubtitlesViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IFiltersViewModel>().ImplementedBy<FiltersViewModel>().LifeStyle.Is(LifestyleType.Singleton));
            this.windsorContainer.Register(Component.For<IVideoViewModel>().ImplementedBy<VideoViewModel>().LifeStyle.Is(LifestyleType.Singleton));
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
            return string.IsNullOrWhiteSpace(key) ? this.windsorContainer.Resolve(service) : this.windsorContainer.Resolve(key, new { });
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
            return this.windsorContainer.ResolveAll(service).Cast<object>();
        }

        /// <summary>
        /// Build Up
        /// </summary>
        /// <param name="instance">
        /// The instance.
        /// </param>
        protected override void BuildUp(object instance)
        {
            instance.GetType().GetProperties()
                .Where(property => property.CanWrite && property.PropertyType.IsPublic)
                .Where(property => this.windsorContainer.Kernel.HasComponent(property.PropertyType))
                .ForEach(property => property.SetValue(instance, this.windsorContainer.Resolve(property.PropertyType), null));
        }
    }
}
