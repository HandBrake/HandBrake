// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DetelecineItem.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A wrapper around IoC to decouple us from 3rd party libraries
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using Autofac;

    public class IoCHelper
    {
        private static IContainer container;
        public static T Get<T>()
        {
            return container.Resolve<T>();
        }

        public static void Setup(IContainer autofacContainer)
        {
            container = autofacContainer;
        }
    }
}
