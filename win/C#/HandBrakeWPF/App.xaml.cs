/*  App.xaml.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrakeWPF
{
    using Caliburn.PresentationFramework.ApplicationModel;

    using HandBrakeWPF.ViewModels;

    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : CaliburnApplication
    {
        /*
         * TODO:
         * - Setup Castle Windsor support for services.
         * 
         * 
         */


        /// <summary>
        /// Initializes a new instance of the <see cref="App"/> class.
        /// </summary>
        public App()
        {
        }

        /// <summary>
        /// Create the Root View
        /// </summary>
        /// <returns>
        /// A MainViewMOdel
        /// </returns>
        protected override object CreateRootModel()
        {
            var binder = (DefaultBinder)Container.GetInstance<IBinder>();

            binder.EnableBindingConventions();
            binder.EnableMessageConventions();

            return new MainViewModel();
        }
    }
}
