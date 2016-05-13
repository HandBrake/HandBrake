// --------------------------------------------------------------------------------------------------------------------
// <copyright company="HandBrake Project (http://handbrake.fr)" file="PropertyChangedBase.cs">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A base class that implements the infrastructure for property change notification and automatically performs UI thread marshalling.
//   Borrowed from Caliburn Micro
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System;
    using System.ComponentModel;
    using System.Linq.Expressions;
    using System.Runtime.Serialization;

    using INotifyPropertyChangedEx = HandBrakeWPF.Utilities.Interfaces.INotifyPropertyChangedEx;

    /// <summary>
    /// A base class that implements the infrastructure for property change notification and automatically performs UI thread marshalling.
    /// </summary>
    [Serializable]
    public class PropertyChangedBase : INotifyPropertyChangedEx, INotifyPropertyChanged
    {
        [NonSerialized]
        private bool isNotifying;

        /// <summary>
        /// Gets or sets a value indicating whether the Enables/Disables property change notification.
        /// </summary>
        [Browsable(false)]
        public bool IsNotifying
        {
            get
            {
                return this.isNotifying;
            }
            set
            {
                this.isNotifying = value;
            }
        }

        /// <summary>
        /// Occurs when a property value changes.
        /// </summary>
        public event PropertyChangedEventHandler PropertyChanged = (param0, param1) => { };

        /// <summary>
        /// Initializes a new instance of the <see cref="PropertyChangedBase"/> class. 
        /// Creates an instance of <see cref="T:HandBrakeWPF.Utilities.PropertyChangedBase"/>.
        /// </summary>
        public PropertyChangedBase()
        {
            this.IsNotifying = true;
        }

        /// <summary>
        /// Raises a change notification indicating that all bindings should be refreshed.
        /// </summary>
        public void Refresh()
        {
            this.NotifyOfPropertyChange(string.Empty);
        }

        /// <summary>
        /// Notifies subscribers of the property change.
        /// </summary>
        /// <param name="propertyName">Name of the property.</param>
        public virtual void NotifyOfPropertyChange(string propertyName)
        {
            if (!this.IsNotifying)
                return;
            Execute.OnUIThread((System.Action)(() => this.OnPropertyChanged(new PropertyChangedEventArgs(propertyName))));
        }

        /// <summary>
        /// Notifies subscribers of the property change.
        /// </summary>
        /// <typeparam name="TProperty">The type of the property.</typeparam><param name="property">The property expression.</param>
        public void NotifyOfPropertyChange<TProperty>(Expression<Func<TProperty>> property)
        {
            this.NotifyOfPropertyChange(ExtensionMethods.GetMemberInfo((Expression)property).Name);
        }

        /// <summary>
        /// Raises the <see cref="E:PropertyChanged"/> event directly.
        /// </summary>
        /// <param name="e">The <see cref="T:System.ComponentModel.PropertyChangedEventArgs"/> instance containing the event data.</param>
        [EditorBrowsable(EditorBrowsableState.Never)]
        protected void OnPropertyChanged(PropertyChangedEventArgs e)
        {
            PropertyChangedEventHandler changedEventHandler = this.PropertyChanged;
            if (changedEventHandler == null)
                return;
            changedEventHandler((object)this, e);
        }

        /// <summary>
        /// Called when the object is deserialized.
        /// </summary>
        /// <param name="c">The streaming context.</param>
        [OnDeserialized]
        public void OnDeserialized(StreamingContext c)
        {
            this.IsNotifying = true;
        }

        /// <summary>
        /// Used to indicate whether or not the IsNotifying property is serialized to Xml.
        /// </summary>
        /// <returns>
        /// Whether or not to serialize the IsNotifying property. The default is false.
        /// </returns>
        public virtual bool ShouldSerializeIsNotifying()
        {
            return false;
        }
    }
}
