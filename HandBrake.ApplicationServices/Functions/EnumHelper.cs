/*  EnumHelper.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
{
    using System;
    using System.ComponentModel;
    using System.Reflection;

    /// <summary>
    /// Enum Helpers
    /// </summary>
    /// <typeparam name="T">
    /// The Type Parameter
    /// </typeparam>
    public class EnumHelper<T>
    {
        /// <summary>
        /// Get the description of an Enum
        /// </summary>
        /// <param name="value">
        /// The value.
        /// </param>
        /// <returns>
        /// The Description string
        /// </returns>
        public static string GetDescription(T value)
        {
            FieldInfo fieldInfo = value.GetType().GetField(value.ToString());
            DescriptionAttribute[] attributes =
                  (DescriptionAttribute[])fieldInfo.GetCustomAttributes(
                  typeof(DescriptionAttribute), false);
            return (attributes.Length > 0) ? attributes[0].Description : value.ToString();
        }

        /// <summary>
        /// Get the Enumeration for a given Enum Description
        /// </summary>
        /// <param name="description">The String description</param>
        /// <returns>The Enum Value</returns>
        public static T GetValue(string description)
        {
            foreach (T val in Enum.GetValues(typeof(T)))
            {
                string currDescription = GetDescription(val);
                if (currDescription == description)
                {
                    return val;
                }
            }

            throw new ArgumentOutOfRangeException("The Description for the enum was not recognized.");
        }
    }
}
