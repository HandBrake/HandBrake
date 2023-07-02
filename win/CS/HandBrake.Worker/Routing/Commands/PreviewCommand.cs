// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PreviewCommand.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EncodeCommand type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Worker.Routing.Commands
{
    using HandBrake.Interop.Interop.Json.Encode;

    public class PreviewCommand
    {
        public JsonEncodeObject EncodeSettings { get; set; }

        public int PreviewNumber { get; set; }
    }
}