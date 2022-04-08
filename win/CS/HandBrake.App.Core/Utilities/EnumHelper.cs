// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EnumHelper.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Enum Helpers
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.App.Core.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Linq;
    using System.Reflection;

    using HandBrake.Interop.Attributes;

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
            if (value == null)
            {
                return string.Empty;
            }

            FieldInfo fieldInfo = value.GetType().GetField(value.ToString());
            if (fieldInfo != null)
            {
                DescriptionAttribute[] attributes =
                  (DescriptionAttribute[])fieldInfo.GetCustomAttributes(
                  typeof(DescriptionAttribute), false);
                return (attributes.Length > 0) ? attributes[0].Description : value.ToString();
            }

            return string.Empty;
        }

        /// <summary>
        /// Get the Display Value of the Enum Model
        /// </summary>
        /// <param name="value">An Enum with Display Attributes</param>
        /// <returns>A string name</returns>
        public static string GetDisplay(T value)
        {
            if (value == null)
            {
                return string.Empty;
            }

            FieldInfo fieldInfo = value.GetType().GetField(value.ToString());
            if (fieldInfo != null)
            {
                DisplayName[] attributes = (DisplayName[])fieldInfo.GetCustomAttributes(typeof(DisplayName), false);

                return (attributes.Length > 0) ? attributes[0].Name : value.ToString();
            }

            return string.Empty;
        }

        /// <summary>
        /// Get the Enumeration for a given Enum Description
        /// </summary>
        /// <param name="description">The String description</param>
        /// <returns>The Enum Value</returns>
        public static T GetValue(string description)
        {
            return GetValue(description, false);
        }

        /// <summary>
        /// Get the Enumeration for a given Enum Description
        /// </summary>
        /// <param name="description">The String description</param>
        /// <param name="insensitiveCase">Turn of sensitivity to cases.</param>
        /// <returns>The Enum Value</returns>
        public static T GetValue(string description, bool insensitiveCase)
        {
            foreach (T val in Enum.GetValues(typeof(T)))
            {
                string currDescription = GetDescription(val);
                string currDisplay = GetDisplay(val);
                string shortName = GetShortName(val);
                if (currDescription == description || currDisplay == description || shortName == description)
                {
                    return val;
                }

                if (insensitiveCase && (currDescription.ToLower() == description.ToLower() || currDisplay.ToLower() == description.ToLower() || shortName.ToLower() == description.ToLower()))
                {
                    return val;
                }
            }

            Debug.WriteLine("EnumHelper.GetValue: The Description for the enum was not recognized: " + description);

            return default(T);
        }

        /// <summary>
        /// The get short name.
        /// </summary>
        /// <param name="value">
        /// The value.
        /// </param>
        /// <returns>
        /// The <see cref="string"/>.
        /// </returns>
        public static string GetShortName(T value)
        {
            if (value == null)
            {
                return string.Empty;
            }

            FieldInfo fieldInfo = value.GetType().GetField(value.ToString());
            if (fieldInfo != null)
            {
                ShortName[] attributes = (ShortName[])fieldInfo.GetCustomAttributes(typeof(ShortName), false);
                return (attributes.Length > 0) ? attributes[0].Name : value.ToString();
            }

            return string.Empty;
        }

        public static TH GetAttribute<TH, TK>(TK value) where TH : Attribute
        {
            if (value == null)
            {
                return null;
            }

            FieldInfo fieldInfo = value.GetType().GetField(value.ToString());
            if (fieldInfo != null)
            {
                TH[] attributes = (TH[])fieldInfo.GetCustomAttributes(typeof(TH), false);
                return (attributes.Length > 0) ? attributes[0] : null;
            }

            return null;
        }

        /// <summary>
        /// Return a list of all the enum values.
        /// </summary>
        /// <returns>
        /// An Enum Object List
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
        /// <returns>
        /// A collection of strings that represent all the enum values
        /// </returns>
        public static IEnumerable<string> GetEnumDisplayValues(Type enumType)
        {
            var strings = new Collection<string>();
            foreach (T e in Enum.GetValues(enumType))
            {
                strings.Add(GetDisplay(e));
            }

            return strings;
        }

        /// <summary>
        /// Get a list of string names for each enum value passed in.
        /// </summary>
        /// <param name="items">
        /// The items.
        /// </param>
        /// <returns>
        /// A collection of strings that represent all the enum values
        /// </returns>
        public static IEnumerable<string> GetEnumDisplayValuesSubset(IEnumerable<T> items)
        {
            return items.Select(GetDisplay).ToList();
        }
    }
}
