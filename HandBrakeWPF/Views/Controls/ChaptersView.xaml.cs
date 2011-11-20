// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ChaptersView.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Interaction logic for ChaptersView.xaml
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Views.Controls
{
    using System;
    using System.Collections.Generic;
    using System.Windows;
    using System.Windows.Controls;

    using HandBrake.ApplicationServices.Parsing;

    /// <summary>
    /// Interaction logic for ChaptersView.xaml
    /// </summary>
    public partial class ChaptersView : UserControl
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ChaptersView"/> class.
        /// </summary>
        public ChaptersView()
        {
            InitializeComponent();
        }

        /// <summary>
        /// The "Query" Dependancy Property
        /// </summary>
        public static readonly DependencyProperty QueryProperty = DependencyProperty.Register("Chapters", typeof(IList<Chapter>), typeof(ChaptersView), new PropertyMetadata(null));

        /// <summary>
        /// Gets or sets State.
        /// </summary>
        public IList<Chapter> Query
        {
            get { return (IList<Chapter>)this.GetValue(QueryProperty); }
            set { this.SetValue(QueryProperty, value); }
        }

        /// <summary>
        /// Import from CSV
        /// </summary>
        public void Import()
        {
            throw new NotImplementedException("Not Implemented Yet");
        }

        /// <summary>
        /// Export to CSV
        /// </summary>
        public void Export()
        {
            throw new NotImplementedException("Not Implemented Yet");
        }
    }
}
