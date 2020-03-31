// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EncodeCommand type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Routing.Commands
{
    using HandBrake.Interop.Interop.Json.Encode;

    public class EncodeCommand
    { 
        public InitCommand InitialiseCommand { get; set; }

        public JsonEncodeObject EncodeJob { get; set; }
    }
}
