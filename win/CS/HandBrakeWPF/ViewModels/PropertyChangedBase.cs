// --------------------------------------------------------------------------------------------------------------------
// <copyright file="AudioBehaviours.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Decouple us from 3rd party libraries
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.ViewModels
{
    using System;
    using System.ComponentModel;
    using System.Linq.Expressions;
    using System.Reflection;
    using System.Runtime.CompilerServices;

    public class PropertyChangedBase : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;

        public void NotifyOfPropertyChange<T>(Expression<Func<T>> expression)
        {
            if (expression != null)
            {
                MemberInfo memberInfo = ((MemberExpression)expression.Body).Member;
                this.OnPropertyChanged(memberInfo.Name);
            }
        }

        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            this.PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
