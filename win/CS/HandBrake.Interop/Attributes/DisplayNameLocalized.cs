// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DisplayNameLocalized.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Localised version of the DisplayName attribute.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Attributes
{
    using System;
    using System.ComponentModel;
    using System.Reflection;

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Method | AttributeTargets.Property | AttributeTargets.Event | AttributeTargets.All)]
    public class DisplayNameLocalized : DisplayNameAttribute
    {
        public DisplayNameLocalized(Type resourceManagerProvider, string resourceKey)
            : base(LookupResource(resourceManagerProvider, resourceKey))
        {
        }

        internal static string LookupResource(Type resourceManagerProvider, string resourceKey)
        {
            foreach (PropertyInfo staticProperty in resourceManagerProvider.GetProperties(BindingFlags.Static | BindingFlags.NonPublic))
            {
                if (staticProperty.PropertyType == typeof(System.Resources.ResourceManager))
                {
                    System.Resources.ResourceManager resourceManager = (System.Resources.ResourceManager)staticProperty.GetValue(null, null);
                    return resourceManager.GetString(resourceKey);
                }
            }

            return resourceKey; 
        }
    }
}
