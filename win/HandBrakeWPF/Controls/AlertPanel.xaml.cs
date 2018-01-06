// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AlertPanel.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for AlertPanel.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Controls
{
    using System;
    using System.Windows;
    using System.Windows.Controls;

    /// <summary>
    /// Interaction logic for AlertPanel.xaml
    /// </summary>
    public partial class AlertPanel : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AlertPanel"/> class.
        /// </summary>
        public AlertPanel()
        {
            InitializeComponent();
            this.Message = "Message";
        }

        /// <summary>
        /// Dependancy Property for the Message Property
        /// </summary>
        public static readonly DependencyProperty MessageProperty =
            DependencyProperty.Register("Message", typeof(string), typeof(AlertPanel), new UIPropertyMetadata("Loading..."));

        /// <summary>
        /// Dependancy Property for the submessage propery
        /// </summary>
        public static readonly DependencyProperty SubMessageProperty =
            DependencyProperty.Register("SubMessage", typeof(string), typeof(AlertPanel), new FrameworkPropertyMetadata("Please Wait", FrameworkPropertyMetadataOptions.AffectsRender));

        /// <summary>
        /// Dependancy Property for the submessage propery
        /// </summary>
        public static readonly DependencyProperty DefaultActionProperty =
            DependencyProperty.Register("DefaultAction", typeof(Action), typeof(AlertPanel), new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.None, DefaultActionSet));

        /// <summary>
        /// Dependancy Property for the submessage propery
        /// </summary>
        public static readonly DependencyProperty ActionTextProperty =
            DependencyProperty.Register("ActionText", typeof(string), typeof(AlertPanel), new UIPropertyMetadata("Cancel"));

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
        public Action DefaultAction
        {
            get { return (Action)GetValue(DefaultActionProperty); }
            set { SetValue(DefaultActionProperty, value); }
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
        private static void DefaultActionSet(DependencyObject d, DependencyPropertyChangedEventArgs e)
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
            if (this.DefaultAction != null)
            {
                this.DefaultAction();
            }
        }
    }
}
