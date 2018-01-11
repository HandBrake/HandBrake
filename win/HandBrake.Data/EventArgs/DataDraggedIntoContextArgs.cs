namespace HandBrake.EventArgs
{
    using System;

    /// <summary>
    /// Arguments for Data being dragged into the UI Context.
    /// </summary>
    public abstract class DataDraggedIntoContextArgs : EventArgs
    {
        public abstract bool DataPresent(string DataType);

        public abstract object GetData(string Format, bool AutoConvert);
    }
}