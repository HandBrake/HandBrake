// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SourceItem.xaml.cs" company="HandBrake Project (http://handbrake.fr)">
// This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Controls
{
    using Windows.UI.Xaml;
    using Windows.UI.Xaml.Controls;

    public sealed partial class SourceItem : Button
    {
        // Using a DependencyProperty as the backing store for Symbol.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty SymbolProperty =
            DependencyProperty.Register(nameof(Symbol), typeof(Symbol), typeof(SourceItem), new PropertyMetadata(Symbol.MapDrive));

        // Using a DependencyProperty as the backing store for SourceDescription.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty SourceDescriptionProperty =
            DependencyProperty.Register(nameof(SourceDescription), typeof(string), typeof(SourceItem), new PropertyMetadata(string.Empty));

        // Using a DependencyProperty as the backing store for SourceName.  This enables animation, styling, binding, etc...
        public static readonly DependencyProperty SourceNameProperty =
            DependencyProperty.Register(nameof(SourceName), typeof(string), typeof(SourceItem), new PropertyMetadata("SOURCE"));

        public SourceItem()
        {
            this.InitializeComponent();
        }

        public string SourceName
        {
            get { return (string)GetValue(SourceNameProperty); }
            set { SetValue(SourceNameProperty, value); }
        }

        public string SourceDescription
        {
            get { return (string)GetValue(SourceDescriptionProperty); }
            set { SetValue(SourceDescriptionProperty, value); }
        }

        public Symbol Symbol
        {
            get { return (Symbol)GetValue(SymbolProperty); }
            set { SetValue(SymbolProperty, value); }
        }
    }
}