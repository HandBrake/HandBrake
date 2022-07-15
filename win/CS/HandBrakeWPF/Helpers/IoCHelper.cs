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
    using Caliburn.Micro;

    public class IoCHelper
    {
        public static T Get<T>()
        {
           return IoC.Get<T>();
        }
    }
}
