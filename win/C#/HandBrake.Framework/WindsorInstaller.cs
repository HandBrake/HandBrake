/*  WindsorInstaller.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.Framework
{
    using Castle.MicroKernel;
    using Castle.Windsor;

    using HandBrake.Framework.Services;
    using HandBrake.Framework.Services.Interfaces;

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
            container.AddComponent<IErrorService, ErrorService>();
        }
    }
}
