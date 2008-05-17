using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace Handbrake.Functions
{
    class x264Panel
    {
        Boolean NoWidgetUpdate = false;

        /// <summary>
        /// Reset all components to defaults and clears the x264 rtf box
        /// </summary>
        public void reset2Defaults(frmMain mainWindow)
        {
            mainWindow.check_8x8DCT.CheckState = CheckState.Unchecked;
            mainWindow.check_bFrameDistortion.CheckState = CheckState.Unchecked;
            mainWindow.check_BidirectionalRefinement.CheckState = CheckState.Unchecked;
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

            mainWindow.rtf_x264Query.Text = "";
        }

        /// <summary>
        /// Update GUI componets from the current x264 rtf string
        /// </summary>
        public void X264_SetCurrentSettingsInPanel(frmMain mainWindow)
        {
            // When the widgets are changed, we don't want them to update the text box again. No Need.
            // This boolean controls the Widget Change function


            /* Set widgets depending on the opt string in field */
            String thisOpt; // The separated option such as "bframes=3"
            String optName = ""; // The option name such as "bframes"
            String optValue = "";// The option value such as "3"
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

                        }
                        /*ME Range NSPopUpButton*/
                        else if (optName.Equals("merange"))
                            mainWindow.drop_MotionEstimationRange.SelectedItem = optValue;

                        /*Weighted B-Frames NSPopUpButton*/
                        else if (optName.Equals("weightb"))
                            mainWindow.check_weightedBFrames.CheckState = CheckState.Checked;

                        /*BRDO NSPopUpButton*/
                        else if (optName.Equals("brdo"))
                            mainWindow.check_bFrameDistortion.CheckState = CheckState.Checked;

                        /*B Pyramid NSPopUpButton*/
                        else if (optName.Equals("b-pyramid"))
                            mainWindow.check_pyrmidalBFrames.CheckState = CheckState.Checked;

                        /*Bidirectional Motion Estimation Refinement NSPopUpButton*/
                        else if (optName.Equals("bime"))
                            mainWindow.check_BidirectionalRefinement.CheckState = CheckState.Checked;

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
                            string alphaDeblock = "";
                            string betaDeblock = "";

                            string[] splitDeblock = optValue.Split(',');
                            alphaDeblock = splitDeblock[0];
                            betaDeblock = splitDeblock[1];

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
            String optName = ""; // The option name such as "bframes"
            String optValue = "";// The option value such as "3"
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
                    if (currentOptsArray[currentOptsArrayCount - 1] == "")
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
                    if (thisOpt == "")
                        changedOptString = changedOptString + thisOpt;
                    else
                    {
                        if (changedOptString == "")
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

        /// <summary>
        /// This function will update the X264 Query when one of the GUI widgets changes.
        /// </summary>
        public void on_x264_WidgetChange(string sender, frmMain mainWindow)
        {
            if (NoWidgetUpdate == false)
            {
                String optNameToChange = sender;
                String currentOptString = mainWindow.rtf_x264Query.Text;

                /*First, we create a pattern to check for ":"optNameToChange"=" to modify the option if the name falls after
                    the first character of the opt string (hence the ":") */
                String checkOptNameToChange = ":" + optNameToChange + "=";
                String checkOptNameToChangeBegin = optNameToChange + "=";

                // IF the current H264 Option String Contains Multiple Items or Just 1 Item
                if ((currentOptString.Contains(checkOptNameToChange)) || (currentOptString.StartsWith(checkOptNameToChangeBegin)))
                    hasFullOption(currentOptString, optNameToChange, mainWindow);
                else // IF there is no options in the rich text box!
                    hasNoOptions(optNameToChange, mainWindow);
            }
            else
            {
                NoWidgetUpdate = false;
            }
        }
        

        // Some Private Functions used by the above Public Functions
        /*
         * Used by on_x264_WidgetChange()
         * Called when the current x264 option string contains multiple (or a single) item(s) in it seperated by :
         * Basically, it updates the current option that the widget corrosponds to, if it is already in thes string
         * otherwise, it adds it to the string.
         */
        private void hasFullOption(string currentOptString, string optNameToChange, frmMain mainWindow)
        {
            String thisOpt;             // The separated option such as "bframes=3"
            String optName = "";        // The option name such as "bframes"
            String optValue = "";       // The option value such as "3"
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
                    optValue = splitOptRange[1];    // e.g 2

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
                        else if (optNameToChange.Equals("brdo"))
                        {
                            if (mainWindow.check_bFrameDistortion.CheckState == CheckState.Checked)
                                thisOpt = "brdo=1";
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("bime"))
                        {
                            if (mainWindow.check_BidirectionalRefinement.CheckState == CheckState.Checked)
                                thisOpt = "bime=1";
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
                                thisOpt = "merange=" + mainWindow.drop_MotionEstimationRange.SelectedItem.ToString();
                            else
                                thisOpt = "";
                        }
                        else if (optNameToChange.Equals("ref"))
                        {
                            if (!mainWindow.drop_refFrames.SelectedItem.ToString().Contains("Default"))
                                thisOpt = "ref=" + mainWindow.drop_refFrames.SelectedItem.ToString();
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

        /*
         * Used by on_x264_WidgetChange()
         * Called when the current x264 option string contains no options.
         * This simply adds the option to the x264 query in the gui.
         */
        private void hasNoOptions(string optNameToChange, frmMain mainWindow)
        {
            // If the text box is blank
            if (mainWindow.rtf_x264Query.Text == "")
            {
                if (optNameToChange.Equals("me"))
                {
                    switch (mainWindow.drop_MotionEstimationMethod.SelectedIndex)
                    {
                        case 1:
                            mainWindow.rtf_x264Query.Text = "me=dia";
                            break;

                        case 2:
                            mainWindow.rtf_x264Query.Text = "me=hex";

                            break;

                        case 3:
                            mainWindow.rtf_x264Query.Text = "me=umh";
                            break;

                        case 4:
                            mainWindow.rtf_x264Query.Text = "me=esa";
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
                            mainWindow.rtf_x264Query.Text = "direct=none";
                            break;

                        case 2:
                            mainWindow.rtf_x264Query.Text = "direct=spatial";
                            break;

                        case 3:
                            mainWindow.rtf_x264Query.Text = "direct=temporal";
                            break;

                        case 4:
                            mainWindow.rtf_x264Query.Text = "direct=auto";
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
                            mainWindow.rtf_x264Query.Text = "analyse=none";
                            break;

                        case 2:
                            mainWindow.rtf_x264Query.Text = "analyse=all";
                            break;

                        default:
                            break;
                    }
                }

                else if (optNameToChange.Equals("merange"))
                {
                    int value = mainWindow.drop_MotionEstimationRange.SelectedIndex + 3;
                    mainWindow.rtf_x264Query.Text = "merange=" + value.ToString();
                }
                else if (optNameToChange.Equals("deblock"))
                {
                    String da = mainWindow.drop_deblockAlpha.SelectedItem.ToString();
                    String db = mainWindow.drop_deblockBeta.SelectedItem.ToString();

                    if (((da.Contains("Default")) && (db.Contains("Default"))) || ((da.Contains("0")) && (db.Contains("0"))))
                    {
                        mainWindow.drop_deblockBeta.SelectedItem = "Default (0)";
                        mainWindow.drop_deblockAlpha.SelectedItem = "Default (0)";
                        mainWindow.rtf_x264Query.Text = "";
                    }
                    else if ((!da.Contains("Default")) && (db.Contains("Default")))
                    {
                        mainWindow.drop_deblockBeta.SelectedItem = "0";
                        mainWindow.rtf_x264Query.Text = "deblock=" + da + ",0";
                    }
                    else if ((da.Contains("Default")) && (!db.Contains("Default")))
                    {
                        mainWindow.drop_deblockAlpha.SelectedItem = "0";
                        mainWindow.rtf_x264Query.Text = "deblock=0," + db;
                    }
                    else if ((!da.Contains("Default")) && (!db.Contains("Default")))
                    {
                        mainWindow.rtf_x264Query.Text = "deblock=" + da + "," + db;
                    }
                }
                else if (optNameToChange.Equals("mixed-refs"))
                {
                    if (mainWindow.check_mixedReferences.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = "mixed-refs=1";
                    else
                        mainWindow.rtf_x264Query.Text = "";
                }
                else if (optNameToChange.Equals("weightb"))
                {
                    if (mainWindow.check_weightedBFrames.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = "weightb=1";
                    else
                        mainWindow.rtf_x264Query.Text = "";
                }
                else if (optNameToChange.Equals("brdo"))
                {
                    if (mainWindow.check_bFrameDistortion.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = "brdo=1";
                    else
                        mainWindow.rtf_x264Query.Text = "";
                }
                else if (optNameToChange.Equals("bime"))
                {
                    if (mainWindow.check_BidirectionalRefinement.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = "bime=1";
                    else
                        mainWindow.rtf_x264Query.Text = "";
                }
                else if (optNameToChange.Equals("b-pyramid"))
                {
                    if (mainWindow.check_pyrmidalBFrames.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = "b-pyramid=1";
                    else
                        mainWindow.rtf_x264Query.Text = "";
                }
                else if (optNameToChange.Equals("no-fast-pskip"))
                {
                    if (mainWindow.check_noFastPSkip.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = "no-fast-pskip=1";
                    else
                        mainWindow.rtf_x264Query.Text = "";
                }
                else if (optNameToChange.Equals("no-dct-decimate"))
                {
                    if (mainWindow.check_noDCTDecimate.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = "no-dct-decimate=1";
                    else
                        mainWindow.rtf_x264Query.Text = "";
                }
                else if (optNameToChange.Equals("8x8dct"))
                {
                    if (mainWindow.check_8x8DCT.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = "8x8dct=1";
                    else
                        mainWindow.rtf_x264Query.Text = "";
                }
                else if (optNameToChange.Equals("cabac"))
                {
                    if (mainWindow.check_Cabac.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = "";
                    else
                        mainWindow.rtf_x264Query.Text = "cabac=0";
                }
                else if (optNameToChange.Equals("ref"))
                {
                    string refItem = mainWindow.drop_refFrames.SelectedItem.ToString();
                    if (!refItem.Contains("default"))
                        mainWindow.rtf_x264Query.Text = "ref=" + mainWindow.drop_refFrames.SelectedItem.ToString();
                }
                else if (optNameToChange.Equals("bframes"))
                {
                    String value = mainWindow.drop_bFrames.SelectedItem.ToString();
                    if (!mainWindow.drop_bFrames.SelectedItem.ToString().Contains("Default"))
                        mainWindow.rtf_x264Query.Text = "bframes=" + value;
                }
                else if (optNameToChange.Equals("subq"))
                {
                    String value = mainWindow.drop_subpixelMotionEstimation.SelectedItem.ToString();
                    if (!mainWindow.drop_subpixelMotionEstimation.SelectedItem.ToString().Contains("Default"))
                        mainWindow.rtf_x264Query.Text = "subq=" + value;
                }
                else if (optNameToChange.Equals("trellis"))
                {
                    String value = mainWindow.drop_trellis.SelectedItem.ToString();
                    if (!mainWindow.drop_trellis.SelectedItem.ToString().Contains("Default"))
                        mainWindow.rtf_x264Query.Text = "trellis=" + value;
                }

                //*******************************************************
            }
            else // There was some text in the box. This deals with options with no value. e.g "no-fast-pskip"
            {
                //*******************************************************
                if (optNameToChange.Equals("me"))
                {
                    switch (mainWindow.drop_MotionEstimationMethod.SelectedIndex)
                    {
                        case 1:
                            mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":me=dia";
                            break;

                        case 2:
                            mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":me=hex";
                            break;

                        case 3:
                            mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":me=umh";
                            break;

                        case 4:
                            mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":me=esa";
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
                            mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":direct=none";
                            break;

                        case 2:
                            mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":direct=spatial";
                            break;

                        case 3:
                            mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":direct=temporal";
                            break;

                        case 4:
                            mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":direct=auto";
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
                            mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":analyse=none";
                            break;

                        case 2:
                            mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":analyse=all";
                            break;

                        default:
                            break;
                    }
                }

                else if (optNameToChange.Equals("merange"))
                {
                    int value = mainWindow.drop_MotionEstimationRange.SelectedIndex + 3;
                    mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":merange=" + value.ToString();
                }
                else if (optNameToChange.Equals("deblock"))
                {
                    String da = mainWindow.drop_deblockAlpha.SelectedItem.ToString();
                    String db = mainWindow.drop_deblockBeta.Text.ToString();

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

                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":deblock=" + da + "," + db;
                    }
                }
                else if (optNameToChange.Equals("mixed-refs"))
                {
                    if (mainWindow.check_mixedReferences.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":mixed-refs=1";
                }
                else if (optNameToChange.Equals("weightb"))
                {
                    if (mainWindow.check_weightedBFrames.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":weightb=1";
                }
                else if (optNameToChange.Equals("brdo"))
                {
                    if (mainWindow.check_bFrameDistortion.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":brdo=1";
                }
                else if (optNameToChange.Equals("bime"))
                {
                    if (mainWindow.check_BidirectionalRefinement.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":bime=1";
                }
                else if (optNameToChange.Equals("b-pyramid"))
                {
                    if (mainWindow.check_pyrmidalBFrames.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":b-pyramid=1";
                }
                else if (optNameToChange.Equals("no-fast-pskip"))
                {
                    if (mainWindow.check_noFastPSkip.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":no-fast-pskip=1";
                }
                else if (optNameToChange.Equals("no-dct-decimate"))
                {
                    if (mainWindow.check_noDCTDecimate.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":no-dct-decimate=1";
                }
                else if (optNameToChange.Equals("8x8dct"))
                {
                    if (mainWindow.check_8x8DCT.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":8x8dct=1";
                }
                else if (optNameToChange.Equals("cabac"))
                {
                    if (mainWindow.check_Cabac.CheckState == CheckState.Checked)
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text;
                    else
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":cabac=0";
                }
                else if (optNameToChange.Equals("ref"))
                {
                    if (!mainWindow.drop_refFrames.SelectedItem.ToString().Contains("Default"))
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":ref=" + mainWindow.drop_refFrames.SelectedItem.ToString();
                }
                else if (optNameToChange.Equals("bframes"))
                {
                    int value = mainWindow.drop_bFrames.SelectedIndex;
                    value = value - 1;
                    if (!mainWindow.drop_bFrames.SelectedItem.ToString().Contains("Default"))
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":bframes=" + value.ToString();
                }
                else if (optNameToChange.Equals("subq"))
                {
                    String value = mainWindow.drop_subpixelMotionEstimation.SelectedItem.ToString();
                    if (!mainWindow.drop_subpixelMotionEstimation.SelectedItem.ToString().Contains("Default"))
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":subq=" + value;
                }
                else if (optNameToChange.Equals("trellis"))
                {
                    if (!mainWindow.drop_trellis.SelectedItem.ToString().Contains("Default"))
                        mainWindow.rtf_x264Query.Text = mainWindow.rtf_x264Query.Text + ":trellis=" + mainWindow.drop_trellis.SelectedItem.ToString();
                }
            }
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
            {
                cleanOptNameString = "ref";
            }

            /*No Fast PSkip nofast_pskip*/
            if (input.Equals("no-fast-pskip") || input.Equals("no_fast_pskip") || input.Equals("nofast_pskip"))
            {
                cleanOptNameString = "no-fast-pskip";
            }

            /*No Dict Decimate*/
            if (input.Equals("no-dct-decimate") || input.Equals("no_dct_decimate") || input.Equals("nodct_decimate"))
            {
                cleanOptNameString = "no-dct-decimate";
            }

            /*Subme*/
            if (input.Equals("subme"))
            {
                cleanOptNameString = "subq";
            }

            /*ME Range*/
            if (input.Equals("me-range") || input.Equals("me_range"))
            {
                cleanOptNameString = "merange";
            }

            /*WeightB*/
            if (input.Equals("weight-b") || input.Equals("weight_b"))
            {
                cleanOptNameString = "weightb";
            }

            /*BRDO*/
            if (input.Equals("b-rdo") || input.Equals("b_rdo"))
            {
                cleanOptNameString = "brdo";
            }

            /*B Pyramid*/
            if (input.Equals("b_pyramid"))
            {
                cleanOptNameString = "b-pyramid";
            }

            /*Direct Prediction*/
            if (input.Equals("direct-pred") || input.Equals("direct_pred"))
            {
                cleanOptNameString = "direct";
            }

            /*Deblocking*/
            if (input.Equals("filter"))
            {
                cleanOptNameString = "deblock";
            }

            /*Analysis*/
            if (input.Equals("partitions"))
            {
                cleanOptNameString = "analyse";
            }

            return cleanOptNameString;
        }
    }
}
