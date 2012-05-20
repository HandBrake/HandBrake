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
        }

        /// <summary>
        /// Dependancy Property for the IsLoading Property
        /// </summary>
        public static readonly DependencyProperty IsLoadingProperty =
          DependencyProperty.Register("IsLoading", typeof(bool), typeof(StatusPanel), new UIPropertyMetadata(false));

        /// <summary>
        /// Dependancy Property for the Message Property
        /// </summary>
        public static readonly DependencyProperty MessageProperty =
            DependencyProperty.Register("Message", typeof(string), typeof(StatusPanel), new UIPropertyMetadata(string.Empty));

        /// <summary>
        /// Dependancy Property for the submessage propery
        /// </summary>
        public static readonly DependencyProperty SubMessageProperty =
            DependencyProperty.Register("SubMessage", typeof(string), typeof(StatusPanel), new UIPropertyMetadata(string.Empty));

        /// <summary>
        /// Gets or sets a value indicating whether IsLoading.
        /// </summary>
        public bool IsLoading
        {
            get { return (bool)GetValue(IsLoadingProperty); }
            set { SetValue(IsLoadingProperty, value); }
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
    }
}
