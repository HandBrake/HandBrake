// --------------------------------------------------------------------------------------------------------------------
// <copyright file="DisplayName.cs" company="HandBrake Project (https://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Display Name for an enum value
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Attributes
{
    using System;
    using System.Reflection;

    public class DisplayName : Attribute
    {
        public DisplayName(string displayName)
        {
            this.Name = displayName;
        }

        public DisplayName(Type resourceManagerProvider, string resourceKey)
        {
            this.Name = LookupResource(resourceManagerProvider, resourceKey);
        }

        public string Name { get; }
        
        internal static string LookupResource(Type resourceManagerProvider, string resourceKey)
        {
            foreach (PropertyInfo staticProperty in resourceManagerProvider.GetProperties(BindingFlags.Static | BindingFlags.Public))
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
