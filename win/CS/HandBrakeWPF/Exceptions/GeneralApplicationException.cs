// --------------------------------------------------------------------------------------------------------------------
// <copyright file="GeneralApplicationException.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   The Encode Failure Exception
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Exceptions
{
    using System;

    /// <summary>
    /// The Encode Failure Exception
    /// </summary>
    public class GeneralApplicationException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GeneralApplicationException"/> class. 
        /// </summary>
        /// <param name="error">
        /// The error.
        /// </param>
        /// <param name="solution">
        /// The solution.
        /// </param>
        /// <param name="innerException">
        /// The inner Exception.
        /// </param>
        public GeneralApplicationException(string error, string solution, Exception innerException)
        {
            this.Error = error;
            this.Solution = solution;
            this.ActualException = innerException;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="GeneralApplicationException"/> class with no wrapped exception.
        /// </summary>
        /// <param name="error">
        /// The error.
        /// </param>
        /// <param name="solution">
        /// The solution.
        /// </param>
        public GeneralApplicationException(string error, string solution) : this(error, solution, null)
        {}

        /// <summary>
        /// Gets or sets FailureReason.
        /// </summary>
        public string Error { get; set; }

        /// <summary>
        /// Gets or sets Solution.
        /// </summary>
        public string Solution { get; set; }

        /// <summary>
        /// Gets or sets InnerException.
        /// </summary>
        public Exception ActualException { get; set; }
    }
}
