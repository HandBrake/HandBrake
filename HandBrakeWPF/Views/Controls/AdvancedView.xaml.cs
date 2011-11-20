// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AdvancedView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for AdvancedView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views.Controls
{
    using System.Windows;
    using System.Windows.Controls;

    /// <summary>
    /// Interaction logic for AdvancedView.xaml
    /// </summary>
    public partial class AdvancedView : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AdvancedView"/> class.
        /// </summary>
        public AdvancedView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// The "Query" Dependancy Property
        /// </summary>
        public static readonly DependencyProperty QueryProperty = DependencyProperty.Register("Query", typeof(string), typeof(AdvancedView), new PropertyMetadata(null));

        /// <summary>
        /// Gets or sets State.
        /// </summary>
        public string Query
        {
            get { return (string)this.GetValue(QueryProperty); }
            set { this.SetValue(QueryProperty, value); }
        }
    }
}
