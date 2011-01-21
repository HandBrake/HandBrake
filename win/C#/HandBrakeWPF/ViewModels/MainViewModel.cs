/*  MainViewModel.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrakeWPF.ViewModels
{
    using Caliburn.Core;

    /// <summary>
    /// HandBrakes Main Window
    /// </summary>
    public class MainViewModel : PropertyChangedBase
    {
        /// <summary>
        /// HandBrakes Main Window Title
        /// </summary>
        private string windowName;

        /// <summary>
        /// Initializes a new instance of the <see cref="MainViewModel"/> class.
        /// </summary>
        public MainViewModel()
        {
            this.WindowTitle = "HandBrake WPF Test Application";
        }

        /// <summary>
        /// Gets or sets TestProperty.
        /// </summary>
        public string WindowTitle
        {
            get
            {
                return this.windowName;
            }

            set
            {
                if (!object.Equals(this.windowName, value))
                {
                    this.windowName = value;
                    this.NotifyOfPropertyChange("TestProperty");
                }
            }
        }
    }
}
