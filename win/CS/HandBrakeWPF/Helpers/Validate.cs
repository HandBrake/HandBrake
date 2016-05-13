// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Validate.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The validate.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Helpers
{
    using System;

    /// <summary>
    /// The validate.
    /// </summary>
    public class Validate
    {
        /// <summary>
        /// The not null.
        /// </summary>
        /// <param name="item">
        /// The item.
        /// </param>
        /// <param name="message">
        /// The message.
        /// </param>
        /// <exception cref="ArgumentException">
        /// Thrown when the input object is null
        /// </exception>
        public static void NotNull(object item, string message)
        {
            if (item == null)
            {
                throw new ArgumentException(message);
            }
        }
    }
}
