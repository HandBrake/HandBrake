/*  WindsorInstaller.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices
{
    using Castle.MicroKernel;
    using Castle.Windsor;

    using HandBrake.ApplicationServices.Services;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// This is the Windsor Installer class.
    /// </summary>
    public class WindsorInstaller : IWindsorInstaller
    {
        /// <summary>
        /// Setup the Services for this Library
        /// </summary>
        /// <param name="container">
        /// The container.
        /// </param>
        /// <param name="store">
        /// The store.
        /// </param>
        public void Install(IWindsorContainer container, IConfigurationStore store)
        {
            container.AddComponent<IScan, ScanService>();
            container.AddComponent<IQueue, Queue>();
            container.AddComponent<IEncode, Encode>();
        }
    }
}
