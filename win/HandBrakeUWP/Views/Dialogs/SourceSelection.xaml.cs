// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceSelection.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Views.Dialogs
{
    using HandBrake.ViewModels;
    using System;
    using Windows.UI.Xaml;
    using Windows.UI.Xaml.Controls;

    public sealed partial class SourceSelection : ContentDialog
    {
        // Using a DependencyProperty as the backing store for Show.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty ShowProperty =
            DependencyProperty.Register(nameof(Show), typeof(bool), typeof(SourceSelection), new PropertyMetadata(false, ShowStateChanged));

        public MainViewModel Viewmodel => this.DataContext as MainViewModel;

        public SourceSelection()
        {
            this.InitializeComponent();
            this.Opened += SourceSelection_Opened;
        }

        private void SourceSelection_Opened(ContentDialog sender, ContentDialogOpenedEventArgs args)
        {
            this.FolderScan.Focus(FocusState.Keyboard);
        }

        public bool Show
        {
            get { return (bool)GetValue(ShowProperty); }
            set { SetValue(ShowProperty, value); }
        }

        public static async void ShowStateChanged(DependencyObject obj, DependencyPropertyChangedEventArgs args)
        {
            if (obj is SourceSelection select)
            {
                if (select.Show)
                {
                    await select.ShowAsync();
                }
                else
                {
                    select.Hide();
                }
            }
        }
    }
}