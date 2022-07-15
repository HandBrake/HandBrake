// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ThreadHelper.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A helper class for managing threads that decouples us from 3rd party libraries.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.Windows;

    public class ThreadHelper
    {
        public static void OnUIThread(Action action)
        {
            Application.Current.Dispatcher.Invoke(
                () =>
                {
                    action();
                });
        }
    }
}
