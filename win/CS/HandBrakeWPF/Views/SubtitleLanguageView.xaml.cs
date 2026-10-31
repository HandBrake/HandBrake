namespace HandBrakeWPF.Views
{
    using System;
    using System.Windows;

    using HandBrakeWPF.Helpers;

    public partial class SubtitleLanguageView : Window
    {
        public SubtitleLanguageView()
        {
            this.InitializeComponent();
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            WindowHelper.SetDarkMode(this);
        }
    }
}
