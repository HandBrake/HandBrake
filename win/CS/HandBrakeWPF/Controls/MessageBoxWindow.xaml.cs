using System.Windows;

namespace HandBrakeWPF.Controls
{
    using System;
    using System.Drawing;
    using System.Windows.Controls;
    using System.Windows.Interop;
    using System.Windows.Media.Imaging;

    using HandBrakeWPF.Helpers;

    public partial class MessageBoxWindow : Window
    {
        public MessageBoxWindow()
        {
            InitializeComponent();
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }

        public MessageBoxResult MessageBoxResult { get; private set; }

        public void Setup(string header, string messageBoxText, MessageBoxButton buttons, MessageBoxImage icon)
        {
            this.Title = header;
            this.message.Text = messageBoxText;

            switch (icon)
            {
                case MessageBoxImage.Error:
                    this.image.Source = new BitmapImage(new Uri("pack://application:,,,/Views/Images/error.png"));
                    break;
                case MessageBoxImage.Warning:
                    this.image.Source = Imaging.CreateBitmapSourceFromHIcon(SystemIcons.Warning.Handle, Int32Rect.Empty, BitmapSizeOptions.FromEmptyOptions());
                    break;
                case MessageBoxImage.Question:
                    this.image.Source = new BitmapImage(new Uri("pack://application:,,,/Views/Images/question.png"));
                    break;
                case MessageBoxImage.Information:
                    this.image.Source = new BitmapImage(new Uri("pack://application:,,,/Views/Images/information.png"));
                    break;
            }

            switch (buttons)
            {
                case MessageBoxButton.OK:
                    this.ok.Visibility = Visibility.Visible;
                    this.yes.Visibility = Visibility.Collapsed;
                    this.no.Visibility = Visibility.Collapsed;
                    this.Cancel.Visibility = Visibility.Collapsed;
                    break;
                case MessageBoxButton.YesNo:
                    this.ok.Visibility = Visibility.Collapsed;
                    this.yes.Visibility = Visibility.Visible;
                    this.no.Visibility = Visibility.Visible;
                    this.Cancel.Visibility = Visibility.Collapsed;
                    break;
                case MessageBoxButton.YesNoCancel:
                    this.ok.Visibility = Visibility.Collapsed;
                    this.yes.Visibility = Visibility.Visible;
                    this.no.Visibility = Visibility.Visible;
                    this.Cancel.Visibility = Visibility.Visible;
                    break;
                case MessageBoxButton.OKCancel:
                    this.ok.Visibility = Visibility.Visible;
                    this.yes.Visibility = Visibility.Collapsed;
                    this.no.Visibility = Visibility.Collapsed;
                    this.Cancel.Visibility = Visibility.Visible;
                    break;
            }
        }

        private void Proceed_OnClick(object sender, RoutedEventArgs e)
        {
            Button button = e.Source as Button;
            if (button != null)
            {
                switch (button.Name)
                {
                    case "ok":
                        MessageBoxResult = MessageBoxResult.OK;
                        break;
                    case "yes":
                        MessageBoxResult = MessageBoxResult.Yes;
                        break;
                    case "no":
                        MessageBoxResult = MessageBoxResult.No;
                        break;
                }
            }

            this.Close();
        }

        private void Cancel_OnClick(object sender, RoutedEventArgs e)
        {
            MessageBoxResult = MessageBoxResult.Cancel;
            this.Close();
        }
    }
}
