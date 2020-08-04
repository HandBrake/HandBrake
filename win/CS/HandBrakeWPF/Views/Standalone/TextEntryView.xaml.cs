// --------------------------------------------------------------------------------------------------------------------
// <copyright file="TextEntryView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views.Standalone
{
    using System.Windows;

    public partial class TextEntryView : Window
    {
        public TextEntryView()
        {
            this.InitializeComponent();
            this.IsCancelled = false;
        }

        public string EntryBoxContent
        {
            get
            {
                return this.entryBox.Text;
            }
        }

        public bool IsCancelled { get; private set; }

        public void Save(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        public void Cancel(object sender, RoutedEventArgs e)
        {
            this.IsCancelled = true;
            this.Close();
        }
    }
}
