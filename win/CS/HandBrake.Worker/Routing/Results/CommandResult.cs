// --------------------------------------------------------------------------------------------------------------------
// <copyright file="CommandResult.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the CommandResult type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Routing.Results
{
    public class CommandResult
    {
        public bool WasSuccessful { get; set; }
        public string Error { get; set; }
    }
}
