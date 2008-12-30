using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Text.RegularExpressions;
using System.Diagnostics;

namespace Handbrake.Presets
{
    public class Preset
    {

        private int level = 0;
        private string category = null;
        private string name;
        private string query;
        private Boolean pictureSettings;

        /// <summary>
        /// Get or Set the preset's level. This indicated if it is a root or child node
        /// </summary>
        public int Level
        {
            get { return level; }
            set { this.level = value; }
        }

        /// <summary>
        /// Get or Set the category which the preset resides under
        /// </summary>
        public string Category
        {
            get { return category; }
            set { this.category = value; }
        }

        /// <summary>
        /// Get or Set the preset name
        /// </summary>
        public string Name
        {
            get { return name; }
            set { this.name = value; }
        }

        /// <summary>
        /// Get or set the preset query
        /// </summary>
        public string Query
        {
            get { return query; }
            set { this.query = value; }
        }

        /// <summary>
        /// Get or set the usage of Picture Settings in presets.
        /// </summary>
        public Boolean PictureSettings
        {
            get { return pictureSettings; }
            set { this.pictureSettings = value; }
        }

    }
}