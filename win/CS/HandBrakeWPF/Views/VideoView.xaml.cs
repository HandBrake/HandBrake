// --------------------------------------------------------------------------------------------------------------------
// <copyright file="VideoView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for VideoView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views
{
    using System.Collections.Generic;
    using System.Windows;
    using System.Windows.Controls;

    using Caliburn.Micro;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Utilities;
    using HandBrake.Interop.Model.Encoding;

    using HandBrakeWPF.ViewModels;
    using HandBrakeWPF.ViewModels.Interfaces;

    /// <summary>
    /// Interaction logic for VideoView.xaml
    /// </summary>
    public partial class VideoView : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="VideoView"/> class.
        /// </summary>
        public VideoView()
        {
            InitializeComponent();
        }


        private void qsv_preset_radiobutton(object sender, System.Windows.RoutedEventArgs e)
        {
            qsv_preset_ValueChanged(sender, null);
        }

        private void qsv_preset_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            VideoViewModel mvm = ((VideoViewModel)this.DataContext);
            EncodeTask task = mvm.Task;

            string full_string = "";

            IDictionary<string, string> newOptions = new Dictionary<string, string>();
            string[] existingSegments = full_string.Split(':');
            foreach (string existingSegment in existingSegments)
            {
                string optionName = existingSegment;
                string optionValue = string.Empty;
                int equalsIndex = existingSegment.IndexOf('=');
                if (equalsIndex >= 0)
                {
                    optionName = existingSegment.Substring(
                        0, existingSegment.IndexOf("=", System.StringComparison.Ordinal));
                    optionValue = existingSegment.Substring(equalsIndex);
                }

                if (optionName != string.Empty)
                {
                    if (newOptions.ContainsKey(optionName))
                        newOptions.Remove(optionName);
                    newOptions.Add(optionName, optionValue);
                }
            }

            full_string = "";
            foreach (KeyValuePair<string, string> entry in newOptions)
            {
                full_string += entry.Key;
                if (entry.Value != string.Empty)
                    full_string += entry.Value;
                full_string += ":";
            }
            full_string = full_string.TrimEnd(':');

            task.AdvancedEncoderOptions = full_string;
            task.NotifyOfPropertyChange(() => task.AdvancedEncoderOptions);
        }
    }
}
