/*  EnumHelper.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Functions
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.ComponentModel.DataAnnotations;
    using System.Linq;
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
        /// Get the Display Value of the Enum Model
        /// </summary>
        /// <param name="value">An Enum with Display Attributes</param>
        /// <returns>A string name</returns>
        public static string GetDisplay(T value)
        {
            FieldInfo fieldInfo = value.GetType().GetField(value.ToString());
            DisplayAttribute[] attributes = (DisplayAttribute[])fieldInfo.GetCustomAttributes(typeof(DisplayAttribute), false);

            return (attributes.Length > 0) ? attributes[0].Name : value.ToString();
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
                string currDisplay = GetDisplay(val);
                if (currDescription == description || currDisplay == description)
                {
                    return val;
                }
            }

            throw new ArgumentOutOfRangeException("The Description for the enum was not recognized.");
        }

        /// <summary>
        /// Return a list of all the enum values.
        /// </summary>
        /// <returns>
        /// An Enum Oject List
        /// </returns>
        public static IEnumerable<T> GetEnumList()
        {
            return Enum.GetValues(typeof(T)).Cast<T>().ToList();
        }

        /// <summary>
        /// Get a list of string names for each enum value.
        /// </summary>
        /// <param name="enumType">
        /// The enum type.
        /// </param>
        /// <typeparam name="T">
        /// The type of the enum
        /// </typeparam>
        /// <returns>
        /// A collection of strings that represent all the enum values
        /// </returns>
        public static IEnumerable<string> GetEnumDisplayValues(Type enumType)
        {
            var strings = new Collection<string>();
            foreach (T e in Enum.GetValues(enumType))
                strings.Add(GetDisplay(e));
            return strings;
        }
    }
}
