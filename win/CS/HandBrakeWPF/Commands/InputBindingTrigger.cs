// --------------------------------------------------------------------------------------------------------------------
// <copyright file="InputBindingTrigger.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The input binding trigger.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Commands
{
    using System;
    using System.Diagnostics;
    using System.Windows;
    using System.Windows.Input;

    using Microsoft.Xaml.Behaviors;

    /// <summary>
    /// The input binding trigger.
    /// </summary>
    public class InputBindingTrigger : TriggerBase<FrameworkElement>, ICommand
    {
        public static readonly DependencyProperty InputBindingProperty = DependencyProperty.Register("InputBinding", typeof(InputBinding), typeof(InputBindingTrigger), new UIPropertyMetadata(null));

        /// <summary>
        /// Gets or sets the input binding.
        /// </summary>
        public InputBinding InputBinding
        {
            get { return (InputBinding)GetValue(InputBindingProperty); }
            set { SetValue(InputBindingProperty, value); }
        }

        /// <summary>
        /// The can execute changed.
        /// </summary>
        public event EventHandler CanExecuteChanged = delegate { };

        /// <summary>
        /// The can execute.
        /// </summary>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        /// <returns>
        /// The <see cref="bool"/>.
        /// </returns>
        public bool CanExecute(object parameter)
        {
            return true;
        }

        /// <summary>
        /// The execute.
        /// </summary>
        /// <param name="parameter">
        /// The parameter.
        /// </param>
        public void Execute(object parameter)
        {
            InvokeActions(parameter);
        }

        /// <summary>
        /// The on attached.
        /// </summary>
        protected override void OnAttached()
        {
            if (InputBinding != null)
            {
                InputBinding.Command = this;
                AssociatedObject.Loaded += delegate
                {
                    var window = GetWindow(AssociatedObject);
                    window.InputBindings.Add(InputBinding);
                };
            }
            base.OnAttached();
        }

        /// <summary>
        /// The get window.
        /// </summary>
        /// <param name="frameworkElement">
        /// The framework element.
        /// </param>
        /// <returns>
        /// The <see cref="Window"/>.
        /// </returns>
        private Window GetWindow(FrameworkElement frameworkElement)
        {
            if (frameworkElement is Window)
                return frameworkElement as Window;

            var parent = frameworkElement.Parent as FrameworkElement;
            Debug.Assert(parent != null, "Null Parent");

            return GetWindow(parent);
        }
    }
}
