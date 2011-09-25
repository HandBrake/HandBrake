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

    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

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
            this.windsorContainer.Register(Component.For<IMainViewModel>().ImplementedBy<MainViewModel>().LifeStyle.Is(LifestyleType.Singleton));
        }

        protected override IEnumerable<Assembly> SelectAssemblies()
        {
            return AppDomain.CurrentDomain.GetAssemblies();
        }

        protected override object GetInstance(Type service, string key)
        {
            return string.IsNullOrWhiteSpace(key) ? this.windsorContainer.Resolve(service) : this.windsorContainer.Resolve(key, new { });
        }

        protected override IEnumerable<object> GetAllInstances(Type service)
        {
            return this.windsorContainer.ResolveAll(service).Cast<object>();
        }

        protected override void BuildUp(object instance)
        {
            instance.GetType().GetProperties()
                .Where(property => property.CanWrite && property.PropertyType.IsPublic)
                .Where(property => this.windsorContainer.Kernel.HasComponent(property.PropertyType))
                .ForEach(property => property.SetValue(instance, this.windsorContainer.Resolve(property.PropertyType), null));
        }
    }
}
