// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InteropWindsorInstaller.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   An Implimentation of IWindsorInstaller for this library.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop
{
    using Castle.Core;
    using Castle.MicroKernel.Registration;
    using Castle.MicroKernel.SubSystems.Configuration;
    using Castle.Windsor;

    using HandBrake.Interop.Interfaces;

    /// <summary>
    /// An Implimentation of IWindsorInstaller for this library.
    /// </summary>
    public class InteropWindsorInstaller : IWindsorInstaller
    {
        /// <summary>
        /// An Implimentation of IWindsorInstaller for this library.
        /// </summary>
        /// <param name="container">
        /// The container.
        /// </param>
        /// <param name="store">
        /// The store.
        /// </param>
        public void Install(IWindsorContainer container, IConfigurationStore store)
        {
            container.Register(Component.For<IHandBrakeInstance>().ImplementedBy<HandBrakeInstance>().LifeStyle.Is(LifestyleType.Singleton));
        }
    }
}
