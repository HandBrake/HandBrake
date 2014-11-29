// --------------------------------------------------------------------------------------------------------------------
// <copyright file="PList.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   A Helper class to parse plist files.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrakeWPF.Utilities
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Xml.Linq;

    /// <summary>
    /// A Helper class to parse plist files.
    /// </summary>
    public class PList : Dictionary<string, dynamic>
    {
        #region Constructors and Destructors

        /// <summary>
        /// Initializes a new instance of the <see cref="PList"/> class.
        /// </summary>
        public PList()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="PList"/> class.
        /// </summary>
        /// <param name="file">
        /// The file.
        /// </param>
        public PList(string file)
        {
            this.Load(file);
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Load a plist file.
        /// </summary>
        /// <param name="file">
        /// The file name / path
        /// </param>
        /// <returns>
        /// True if successful, false otherwise.
        /// </returns>
        public bool Load(string file)
        {
            this.Clear();

            XDocument doc = XDocument.Load(file);
            XElement plist = doc.Element("plist");
            if (plist != null)
            {
                XElement array = plist.Element("array");
                if (array != null)
                {
                    XElement dict = array.Element("dict");

                    if (dict != null)
                    {
                        IEnumerable<XElement> dictElements = dict.Elements();
                        this.Parse(this, dictElements);
                        return true;
                    }
                }
            }

            return false;
        }

        #endregion

        #region Methods

        /// <summary>
        /// Parse a list of elements
        /// </summary>
        /// <param name="dict">
        /// The dict.
        /// </param>
        /// <param name="elements">
        /// The elements.
        /// </param>
        private void Parse(PList dict, IEnumerable<XElement> elements)
        {
            for (int i = 0; i < elements.Count(); i += 2)
            {
                XElement key = elements.ElementAt(i);
                XElement val = elements.ElementAt(i + 1);

                dict[key.Value] = this.ParseValue(val);
            }
        }

        /// <summary>
        /// The parse array.
        /// </summary>
        /// <param name="elements">
        /// The elements.
        /// </param>
        /// <returns>
        /// The <see cref="List"/>.
        /// </returns>
        private List<dynamic> ParseArray(IEnumerable<XElement> elements)
        {
            return elements.Select(e => this.ParseValue(e)).ToList();
        }

        /// <summary>
        /// The parse value.
        /// </summary>
        /// <param name="val">
        /// The XElement.
        /// </param>
        /// <returns>
        /// The parsed value object.
        /// </returns>
        private dynamic ParseValue(XElement val)
        {
            switch (val.Name.ToString())
            {
                case "string":
                    return val.Value;
                case "integer":
                    return int.Parse(val.Value);
                case "real":
                    return float.Parse(val.Value);
                case "true":
                    return true;
                case "false":
                    return false;
                case "dict":
                    var plist = new PList();
                    this.Parse(plist, val.Elements());
                    return plist;
                case "array":
                    List<dynamic> list = this.ParseArray(val.Elements());
                    return list;
                default:
                    throw new ArgumentException("This plist file is not supported.");
            }
        }

        #endregion
    }
}