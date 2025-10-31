// --------------------------------------------------------------------------------------------------------------------
// <copyright file="IoCHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A wrapper around IoC to decouple us from 3rd party libraries
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;

    using Microsoft.Extensions.DependencyInjection;

    public class IoCHelper
    {
        private static IServiceProvider serviceProvider;

        public static T Get<T>()
        {
            return serviceProvider.GetService<T>();
        }

        public static void Setup(IServiceProvider provider)
        {
            serviceProvider = provider;
        }
    }
}