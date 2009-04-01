using System;
using System.Windows.Forms;

namespace Handbrake.Controls
{
    public partial class x264Panel : UserControl
    {
        public x264Panel()
        {
            InitializeComponent();
        }

        public string x264Query
        {
            get { return rtf_x264Query.Text; }
            set { rtf_x264Query.Text = value; }
        }
        public void setToolTipActive(Boolean active)
        {
            ToolTip.Active = active;
        }

        #region Controls Changed
        private void drop_refFrames_SelectedIndexChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("ref");
        }
        private void check_mixedReferences_CheckedChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("mixed-refs");
        }
        private void drop_bFrames_SelectedIndexChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("bframes");
        }
        private void drop_directPrediction_SelectedIndexChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("direct");
        }
        private void check_weightedBFrames_CheckedChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("weightb");
        }
        private void check_pyrmidalBFrames_CheckedChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("b-pyramid");
        }
        private void drop_MotionEstimationMethod_SelectedIndexChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("me");
        }
        private void drop_MotionEstimationRange_SelectedIndexChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("merange");
        }
        private void drop_subpixelMotionEstimation_SelectedIndexChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("subq");
        }
        private void drop_analysis_SelectedIndexChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("analyse");
        }
        private void check_8x8DCT_CheckedChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("8x8dct");
        }
        private void drop_deblockAlpha_SelectedIndexChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("deblock");
        }
        private void drop_deblockBeta_SelectedIndexChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("deblock");
        }
        private void drop_trellis_SelectedIndexChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("trellis");
        }
        private void check_noFastPSkip_CheckedChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("no-fast-pskip");
        }
        private void check_noDCTDecimate_CheckedChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("no-dct-decimate");
        }
        private void check_Cabac_CheckedChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("cabac");
        }
        private void slider_psyrd_Scroll(object sender, EventArgs e)
        {
            on_x264_WidgetChange("psy-rd");
        }
        private void slider_psytrellis_Scroll(object sender, EventArgs e)
        {
            on_x264_WidgetChange("psy-rd");
        }
        private void drop_adaptBFrames_SelectedIndexChanged(object sender, EventArgs e)
        {
            on_x264_WidgetChange("b-adapt");
        }
        private void rtf_x264Query_TextChanged(object sender, EventArgs e)
        {
            if (rtf_x264Query.Text.EndsWith("\n"))
            {
                string query = rtf_x264Query.Text.Replace("\n", "");
                reset2Defaults();
                rtf_x264Query.Text = query;
                X264_StandardizeOptString();
                X264_SetCurrentSettingsInPanel();

                if (rtf_x264Query.Text == string.Empty)
                    reset2Defaults();
            }
        }
        private void btn_reset_Click(object sender, EventArgs e)
        {
            rtf_x264Query.Text = "";
            reset2Defaults();
        }
        #endregion

        /// <summary>
        /// Reset all components to defaults and clears the x264 rtf box
        /// </summary>
        public void reset2Defaults()
        {
            check_8x8DCT.CheckState = CheckState.Unchecked;
            check_Cabac.CheckState = CheckState.Checked;
            check_mixedReferences.CheckState = CheckState.Unchecked;
            check_noDCTDecimate.CheckState = CheckState.Unchecked;
            check_noFastPSkip.CheckState = CheckState.Unchecked;
            check_pyrmidalBFrames.CheckState = CheckState.Unchecked;
            check_weightedBFrames.CheckState = CheckState.Unchecked;
            drop_analysis.SelectedIndex = 0;
            drop_bFrames.SelectedIndex = 0;
            drop_deblockAlpha.SelectedIndex = 0;
            drop_deblockBeta.SelectedIndex = 0;
            drop_directPrediction.SelectedIndex = 0;
            drop_MotionEstimationMethod.SelectedIndex = 0;
            drop_MotionEstimationRange.SelectedIndex = 0;
            drop_refFrames.SelectedIndex = 0;
            drop_subpixelMotionEstimation.SelectedIndex = 0;
            drop_trellis.SelectedIndex = 0;
            slider_psyrd.Value = 10;
            slider_psytrellis.Value = 0;
            drop_adaptBFrames.SelectedIndex = 0;

            rtf_x264Query.Text = "";
        }

        /// <summary>
        /// Iterate over every x264 option, standardize it, write the full string to the x264 rtf box
        /// </summary>
        public void X264_StandardizeOptString()
        {
            /* Set widgets depending on the opt string in field */
            String thisOpt; // The separated option such as "bframes=3"
            String optName; // The option name such as "bframes"
            String optValue;// The option value such as "3"
            String changedOptString = "";
            String[] currentOptsArray;

            /*First, we get an opt string to process */
            String currentOptString = rtf_x264Query.Text;

            /*verify there is an opt string to process */
            if (currentOptString.Contains("="))
            {
                /*Put individual options into an array based on the ":" separator for processing, result is "<opt>=<value>"*/
                currentOptsArray = currentOptString.Split(':');

                /*iterate through the array and get <opts> and <values*/
                int loopcounter;
                int currentOptsArrayCount = currentOptsArray.Length;
                for (loopcounter = 0; loopcounter < currentOptsArrayCount; loopcounter++)
                {
                    thisOpt = currentOptsArray[loopcounter];
                    if (currentOptsArray[currentOptsArrayCount - 1] == string.Empty)
                        break;

                    String[] splitOptRange = thisOpt.Split('=');
                    if (thisOpt != "")
                    {
                        if (thisOpt.Contains("="))
                        {
                            optName = splitOptRange[0];
                            optValue = splitOptRange[1];

                            /* Standardize the names here depending on whats in the string */
                            optName = X264_StandardizeOptNames(optName);
                            thisOpt = optName + "=" + optValue;
                        }
                        else // No value given so we use a default of "1"
                        {
                            optName = thisOpt;
                            /* Standardize the names here depending on whats in the string */
                            optName = X264_StandardizeOptNames(optName);
                            thisOpt = optName + "=1";
                        }
                    }

                    /* Construct New String for opts here */
                    if (thisOpt == string.Empty)
                        changedOptString = changedOptString + thisOpt;
                    else
                    {
                        if (changedOptString == string.Empty)
                            changedOptString = thisOpt;
                        else
                            changedOptString = changedOptString + ":" + thisOpt;
                    }
                }
            }

            /* Change the option string to reflect the new standardized option string */
            if (changedOptString != "")
                rtf_x264Query.Text = changedOptString;
        }

        /// <summary>
        /// Take a single option and standardize it. Returns as a String
        /// Input: String. - Single X264 Option. Name only
        /// Output: String - Single X264 Option. Name only. Changed to standard format
        /// </summary>
        /// <param name="cleanOptNameString"></param>
        /// <returns></returns>
        private string X264_StandardizeOptNames(String cleanOptNameString)
        {
            String input = cleanOptNameString;

            if (input.Equals("ref") || input.Equals("frameref"))
                cleanOptNameString = "ref";

            /*No Fast PSkip nofast_pskip*/
            if (input.Equals("no-fast-pskip") || input.Equals("no_fast_pskip") || input.Equals("nofast_pskip"))
                cleanOptNameString = "no-fast-pskip";

            /*No Dict Decimate*/
            if (input.Equals("no-dct-decimate") || input.Equals("no_dct_decimate") || input.Equals("nodct_decimate"))
                cleanOptNameString = "no-dct-decimate";

            /*Subme*/
            if (input.Equals("subme"))
                cleanOptNameString = "subq";

            /*ME Range*/
            if (input.Equals("me-range") || input.Equals("me_range"))
                cleanOptNameString = "merange";

            /*WeightB*/
            if (input.Equals("weight-b") || input.Equals("weight_b"))
                cleanOptNameString = "weightb";

            /*B Pyramid*/
            if (input.Equals("b_pyramid"))
                cleanOptNameString = "b-pyramid";

            /*Direct Prediction*/
            if (input.Equals("direct-pred") || input.Equals("direct_pred"))
                cleanOptNameString = "direct";

            /*Deblocking*/
            if (input.Equals("filter"))
                cleanOptNameString = "deblock";

            /*Analysis*/
            if (input.Equals("partitions"))
                cleanOptNameString = "analyse";

            return cleanOptNameString;
        }

        /// <summary>
        /// Resets the GUI widgets to the contents of the option string.
        /// </summary>
        public void X264_SetCurrentSettingsInPanel()
        {
            /* Set widgets depending on the opt string in field */
            String thisOpt; // The separated option such as "bframes=3"
            String optName; // The option name such as "bframes"
            String optValue;// The option value such as "3"
            String[] currentOptsArray;

            //Set currentOptString to the contents of the text box.
            String currentOptString = rtf_x264Query.Text.Replace("\n", "");

            /*verify there is an opt string to process */
            if (currentOptString.Contains("="))
            {
                /*Put individual options into an array based on the ":" separator for processing, result is "<opt>=<value>"*/
                currentOptsArray = currentOptString.Split(':');

                /*iterate through the array and get <opts> and <values*/
                int loopcounter;
                int currentOptsArrayCount = currentOptsArray.Length;

                /*iterate through the array and get <opts> and <values*/
                for (loopcounter = 0; loopcounter < currentOptsArrayCount; loopcounter++)
                {
                    thisOpt = currentOptsArray[loopcounter];
                    String[] splitOptRange = thisOpt.Split('=');

                    if (thisOpt.Contains("="))
                    {
                        optName = splitOptRange[0];
                        optValue = splitOptRange[1];

                        /*Run through the available widgets for x264 opts and set them, as you add widgets, 
                            they need to be added here. This should be moved to its own method probably*/
                        switch (optName)
                        {
                            case "bframes":
                                drop_bFrames.SelectedItem = optValue;
                                continue;
                            case "ref":
                                drop_refFrames.SelectedItem = optValue;
                                continue;
                            case "no-fast-pskip":
                                check_noFastPSkip.CheckState = CheckState.Checked;
                                continue;
                            case "no-dct-decimate":
                                check_noDCTDecimate.CheckState = CheckState.Checked;
                                continue;
                            case "subq":
                                drop_subpixelMotionEstimation.SelectedItem = optValue;
                                continue;
                            case "trellis":
                                drop_trellis.SelectedItem = optValue;
                                continue;
                            case "mixed-refs":
                                check_mixedReferences.CheckState = CheckState.Checked;
                                continue;
                            case "me":
                                if (optValue.Equals("dia"))
                                    drop_MotionEstimationMethod.SelectedItem = "Diamond";
                                else if (optValue.Equals("hex"))
                                    drop_MotionEstimationMethod.SelectedItem = "Hexagon";
                                else if (optValue.Equals("umh"))
                                    drop_MotionEstimationMethod.SelectedItem = "Uneven Multi-Hexagon";
                                else if (optValue.Equals("esa"))
                                    drop_MotionEstimationMethod.SelectedItem = "Exhaustive";
                                else if (optValue.Equals("tesa"))
                                    drop_MotionEstimationMethod.SelectedItem = "Transformed Exhaustive";
                                continue;
                            case "merange":
                                drop_MotionEstimationRange.SelectedItem = optValue;
                                continue;
                            case "b-adapt":
                                int badapt;
                                int.TryParse(optValue, out badapt);
                                drop_adaptBFrames.SelectedIndex = (badapt + 1);
                                continue;
                            case "weightb":
                                check_weightedBFrames.CheckState = CheckState.Checked;
                                continue;
                            case "b-pyramid":
                                check_pyrmidalBFrames.CheckState = CheckState.Checked;
                                continue;
                            case "direct":
                                if (optValue == "auto")
                                    optValue = "Automatic";

                                if (optValue != "")
                                {
                                    Char[] letters = optValue.ToCharArray();
                                    letters[0] = Char.ToUpper(letters[0]);
                                    optValue = new string(letters);
                                }

                                drop_directPrediction.SelectedItem = optValue;
                                continue;
                            case "deblock":
                                string[] splitDeblock = optValue.Split(',');
                                string alphaDeblock = splitDeblock[0];
                                string betaDeblock = splitDeblock[1];

                                if (alphaDeblock.Equals("0") && betaDeblock.Replace("\n", "").Equals("0"))
                                {
                                    drop_deblockAlpha.SelectedItem = "Default (0)";
                                    drop_deblockBeta.SelectedItem = "Default (0)";
                                }
                                else
                                {
                                    drop_deblockAlpha.SelectedItem = !alphaDeblock.Equals("0") ? alphaDeblock : "0";

                                    drop_deblockBeta.SelectedItem = !betaDeblock.Replace("\n", "").Equals("0") ? betaDeblock.Replace("\n", "") : "0";
                                }
                                continue;
                            case "analyse":
                                if (optValue.Equals("p8x8,b8x8,i8x8,i4x4"))
                                    drop_analysis.SelectedItem = "Default (some)";
                                if (optValue.Equals("none"))
                                    drop_analysis.SelectedItem = "None";
                                if (optValue.Equals("all"))
                                    drop_analysis.SelectedItem = "All";
                                continue;
                            case "8x8dct":
                                check_8x8DCT.CheckState = CheckState.Checked;
                                continue;
                            case "cabac":
                                check_Cabac.CheckState = CheckState.Unchecked;
                                continue;
                            case "psy-rd":
                                string[] x = optValue.Split(',');

                                double psyrd, psytrellis;
                                int val, val2;

                                // default psy-rd = 1 (10 for the slider)
                                psyrd = double.TryParse(x[0], out psyrd) ? psyrd * 10 : 10.0;
                                // default psy-trellis = 0
                                psytrellis = double.TryParse(x[1], out psytrellis) ? psytrellis * 10 : 0.0; 

                                int.TryParse(psyrd.ToString(), out val);
                                int.TryParse(psytrellis.ToString(), out val2);

                                slider_psyrd.Value = val;
                                slider_psytrellis.Value = val2;
                                continue;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// This function will update the X264 Query when one of the GUI widgets changes.
        /// </summary>
        private void on_x264_WidgetChange(string sender)
        {
            animate(sender);
            String optNameToChange = sender;
            String currentOptString = rtf_x264Query.Text;

            /*First, we create a pattern to check for ":"optNameToChange"=" to modify the option if the name falls after
                the first character of the opt string (hence the ":") */
            String checkOptNameToChange = ":" + optNameToChange + "=";
            String checkOptNameToChangeBegin = optNameToChange + "=";

            // IF the current H264 Option String Contains Multiple Items or Just 1 Item
            if ((currentOptString.Contains(checkOptNameToChange)) || (currentOptString.StartsWith(checkOptNameToChangeBegin)))
                hasOptions(currentOptString, optNameToChange);
            else // IF there is no options in the rich text box!
                hasNoOptions(optNameToChange);
        }

        /// <summary>
        /// Called when the current x264 option string contains multiple (or a single) item(s) in it seperated by :
        /// it updates the current option that the widget corrosponds to, if it is already in thes string.
        /// </summary>
        /// <param name="currentOptString"></param>
        /// <param name="optNameToChange"></param>
        private void hasOptions(string currentOptString, string optNameToChange)
        {
            String thisOpt;             // The separated option such as "bframes=3"
            String optName;        // The option name such as "bframes"
            String[] currentOptsArray;

            /* Create new empty opt string*/
            String changedOptString = "";

            /*Put individual options into an array based on the ":" separator for processing, result is "<opt>=<value>"*/
            currentOptsArray = currentOptString.Split(':');

            /*iterate through the array and get <opts> and <values*/
            for (int loopcounter = 0; loopcounter < currentOptsArray.Length; loopcounter++)
            {
                thisOpt = currentOptsArray[loopcounter];

                if (thisOpt.Contains("="))
                {
                    string[] splitOptRange = thisOpt.Split('=');

                    optName = splitOptRange[0];     // e.g bframes

                    /* 
                     * Run through the available widgets for x264 opts and set them, as you add widgets,
                     * they need to be added here. This should be moved to its own method probably
                     * If the optNameToChange is found, appropriately change the value or delete it if
                     * "unspecified" is set.
                     */
                    if (optName.Equals(optNameToChange))
                    {
                        if (optNameToChange.Equals("deblock"))
                        {
                            String da = drop_deblockAlpha.SelectedItem.ToString();
                            String db = drop_deblockBeta.SelectedItem.ToString();

                            if (((da.Contains("Default")) && (db.Contains("Default"))) || ((da.Contains("0")) && (db.Contains("0"))))
                            {
                                drop_deblockBeta.SelectedItem = "Default (0)";
                                drop_deblockAlpha.SelectedItem = "Default (0)";
                                thisOpt = "";
                            }
                            else if ((!da.Contains("Default")) && (db.Contains("Default")))
                            {
                                drop_deblockBeta.SelectedItem = "0";
                                thisOpt = "deblock=" + da + ",0";
                            }
                            else if ((da.Contains("Default")) && (!db.Contains("Default")))
                            {
                                drop_deblockAlpha.SelectedItem = "0";
                                thisOpt = "deblock=0," + db;
                            }
                            else if ((!da.Contains("Default")) && (!db.Contains("Default")))
                                thisOpt = "deblock=" + da + "," + db;
                        }
                        else if (optNameToChange.Equals("psy-rd"))
                        {
                            if (slider_psyrd.Value == 10 && slider_psytrellis.Value == 0)
                                thisOpt = "";
                            else
                            {
                                double psyrd = slider_psyrd.Value * 0.1;
                                double psytre = slider_psytrellis.Value * 0.1;

                                string rd = psyrd.ToString("f1");
                                string rt = psytre.ToString("f1");

                                thisOpt = "psy-rd=" + rd + "," + rt;
                            }
                        }
                        else if (optNameToChange.Equals("mixed-refs"))
                            thisOpt = check_mixedReferences.CheckState == CheckState.Checked ? "mixed-refs=1" : "";
                        else if (optNameToChange.Equals("weightb"))
                            thisOpt = check_weightedBFrames.CheckState == CheckState.Checked ? "weightb=1" : "";
                        else if (optNameToChange.Equals("b-pyramid"))
                            thisOpt = check_pyrmidalBFrames.CheckState == CheckState.Checked ? "b-pyramid=1" : "";
                        else if (optNameToChange.Equals("no-fast-pskip"))
                            thisOpt = check_noFastPSkip.CheckState == CheckState.Checked ? "no-fast-pskip=1" : "";
                        else if (optNameToChange.Equals("no-dct-decimate"))
                            thisOpt = check_noDCTDecimate.CheckState == CheckState.Checked ? "no-dct-decimate=1" : "";
                        else if (optNameToChange.Equals("8x8dct"))
                            thisOpt = check_8x8DCT.CheckState == CheckState.Checked ? "8x8dct=1" : "";
                        else if (optNameToChange.Equals("cabac"))
                            thisOpt = check_Cabac.CheckState == CheckState.Checked ? "" : "cabac=0";
                        else if (optNameToChange.Equals("me"))
                        {
                            switch (drop_MotionEstimationMethod.SelectedIndex)
                            {
                                case 1:
                                    thisOpt = "me=dia";
                                    break;

                                case 2:
                                    thisOpt = "me=hex";
                                    break;

                                case 3:
                                    thisOpt = "me=umh";
                                    break;

                                case 4:
                                    thisOpt = "me=esa";
                                    break;

                                case 5:
                                    thisOpt = "me=tesa";
                                    break;

                                default:
                                    thisOpt = "";
                                    break;
                            }
                        }
                        else if (optNameToChange.Equals("direct"))
                        {
                            switch (drop_directPrediction.SelectedIndex)
                            {
                                case 1:
                                    thisOpt = "direct=none";
                                    break;

                                case 2:
                                    thisOpt = "direct=spatial";
                                    break;

                                case 3:
                                    thisOpt = "direct=temporal";
                                    break;

                                case 4:
                                    thisOpt = "direct=auto";
                                    break;

                                default:
                                    thisOpt = "";
                                    break;
                            }
                        }
                        else if (optNameToChange.Equals("analyse"))
                        {
                            switch (drop_analysis.SelectedIndex)
                            {
                                case 1:
                                    thisOpt = "analyse=none";
                                    break;

                                case 2:
                                    thisOpt = "analyse=all";
                                    break;

                                default:
                                    thisOpt = "";
                                    break;
                            }
                        }
                        else if (optNameToChange.Equals("merange"))
                        {
                            thisOpt = !drop_MotionEstimationRange.SelectedItem.ToString().Contains("Default")
                                          ? "merange=" + drop_MotionEstimationRange.SelectedItem
                                          : "";
                        }
                        else if (optNameToChange.Equals("b-adapt"))
                        {
                            thisOpt = !drop_adaptBFrames.SelectedItem.ToString().Contains("Default")
                                          ? "b-adapt=" + (drop_adaptBFrames.SelectedIndex - 1)
                                          : "";
                        }
                        else if (optNameToChange.Equals("ref"))
                        {
                            thisOpt = !drop_refFrames.SelectedItem.ToString().Contains("Default")
                                          ? "ref=" + drop_refFrames.SelectedItem
                                          : "";
                        }
                        else if (optNameToChange.Equals("bframes"))
                        {
                            String value = drop_bFrames.SelectedItem.ToString();
                            thisOpt = !drop_bFrames.SelectedItem.ToString().Contains("Default")
                                          ? "bframes=" + value
                                          : "";
                        }
                        else if (optNameToChange.Equals("subq"))
                        {
                            String value = drop_subpixelMotionEstimation.SelectedItem.ToString();
                            thisOpt = !drop_subpixelMotionEstimation.SelectedItem.ToString().Contains("Default")
                                          ? "subq=" + value
                                          : "";
                        }
                        else if (optNameToChange.Equals("trellis"))
                        {
                            String value = drop_trellis.SelectedItem.ToString();
                            thisOpt = !drop_trellis.SelectedItem.ToString().Contains("Default")
                                          ? "trellis=" + value
                                          : "";
                        }
                    }
                }

                /* Construct New String for opts here */
                if (!thisOpt.Equals(""))
                    changedOptString = changedOptString.Equals("") ? thisOpt : changedOptString + ":" + thisOpt;
            }

            /* Change the option string to reflect the new mod settings */
            rtf_x264Query.Text = changedOptString;
        }

        /// <summary>
        /// Add's an option to the x264 query string.
        /// Handles 2 cases.  1 Where rtf_x264Query.Text is empty, and one where there is an option with no value,
        /// e.g no-fast-pskip
        /// </summary>
        /// <param name="optNameToChange"></param>
        private void hasNoOptions(string optNameToChange)
        {
            string colon = "";
            if (rtf_x264Query.Text != "")
                colon = ":";

            string query = rtf_x264Query.Text;
            if (optNameToChange.Equals("me"))
            {
                switch (drop_MotionEstimationMethod.SelectedIndex)
                {
                    case 1:
                        query = query + colon + "me=dia";
                        break;

                    case 2:
                        query = query + colon + "me=hex";
                        break;

                    case 3:
                        query = query + colon + "me=umh";
                        break;

                    case 4:
                        query = query + colon + "me=esa";
                        break;

                    case 5:
                        query = query + colon + "me=tesa";
                        break;

                    default:
                        break;
                }
            }
            else if (optNameToChange.Equals("direct"))
            {
                switch (drop_directPrediction.SelectedIndex)
                {
                    case 1:
                        query = query + colon + "direct=none";
                        break;

                    case 2:
                        query = query + colon + "direct=spatial";
                        break;

                    case 3:
                        query = query + colon + "direct=temporal";
                        break;

                    case 4:
                        query = query + colon + "direct=auto";
                        break;

                    default:
                        break;
                }
            }
            else if (optNameToChange.Equals("analyse"))
            {
                switch (drop_analysis.SelectedIndex)
                {
                    case 1:
                        query = query + colon + "analyse=none";
                        break;

                    case 2:
                        query = query + colon + "analyse=all";
                        break;

                    default:
                        break;
                }
            }
            else if (optNameToChange.Equals("merange"))
            {
                int value = drop_MotionEstimationRange.SelectedIndex + 3;
                query = query + colon + "merange=" + value;
            }
            else if (optNameToChange.Equals("b-adapt"))
            {
                int value = drop_adaptBFrames.SelectedIndex - 1;
                query = query + colon + "b-adapt=" + value;
            }

            else if (optNameToChange.Equals("deblock"))
            {
                String da = drop_deblockAlpha.SelectedItem.ToString();
                String db = drop_deblockBeta.Text;

                if (((da.Contains("Default")) && (db.Contains("Default"))) || ((da.Contains("0")) && (db.Contains("0"))))
                {
                    drop_deblockBeta.SelectedItem = "Default (0)";
                    drop_deblockAlpha.SelectedItem = "Default (0)";
                }
                else
                {
                    if (db.Contains("Default"))
                        db = "0";

                    if (da.Contains("Default"))
                        da = "0";

                    query = query + colon + "deblock=" + da + "," + db;
                }
            }
            else if (optNameToChange.Equals("psy-rd"))
            {
                if (slider_psyrd.Value == 10 && slider_psytrellis.Value == 0)
                    query += "";
                else
                {
                    double psyrd = slider_psyrd.Value * 0.1;
                    double psytre = slider_psytrellis.Value * 0.1;

                    string rd = psyrd.ToString("f1");
                    string rt = psytre.ToString("f1");

                    query += colon + "psy-rd=" + rd + "," + rt;
                }
            }
            else if (optNameToChange.Equals("mixed-refs"))
            {
                if (check_mixedReferences.CheckState == CheckState.Checked)
                    query = query + colon + "mixed-refs=1";
            }
            else if (optNameToChange.Equals("weightb"))
            {
                if (check_weightedBFrames.CheckState == CheckState.Checked)
                    query = query + colon + "weightb=1";
            }
            else if (optNameToChange.Equals("b-pyramid"))
            {
                if (check_pyrmidalBFrames.CheckState == CheckState.Checked)
                    query = query + colon + "b-pyramid=1";
            }
            else if (optNameToChange.Equals("no-fast-pskip"))
            {
                if (check_noFastPSkip.CheckState == CheckState.Checked)
                    query = query + colon + "no-fast-pskip=1";
            }
            else if (optNameToChange.Equals("no-dct-decimate"))
            {
                if (check_noDCTDecimate.CheckState == CheckState.Checked)
                    query = query + colon + "no-dct-decimate=1";
            }
            else if (optNameToChange.Equals("8x8dct"))
            {
                if (check_8x8DCT.CheckState == CheckState.Checked)
                    query = query + colon + "8x8dct=1";
            }
            else if (optNameToChange.Equals("cabac"))
            {
                if (check_Cabac.CheckState != CheckState.Checked)
                    query = query + colon + "cabac=0";
            }
            else if (optNameToChange.Equals("ref"))
            {
                if (!drop_refFrames.SelectedItem.ToString().Contains("Default"))
                    query = query + colon + "ref=" + drop_refFrames.SelectedItem;
            }
            else if (optNameToChange.Equals("bframes"))
            {
                String value = drop_bFrames.SelectedItem.ToString();
                if (!drop_bFrames.SelectedItem.ToString().Contains("Default"))
                    query = query + colon + "bframes=" + value;
            }
            else if (optNameToChange.Equals("subq"))
            {
                String value = drop_subpixelMotionEstimation.SelectedItem.ToString();
                if (!drop_subpixelMotionEstimation.SelectedItem.ToString().Contains("Default"))
                    query = query + colon + "subq=" + value;
            }
            else if (optNameToChange.Equals("trellis"))
            {
                if (!drop_trellis.SelectedItem.ToString().Contains("Default"))
                    query = query + colon + "trellis=" + drop_trellis.SelectedItem;
            }

            rtf_x264Query.Text = query;
        }

        /// <summary>
        /// Shows and hides controls based on the values of other controls.
        /// </summary>
        /// <param name="sender"></param>
        private void animate(string sender)
        {
            /* Lots of situations to cover.
 	           - B-frames (when 0 turn of b-frame specific stuff, when < 2 disable b-pyramid)
 	           - CABAC (when 0 turn off trellis)
 	           - analysis (if none, turn off 8x8dct)
 	           - refs (under 2, disable mixed-refs)
 	           - subme (if under 6, turn off psy-rd and psy-trel)
 	           - trellis (if 0, turn off psy-trel)
 	        */

            switch(sender)
            {
                case "bframes":
                    if (drop_bFrames.SelectedIndex < 2)
                    {
                        /* If the b-frame widget is at 0 or 1, the user has chosen
                           not to use b-frames at all. So disable the options
                           that can only be used when b-frames are enabled.        */
                        check_weightedBFrames.Visible = false;
                        check_pyrmidalBFrames.Visible = false;
                        drop_directPrediction.Visible = false;
                        lbl_direct_prediction.Visible = false;

                        check_weightedBFrames.CheckState = CheckState.Unchecked;
                        check_pyrmidalBFrames.CheckState = CheckState.Unchecked;
                        drop_directPrediction.SelectedIndex = 0;

                        drop_adaptBFrames.Visible = false;
                        lbl_adaptBFrames.Visible = false;
                        drop_adaptBFrames.SelectedIndex = 0;
                    }
                    else if (drop_bFrames.SelectedIndex == 2)
                    {
                        /* Only 1 b-frame? Disable b-pyramid. */
                        check_pyrmidalBFrames.Visible = false;
                        check_pyrmidalBFrames.CheckState = CheckState.Unchecked;

                        check_weightedBFrames.Visible = true;
                        drop_directPrediction.Visible = true;
                        lbl_direct_prediction.Visible = true;

                        drop_adaptBFrames.Visible = true;
                        lbl_adaptBFrames.Visible = true;
                    }
                    else
                    {
                        check_weightedBFrames.Visible = true;
                        check_pyrmidalBFrames.Visible = true;
                        drop_directPrediction.Visible = true;
                        lbl_direct_prediction.Visible = true;

                        drop_adaptBFrames.Visible = true;
                        lbl_adaptBFrames.Visible = true;
                    }
                break;
                case "cabac":
                    if (check_Cabac.Checked == false)
                    {
                        /* Without CABAC entropy coding, trellis doesn't run. */
                        drop_trellis.Visible = false;
                        drop_trellis.SelectedIndex = 0;
                        lbl_trellis.Visible = false;
                    }
                    else
                    {
                        drop_trellis.Visible = true;
                        lbl_trellis.Visible = true;
                    }
                break;
                case "analyse":
                    if (drop_analysis.SelectedIndex == 1)
                    {
                        /* No analysis? Disable 8x8dct */
                        check_8x8DCT.Visible = false;
                        if (sender != "8x8dct")
                            check_8x8DCT.CheckState = CheckState.Unchecked;
                    }
                    else
                        check_8x8DCT.Visible = true;
                break;
                case "ref":
                    if (drop_refFrames.SelectedIndex < 3)
                    {
                        check_mixedReferences.Visible = false;
                        if (sender != "mixed-refs")
                            check_mixedReferences.CheckState = CheckState.Unchecked;
                    }
                    else
                        check_mixedReferences.Visible = true;
                break;
                case "me":        // Motion Estimation
                    if (drop_MotionEstimationMethod.SelectedIndex < 3)
                    {
                        drop_MotionEstimationRange.Visible = false;
                        lbl_merange.Visible = false;
                        drop_MotionEstimationRange.SelectedIndex = 0;
                    }
                    else
                    {
                        drop_MotionEstimationRange.Visible = true;
                        lbl_merange.Visible = true;
                    }
                break;
                case "subq":        // subme
                    if (drop_subpixelMotionEstimation.SelectedIndex != 0 && drop_subpixelMotionEstimation.SelectedIndex < 7)
                    {
                        slider_psyrd.Visible = false;
                        slider_psyrd.Value = 10;
                        lbl_psyrd.Visible = false;


                        slider_psytrellis.Visible = false;
                        slider_psytrellis.Value = 0;
                        lbl_psytrellis.Visible = false;
                    }
                    else
                    {
                        slider_psyrd.Visible = true;
                        lbl_psyrd.Visible = true;

                        if (drop_trellis.SelectedIndex >= 2 && check_Cabac.Checked && slider_psytrellis.Visible == false)
                        {
                            slider_psytrellis.Visible = true;
                            lbl_psytrellis.Visible = true;
                        }
                    }
                break;
                case "trellis":        // subme
                    if (drop_trellis.SelectedIndex < 2)
                    {
                        slider_psytrellis.Visible = false;
                        slider_psytrellis.Value = 0;
                        lbl_psytrellis.Visible = false;
                    }
                    else
                    {
                        if ((drop_subpixelMotionEstimation.SelectedIndex == 0 || drop_subpixelMotionEstimation.SelectedIndex >= 7) && check_Cabac.Checked && slider_psytrellis.Visible == false)
                        {
                            slider_psytrellis.Visible = true;
                            lbl_psytrellis.Visible = true;
                        }
                    }
                break;
            }
        }
    }
}
