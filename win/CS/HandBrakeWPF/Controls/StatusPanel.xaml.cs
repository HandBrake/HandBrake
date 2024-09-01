// --------------------------------------------------------------------------------------------------------------------
// <copyright file="StatusPanel.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for StatusPanel.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Controls
{
    using System;
    using System.Windows;
    using System.Windows.Controls;

    /// <summary>
    /// Interaction logic for StatusPanel.xaml
    /// </summary>
    public partial class StatusPanel : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="StatusPanel"/> class.
        /// </summary>
        public StatusPanel()
        {
            InitializeComponent();
            this.Message = "Message";
        }

        public static readonly DependencyProperty IsLoadingProperty =
          DependencyProperty.Register("IsLoading", typeof(bool), typeof(StatusPanel), new UIPropertyMetadata(false));

        public static readonly DependencyProperty MessageProperty =
            DependencyProperty.Register("Message", typeof(string), typeof(StatusPanel), new UIPropertyMetadata("Loading..."));

        public static readonly DependencyProperty SubMessageProperty =
            DependencyProperty.Register("SubMessage", typeof(string), typeof(StatusPanel), new FrameworkPropertyMetadata("Please Wait", FrameworkPropertyMetadataOptions.AffectsRender));

        public static readonly DependencyProperty ActionProperty =
            DependencyProperty.Register("CancelAction", typeof(Action), typeof(StatusPanel), new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.None, OnCancelActionSet));

        public static readonly DependencyProperty SecondaryActionProperty =
            DependencyProperty.Register("SecondaryAction", typeof(Action), typeof(StatusPanel), new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.None, OnSecondaryActionSet));

        public static readonly DependencyProperty ActionTextProperty =
            DependencyProperty.Register("ActionText", typeof(string), typeof(StatusPanel), new UIPropertyMetadata("Cancel"));

        public static readonly DependencyProperty SecondaryActionTextProperty =
            DependencyProperty.Register("SecondaryActionText", typeof(string), typeof(StatusPanel), new UIPropertyMetadata("Open Log Window"));

        public static readonly DependencyProperty ProgressPercentageProperty =
            DependencyProperty.Register("ProgressPercentage", typeof(double), typeof(StatusPanel), new UIPropertyMetadata(0d, ProgressPercentagePropertyCallback));

        private static void ProgressPercentagePropertyCallback(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            StatusPanel p = d as StatusPanel;
            if (p != null)
            {
                if (e.NewValue is double && (double)e.NewValue > 0)
                {
                    p.progressBar.IsIndeterminate = false;
                    p.IsIndeterminate = false;
                }
            }
        }

        public static readonly DependencyProperty IsIndeterminateProperty =
            DependencyProperty.Register("IsIndeterminate", typeof(bool), typeof(StatusPanel), new UIPropertyMetadata(true));
        

        /// <summary>
        /// Gets or sets a value indicating whether IsLoading.
        /// </summary>
        public bool IsLoading
        {
            get { return (bool)GetValue(IsLoadingProperty); }
            set { SetValue(IsLoadingProperty, value); }
        }

        public double ProgressPercentage
        {
            get { return (double)GetValue(ProgressPercentageProperty); }
            set { SetValue(ProgressPercentageProperty, value); }
        }

        public bool IsIndeterminate
        {
            get { return (bool)GetValue(IsIndeterminateProperty); }
            set { SetValue(IsIndeterminateProperty, value); }
        }

        /// <summary>
        /// Gets or sets Message.
        /// </summary>
        public string Message
        {
            get { return (string)GetValue(MessageProperty); }
            set { SetValue(MessageProperty, value); }
        }

        /// <summary>
        /// Gets or sets SubMessage.
        /// </summary>
        public string SubMessage
        {
            get { return (string)GetValue(SubMessageProperty); }
            set { SetValue(SubMessageProperty, value); }
        }

        /// <summary>
        /// Gets or sets the cancel action.
        /// </summary>
        public Action CancelAction
        {
            get { return (Action)GetValue(ActionProperty); }
            set { SetValue(ActionProperty, value); }
        }

        /// <summary>
        /// Gets or sets the cancel action.
        /// </summary>
        public Action SecondaryAction
        {
            get { return (Action)GetValue(SecondaryActionProperty); }
            set { SetValue(SecondaryActionProperty, value); }
        }

        /// <summary>
        /// Gets or sets SecondaryActionText.
        /// </summary>
        public string SecondaryActionText
        {
            get { return (string)GetValue(SecondaryActionTextProperty); }
            set { SetValue(SecondaryActionTextProperty, value); }
        }

        /// <summary>
        /// The on cancel action set.
        /// </summary>
        /// <param name="d">
        /// The d.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private static void OnCancelActionSet(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
        }

        /// <summary>
        /// The on Secondary Action Set.
        /// </summary>
        /// <param name="d">
        /// The d.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private static void OnSecondaryActionSet(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
        }

        /// <summary>
        /// Gets or sets the action text.
        /// </summary>
        public string ActionText
        {
            get { return (string)GetValue(ActionTextProperty); }
            set { SetValue(ActionTextProperty, value); }
        }

        /// <summary>
        /// Gets a value indicating whether is action button visible.
        /// </summary>
        public bool IsActionButtonVisible
        {
            get
            {
                return true; // this.CancelAction != null;
            }
        }

        /// <summary>
        /// Gets a value indicating whether is action 2 button visible.
        /// </summary>
        public bool IsActionButton2Visible
        {
            get
            {
                return true; // SecondaryAction != null;
            }
        }

        /// <summary>
        /// The status action button_ on click.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void StatusActionButton_OnClick(object sender, RoutedEventArgs e)
        {
            if (this.CancelAction != null)
            {
                this.CancelAction();
            }
        }

        private void PerformSecondaryAction(object sender, RoutedEventArgs e)
        {
            if (this.SecondaryAction != null)
            {
                this.SecondaryAction();
            }
        }
    }
}
