// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ServicesWindsorInstaller.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Windsor Installer Interface Implementation
// </summary>
// --------------------------------------------------------------------------------------------------------------------

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
            container.Register(Component.For<IQueueProcessor>().ImplementedBy<QueueProcessor>());
        }

        #endregion
    }
}
