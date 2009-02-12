using System;
using System.Windows.Forms;

namespace Handbrake
{
    class x264Panel
    {
        /// <summary>
        /// Reset all components to defaults and clears the x264 rtf box
        /// </summary>
        public void reset2Defaults(frmMain mainWindow)
        {
            mainWindow.check_8x8DCT.CheckState = CheckState.Unchecked;
            mainWindow.check_Cabac.CheckState = CheckState.Checked;
            mainWindow.check_mixedReferences.CheckState = CheckState.Unchecked;
            mainWindow.check_noDCTDecimate.CheckState = CheckState.Unchecked;
            mainWindow.check_noFastPSkip.CheckState = CheckState.Unchecked;
            mainWindow.check_pyrmidalBFrames.CheckState = CheckState.Unchecked;
            mainWindow.check_weightedBFrames.CheckState = CheckState.Unchecked;
            mainWindow.drop_analysis.SelectedIndex = 0;
            mainWindow.drop_bFrames.SelectedIndex = 0;
            mainWindow.drop_deblockAlpha.SelectedIndex = 0;
            mainWindow.drop_deblockBeta.SelectedIndex = 0;
            mainWindow.drop_directPrediction.SelectedIndex = 0;
            mainWindow.drop_MotionEstimationMethod.SelectedIndex = 0;
            mainWindow.drop_MotionEstimationRange.SelectedIndex = 0;
            mainWindow.drop_refFrames.SelectedIndex = 0;
            mainWindow.drop_subpixelMotionEstimation.SelectedIndex = 0;
            mainWindow.drop_trellis.SelectedIndex = 0;
            mainWindow.slider_psyrd.Value = 0;
            mainWindow.slider_psytrellis.Value = 0;
            mainWindow.drop_adaptBFrames.SelectedIndex = 0;

            mainWindow.rtf_x264Query.Text = "";
        }

        /// <summary>
        /// Update GUI componets from the current x264 rtf string
        /// </summary>
        public void X264_SetCurrentSettingsInPanel(frmMain mainWindow)
        {
            /* Set widgets depending on the opt string in field */
            String thisOpt; // The separated option such as "bframes=3"
            String optName; // The option name such as "bframes"
            String optValue;// The option value such as "3"
            String[] currentOptsArray;

            //Set currentOptString to the contents of the text box.
            String currentOptString = mainWindow.rtf_x264Query.Text.Replace("\n", "");

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

                        /*bframes NSPopUpButton*/
                        if (optName.Equals("bframes"))
                            mainWindow.drop_bFrames.SelectedItem = optValue;

                        /*ref NSPopUpButton*/
                        else if (optName.Equals("ref"))
                            mainWindow.drop_refFrames.SelectedItem = optValue;

                        /*No Fast PSkip NSPopUpButton*/
                        else if (optName.Equals("no-fast-pskip"))
                            mainWindow.check_noFastPSkip.CheckState = CheckState.Checked;

                        /*No Dict Decimate NSPopUpButton*/
                        else if (optName.Equals("no-dct-decimate"))
                            mainWindow.check_noDCTDecimate.CheckState = CheckState.Checked;

                        /*Sub Me NSPopUpButton*/
                        else if (optName.Equals("subq"))
                            mainWindow.drop_subpixelMotionEstimation.SelectedItem = optValue;

                        /*Trellis NSPopUpButton*/
                        else if (optName.Equals("trellis"))
                            mainWindow.drop_trellis.SelectedItem = optValue;

                        /*Mixed Refs NSButton*/
                        else if (optName.Equals("mixed-refs"))
                            mainWindow.check_mixedReferences.CheckState = CheckState.Checked;

                        /*Motion Estimation NSPopUpButton*/
                        else if (optName.Equals("me"))
                        {
                            if (optValue.Equals("dia"))
                                mainWindow.drop_MotionEstimationMethod.SelectedItem = "Diamond";
                            else if (optValue.Equals("hex"))
                                mainWindow.drop_MotionEstimationMethod.SelectedItem = "Hexagon";
                            else if (optValue.Equals("umh"))
                                mainWindow.drop_MotionEstimationMethod.SelectedItem = "Uneven Multi-Hexagon";
                            else if (optValue.Equals("esa"))
                                mainWindow.drop_MotionEstimationMethod.SelectedItem = "Exhaustive";
                            else if (optValue.Equals("tesa"))
                                mainWindow.drop_MotionEstimationMethod.SelectedItem = "Transformed Exhaustive";

                        }
                        /*ME Range NSPopUpButton*/
                        else if (optName.Equals("merange"))
                            mainWindow.drop_MotionEstimationRange.SelectedItem = optValue;

                        else if (optName.Equals("b-adapt"))
                            mainWindow.drop_adaptBFrames.SelectedItem = optValue; 

                        /*Weighted B-Frames NSPopUpButton*/
                        else if (optName.Equals("weightb"))
                            mainWindow.check_weightedBFrames.CheckState = CheckState.Checked;

                        /*B Pyramid NSPopUpButton*/
                        else if (optName.Equals("b-pyramid"))
                            mainWindow.check_pyrmidalBFrames.CheckState = CheckState.Checked;

                        /*Direct B-frame Prediction NSPopUpButton*/
                        else if (optName.Equals("direct"))
                        {
                            if (optValue == "auto")
                                optValue = "Automatic";

                            if (optValue != "")
                            {
                                Char[] letters = optValue.ToCharArray();
                                letters[0] = Char.ToUpper(letters[0]);
                                optValue = new string(letters);
                            }

                            mainWindow.drop_directPrediction.SelectedItem = optValue;
                        }

                        /*Deblocking NSPopUpButtons*/
                        else if (optName.Equals("deblock"))
                        {
                            string[] splitDeblock = optValue.Split(',');
                            string alphaDeblock = splitDeblock[0];
                            string betaDeblock = splitDeblock[1];

                            if (alphaDeblock.Equals("0") && betaDeblock.Replace("\n", "").Equals("0"))
                            {
                                mainWindow.drop_deblockAlpha.SelectedItem = "Default (0)";
                                mainWindow.drop_deblockBeta.SelectedItem = "Default (0)";
                            }
                            else
                            {
                                if (!alphaDeblock.Equals("0"))
                                    mainWindow.drop_deblockAlpha.SelectedItem = alphaDeblock;
                                else
                                    mainWindow.drop_deblockAlpha.SelectedItem = "0";

                                if (!betaDeblock.Replace("\n", "").Equals("0"))
                                    mainWindow.drop_deblockBeta.SelectedItem = betaDeblock.Replace("\n", "");
                                else
                                    mainWindow.drop_deblockBeta.SelectedItem = "0";
                            }
                        }
                        /* Analysis NSPopUpButton */
                        else if (optName.Equals("analyse"))
                        {

                            if (optValue.Equals("p8x8,b8x8,i8x8,i4x4"))
                                mainWindow.drop_analysis.SelectedItem = "Default (some)";
                            if (optValue.Equals("none"))
                                mainWindow.drop_analysis.SelectedItem = "None";
                            if (optValue.Equals("all"))
                                mainWindow.drop_analysis.SelectedItem = "All";
                        }
                        /* 8x8 DCT NSButton */
                        else if (optName.Equals("8x8dct"))
                            mainWindow.check_8x8DCT.CheckState = CheckState.Checked;

                        /* CABAC NSButton */
                        else if (optName.Equals("cabac"))
                            mainWindow.check_Cabac.CheckState = CheckState.Unchecked;

                        /* Psy-RD and Psy-Trellis NSSliders */
                        else if (optName.Equals("psy-rd"))
                        {
                            string[] x = optValue.Split(',');

                            double psyrd = 0, psytrellis = 0;
                            int val, val2;
                            double.TryParse(x[0], out psyrd);
                            double.TryParse(x[1], out psytrellis);

                            psyrd = psyrd * 10;
                            psytrellis = psytrellis * 10;

                            int.TryParse(psyrd.ToString(), out val);
                            int.TryParse(psytrellis.ToString(), out val2);

                            mainWindow.slider_psyrd.Value = val;
                            mainWindow.slider_psytrellis.Value = val2;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Iterate over every x264 option, standardize it, write the full string to the x264 rtf box
        /// </summary>
        public void X264_StandardizeOptString(frmMain mainWindow)
        {
            /* Set widgets depending on the opt string in field */
            String thisOpt; // The separated option such as "bframes=3"
            String optName; // The option name such as "bframes"
            String optValue;// The option value such as "3"
            String changedOptString = "";
            String[] currentOptsArray;

            /*First, we get an opt string to process */
            String currentOptString = mainWindow.rtf_x264Query.Text;

            /*verify there is an opt string to process */
            if (currentOptString.Contains("="))
            {
                /*Put individual options into an array based on the ":" separator for processing, result is "<opt>=<value>"*/
                currentOptsArray = currentOptString.Split(':');

                /*iterate through the array and get <opts> and <values*/
                //NSEnumerator * enumerator = [currentOptsArray objectEnumerator];
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
                mainWindow.rtf_x264Query.Text = changedOptString;
        }
        /*
         * Take a single option and standardize it. Returns as a String
         * Input: String. - Single X264 Option. Name only
         * Output: String - Single X264 Option. Name only. Changed to standard format
         */
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
        /// This function will update the X264 Query when one of the GUI widgets changes.
        /// </summary>
        public void on_x264_WidgetChange(string sender, frmMain mainWindow)
        {
            animate(mainWindow, sender);
            String optNameToChange = sender;
            String currentOptString = mainWindow.rtf_x264Query.Text;

            /*First, we create a pattern to check for ":"optNameToChange"=" to modify the option if the name falls after
                the first character of the opt string (hence the ":") */
            String checkOptNameToChange = ":" + optNameToChange + "=";
            String checkOptNameToChangeBegin = optNameToChange + "=";

            // IF the current H264 Option String Contains Multiple Items or Just 1 Item
            if ((currentOptString.Contains(checkOptNameToChange)) || (currentOptString.StartsWith(checkOptNameToChangeBegin)))
                hasOptions(currentOptString, optNameToChange, mainWindow);
            else // IF there is no options in the rich text box!
                hasNoOptions(optNameToChange, mainWindow);
        }
        /*
         * Used by on_x264_WidgetChange()
         ** hasOptions - Called when the current x264 option string contains multiple (or a single) item(s) in it seperated by :
         * it updates the current option that the widget corrosponds to, if it is already in thes string.
         ** hasNoOptions - Add's an option to the x264 query string.
         * Handles 2 cases.  1 Where rtf_x264Query.Text is empty, and one where there is an option with no value,
         * e.g no-fast-pskip
         */
        private void hasOptions(string currentOptString, string optNameToChange, frmMain mainWindow)
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
                            String da = mainWindow.drop_deblockAlpha.SelectedItem.ToString();
                            String db = mainWindow.drop_deblockBeta.SelectedItem.ToString();

                            if (((da.Contains("Default")) && (db.Contains("Default"))) || ((da.Contains("0")) && (db.Contains("0"))))
                            {
                                mainWindow.drop_deblockBeta.SelectedItem = "Default (0)";
                                mainWindow.drop_deblockAlpha.SelectedItem = "Default (0)";
                                thisOpt = "";
                            }
                            else if ((!da.Contains("Default")) && (db.Contains("Default")))
                            {
                                mainWindow.drop_deblockBeta.SelectedItem = "0";
                                thisOpt = "deblock=" + da + ",0";
                            }
                            else if ((da.Contains("Default")) && (!db.Contains("Default")))
                            {
                                mainWindow.drop_deblockAlpha.SelectedItem = "0";
                                thisOpt = "deblock=0," + db;
                            }
                            else if ((!da.Contains("Default")) && (!db.Contains("Default")))
                                thisOpt = "deblock=" + da + "," + db;
                        }
                        else if (optNameToChange.Equals("psy-rd"))
                        {
                            if (mainWindow.slider_psyrd.Value == 10 && mainWindow.slider_psytrellis.Value == 0)
                                thisOpt = "";
                            else
                            {
                                double psyrd = mainWindow.slider_psyrd.Value * 0.1;
                                double psytre = mainWindow.slider_psytrellis.Value * 0.1;
                                string rd, rt;
                                if (psyrd == 1)
                                    rd = "1.0";
                                else
                                    rd = psyrd.ToString();

                                if (psytre == 1)
                                    rt = "1.0";
                                else
                                    rt = psytre.ToString();

                                thisOpt = "psy-rd=" + rd + "," + rt;
                            }
                        }
                        else if (optNameToChange.Equals("mixed-refs"))
                        {
                            if (mainWindow.check_mixedReferences.CheckState == CheckState.Checked)
                                thisOpt = "mixed-refs=1";
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("weightb"))
                        {
                            if (mainWindow.check_weightedBFrames.CheckState == CheckState.Checked)
                                thisOpt = "weightb=1";
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("b-pyramid"))
                        {
                            if (mainWindow.check_pyrmidalBFrames.CheckState == CheckState.Checked)
                                thisOpt = "b-pyramid=1";
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("no-fast-pskip"))
                        {
                            if (mainWindow.check_noFastPSkip.CheckState == CheckState.Checked)
                                thisOpt = "no-fast-pskip=1";
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("no-dct-decimate"))
                        {
                            if (mainWindow.check_noDCTDecimate.CheckState == CheckState.Checked)
                                thisOpt = "no-dct-decimate=1";
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("8x8dct"))
                        {
                            if (mainWindow.check_8x8DCT.CheckState == CheckState.Checked)
                                thisOpt = "8x8dct=1";
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("cabac"))
                        {
                            if (mainWindow.check_Cabac.CheckState == CheckState.Checked)
                                thisOpt = "";
                            else
                                thisOpt = "cabac=0";
                        }
                        else if (optNameToChange.Equals("me"))
                        {
                            switch (mainWindow.drop_MotionEstimationMethod.SelectedIndex)
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
                            switch (mainWindow.drop_directPrediction.SelectedIndex)
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
                            switch (mainWindow.drop_analysis.SelectedIndex)
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
                            if (!mainWindow.drop_MotionEstimationRange.SelectedItem.ToString().Contains("Default"))
                                thisOpt = "merange=" + mainWindow.drop_MotionEstimationRange.SelectedItem;
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("b-adapt"))
                        {
                            if (!mainWindow.drop_adaptBFrames.SelectedItem.ToString().Contains("Default"))
                                thisOpt = "b-adapt=" + (mainWindow.drop_adaptBFrames.SelectedIndex - 1);
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("ref"))
                        {
                            if (!mainWindow.drop_refFrames.SelectedItem.ToString().Contains("Default"))
                                thisOpt = "ref=" + mainWindow.drop_refFrames.SelectedItem;
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("bframes"))
                        {
                            String value = mainWindow.drop_bFrames.SelectedItem.ToString();
                            if (!mainWindow.drop_bFrames.SelectedItem.ToString().Contains("Default"))
                                thisOpt = "bframes=" + value;
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("subq"))
                        {
                            String value = mainWindow.drop_subpixelMotionEstimation.SelectedItem.ToString();
                            if (!mainWindow.drop_subpixelMotionEstimation.SelectedItem.ToString().Contains("Default"))
                                thisOpt = "subq=" + value;
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("trellis"))
                        {
                            String value = mainWindow.drop_trellis.SelectedItem.ToString();
                            if (!mainWindow.drop_trellis.SelectedItem.ToString().Contains("Default"))
                                thisOpt = "trellis=" + value;
                            else
                                thisOpt = "";
                        }
                    }
                }

                /* Construct New String for opts here */
                if (!thisOpt.Equals(""))
                {
                    if (changedOptString.Equals(""))
                        changedOptString = thisOpt;
                    else
                        changedOptString = changedOptString + ":" + thisOpt;
                }
            }

            /* Change the option string to reflect the new mod settings */
            mainWindow.rtf_x264Query.Text = changedOptString;
        }
        private void hasNoOptions(string optNameToChange, frmMain mainWindow)
        {
            string colon = "";
            if (mainWindow.rtf_x264Query.Text != "")
                colon = ":";

            string query = mainWindow.rtf_x264Query.Text;
            if (optNameToChange.Equals("me"))
            {
                switch (mainWindow.drop_MotionEstimationMethod.SelectedIndex)
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
                switch (mainWindow.drop_directPrediction.SelectedIndex)
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
                switch (mainWindow.drop_analysis.SelectedIndex)
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
                int value = mainWindow.drop_MotionEstimationRange.SelectedIndex + 3;
                query = query + colon + "merange=" + value;
            }
            else if (optNameToChange.Equals("b-adapt"))
            {
                int value = mainWindow.drop_adaptBFrames.SelectedIndex -1;
                query = query + colon + "b-adapt=" + value;
            }

            else if (optNameToChange.Equals("deblock"))
            {
                String da = mainWindow.drop_deblockAlpha.SelectedItem.ToString();
                String db = mainWindow.drop_deblockBeta.Text;

                if (((da.Contains("Default")) && (db.Contains("Default"))) || ((da.Contains("0")) && (db.Contains("0"))))
                {
                    mainWindow.drop_deblockBeta.SelectedItem = "Default (0)";
                    mainWindow.drop_deblockAlpha.SelectedItem = "Default (0)";
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
                if (mainWindow.slider_psyrd.Value == 10 && mainWindow.slider_psytrellis.Value == 0)
                    query += "";
                else
                {
                    double psyrd = mainWindow.slider_psyrd.Value * 0.1;
                    double psytre = mainWindow.slider_psytrellis.Value * 0.1;
                    string rd, rt;
                    if (psyrd == 1)
                        rd = "1.0";
                    else
                        rd = psyrd.ToString();

                    if (psytre == 1)
                        rt = "1.0";
                    else
                        rt = psytre.ToString();

                    query += colon + "psy-rd=" + rd + "," + rt;
                }
            }
            else if (optNameToChange.Equals("mixed-refs"))
            {
                if (mainWindow.check_mixedReferences.CheckState == CheckState.Checked)
                    query = query + colon + "mixed-refs=1";
            }
            else if (optNameToChange.Equals("weightb"))
            {
                if (mainWindow.check_weightedBFrames.CheckState == CheckState.Checked)
                    query = query + colon + "weightb=1";
            }
            else if (optNameToChange.Equals("b-pyramid"))
            {
                if (mainWindow.check_pyrmidalBFrames.CheckState == CheckState.Checked)
                    query = query + colon + "b-pyramid=1";
            }
            else if (optNameToChange.Equals("no-fast-pskip"))
            {
                if (mainWindow.check_noFastPSkip.CheckState == CheckState.Checked)
                    query = query + colon + "no-fast-pskip=1";
            }
            else if (optNameToChange.Equals("no-dct-decimate"))
            {
                if (mainWindow.check_noDCTDecimate.CheckState == CheckState.Checked)
                    query = query + colon + "no-dct-decimate=1";
            }
            else if (optNameToChange.Equals("8x8dct"))
            {
                if (mainWindow.check_8x8DCT.CheckState == CheckState.Checked)
                    query = query + colon + "8x8dct=1";
            }
            else if (optNameToChange.Equals("cabac"))
            {
                if (mainWindow.check_Cabac.CheckState != CheckState.Checked)
                    query = query + colon + "cabac=0";
            }
            else if (optNameToChange.Equals("ref"))
            {
                if (!mainWindow.drop_refFrames.SelectedItem.ToString().Contains("Default"))
                    query = query + colon + "ref=" + mainWindow.drop_refFrames.SelectedItem;
            }
            else if (optNameToChange.Equals("bframes"))
            {
                String value = mainWindow.drop_bFrames.SelectedItem.ToString();
                if (!mainWindow.drop_bFrames.SelectedItem.ToString().Contains("Default"))
                    query = query + colon + "bframes=" + value;
            }
            else if (optNameToChange.Equals("subq"))
            {
                String value = mainWindow.drop_subpixelMotionEstimation.SelectedItem.ToString();
                if (!mainWindow.drop_subpixelMotionEstimation.SelectedItem.ToString().Contains("Default"))
                    query = query + colon + "subq=" + value;
            }
            else if (optNameToChange.Equals("trellis"))
            {
                if (!mainWindow.drop_trellis.SelectedItem.ToString().Contains("Default"))
                    query = query + colon + "trellis=" + mainWindow.drop_trellis.SelectedItem;
            }

            mainWindow.rtf_x264Query.Text = query;
        }
        private void animate(frmMain mainWindow, string sender)
        {
            /* Lots of situations to cover.
               - B-frames (when 0 turn of b-frame specific stuff, when < 2 disable b-pyramid)
               - CABAC (when 0 turn off trellis)
               - analysis (if none, turn off 8x8dct)
               - refs (under 2, disable mixed-refs)
            */
            if (mainWindow.drop_bFrames.SelectedIndex < 2)
            {
                /* If the b-frame widget is at 0 or 1, the user has chosen
                   not to use b-frames at all. So disable the options
                   that can only be used when b-frames are enabled.        */
                mainWindow.check_weightedBFrames.Visible = false;
                mainWindow.check_pyrmidalBFrames.Visible = false;
                mainWindow.drop_directPrediction.Visible = false;
                mainWindow.lbl_direct_prediction.Visible = false;

                mainWindow.check_weightedBFrames.CheckState = CheckState.Unchecked;
                mainWindow.check_pyrmidalBFrames.CheckState = CheckState.Unchecked;
                mainWindow.drop_directPrediction.SelectedIndex = 0;

                mainWindow.drop_adaptBFrames.Visible = false;
                mainWindow.lbl_adaptBFrames.Visible = false;
                mainWindow.drop_adaptBFrames.SelectedIndex = 0;
            }
            else if (mainWindow.drop_bFrames.SelectedIndex == 2)
            {
                /* Only 1 b-frame? Disable b-pyramid. */
                mainWindow.check_pyrmidalBFrames.Visible = false;
                mainWindow.check_pyrmidalBFrames.CheckState = CheckState.Unchecked;

                mainWindow.check_weightedBFrames.Visible = true;
                mainWindow.drop_directPrediction.Visible = true;
                mainWindow.lbl_direct_prediction.Visible = true;

                mainWindow.drop_adaptBFrames.Visible = true;
                mainWindow.lbl_adaptBFrames.Visible = true;
            }
            else
            {
                mainWindow.check_weightedBFrames.Visible = true;
                mainWindow.check_pyrmidalBFrames.Visible = true;
                mainWindow.drop_directPrediction.Visible = true;
                mainWindow.lbl_direct_prediction.Visible = true;

                mainWindow.drop_adaptBFrames.Visible = true;
                mainWindow.lbl_adaptBFrames.Visible = true;
            }

            if (mainWindow.check_Cabac.Checked == false)
            {
                /* Without CABAC entropy coding, trellis doesn't run. */
                mainWindow.drop_trellis.Visible = false;
                mainWindow.drop_trellis.SelectedIndex = 0;
                mainWindow.lbl_trellis.Visible = false;
            }
            else
            {
                mainWindow.drop_trellis.Visible = true;
                mainWindow.lbl_trellis.Visible = true;
            }

            if (mainWindow.drop_analysis.SelectedIndex == 1)
            {
                /* No analysis? Disable 8x8dct */
                mainWindow.check_8x8DCT.Visible = false;
                if (sender != "8x8dct")
                    mainWindow.check_8x8DCT.CheckState = CheckState.Unchecked;
            }
            else
                mainWindow.check_8x8DCT.Visible = true;

            if (mainWindow.drop_refFrames.SelectedIndex < 3)
            {
                mainWindow.check_mixedReferences.Visible = false;
                if (sender != "mixed-refs")
                    mainWindow.check_mixedReferences.CheckState = CheckState.Unchecked;
            }
            else
                mainWindow.check_mixedReferences.Visible = true;

            if (mainWindow.drop_MotionEstimationMethod.SelectedIndex < 3)
            {
                mainWindow.drop_MotionEstimationRange.Visible = false;
                mainWindow.lbl_merange.Visible = false;
                mainWindow.drop_MotionEstimationRange.SelectedIndex = 0;
            }
            else
            {
                mainWindow.drop_MotionEstimationRange.Visible = true;
                mainWindow.lbl_merange.Visible = true;
            }

            if (mainWindow.drop_subpixelMotionEstimation.SelectedIndex != 0 && mainWindow.drop_subpixelMotionEstimation.SelectedIndex < 7)
            {
                mainWindow.slider_psyrd.Visible = false;
                mainWindow.slider_psyrd.Value = 10;
                mainWindow.lbl_psyrd.Visible = false;


                mainWindow.slider_psytrellis.Visible = false;
                mainWindow.slider_psytrellis.Value = 0;
                mainWindow.lbl_psytrellis.Visible = false;
            }
            else
            {
                mainWindow.slider_psyrd.Visible = true;
                mainWindow.lbl_psyrd.Visible = true;

                if (mainWindow.drop_trellis.SelectedIndex >= 2 && mainWindow.check_Cabac.Checked && mainWindow.slider_psytrellis.Visible == false)
                {
                    mainWindow.slider_psytrellis.Visible = true;
                    mainWindow.lbl_psytrellis.Visible = true;
                }
            }

            if (mainWindow.drop_trellis.SelectedIndex < 2)
            {
                mainWindow.slider_psytrellis.Visible = false;
                mainWindow.slider_psytrellis.Value = 0;
                mainWindow.lbl_psytrellis.Visible = false;
            }
            else
            {
                if ((mainWindow.drop_subpixelMotionEstimation.SelectedIndex == 0 || mainWindow.drop_subpixelMotionEstimation.SelectedIndex >= 7) && mainWindow.check_Cabac.Checked && mainWindow.slider_psytrellis.Visible == false)
                {
                    mainWindow.slider_psytrellis.Visible = true;
                    mainWindow.lbl_psytrellis.Visible = true;
                }
            }

        }

    }
}