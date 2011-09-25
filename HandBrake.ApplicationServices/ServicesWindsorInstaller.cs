namespace HandBrake.ApplicationServices
{
    using Castle.MicroKernel.Registration;
    using Castle.MicroKernel.SubSystems.Configuration;
    using Castle.Windsor;

    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// An Implimentation of IWindsorInstaller for this library.
    /// </summary>
    public class ServicesWindsorInstaller : IWindsorInstaller
    {
        #region Implementation of IWindsorInstaller

        /// <summary>
        /// Performs the installation in the <see cref="T:Castle.Windsor.IWindsorContainer"/>.
        /// </summary>
        /// <param name="container">The container.</param><param name="store">The configuration store.</param>
        public void Install(IWindsorContainer container, IConfigurationStore store)
        {
            container.Register(Component.For<IPresetService>().ImplementedBy<PresetService>());
            container.Register(Component.For<IQueueManager>().ImplementedBy<QueueManager>());
            container.Register(Component.For<IQueueProcessor>().ImplementedBy<QueueProcessor>());
            container.Register(Component.For<IUserSettingService>().ImplementedBy<UserSettingService>());
            container.Register(Component.For<IScan>().ImplementedBy<ScanService>());
            container.Register(Component.For<IEncode>().ImplementedBy<Encode>());
        }

        #endregion
    }
}
