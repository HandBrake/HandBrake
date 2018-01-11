using HandBrake.ViewModels;
using Windows.UI.Xaml.Controls;

// The User Control item template is documented at https://go.microsoft.com/fwlink/?LinkId=234236

namespace HandBrake.Views
{
    public sealed partial class MainView : UserControl
    {
        public MainViewModel Viewmodel => this.DataContext as MainViewModel;

        public MainView()
        {
            this.InitializeComponent();
        }
    }
}