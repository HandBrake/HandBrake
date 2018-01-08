// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ViewManager.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Services
{
    using System.Linq;
    using System.Windows;
    using Caliburn.Micro;
    using HandBrake.Services.Interfaces;

    public class ViewManager : ViewManagerBase
    {
        public IWindowManager WindowManager => windowManager ?? (windowManager = IoC.Get<IWindowManager>());

        private IWindowManager windowManager;

        public ViewManager()
        {
        }

        public override bool SupportsWindow => true;

        public override bool? ShowDialog<TViewModel>(TViewModel viewmodel = default(TViewModel))
        {
            if (viewmodel == null)
            {
                viewmodel = IoC.Get<TViewModel>();
            }

            return this.WindowManager.ShowDialog(viewmodel);
        }

        public override void ShowWindow<TViewModel>(TViewModel viewmodel = default(TViewModel))
        {
            if (viewmodel == null)
            {
                viewmodel = IoC.Get<TViewModel>();
            }

            Window window = Application.Current.Windows
                .Cast<Window>()
                .FirstOrDefault(x => x.DataContext?.GetType() == typeof(TViewModel));

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
                this.WindowManager.ShowWindow(viewmodel);
            }
        }
    }
}