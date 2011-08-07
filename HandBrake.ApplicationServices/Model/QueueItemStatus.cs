/*  QueueItemStatus.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Model
{
    using System.ComponentModel;

    using HandBrake.ApplicationServices.Converters;

    /// <summary>
    /// Queue Item Status
    /// </summary>
    [TypeConverter(typeof(EnumToDescConveter))]
    public enum QueueItemStatus
    {
        [Description("Waiting")]
        Waiting = 0,

        [Description("In Progress")]
        InProgress,

        [Description("Completed")]
        Completed,

        [Description("Error")]
        Error,
    }
}
