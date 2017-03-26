// --------------------------------------------------------------------------------------------------------------------
// <copyright file="SplitMenuButton.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Split Menu Button Control
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Controls.SplitButton
{
    using System;
    using System.Collections.ObjectModel;
    using System.Windows;
    using System.Windows.Controls;
    using System.Windows.Input;

    /// <summary>
    ///  A Split Menu Button Control
    /// </summary>
    [TemplatePart(Name = SplitElementName, Type = typeof(UIElement))]
    public class SplitMenuButton : Button
    {
        #region Fields and Constants

        /// <summary>
        /// The split element name.
        /// </summary>
        private const string SplitElementName = "SplitElement";

        /// <summary>
        /// The item source for the context menu
        /// </summary>
        private readonly ObservableCollection<object> itemSource = new ObservableCollection<object>();

        /// <summary>
        /// The is mouse over split element.
        /// </summary>
        private bool isMouseOverSplitElement;

        /// <summary>
        /// The context menu.
        /// </summary>
        private ContextMenu contextMenu;

        /// <summary>
        /// The context menu initial offset.
        /// </summary>
        private Point contextMenuInitialOffset;

        /// <summary>
        /// The logical child.
        /// </summary>
        private DependencyObject logicalChild;

        /// <summary>
        /// The split element.
        /// </summary>
        private UIElement splitElement;

        #endregion

        #region Constructors and Destructors

        /// <summary>
        ///     Initializes a new instance of the <see cref="SplitMenuButton" /> class.
        /// </summary>
        public SplitMenuButton()
        {
            this.DefaultStyleKey = typeof(SplitMenuButton);
        }

        #endregion

        #region Public Properties

        /// <summary>
        ///  Gets the ItemSource for the Context Menu
        /// </summary>
        public Collection<object> ItemSource
        {
            get
            {
                return this.itemSource;
            }
        }

        #endregion

        #region Public Methods and Operators

        /// <summary>
        ///     Called when the template is changed.
        /// </summary>
        public override void OnApplyTemplate()
        {
            // Unhook existing handlers
            if (this.splitElement != null)
            {
                this.splitElement.MouseEnter -= this.SplitElement_MouseEnter;
                this.splitElement.MouseLeave -= this.SplitElement_MouseLeave;
                this.splitElement = null;
            }
            if (this.contextMenu != null)
            {
                this.contextMenu.Opened -= this.ContextMenu_Opened;
                this.contextMenu.Closed -= this.ContextMenu_Closed;
                this.contextMenu = null;
            }

            if (this.logicalChild != null)
            {
                this.RemoveLogicalChild(this.logicalChild);
                this.logicalChild = null;
            }

            // Apply new template
            base.OnApplyTemplate();

            // Hook new event handlers
            this.splitElement = this.GetTemplateChild(SplitElementName) as UIElement;
            if (this.splitElement != null)
            {
                this.splitElement.MouseEnter += this.SplitElement_MouseEnter;
                this.splitElement.MouseLeave += this.SplitElement_MouseLeave;

                this.contextMenu = ContextMenuService.GetContextMenu(this.splitElement);
                if (this.contextMenu != null)
                {
                    // Add the ContextMenu as a logical child (for DataContext and RoutedCommands)
                    this.contextMenu.Visibility = Visibility.Collapsed;
                    this.contextMenu.IsOpen = true;
                    DependencyObject current = this.contextMenu;
                    do
                    {
                        this.logicalChild = current;
                        current = LogicalTreeHelper.GetParent(current);
                    }
                    while (current != null);

                    this.contextMenu.IsOpen = false;     
                    this.AddLogicalChild(this.logicalChild);

                    this.contextMenu.Opened += this.ContextMenu_Opened;
                    this.contextMenu.Closed += this.ContextMenu_Closed;              
                }
            }
        }

        #endregion

        #region Methods

        /// <summary>
        ///     Called when the Button is clicked.
        /// </summary>
        protected override void OnClick()
        {
            if (this.isMouseOverSplitElement)
            {
                this.OpenButtonMenu();
            }
            else
            {
                base.OnClick();
            }
        }

        /// <summary>
        /// Called when a key is pressed.
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (e == null)
            {
                throw new ArgumentNullException("e");
            }

            if (e.Key == Key.Down || e.Key == Key.Up)
            {
                this.Dispatcher.BeginInvoke((Action)this.OpenButtonMenu);
            }
            else
            {
                base.OnKeyDown(e);
            }
        }

        /// <summary>
        /// The open button menu.
        /// </summary>
        protected void OpenButtonMenu()
        {
            if ((this.ItemSource.Count > 0) && (this.contextMenu != null))
            {
                this.contextMenu.HorizontalOffset = 0;
                this.contextMenu.VerticalOffset = 0;
                this.contextMenu.Visibility = Visibility.Visible;
                this.contextMenu.IsOpen = true;
            }
        }

        /// <summary>
        /// The context menu closed.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The RoutedEventArgs.
        /// </param>
        private void ContextMenu_Closed(object sender, RoutedEventArgs e)
        {
            this.LayoutUpdated -= this.SplitButton_LayoutUpdated;
            this.Focus();
        }

        /// <summary>
        /// The context menu opened.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The RoutedEventArgs.
        /// </param>
        private void ContextMenu_Opened(object sender, RoutedEventArgs e)
        {
            this.contextMenuInitialOffset = this.TranslatePoint(new Point(0, this.ActualHeight), this.contextMenu);
            this.UpdateContextMenuOffsets();
            this.LayoutUpdated += this.SplitButton_LayoutUpdated;
        }

        /// <summary>
        /// The split button layout updated.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void SplitButton_LayoutUpdated(object sender, EventArgs e)
        {
            this.UpdateContextMenuOffsets();
        }

        /// <summary>
        /// The split element_ mouse enter.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The MouseEventArgs.
        /// </param>
        private void SplitElement_MouseEnter(object sender, MouseEventArgs e)
        {
            this.isMouseOverSplitElement = true;
        }

        /// <summary>
        /// The split element mouse leave.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The MouseEventArgs
        /// </param>
        private void SplitElement_MouseLeave(object sender, MouseEventArgs e)
        {
            this.isMouseOverSplitElement = false;
        }

        /// <summary>
        /// The update context menu offsets.
        /// </summary>
        private void UpdateContextMenuOffsets()
        {
            var currentOffset = new Point();
            Point desiredOffset = this.contextMenuInitialOffset;

            this.contextMenu.HorizontalOffset = desiredOffset.X - currentOffset.X;
            this.contextMenu.VerticalOffset = desiredOffset.Y - currentOffset.Y;
            if (this.FlowDirection == FlowDirection.RightToLeft)
            {
                this.contextMenu.HorizontalOffset *= -1;
            }
        }

        #endregion
    }
}