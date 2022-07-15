// --------------------------------------------------------------------------------------------------------------------
// <copyright file="WindowHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Generic Window Helper to allow spawning or re-opening windows.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;
    using System.Linq;
    using System.Windows;

    using HandBrakeWPF.Services.Interfaces;

    public class WindowHelper
    {
        public static void SpawnWindow<T>(IWindowManager windowManager, Type viewType)
        {
            Window window = Application.Current.Windows.Cast<Window>().FirstOrDefault(x => x.GetType() == viewType);

            if (window != null)
            {
                if (window.WindowState == WindowState.Minimized)
                {
                    window.WindowState = WindowState.Normal;
                }
                window.Activate();
            }
            else
            {
                T logvm = IoCHelper.Get<T>();
                windowManager.ShowWindowAsync(logvm);
            }
        }
    }
}
