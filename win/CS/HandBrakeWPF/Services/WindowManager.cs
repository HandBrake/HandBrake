// --------------------------------------------------------------------------------------------------------------------
// <copyright file="WindowManager.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Wrapper class to decouple us from 3rd party library
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Services
{
    using System.Collections.Generic;
    using System.Threading.Tasks;

    using HandBrakeWPF.Helpers;
    using HandBrakeWPF.Services.Interfaces;

    public class WindowManager : IWindowManager
    {
        private Caliburn.Micro.IWindowManager windowManager;
        public WindowManager()
        {
            windowManager = IoCHelper.Get<Caliburn.Micro.IWindowManager>();
        }

        public Task<bool?> ShowDialogAsync(object rootModel, object context = null, IDictionary<string, object> settings = null)
        {
            return windowManager.ShowDialogAsync(rootModel, context, settings);
        }

        public Task ShowWindowAsync(object rootModel, object context = null, IDictionary<string, object> settings = null)
        {
            return windowManager.ShowWindowAsync(rootModel, context, settings);
        }

        public Task ShowPopupAsync(object rootModel, object context = null, IDictionary<string, object> settings = null)
        {
            return windowManager.ShowPopupAsync(rootModel, context, settings);
        }
    }
}
